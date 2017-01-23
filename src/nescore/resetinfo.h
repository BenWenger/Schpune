
#ifndef SCHPUNE_NESCORE_RESETINFO_H_INCLUDED
#define SCHPUNE_NESCORE_RESETINFO_H_INCLUDED

#include "schpunetypes.h"
#include "regioninfo.h"


namespace schcore
{
    class           Apu;
    class           Cpu;
    class           Ppu;
    class           CpuBus;
    class           PpuBus;
    class           AudioBuilder;
    class           CpuTracer;
    class           DmaUnit;
    class           NesFile;

    class           EventManager;

    class ResetInfo
    {
    public:
        bool            hardReset;
        bool            suppressIrqs;

        CpuTracer*      cpuTracer;
        Cpu*            cpu;
        Ppu*            ppu;
        Apu*            apu;
        CpuBus*         cpuBus;
        PpuBus*         ppuBus;
        AudioBuilder*   audioBuilder;
        DmaUnit*        dmaUnit;
        EventManager*   eventManager;
        NesFile*        nesfile;

        RegionInfo      region;
    };
}

#endif