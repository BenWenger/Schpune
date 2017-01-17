
#include "apu_tnd.h"
#include "cpubus.h"
#include "resetinfo.h"
#include "dmaunit.h"
#include "eventmanager.h"
#include "apu.h"
#include <algorithm>


namespace schcore
{
    
    const u16 Apu_Tnd::noiseFreqLut[2][0x10] = {
        { 4, 8, 16, 32, 64, 96, 128, 160, 202, 254, 380, 508, 762, 1016, 2034, 4068 },          // NTSC
        { 4, 8, 14, 30, 60, 88, 118, 148, 188, 236, 354, 472, 708,  944, 1890, 3778 }           // PAL
    };
    const int Apu_Tnd::dmcFreqLut[2][0x10] = {
        { 428, 380, 340, 320, 286, 254, 226, 214, 190, 160, 142, 128, 106,  84,  72,  54 },     // NTSC
        { 398, 354, 316, 298, 276, 236, 210, 198, 176, 148, 132, 118,  98,  78,  66,  50 }      // PAL
    };


    void Apu_Tnd::writeMain(u16 a, u8 v)
    {
        switch(a)
        {
            //////////////////////////////////
            //  Tri
        case 0x4008:
            tri.linear.writeLoad(v);
            tri.length.writeHalt(v & 0x80);
            break;
        case 0x400A:
            tri.freqTimer = (tri.freqTimer & 0x0700) | v;
            break;
        case 0x400B:
            tri.freqTimer = (tri.freqTimer & 0x00FF) | ((v & 0x07) << 8);
            tri.length.writeLoad(v);
            tri.linear.writeHigh();
            break;
            
            //////////////////////////////////
            //  Noise
        case 0x400C:
            nse.length.writeHalt(v & 0x20);
            nse.decay.write(v);
            break;
        case 0x400E:
            nse.shiftMode =     (v & 0x80) ? 9 : 14;
            nse.freqTimer =     noiseFreqLut[region][v & 0x0F];
            break;
        case 0x400F:
            nse.length.writeLoad(v);
            nse.decay.writeHigh();
            break;
            
            //////////////////////////////////
            //  DMC
        case 0x4010:
            dmcIrqEnabled = (v & 0x80) != 0;
            if(!dmcIrqEnabled && dmcIrqPending)
            {
                dmcIrqPending = false;
                cpuBus->acknowledgeIrq(dmcIrqBit);
            }

            dmcLoop = (v & 0x40) != 0;
            dmcFreqTimer = dmcFreqLut[region][v & 0x0F];
            
            predictNextEvent();     // time of next fetch may have changed, predict next event
            break;
        case 0x4011:
            dmcOut = v & 0x7F;
            break;
        case 0x4012:
            dmcAddrLoad =       0xC000 | (v << 6);
            break;
        case 0x4013:
            dmcLenLoad =        (v << 4) + 1;
            break;
        }
    }

    void Apu_Tnd::write4015(u8 v)
    {
        tri.length.writeEnable(v & 0x04);
        nse.length.writeEnable(v & 0x08);

        if(dmcIrqPending)
        {
            dmcIrqPending = false;
            cpuBus->acknowledgeIrq( dmcIrqBit );
        }

        if(v & 0x10)
        {
            if(!dmcpu.len)
            {
                // start a new clip
                //   if dmcaud is completely silent, sync with dmcpu
                tryToSyncDmc();

                startDmcClip(dmcpu);
                startDmcClip(dmcaud);

                doDmcFetch(dmcpu,true);
                doDmcFetch(dmcaud,false);

                predictNextEvent();     // length became nonzero, predict next event
            }
        }
        else
        {
            dmcpu.len = dmcaud.len = 0;
        }
    }

    inline void Apu_Tnd::tryToSyncDmc()
    {
        // only sync if the dmcaud is totally silent
        if(dmcaud.audible)                          return;
        if(dmcPeekSampleBuffer.willBeAudible())     return;
        if(dmcaud.len)                              return;

        // OK to sync!
        dmcaud.freqCounter      = dmcpu.freqCounter;
        dmcaud.bitsRemaining    = dmcpu.bitsRemaining;
    }

    inline void Apu_Tnd::startDmcClip(DmcData& dat)
    {
        dat.len =       dmcLenLoad;
        dat.addr =      dmcAddrLoad;
    }
    
    inline void Apu_Tnd::doDmcFetch(DmcData& dat, bool isdmcpu)
    {
        if(!dat.len)        return;

        dat.supplier->triggerFetch( dat.addr );
        dat.addr = (dat.addr + 1) | 0x8000;

        if(!--dat.len)
        {
            if(dmcLoop)
                startDmcClip(dat);
            else if(isdmcpu && dmcIrqEnabled)
            {
                dmcIrqPending = true;
                cpuBus->triggerIrq(dmcIrqBit);
            }
        }
    }

    void Apu_Tnd::read4015(u8& v)
    {
        if(tri.length.isAudible())  v |= 0x04;
        if(nse.length.isAudible())  v |= 0x08;
        if(dmcpu.len)               v |= 0x10;
        if(dmcIrqPending)           v |= 0x80;
    }

    void Apu_Tnd::reset(const ResetInfo& info)
    {
        if(info.hardReset)
        {
            channelHardReset();

            region = (info.region.apuTables == RegionInfo::ApuTables::pal);

            tri.length.hardReset();
            tri.linear.hardReset();
            tri.freqCounter = tri.freqTimer = 0x07FF;
            tri.triStep = 0;

            nse.decay.hardReset();
            nse.length.hardReset();
            nse.freqTimer = nse.freqTimer = noiseFreqLut[region][0x0F];
            nse.shiftMode = 14;
            nse.shifter = 1;

            dmcPeekSampleBuffer.reset(info);
            cpuBus              = info.cpuBus;
            eventManager        = info.eventManager;
            apuHost             = info.apu;
            dmcOut              = 0;
            dmcFreqTimer        = dmcFreqLut[region][0];
            dmcAddrLoad         = 0xC000;
            dmcLenLoad          = 0x0FF1;
            dmcIrqPending       = false;
            dmcIrqEnabled       = false;
            dmcIrqBit           = cpuBus->createIrqCode("DMC");
            dmcLoop             = false;
                dmcpu .supplier =                                   info.dmaUnit;
                dmcaud.supplier =                                   &dmcPeekSampleBuffer;
                dmcpu.freqCounter =     dmcaud.freqCounter =        dmcFreqTimer;
                dmcpu.addr =            dmcaud.addr =               dmcAddrLoad;
                dmcpu.len =             dmcaud.len =                dmcLenLoad;
                dmcpu.outputUnit =      dmcaud.outputUnit =         0;
                dmcpu.bitsRemaining =   dmcaud.bitsRemaining =      8;
                dmcpu.audible =         dmcaud.audible =            false;
        }
        else
        {
            tri.length.writeEnable(0);
            nse.length.writeEnable(0);
            dmcpu.len = 0;
            dmcaud.len = 0;
        }
    }

    void Apu_Tnd::clockSeqHalf()
    {
        tri.length.clock();
        nse.length.clock();
    }
    
    void Apu_Tnd::clockSeqQuarter()
    {
        tri.linear.clock();
        nse.decay.clock();
    }

    timestamp_t Apu_Tnd::clocksToNextUpdate()
    {
        timestamp_t out = Time::Never;
        
        if(tri.length.isAudible() && tri.linear.isAudible())        out = std::min( out, tri.freqCounter );
        if(nse.length.isAudible() && nse.decay.getOutput())         out = std::min( out, nse.freqCounter );
        if(dmcaud.audible || dmcPeekSampleBuffer.willBeAudible())   out = std::min( out, dmcaud.freqCounter );

        return out;
    }

    int Apu_Tnd::doTicks(timestamp_t ticks, bool doaudio, bool docpu)
    {
        if(docpu)               runDmc(dmcpu, ticks, true);
        if(!doaudio)            return 0;

        int out;

        ////////////////////////
        // triangle
        tri.freqCounter -= ticks;
        while(tri.freqCounter <= 0)
        {
            tri.freqCounter += tri.freqTimer + 1;
            if(tri.length.isAudible() && tri.linear.isAudible())
                tri.triStep = (tri.triStep + 1) & 0x1F;
        }

        if(tri.triStep & 0x10)      out = tri.triStep ^ 0x1F;
        else                        out = tri.triStep;
        
        ////////////////////////
        // noise
        nse.freqCounter -= ticks;
        while(nse.freqCounter <= 0)
        {
            nse.freqCounter += nse.freqTimer;

            nse.shifter |= 0x8000 & ( (nse.shifter << 15) ^ (nse.shifter << nse.shiftMode) );
            nse.shifter >>= 1;
        }

        if(nse.length.isAudible() && !(nse.shifter & 0x0001))
            out |= nse.decay.getOutput() << 4;
        
        ////////////////////////
        // DMC
        runDmc(dmcaud, ticks, false);
        out |= dmcOut << 8;

        return out;
    }

    void Apu_Tnd::runDmc(DmcData& dat, timestamp_t ticks, bool isdmcpu)
    {
        dat.freqCounter -= ticks;
        while(dat.freqCounter <= 0)
        {
            dat.freqCounter += dmcFreqTimer;
            --dat.bitsRemaining;

            if(!isdmcpu && dat.audible)
            {
                if(dat.outputUnit & 0x01)   { if(dmcOut < 0x7E)     dmcOut += 2;    }
                else                        { if(dmcOut >    1)     dmcOut -= 2;    }
            }
            dat.outputUnit >>= 1;

            if(!dat.bitsRemaining)
            {
                dat.bitsRemaining = 8;
                dat.supplier->getFetchedByte( dat.outputUnit, dat.audible );
                doDmcFetch( dat, isdmcpu );

                if(isdmcpu)
                    predictNextEvent();
            }
        }
    }

    void Apu_Tnd::predictNextEvent()
    {
        //////////////////////////////////////////
        //  DMC has 2 noteworthy events:  DMA cycle stealing, and DMC IRQ
        //
        //  fortunately, the IRQ is performed at the same time as the cycle stealing (it happens when the last
        //     sample byte is fetched), therefore we only need to report the cycle stealing event.


        //  cycle stealing will happen only if the dmcpu has a nonzero length
        if(!dmcpu.len)      return;

        // next stolen cycle will happen when all remaining bits get shifted out
        timestamp_t ticks = dmcpu.freqCounter;
        if(dmcpu.bitsRemaining > 1)
            ticks += dmcFreqTimer * (dmcpu.bitsRemaining-1);

        // scale that up to an actual timestamp
        ticks *= apuHost->getClockBase();
        ticks += apuHost->curCyc();

        eventManager->addEvent( ticks, EventType::evt_apu );
    }

    
    
    void Apu_Tnd::recalcOutputLevels(const AudioSettings& settings, ChannelId chanid, std::vector<float> (&levels)[2])
    {
        levels[0].resize(0x8000);
        levels[1].resize(0x8000);

        /*
                                       159.79
tnd_out = -------------------------------------------------------------
                                    1
           ----------------------------------------------------- + 100
            (triangle / 8227) + (noise / 12241) + (dmc / 22638)
        */

        float mvol = static_cast<float>( settings.masterVol * baseNativeOutputLevel );
        
        auto tri = getVolMultipliers( settings, ChannelId::triangle );
        auto nse = getVolMultipliers( settings, ChannelId::noise );
        auto dmc = getVolMultipliers( settings, ChannelId::dmc );
        
        float t;
        for(int i = 0; i < 0x8000; ++i)
        {
            // L
            t       = (i & 0x000F) * tri.first / 8227;
            t      += ((i & 0x00F0) >> 4) * nse.first / 12241;
            t      += ( i >> 8 ) * dmc.first / 22638;
            if(t != 0)
            {
                t = 1 / t;
                t += 100;
                t = 159.79 / t;
                t *= mvol;
            }
            levels[0][i] = t;
                
            // R
            t       = (i & 0x000F) * tri.second / 8227;
            t      += ((i & 0x00F0) >> 4) * nse.second / 12241;
            t      += ( i >> 8 ) * dmc.second / 22638;
            if(t != 0)
            {
                t = 1 / t;
                t += 100;
                t = 159.79 / t;
                t *= mvol;
            }
            levels[1][i] = t;
        }
    }
}