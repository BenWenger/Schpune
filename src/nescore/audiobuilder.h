
#ifndef SCHPUNE_NESCORE_AUDIOBUILDER_H_INCLUDED
#define SCHPUNE_NESCORE_AUDIOBUILDER_H_INCLUDED

#include <vector>
#include "schpunetypes.h"


namespace schcore
{
    class AudioTimestampHolder;
    class Apu;

    class AudioBuilder
    {
    public:
                                AudioBuilder();

        void                    hardReset( timestamp_t clocks_per_second, timestamp_t clocks_per_frame );

        void                    addTransition( timestamp_t clocktime, float l, float r );
        int                     generateSamples( int startbytepos, s16* audio, int sizeinbytes );
        void                    wipeSamples( int sizeinbytes );

        int                     audioAvailableAtTimestamp( timestamp_t time );          // returns how many bytes of audio will be available at given [audio] timestamp
        timestamp_t             timestampToProduceBytes( int bytes );                   // returns what [audio] timestamp you need to run to to get this many bytes of audio

        void                    setClockRates( timestamp_t clocks_per_second, timestamp_t clocks_per_frame );

        timestamp_t             getMaxAllowedTimestamp() const  { return clocksPerFrame;        }
        int                     getSampleRate() const           { return sampleRate;            }
        bool                    isStereo() const                { return stereo;                }

    private:
        // Interface for the APU
        friend class Apu;
        void                    addTimestampHolder(AudioTimestampHolder* holder)        { audioTimestampHolders.push_back(holder);      }
        void                    setFormat( int samplerate, bool stereo );

    private:
        class LowPassFilter
        {
        public:
            void                setBase(float b);
            void                reset();
            void                setSamplerate(int samplerate);
            float               samp(float in);

        private:
            float               prev_out = 0;
            float               k = 0;
            float               base = 0;
        };

        class HighPassFilter
        {
        public:
            void                setBase(float b);
            void                reset();
            void                setSamplerate(int samplerate);
            float               samp(float in);

        private:
            float               prev_in = 0;
            float               prev_out = 0;
            float               k = 0;
            float               base = 0;
        };


        void                    recalc();
        void                    flushTransitionBuffers();
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
        
        LowPassFilter           lp[2];
        HighPassFilter          hp1[2];
        HighPassFilter          hp2[2];

        std::vector<AudioTimestampHolder*>  audioTimestampHolders;
    };


}

#endif