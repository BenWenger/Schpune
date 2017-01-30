
#include "vrc7.h"
#include "../resetinfo.h"
#include "../cpubus.h"
#include <cmath>
#include <algorithm>

namespace schcore
{
    namespace
    {
        const ChannelId vrc7ids[6] = {
            ChannelId::vrc7_0,
            ChannelId::vrc7_1,
            ChannelId::vrc7_2,
            ChannelId::vrc7_3,
            ChannelId::vrc7_4,
            ChannelId::vrc7_5
        };

        const double pi = 3.1415926535897932384626433832795;

        const double fullln = std::log( static_cast<double>(1<<23) );

        u32 calcAttackEgc(u32 egc)
        {
            return (1<<23) - static_cast<u32>( (1<<23) * std::log(static_cast<double>(egc)) / fullln );
        }

        const u8 fixedInstruments[0xF][8] = {
            0x03, 0x21, 0x05, 0x06, 0xB8, 0x82, 0x42, 0x27,
            0x13, 0x41, 0x13, 0x0D, 0xD8, 0xD6, 0x23, 0x12,
            0x31, 0x11, 0x08, 0x08, 0xFA, 0x9A, 0x22, 0x02,
            0x31, 0x61, 0x18, 0x07, 0x78, 0x64, 0x30, 0x27,
            0x22, 0x21, 0x1E, 0x06, 0xF0, 0x76, 0x08, 0x28,
            0x02, 0x01, 0x06, 0x00, 0xF0, 0xF2, 0x03, 0xF5,
            0x21, 0x61, 0x1D, 0x07, 0x82, 0x81, 0x16, 0x07,
            0x23, 0x21, 0x1A, 0x17, 0xCF, 0x72, 0x25, 0x17,
            0x15, 0x11, 0x25, 0x00, 0x4F, 0x71, 0x00, 0x11,
            0x85, 0x01, 0x12, 0x0F, 0x99, 0xA2, 0x40, 0x02,
            0x07, 0xC1, 0x69, 0x07, 0xF3, 0xF5, 0xA7, 0x12,
            0x71, 0x23, 0x0D, 0x06, 0x66, 0x75, 0x23, 0x16,
            0x01, 0x02, 0xD3, 0x05, 0xA3, 0x92, 0xF7, 0x52,
            0x61, 0x63, 0x0C, 0x00, 0x94, 0xAF, 0x34, 0x06,
            0x21, 0x62, 0x0D, 0x00, 0xB1, 0xA0, 0x54, 0x17
        };

        const int multiLut[0x10] = {
            1, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 20, 24, 24, 30, 30
        };

        const double keyScaleLevelBuilderLut[0x10] = {     // dB attenuation
               0,18.00,24.00,27.75,30.00,32.25,33.75,35.25,36.00,37.50,38.25,39.00,39.75,40.50,41.25,42.00
        };
    }


    //////////////////////////////////////////
    //////////////////////////////////////////
    //  Host

    Vrc7Audio::Vrc7Audio()
    {
        quarterSineLut.resize(0x80);

        quarterSineLut[0] = 1<<23;
        for(int i = 1; i < 0x80; ++i)
        {
            quarterSineLut[i] = static_cast<int>( -20 * std::log10( std::sin( pi * i / 0x80 / 2 ) ) * (1<<23)/48 );
        }

        /*
  F = high 4 bits of the current F-Num
  B = 3-bit Block (Octave)
  A = table[ F ] - 6 * (7-B)

  if A < 0:   key_scale = 0
  otherwise:  key_scale = A >> (3-K)
  */
        keyScaleLevelLut.resize(0x80 * 3);
        for(int i = 0; i < (0x80 * 3); ++i)
        {
            int f = i & 0x0F;
            int b = (i >> 4) & 0x07;
            int k = (i >> 7) & 0x03;

            double out = keyScaleLevelBuilderLut[f] - (6*(7-b));
            if((out > 0) && (k > 0))
            {
                out /= std::pow<double>(2, k);
                keyScaleLevelLut[i] = static_cast<int>( std::pow<double>(10, out / -20 / ((1<<23)/48.0)) );
            }
            else
                keyScaleLevelLut[i] = 0;
        }
    }

    void Vrc7Audio::reset(const ResetInfo& info)
    {
        if(info.hardReset)
        {
            info.cpuBus->addWriter(0x9, 0x9, this, &Vrc7Audio::onWrite );
            setApuObj(info.apu);
            for(int i = 0; i < 6; ++i)
            {
                addChannel( vrc7ids[i], &channels[i], true );
                channels[i].setClockRate( channels[i].getClockRate() * 36 );        // TODO - replace this with an actual region-set timestamp

                channels[i].hardReset(this);

                addr = 0;
            }
        }
    }

    void Vrc7Audio::onWrite(u16 a, u8 v)
    {
        a &= 0x9030;
        if(a == 0x9010)
        {
            addr = (v & 0x3F);
            return;
        }
        if(a != 0x9030)         return;

        //////////////////////////////////////////////
        //////////////////////////////////////////////
        //  actual writes to snd registers here

        // custom intrument settings?
        if(addr < 0x08)
        {
            customInstData[addr] = v;
            return;
        }

        // otherwise, is it a write to a channel setting?
        if((addr & 0x0F) >= 6)      return;     // non-existant channel index
        Channel& ch = channels[addr & 0x0F];
        switch(addr & 0x30)
        {
        case 0x10:      catchUp();      ch.write_1(v);      break;
        case 0x20:      catchUp();      ch.write_2(v);      break;
        case 0x30:      catchUp();      ch.write_3(v);      break;
        }
    }

    
    /////////////////////////////////////
    /////////////////////////////////////
    /////////////////////////////////////
    
    void Vrc7Audio::Channel::write_1(u8 v)
    {
        fNum = (fNum & 0x0100) | v;
    }

    void Vrc7Audio::Channel::write_2(u8 v)
    {
        fNum =          (fNum & 0x00FF) | ((v & 0x01) << 8);
        block =         (v >> 1) & 7;
        slowRelease =   (v & 0x20) != 0;

        //key on or off?
        if(slots[0].adsr == Adsr::Release || slots[0].adsr == Adsr::Idle)
        {
            // currently keyed off.... are they keying on?
            if(v & 0x10)        keyOn();
        }
        else
        {
            // currently keyed on... are they keying off?
            if(!(v & 0x10))     keyOff();
        }
    }

    void Vrc7Audio::Channel::write_3(u8 v)
    {
        instSelect = (v >> 4) & 0x0F;
        slots[1].baseAtten = (v & 0x0F) << (23-4);
        updateInstrumentInfo();
    }

    void Vrc7Audio::Channel::keyOff()
    {
        for(auto& s : slots)
        {
            // Attack needs to be converted
            if(s.adsr == Adsr::Attack)
                s.egc = calcAttackEgc(s.egc);

            // then, just switch to Release
            if(s.adsr != Adsr::Idle)
                s.adsr = Adsr::Release;
        }
    }

    void Vrc7Audio::Channel::keyOn()
    {
        for(auto& s : slots)
        {
            s.adsr =    Adsr::Attack;
            s.egc =     0;
            s.phase =   0;
        }
        updateInstrumentInfo();
    }
    
    void Vrc7Audio::Channel::updateInstrumentInfo()
    {
        const u8* inst = (instSelect ? fixedInstruments[instSelect-1] : host->customInstData);

        for(int i = 0; i < 2; ++i)
        {
            // 00 / 01
            slots[i].enableAm =         (inst[i+0] & 0x80) != 0;
            slots[i].enableFm =         (inst[i+0] & 0x40) != 0;
            slots[i].percussive =       (inst[i+0] & 0x20) != 0;
            slots[i].ksr =              (inst[i+0] & 0x10) != 0;
            slots[i].multi =            multiLut[ inst[i+0] & 0x0F ];

            // 02 / 03
            slots[i].ksl =              (inst[i+2] & 0xC0) >> 6;

            // 04 / 05
            slots[i].attackRate =       (inst[i+4] & 0xF0) >> 4;
            slots[i].decayRate =        (inst[i+4] & 0x0F);
            
            // 06 / 07
            slots[i].sustainLevel =     3 * ((inst[i+6] & 0xF0) >> 4) * (1<<23) / 48;
            slots[i].releaseRate =      (inst[i+6] & 0x0F);
        }
        
        slots[0].baseAtten =            (inst[2] & 0x3F) << (23-6);
        slots[0].rectifySine =          (inst[3] & 0x08) != 0;
        slots[1].rectifySine =          (inst[3] & 0x10) != 0;
        feedback =                      (inst[3] & 0x07);
    }


    /////////////////////////////////////////////////
    
    void Vrc7Audio::Channel::makeSilent()
    {
        slots[0].adsr = slots[1].adsr = Adsr::Idle;
        slots[0].egc =  slots[1].egc  = (1<<23);
    }
    
    timestamp_t Vrc7Audio::Channel::clocksToNextUpdate()
    {
        if(slots[1].adsr == Adsr::Idle)     return Time::Never;
        return 1;
    }

    void Vrc7Audio::Channel::recalcOutputLevels(const AudioSettings& settings, ChannelId chanid, std::vector<float> (&levels)[2])
    {
        doLinearOutputLevels( settings, chanid, levels, 0, 0.2f / (1<<20) );
    }
    
    /////////////////////////////////////////////////
    /////////////////////////////////////////////////
    /////////////////////////////////////////////////
    float Vrc7Audio::Channel::doTicks_raw(timestamp_t ticks, bool doaudio, bool docpu)
    {
        if(!doaudio)                        return 0;
        if(slots[1].adsr == Adsr::Idle)     return 0;

        int fb = 0;
        if(feedback)        fb = slots[0].output >> (8-feedback);
        
        clockSlot( slots[0], fb );
        clockSlot( slots[1], 0/*slots[0].output*/ );

        return static_cast<float>( slots[1].output );
    }


    void Vrc7Audio::Channel::clockSlot(Slot& slt, int phaseadj)
    {
        /////////////////////////////////////////
        // Envelope generation
        int r = 0;

        switch(slt.adsr)
        {
        case Adsr::Attack:      r = slt.attackRate;                         break;
        case Adsr::Decay:       r = slt.decayRate;                          break;
        case Adsr::Sustain:     r = slt.percussive ? slt.releaseRate : 0;   break;
        case Adsr::Release:
            if(slowRelease)             r = 5;
            else if(slt.percussive)     r = slt.releaseRate;
            else                        r = 7;
        }

        int rl = (block << 1) | (fNum >> 8);    // 'BF'
        if(!slt.ksr)        rl >>= 2;           // 'KB'
        rl += (r*4);                            // 'RKS'
        int rh = (rl >> 2);         // 'RH'
        if(rh > 15)   rh = 15;
        rl &= 3;                    // 'RL'

        int rh1 = std::max(rh-1,0); // 'RH-1'

        switch(slt.adsr)
        {
        case Adsr::Attack:
            slt.egc += (12 * (rl+4)) << rh;
            if(slt.egc >= (1<<23))
            {
                slt.egc = 0;
                slt.adsr = Adsr::Decay;
            }
            break;

        case Adsr::Decay:
            slt.egc += (rl+4) << rh1;

            if(slt.egc >= slt.sustainLevel)
                slt.adsr = Adsr::Sustain;
            break;

        case Adsr::Sustain:
        case Adsr::Release:
            slt.egc += (rl+4) << rh1;
            if(slt.egc >= (1<<23))
                slt.adsr = Adsr::Idle;
            break;
        }
        
        /////////////////////////////////////////
        // Phase motion
        int nextout;

        slt.phase += fNum * (1<<block) * slt.multi / 2;         // TODO include '* V' for vibrato .. use fmPhase

        u32 phase = slt.phase + phaseadj;

        bool highbit = (phase & 0x20000) != 0;
        if(highbit && slt.rectifySine)
            nextout = (1<<23);
        else
        {
            phase >>= 20-7;
            phase &= 0xFF;
            if(phase & 0x80)
                phase ^= 0xFF;
            nextout = host->quarterSineLut[phase];
        }

        // nextout has the current sine output
        nextout += slt.baseAtten + getKeyScaleAttenuation(slt) + getEnvelope(slt);      // TODO - plus AM

        // convert from dB to linear
        nextout = static_cast<int>( (1<<20) * std::pow<double>( 10.0, nextout / -20 / ((1<<23)/48.0) ) );

        if(highbit)     nextout = -nextout;

        slt.output = (slt.output + nextout)/2;
    }
    
    int Vrc7Audio::Channel::getKeyScaleAttenuation( Slot& slt )
    {
        if(!slt.ksl)        return 0;

        return host->keyScaleLevelLut[ (fNum >> 5) | (block << 4) | ((3-slt.ksl) << 7) ];
    }
    
    int Vrc7Audio::Channel::getEnvelope( Slot& slt )
    {
        if(slt.adsr == Adsr::Attack)        return calcAttackEgc(slt.egc);
        else                                return slt.egc;
    }

    void Vrc7Audio::Channel::hardReset(Vrc7Audio* thehost)
    {
        host =          thehost;
        fNum =          0;
        block =         0;
        slowRelease =   false;
        instSelect =    0;
        feedback =      0;

        for(auto& s : slots)
        {
            s.phase =       0;
            s.output =      0;
            s.egc =         (1<<23);
            s.adsr =        Adsr::Idle;
            s.baseAtten =   (1<<23);
            
            s.enableAm =    false;
            s.enableFm =    false;
            s.percussive =  false;
            s.ksr =         false;
            s.rectifySine = false;
            s.multi =       1;
            s.ksl =         0;
            s.attackRate =  0;
            s.decayRate =   0;
            s.sustainLevel =0;
            s.releaseRate = 0;
        }
    }

}