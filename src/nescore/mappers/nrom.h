
#ifndef SCHPUNE_NESCORE_MAPPERS_NROM_H_INCLUDED
#define SCHPUNE_NESCORE_MAPPERS_NROM_H_INCLUDED

#include "../cartridge.h"

namespace schcore
{
    namespace mpr
    {
        class Nrom : public Cartridge
        {
        protected:
            virtual void cartReset(const ResetInfo& info) override
            {
                setPrgCallbacks(0x8,0xF,-1,-1,false);
                swapPrg_32k(8, ~0);
                swapChr_8k(0,0);
            }
        };
    }
}


#endif