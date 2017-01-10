
#include "apu_pulse.h"
#include <algorithm>

namespace schcore
{
    void Apu_Pulse::write(u16 a, u8 v)
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


    void Apu_Pulse::run(timestamp_t starttick, timestamp_t runfor)
    {
        timestamp_t tick = 1;

        while(runfor > 0)
        {
            starttick += tick;
            runfor -= tick;

            //////////////////////////////////////
            int out  = dat[0].clock(tick);
            out     |= dat[1].clock(tick) << 4;

            changeOutput( starttick, out );


            //////////////////////////////////////
            tick = runfor;
            if(dat[0].isAudible())      tick = std::min(tick, dat[0].freqCounter);
            if(dat[1].isAudible())      tick = std::min(tick, dat[1].freqCounter);
        }
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
        }
        else
        {
            dat[0].length.writeEnable(0);
            dat[1].length.writeEnable(0);
        }
    }
}
