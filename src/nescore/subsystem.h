
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

    protected:
        void            cyc()           { timestamp += clockBase;           }
        void            cyc(int cycs)   { timestamp += cycs * clockBase;    }

        timestamp_t     curCyc() const  { return timestamp;                 }

    private:
        timestamp_t     timestamp;
        timestamp_t     clockBase;
    };


}

#endif