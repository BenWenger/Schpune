
#ifndef SCHPUNE_NESCORE_APU_PULSE_H_INCLUDED
#define SCHPUNE_NESCORE_APU_PULSE_H_INCLUDED

#include "schpunetypes.h"
#include "apu_support.h"
#include "audiochannel.h"


namespace schcore
{
    /////////////////////////////////////
    //   This class is for BOTH pulse channels, since their output interferes
    // with each other

    class Apu_Pulse : public AudioChannel
    {
    public:

        void                run(timestamp_t starttick, timestamp_t runfor);
        void                write(u16 a, u8 v);
        void                reset(bool hard);


    private:
        struct Data
        {
            Apu_Length      length;
            Apu_Decay       decay;
            Apu_Sweep       sweep;

            int             freqCounter;
            int             dutyPhase;
            int             dutyMode;

            bool            isAudible()             { return length.isAudible() && sweep.isAudible() && decay.getOutput();  }

            int             clock(timestamp_t ticks)
            {
                freqCounter -= ticks;
                while(freqCounter <= 0)
                {
                    freqCounter += sweep.getFreqTimer();
                    dutyPhase = (dutyPhase + 1) & 0x07;
                }

                if(!length.isAudible())             return 0;
                if(!sweep.isAudible())              return 0;
                if(!dutyLut[dutyMode][dutyPhase])   return 0;
                return decay.getOutput();
            }

            static const bool dutyLut[4][8];
        };

        Data    dat[2];
    };


}

#endif