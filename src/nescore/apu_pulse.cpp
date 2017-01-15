
#include "apu_pulse.h"
#include <algorithm>

namespace schcore
{
    
    const bool Apu_Pulse::Data::dutyLut[4][8] = 
    {
        {false,true ,false,false,false,false,false,false},
        {false,true ,true ,false,false,false,false,false},
        {false,true ,true ,true ,true ,false,false,false},
        {true ,false,false,true ,true ,true ,true ,true }
    };

    void Apu_Pulse::writeMain(u16 a, u8 v)
    {
        auto& ch = dat[(a >> 2) & 1];

        switch(a & 3)
        {
        case 0: ch.decay.write(v);
                ch.dutyMode = (v >> 6);
                ch.length.writeHalt(v & 0x20);
                break;

        case 1: ch.sweep.write(v);
                break;

        case 2: ch.sweep.writeFLo(v);
                break;

        case 3: ch.sweep.writeFHi(v);
                ch.length.writeLoad(v);
                ch.decay.writeHigh();
                ch.dutyPhase = 0;
                break;
        }
    }

    void Apu_Pulse::write4015(u8 v)
    {
        dat[0].length.writeEnable(v & 0x01);
        dat[1].length.writeEnable(v & 0x02);
    }

    void Apu_Pulse::read4015(u8& v)
    {
        if(dat[0].length.isAudible())   v |= 0x01;
        if(dat[1].length.isAudible())   v |= 0x02;
    }
    
    int Apu_Pulse::doTicks(timestamp_t ticks, bool doaudio, bool docpu)
    {
        if(!doaudio)
            return 0;

        return dat[0].clock(ticks) | (dat[1].clock(ticks) << 4);
    }

    void Apu_Pulse::clockSeqHalf()
    {
        for(auto& ch : dat)
        {
            ch.length.clock();
            ch.sweep.clock();
        }
    }
    
    void Apu_Pulse::clockSeqQuarter()
    {
        for(auto& ch : dat)
        {
            ch.decay.clock();
        }
    }

    timestamp_t Apu_Pulse::clocksToNextUpdate()
    {
        timestamp_t next = Time::Never;

        if(dat[0].isAudible())      next = dat[0].freqCounter;
        if(dat[1].isAudible())      next = std::min(next, dat[1].freqCounter);

        return next;
    }

    void Apu_Pulse::reset(bool hard)
    {
        if(hard)
        {
            dat[0].sweep.hardReset(false);
            dat[1].sweep.hardReset(true );

            for(auto& i : dat)
            {
                i.decay.hardReset();
                i.length.hardReset();
                i.dutyMode = 0;
                i.dutyPhase = 0;
                i.freqCounter = 1;
            }

            // TODO -- move this somewhere more appropriate
            outputLevels[0].resize(0x100);
            outputLevels[1].resize(0x100);
            for(int i = 0; i < 0x100; ++i)
            {

                float lev = (  ( i     & 0x0F) + ((i>>4) & 0x0F)  ) * 0.00752f * 0.8f;
                outputLevels[0][i] = lev;
                outputLevels[1][i] = lev;
            }
        }
        else
        {
            dat[0].length.writeEnable(0);
            dat[1].length.writeEnable(0);
        }
    }
}
