
#ifndef SCHPUNE_NESCORE_APU_H_INCLUDED
#define SCHPUNE_NESCORE_APU_H_INCLUDED

#include "schpunetypes.h"
#include "subsystem.h"
#include "apu_pulse.h"
#include "audiotimestampholder.h"


namespace schcore
{
    struct ResetInfo;
    class CpuBus;
    class AudioBuilder;

    class Apu : public SubSystem, public AudioTimestampHolder
    {
    public:
        //////////////////////////////////////////////////
        //  Resetting
        void                reset(const ResetInfo& info);

        //////////////////////////////////////////////////
        //  Running
        virtual void        run(timestamp_t runto) override;
        void                fabricateMoreAudio(int count_in_s16s);

        
        virtual void        subtractFromAudioTimestamp(timestamp_t sub) override    { audTimestamp -= sub;      }
        timestamp_t         getAudTimestamp() const                                 { return audTimestamp;      }

    private:

        timestamp_t         calcTicksToRun( timestamp_t now, timestamp_t target ) const;
        
        void                onWrite(u16 a, u8 v);
        void                onRead(u16 a, u8& v);
        
        void                clockSeqHalf();
        void                clockSeqQuarter();

        CpuBus*             bus;
        AudioBuilder*       builder;

        bool                oddCycle;
        timestamp_t         seqCounter;
        int                 nextSeqPhase;
        int                 seqMode;                // 0=4-step mode, 1=5-step

        timestamp_t         modeResetCounter;
        u8                  newSeqMode;             // mode to be reset to

        bool                frameIrqEnabled;
        bool                frameIrqPending;
        irqsource_t         frameIrqBit;

        timestamp_t         audTimestamp;


        Apu_Pulse           pulses;
    };


}

#endif