
#ifndef SCHPUNE_NESCORE_AUDIOCHANNEL_H_INCLUDED
#define SCHPUNE_NESCORE_AUDIOCHANNEL_H_INCLUDED

#include <vector>
#include "schpunetypes.h"


namespace schcore
{
    class AudioBuilder;

    class AudioChannel
    {
    public:
        virtual ~AudioChannel() {}

    protected:
        std::vector<float>      outputLevels[2];
        

        void                changeOutput( timestamp_t tick, int out );

    private:
        int                 prevOut;
        AudioBuilder*       builder;
    };


}

#endif