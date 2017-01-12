
#ifndef SCHPUNE_NESCORE_AUDIOTIMESTAMPHOLDER_H_INCLUDED
#define SCHPUNE_NESCORE_AUDIOTIMESTAMPHOLDER_H_INCLUDED

#include "schpunetypes.h"


namespace schcore
{
    class AudioTimestampHolder
    {
    public:
        virtual void        subtractFromAudioTimestamp(timestamp_t sub) = 0;
    };

}

#endif
