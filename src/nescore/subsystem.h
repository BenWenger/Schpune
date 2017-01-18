
#ifndef SCHPUNE_NESCORE_SUBSYSTEM_H_INCLUDED
#define SCHPUNE_NESCORE_SUBSYSTEM_H_INCLUDED

#include "schpunetypes.h"
#include "cpustate.h"


namespace schcore
{
    class SubSystem
    {
    public:
        virtual         ~SubSystem()            { }

        virtual void    run(timestamp_t runto) = 0;

        timestamp_t     curCyc() const                  { return timestamp;             }
        timestamp_t     getClockBase() const            { return clockBase;             }
        void            setClockBase(timestamp_t base)  { clockBase = base;             }

        virtual void    endFrame(timestamp_t subadjust) = 0;
        void            setMainTimestamp(timestamp_t set)           { timestamp = set;      }

    protected:
        void            subtractFromMainTimestamp(timestamp_t sub)  { timestamp -= sub;     }
        void            cyc()           { timestamp += clockBase;           }
        void            cyc(int cycs)   { timestamp += cycs * clockBase;    }
        void            catchUp()       { run( drivingClock->curCyc() );    }

        timestamp_t     unitsToTimestamp(timestamp_t target)
        {
            return ((target - timestamp) + (clockBase - 1)) / clockBase;
        }

        void            subSystem_HardReset(SubSystem* drivingclock, timestamp_t clkbase)
        {
            timestamp = 0;
            clockBase = clkbase;
            drivingClock = drivingclock;
        }

    private:
        timestamp_t     timestamp;
        timestamp_t     clockBase;
        SubSystem*      drivingClock;
    };


}

#endif