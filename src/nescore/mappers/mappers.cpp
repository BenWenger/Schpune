
#include "schpunetypes.h"
#include "mappers.h"

////////////////////////////////////////
#include "nrom.h"

namespace schcore
{
    namespace mpr
    {
        std::unique_ptr<Cartridge> buildCartridgeFromFile(NesFile& file)
        {
            // TODO - actually impliment this
            std::unique_ptr<Cartridge> out;

            //...
            out = std::make_unique<Nrom>();
            //...

            out->load(file);

            return std::move(out);
        }
    }

}