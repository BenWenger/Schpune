
#ifndef SCHPUNE_NESCORE_AUDIOCHANNEL_H_INCLUDED
#define SCHPUNE_NESCORE_AUDIOCHANNEL_H_INCLUDED

#include <vector>
#include "schpunetypes.h"
#include "audiotimestampholder.h"


namespace schcore
{
    class AudioBuilder;

    class AudioChannel : public AudioTimestampHolder
    {
    public:
        virtual                 ~AudioChannel() {}

        // For run... give 'Time::Now' if cpu/aud is not to be run
        void                    run(timestamp_t cputarget, timestamp_t audiotarget);

        void                    setBuilder(AudioBuilder* bldr)          { builder = bldr;       }
        virtual void            subtractFromAudioTimestamp(timestamp_t sub) override    { audTimestamp -= sub;      }
        void                    subtractFromCpuTimestamp(timestamp_t sub)               { cpuTimestamp -= sub;      }
        void                    setClockRate(timestamp_t rate)                          { clockRate = rate;         }

    protected:
        //  To be implemented by derived classes
        virtual int             doTicks(timestamp_t ticks, bool doaudio, bool docpu) = 0;
        virtual timestamp_t     clocksToNextUpdate() = 0;

        // TODO - calculate output levels

    private:
        timestamp_t             calcTicksToRun( timestamp_t now, timestamp_t target ) const;
    protected:  // TODO
        std::vector<float>      outputLevels[2];
    private:    // TODO

        timestamp_t             clockRate;
        timestamp_t             audTimestamp;
        timestamp_t             cpuTimestamp;

        int                     prevOut;
        AudioBuilder*           builder;
    };


}

#endif