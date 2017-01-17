
#ifndef SCHPUNE_NESCORE_AUDIOSETTINGS_H_INCLUDED
#define SCHPUNE_NESCORE_AUDIOSETTINGS_H_INCLUDED

#include "schpunetypes.h"

namespace schcore
{
    struct ChannelSettings
    {
        float           vol = 1.0;
        float           pan = 0.0;
    };

    enum ChannelId
    {
        pulse0 =        0,
        pulse1,
        triangle,
        noise,
        dmc,


        count           // must be last
    };

    struct AudioSettings
    {
        int                 sampleRate      = 48000;
        bool                stereo          = false;
        float               masterVol       = 1.0f;
        bool                nonLinearPulse  = true;

        ChannelSettings     chans[ChannelId::count];
    };
}

#endif