
#include "eventmanager.h"
#include "subsystem.h"
#include "resetinfo.h"
#include "apu.h"
#include "ppu.h"

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
        if(checktime >= nextEvent)
        {
            nextEvent = Time::Never;

            auto i = events.begin();
            while(i != events.end())
            {
                if(i->first > checktime)
                {
                    nextEvent = i->first;
                    break;
                }
                
                ///////////////////////

                // TODO -- add all subsystems here
                if(i->second & EventType::evt_apu)      apu->run(checktime);
                if(i->second & EventType::evt_ppu)      ppu->run(checktime);

                ///////////////////////
                i = events.erase(i);
            }
        }
    }

    void EventManager::addEvent(timestamp_t time, EventType subsystem)
    {
        if(!subsystem)                  return;

        if(time < nextEvent)            nextEvent = time;
        events[time] |= subsystem;
    }

}