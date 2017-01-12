
#ifndef SCHPUNE_NESCORE_AUDIOBUILDER_H_INCLUDED
#define SCHPUNE_NESCORE_AUDIOBUILDER_H_INCLUDED

#include <vector>
#include "schpunetypes.h"


namespace schcore
{
    class AudioTimestampHolder;

    class AudioBuilder
    {
    public:
                                AudioBuilder();
        void                    addTransition( timestamp_t clocktime, float l, float r );
        int                     generateSamples( int startpos, s16* audio, int count_in_s16s );
        void                    wipeSamples( int count_in_s16s );

        int                     samplesAvailableAtTimestamp( timestamp_t time );        // returns how many samples will be available at given [audio] timestamp
        timestamp_t             timestampToProduceSamples( int s16s );                  // returns what [audio] timestamp you need to run to to get this many samples

        void                    setFormat( int samplerate, bool stereo );
        void                    setClockRates( timestamp_t clocks_per_second, timestamp_t clocks_per_frame );
        void                    flushTransitionBuffers();

        timestamp_t             getMaxAllowedTimestamp() const  { return clocksPerFrame;        }
        int                     getSampleRate() const           { return sampleRate;            }
        bool                    isStereo() const                { return stereo;                }

    private:
        void                    recalc();
        int                     sampleRate;
        timestamp_t             bufferSizeInElements;
        timestamp_t             clocksPerSecond;
        timestamp_t             clocksPerFrame;

        timestamp_t             timeScalar;
        timestamp_t             timeOverflow;
        int                     timeShift;


        float                   outSample[2];
        std::vector<float>      transitionBuffer[2];        // [0] = left, [1] = right
        bool                    stereo;

        std::vector<AudioTimestampHolder*>  audioTimestampHolders;
    };


}

#endif