
#include "apu_tnd.h"
#include <algorithm>


namespace schcore
{
    
    const u16 Apu_Tnd::noiseFreqLut[2][0x10] = 
    {
        // NTSC
        { 4, 8, 16, 32, 64, 96, 128, 160, 202, 254, 380, 508, 762, 1016, 2034, 4068 },
        // PAL
        { 4, 8, 14, 30, 60, 88, 118, 148, 188, 236, 354, 472, 708,  944, 1890, 3778 }
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
            nse.freqTimer =     noiseFreqLut[0][v & 0x0F];           // TODO - catch PAL
            break;
        case 0x400F:
            nse.length.writeLoad(v);
            nse.decay.writeHigh();
            break;
            
            //////////////////////////////////
            //  DMC - TODO
        }
    }

    void Apu_Tnd::write4015(u8 v)
    {
        tri.length.writeEnable(v & 0x04);
        nse.length.writeEnable(v & 0x08);

        // TODO DMC
    }

    void Apu_Tnd::read4015(u8& v)
    {
        if(tri.length.isAudible())  v |= 0x04;
        if(nse.length.isAudible())  v |= 0x08;

        // TODO DMC
    }

    void Apu_Tnd::reset(bool hard)
    {
        if(hard)
        {
            tri.length.hardReset();
            tri.linear.hardReset();
            tri.freqCounter = tri.freqTimer = 0x07FF;
            tri.triStep = 0;

            nse.decay.hardReset();
            nse.length.hardReset();
            nse.freqTimer = nse.freqTimer = noiseFreqLut[0][0x0F];       // TODO - catch PAL
            nse.shiftMode = 14;
            nse.shifter = 1;
            
            // TODO -- move this somewhere more appropriate
            outputLevels[0].resize(0x8000);
            outputLevels[1].resize(0x8000);
            for(int i = 0; i < 0x8000; ++i)
            {
                int t = (i & 0x000F);
                int n = (i >> 4) & 0x0F;
                int d = (i >> 8) & 0x7F;
                float lev = (0.00851f * t) + (0.00494f * n) + (0.00335f * d);
                lev *= 0.8f;
                outputLevels[0][i] = lev;
                outputLevels[1][i] = lev;
            }
        }
        else
        {
            tri.length.writeEnable(0);
            nse.length.writeEnable(0);
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
        // TODO - DMC

        return out;
    }

    int Apu_Tnd::doTicks(timestamp_t ticks, bool doaudio, bool docpu)
    {
        if(!doaudio)
        {
            // TODO - clock DMCPU
            return 0;
        }
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


        return out;
    }
}