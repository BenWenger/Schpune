
#ifndef SCHPUNE_NESCORE_APU_H_INCLUDED
#define SCHPUNE_NESCORE_APU_H_INCLUDED

#include "schpunetypes.h"
#include "subsystem.h"
#include "apu_pulse.h"


namespace schcore
{
    struct ResetInfo;
    class CpuBus;
    class AudioBuilder;

    class Apu : public SubSystem
    {
    public:
        //////////////////////////////////////////////////
        //  Resetting
        void                reset(const ResetInfo& info);

        //////////////////////////////////////////////////
        //  Running
        virtual void        run(timestamp_t runto) override;

    private:
        void                onWrite(u16 a, u8 v);
        void                onRead(u16 a, u8& v);
        
        void                clockSeqHalf();
        void                clockSeqQuarter();
        void                doSeqIrq();

        CpuBus*             bus;
        AudioBuilder*       builder;

        bool                oddCycle;
        int                 seqCounter;
        int                 nextSeqPhase;
        int                 seqMode;

        int                 modeResetCounter;
        u8                  newSeqMode;             // mode to be reset to

        bool                frameIrqEnabled;
        bool                frameIrqPending;
        irqsource_t         frameIrqBit;


        Apu_Pulse           pulses;
    };


}

#endif