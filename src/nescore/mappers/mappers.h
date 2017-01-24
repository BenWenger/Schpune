
#ifndef SCHPUNE_NESCORE_MAPPERS_MAPPERS_H_INCLUDED
#define SCHPUNE_NESCORE_MAPPERS_MAPPERS_H_INCLUDED

#include <memory>

namespace schcore
{
    class Cartridge;
    class NesFile;

    namespace mpr
    {
        std::unique_ptr<Cartridge>  buildCartridgeFromFile(NesFile& file);
    }
}


#endif