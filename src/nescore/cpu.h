
#ifndef SCHPUNE_NESCORE_CPU_H_INCLUDED
#define SCHPUNE_NESCORE_CPU_H_INCLUDED

#include "schpunetypes.h"
#include "cpustate.h"
#include "subsystem.h"


namespace schcore
{
    struct ResetInfo;
    class CpuBus;

    class Cpu : public SubSystem
    {
    public:
        //////////////////////////////////////////////////
        //  Resetting
        void                reset(const ResetInfo& info);

        //////////////////////////////////////////////////
        //  Running
        virtual void        run(timestamp_t runto) override;

    private:
        //////////////////////////////////////////
        //  Interface for CpuBus:  consuming a cycle
        //    CpuBus will call this for every read/write, even those initiated by other
        //  subsystems (DMA unit)
        friend class CpuBus;
        void                consumeCycle();


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