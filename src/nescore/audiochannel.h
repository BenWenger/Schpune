
#ifndef SCHPUNE_NESCORE_AUDIOCHANNEL_H_INCLUDED
#define SCHPUNE_NESCORE_AUDIOCHANNEL_H_INCLUDED

#include <vector>
#include <utility>
#include <map>
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

        void                    setBuilder(AudioBuilder* bldr)                          { builder = bldr;           }
        virtual void            subtractFromAudioTimestamp(timestamp_t sub) override    { audTimestamp -= sub;      }
        void                    subtractFromCpuTimestamp(timestamp_t sub)               { cpuTimestamp -= sub;      }
        void                    setClockRate(timestamp_t rate)                          { clockRate = rate;         }
        timestamp_t             getClockRate() const                                    { return clockRate;         }

        void                    channelHardReset()                                      { prevOut = 0;  audTimestamp = cpuTimestamp = 0;        }

        void                    updateSettings(const AudioSettings& settings, ChannelId chanid);

        virtual void            makeSilent() = 0;       // for when NSF tracks are changed, all channels need to be turned off somehow

    protected:
        //  To be implemented by derived classes
        virtual int             doTicks(timestamp_t ticks, bool doaudio, bool docpu) = 0;
        virtual float           doTicks_raw(timestamp_t ticks, bool doaudio, bool docpu) { return 0; }
        virtual timestamp_t     clocksToNextUpdate() = 0;
        virtual void            recalcOutputLevels(const AudioSettings& settings, ChannelId chanid, std::vector<float> (&levels)[2]) = 0;
        static void             doLinearOutputLevels(const AudioSettings& settings, ChannelId chanid, std::vector<float> (&levels)[2], int maxstep, float baseoutput);

        // Used by derived classes
        static const float              baseNativeOutputLevel;
        static std::pair<float,float>   getVolMultipliers(const AudioSettings& settings, ChannelId chanid);


    private:
        timestamp_t             calcTicksToRun( timestamp_t now, timestamp_t target ) const;
        std::vector<float>      outputLevels[2];

        timestamp_t             clockRate;
        timestamp_t             audTimestamp;
        timestamp_t             cpuTimestamp;
        
        bool                    useRawOutput;
        int                     prevOut;
        float                   prevRawOut;
        AudioBuilder*           builder;

        std::map<ChannelId, AudioChannel*>  exAudio;
    };


}

#endif