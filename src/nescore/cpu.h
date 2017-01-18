
#ifndef SCHPUNE_NESCORE_CPU_H_INCLUDED
#define SCHPUNE_NESCORE_CPU_H_INCLUDED

#include "schpunetypes.h"
#include "cpustate.h"
#include "subsystem.h"


namespace schcore
{
    class ResetInfo;
    class CpuBus;
    class CpuTracer;
    class EventManager;

    class Cpu : public SubSystem
    {
    public:
        //////////////////////////////////////////////////
        //  Resetting
        void                reset(const ResetInfo& info);


        //////////////////////////////////////////////////
        //  Stuff for NSFs
        void                primeNsf(u8 A, u8 X, u16 PC);
        void                unjam()                                     { cpuJammed = false;            }

        //////////////////////////////////////////////////
        //  Running
        virtual void        run(timestamp_t runto) override;

        
        virtual void        endFrame(timestamp_t subadjust) override    { subtractFromMainTimestamp(subadjust);     }

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

        CpuTracer*          tracer;
        CpuBus*             bus;
        EventManager*       eventManager;
        CpuState            cpu;

        void                pollInterrupt();
        bool                wantInterrupt;
        bool                wantReset;
        bool                cpuJammed;

        void                performInterrupt(bool sw);

        typedef     void (Cpu::*rmw_op)(u8&);

        #define SCHPUNE_NESCORE_CPU_H___SUBHEADERSOK
        #include "cpu_addrmodes.h"
        #include "cpu_instructions.h"
        #undef SCHPUNE_NESCORE_CPU_H___SUBHEADERSOK

    };


}

#endif