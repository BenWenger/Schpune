
#ifndef SCHPUNE_NESCORE_AUDIOCHANNEL_H_INCLUDED
#define SCHPUNE_NESCORE_AUDIOCHANNEL_H_INCLUDED

#include <vector>
#include <utility>
#include "schpunetypes.h"
#include "audiotimestampholder.h"
#include "audiosettings.h"


namespace schcore
{
    class AudioBuilder;
    struct AudioSettings;

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

        void                    channelHardReset()                                      { prevOut = 0;  audTimestamp = cpuTimestamp = 0;        }

        void                    updateSettings(const AudioSettings& settings, ChannelId chanid);

    protected:
        //  To be implemented by derived classes
        virtual int             doTicks(timestamp_t ticks, bool doaudio, bool docpu) = 0;
        virtual timestamp_t     clocksToNextUpdate() = 0;
        virtual void            recalcOutputLevels(const AudioSettings& settings, ChannelId chanid, std::vector<float> (&levels)[2]) = 0;

        // Used by derived classes
        static const float              baseNativeOutputLevel;
        static std::pair<float,float>   getVolMultipliers(const AudioSettings& settings, ChannelId chanid);


    private:
        timestamp_t             calcTicksToRun( timestamp_t now, timestamp_t target ) const;
        std::vector<float>      outputLevels[2];

        timestamp_t             clockRate;
        timestamp_t             audTimestamp;
        timestamp_t             cpuTimestamp;

        int                     prevOut;
        AudioBuilder*           builder;
    };


}

#endif