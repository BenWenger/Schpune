
#ifndef SCHPUNE_NESCORE_RESETINFO_H_INCLUDED
#define SCHPUNE_NESCORE_RESETINFO_H_INCLUDED

#include "schpunetypes.h"


namespace schcore
{
    class           CpuBus;

    struct ResetInfo
    {
        bool        hardReset;
        bool        suppressIrqs;

        CpuBus*     cpuBus;
    };
}

#endif