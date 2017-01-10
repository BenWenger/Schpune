
#ifndef SCHPUNE_NESCORE_AUDIOCHANNEL_H_INCLUDED
#define SCHPUNE_NESCORE_AUDIOCHANNEL_H_INCLUDED

#include <vector>
#include "schpunetypes.h"


namespace schcore
{
    class AudioBuilder
    {
    public:
        void                    addTransition( timestamp_t clocktime, float l, float r );
        int                     completedSamples( timestamp_t clocktime ) const;            //
        //int                     

    private:
        double                  mainClockRate;
        int                     sampleRate;
        int                     bufferSizeInMilliseconds;
        timestamp_t             maxSampleTimestamp;

        timestamp_t             timeBase;
        timestamp_t             timeConversion;
        int                     timeShift;

        std::vector<float>      transitionBuffer[2];        // [0] = left, [1] = right
        bool                    stereo;
    };


}

#endif