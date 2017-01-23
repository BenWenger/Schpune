
#ifndef SCHPUNE_NESCORE_PPUBUS_H_INCLUDED
#define SCHPUNE_NESCORE_PPUBUS_H_INCLUDED

#include <string>
#include <functional>
#include <vector>
#include "schpunetypes.h"


namespace schcore
{
    class ResetInfo;

    class PpuBus
    {
    public:
        void        write(u16 a, u8 v);
        u8          read(u16 a);

        void        reset(const ResetInfo& info);
    };


}

#endif