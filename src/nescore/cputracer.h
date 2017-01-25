
#ifndef SCHPUNE_NESCORE_CPUTRACER_H_INCLUDED
#define SCHPUNE_NESCORE_CPUTRACER_H_INCLUDED

#include <iostream>
#include "schpunetypes.h"


namespace schcore
{
    class CpuState;
    class CpuBus;

    class CpuTracer
    {
    public:
        void                    supplyBus(const CpuBus* cpubus)         { bus = cpubus;             }
        void                    setOutputStream(std::ostream* stream)   { streamOutput = stream;    }

        inline bool             isOn() const            { return active && bus && streamOutput;     }
        inline                  operator bool() const   { return isOn();            }

        inline bool             isEnabled() const       { return active;            }

        void                    enable()                { active = true;                        }
        void                    disable()               { active = false;                       }
        void                    toggle()                { active = !active;                     }
        
        void                    traceCpuLine(const CpuState& stt);
        void                    traceRawLine(const std::string& str);

    private:
        bool                    active = false;
        std::ostream*           streamOutput = nullptr;
        const CpuBus*           bus = nullptr;
    };
}

#endif