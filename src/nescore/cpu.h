
#ifndef SCHPUNE_NESCORE_CPU_H_INCLUDED
#define SCHPUNE_NESCORE_CPU_H_INCLUDED

#include "schpunetypes.h"
#include "cpustate.h"


namespace schcore
{
    struct ResetInfo;
    class CpuBus;

    class Cpu
    {
    public:
        //////////////////////////////////////////////////
        //  Resetting
        void                reset(const ResetInfo& info);

        //////////////////////////////////////////////////
        //  Running
        void                run(timestamp_t runto);

        timestamp_t         curCyc(); // TODO move to subsystem


    private:
        u8                  rd(u16 a);
        void                wr(u16 a, u8 v);

        u8                  pull();
        void                push(u8 v);

        CpuBus*             bus;
        CpuState            cpu;

        void                pollInterrupt();
        bool                wantInterrupt;
        bool                wantReset;

        void                performInterrupt(bool sw);

        typedef     void (Cpu::*rmw_op)(u8&);

        #define SCHPUNE_NESCORE_CPU_H___SUBHEADERSOK
        #include "cpu_addrmodes.h"
        #include "cpu_instructions.h"
        #undef SCHPUNE_NESCORE_CPU_H___SUBHEADERSOK

    };


}

#endif