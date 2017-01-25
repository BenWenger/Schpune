
#include "eventmanager.h"
#include "subsystem.h"
#include "resetinfo.h"
#include "apu.h"
#include "ppu.h"
#include "cputracer.h"

namespace schcore
{
    
    void EventManager::reset(const ResetInfo& info)
    {
        if(info.hardReset)
        {
            apu =           info.apu;
            ppu =           info.ppu;

            events.clear();
            nextEvent = Time::Never;
        }
    }

    void EventManager::subtractFromTimestamps(timestamp_t sub)
    {
        if(events.empty())
            return;

        decltype(events)    newevents;

        for(auto& i : events)
        {
            newevents[i.first - sub] = i.second;
        }

        events = std::move(newevents);
        nextEvent -= sub;
    }

    void EventManager::check(timestamp_t checktime)
    {
        // checktime must be greater (not equal) for us to trigger it.
        //  The idea being the cycle in question has to actually execute for the event to happen...
        //  and timestamps represent the next cycle to execute.  So if your timestamp is '5', cycle 5 is the
        //  next one, but hasn't yet happened.
        if(checktime > nextEvent)
        {
            nextEvent = Time::Never;

            auto i = events.begin();
            int toupdate = 0;
            while(i != events.end())
            {
                if(i->first >= checktime)
                {
                    nextEvent = i->first;
                    break;
                }
                toupdate |= i->second;
                i = events.erase(i);
            }
            
            // TODO -- add all subsystems here
            if(toupdate & EventType::evt_apu)   apu->run(checktime);
            if(toupdate & EventType::evt_ppu)   ppu->run(checktime);
        }
    }

    void EventManager::addEvent(timestamp_t time, EventType subsystem)
    {
        if(!subsystem)                  return;

        if(time < nextEvent)            nextEvent = time;
        events[time] |= subsystem;
    }

}