
#ifndef SCHPUNE_NESCORE_RESETINFO_H_INCLUDED
#define SCHPUNE_NESCORE_RESETINFO_H_INCLUDED

#include "schpunetypes.h"
#include "regioninfo.h"


namespace schcore
{
    class           Apu;
    class           Cpu;
    class           CpuBus;
    class           AudioBuilder;
    class           CpuTracer;
    class           DmaUnit;

    class           EventManager;

    class ResetInfo
    {
    public:
        bool            hardReset;
        bool            suppressIrqs;

        CpuTracer*      cpuTracer;
        Cpu*            cpu;
        Apu*            apu;
        CpuBus*         cpuBus;
        AudioBuilder*   audioBuilder;
        DmaUnit*        dmaUnit;
        EventManager*   eventManager;

        RegionInfo      region;
    };
}

#endif