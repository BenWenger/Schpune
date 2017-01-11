
#ifndef SCHPUNE_NESCORE_AUDIOCHANNEL_H_INCLUDED
#define SCHPUNE_NESCORE_AUDIOCHANNEL_H_INCLUDED

#include <vector>
#include "schpunetypes.h"
#include "subsystem.h"


namespace schcore
{
    class AudioBuilder;

    class AudioChannel
    {
    public:
        virtual                 ~AudioChannel() {}

        void                    run(timestamp_t runto, bool doaudio, bool docpu);
        void                    setBuilder(AudioBuilder* bldr)          { builder = bldr;       }

    protected:
        //  To be implemented by derived classes
        virtual int             doTicks(timestamp_t ticks, bool doaudio, bool docpu) = 0;
        virtual timestamp_t     clocksToNextUpdate() = 0;

        // TODO - calculate output levels

    private:
        std::vector<float>      outputLevels[2];

        timestamp_t             clockRate;
        timestamp_t             timestamp;

        int                     prevOut;
        AudioBuilder*           builder;
    };


}

#endif