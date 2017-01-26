
#ifndef SCHPUNE_NESCORE_EVENTMANAGER_H_INCLUDED
#define SCHPUNE_NESCORE_EVENTMANAGER_H_INCLUDED

#include <map>
#include "schpunetypes.h"

namespace schcore
{
    enum EventType
    {
        evt_apu         = (1<<0),
        evt_ppu         = (1<<1),
        evt_mpr         = (1<<2)
    };

    class ResetInfo;
    class SubSystem;
    class CpuTracer;


    class EventManager
    {
    public:
        void            reset(const ResetInfo& info);

        void            subtractFromTimestamps(timestamp_t sub);
        void            check(timestamp_t checktime);

        void            addEvent(timestamp_t time, EventType subsystem);

    private:
        SubSystem*                  apu;
        SubSystem*                  ppu;
        SubSystem*                  mpr;
        std::map<timestamp_t, int>  events;
        timestamp_t                 nextEvent;
    };


}

#endif