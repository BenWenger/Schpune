
#ifndef SCHPUNE_NESCORE_REGIONINFO_H_INCLUDED
#define SCHPUNE_NESCORE_REGIONINFO_H_INCLUDED

#include "schpunetypes.h"

namespace schcore
{
    struct RegionInfo
    {
        timestamp_t         cpuClockBase;
        timestamp_t         apuClockBase;
        timestamp_t         ppuClockBase;
        
        timestamp_t         masterCyclesPerSecond;
        timestamp_t         masterCyclesPerFrame;
    };
}

#endif