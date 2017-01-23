
#ifndef SCHPUNE_NESCORE_MEMORYCHIP_H_INCLUDED
#define SCHPUNE_NESCORE_MEMORYCHIP_H_INCLUDED

#include <vector>
#include "schpunetypes.h"

namespace schcore
{
    // TODO -- who do I want to own the memory.  Is it the chip?
    //   Does that mean the NesFile class needs to have an array of MemoryChips rather
    //   than a straight memory buffer?  That's probably the best move.  =/
    struct ChipPage
    {
        u8*         mem;
        bool*       readable;
        bool*       writable;
    };

    struct MemoryChip
    {
        std::vector<u8>     ownedData;
        bool                readable;
        bool                writable;
    };
}

#endif
