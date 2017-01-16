
#ifndef SCHPUNE_NESCORE_RESETINFO_H_INCLUDED
#define SCHPUNE_NESCORE_RESETINFO_H_INCLUDED

#include "schpunetypes.h"
#include "regioninfo.h"


namespace schcore
{
    class           Cpu;
    class           CpuBus;
    class           AudioBuilder;
    class           CpuTracer;
    class           DmaUnit;

    class ResetInfo
    {
    public:
        bool            hardReset;
        bool            suppressIrqs;

        CpuTracer*      cpuTracer;
        Cpu*            cpu;
        CpuBus*         cpuBus;
        AudioBuilder*   audioBuilder;
        DmaUnit*        dmaUnit;

        RegionInfo      region;
    };
}

#endif