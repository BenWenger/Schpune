
#ifndef SCHPUNE_NESCORE_AUDIOBUILDER_H_INCLUDED
#define SCHPUNE_NESCORE_AUDIOBUILDER_H_INCLUDED

#include <vector>
#include "schpunetypes.h"


namespace schcore
{
    class AudioBuilder
    {
    public:
        void                    addTransition( timestamp_t clocktime, float l, float r );
        int                     generateSamples( int startpos, s16* audio, int count_in_s16s );
        void                    wipeSamples( int count, timestamp_t timereduction );
        int                     samplesAvailableAtTimestamp( timestamp_t time );
        //int                     completedSamples( timestamp_t clocktime ) const;            //
        //int                     

    private:
        double                  mainClockRate;
        int                     sampleRate;
        int                     bufferSizeInMilliseconds;
        int                     bufferSizeInElements;
        timestamp_t             maxSampleTimestamp;

        timestamp_t             timeScalar;
        timestamp_t             timeOverflow;
        int                     timeShift;


        float                   outSample[2];
        std::vector<float>      transitionBuffer[2];        // [0] = left, [1] = right
        bool                    stereo;
    };


}

#endif