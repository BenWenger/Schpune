
#ifndef SCHPUNE_NESCORE_APU_PULSE_H_INCLUDED
#define SCHPUNE_NESCORE_APU_PULSE_H_INCLUDED

#include "schpunetypes.h"
#include "apu_support.h"


namespace schcore
{
    /////////////////////////////////////
    //   This class is for BOTH pulse channels, since their output interferes
    // with each other

    class Apu_Pulse
    {
    public:

        void                run(timestamp_t starttick, timestamp_t runfor);
        void                write(u16 a, u8 v);


    private:
        struct Data
        {
            Apu_Length      length;
            Apu_Decay       decay;
            Apu_Sweep       sweep;

            int             freqCounter;
            int             dutyPhase;
            int             dutyMode;
        };

        Data    dat[2];
    };


}

#endif