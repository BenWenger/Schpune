
#ifndef SCHPUNE_NESCORE_APU_H_INCLUDED
#define SCHPUNE_NESCORE_APU_H_INCLUDED

#include "schpunetypes.h"
#include "subsystem.h"
#include "apu_pulse.h"
#include "apu_tnd.h"
#include "audiotimestampholder.h"
#include "audiosettings.h"


namespace schcore
{
    class ResetInfo;
    class CpuBus;
    class AudioBuilder;
    class EventManager;

    class Apu : public SubSystem, public AudioTimestampHolder
    {
    public:
                            Apu();

        //////////////////////////////////////////////////
        //  Resetting
        void                reset(const ResetInfo& info);

        //////////////////////////////////////////////////
        //  Output configuration
        AudioSettings       getAudioSettings() const                                { return audSettings;      }
        void                setAudioSettings(const AudioSettings& settings);

        void                silenceAllChannels();

        //////////////////////////////////////////////////
        //  Running
        virtual void        run(timestamp_t runto) override;
        void                fabricateMoreAudio(int bytes);

        
        virtual void        subtractFromAudioTimestamp(timestamp_t sub) override    { audTimestamp -= sub;      }
        timestamp_t         getAudTimestamp() const                                 { return audTimestamp;      }

        
        virtual void        endFrame(timestamp_t subadjust) override;


    private:

        timestamp_t         calcTicksToRun( timestamp_t now, timestamp_t target ) const;
        void                predictNextEvent();
        
        void                onWrite(u16 a, u8 v);
        void                onRead(u16 a, u8& v);
        
        void                clockSeqHalf();
        void                clockSeqQuarter();

        CpuBus*             bus;
        AudioBuilder*       builder;
        EventManager*       eventManager;

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
        Apu_Tnd             tnd;

        AudioSettings       audSettings;
    };


}

#endif