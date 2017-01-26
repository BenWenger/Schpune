
#include "schpunetypes.h"
#include "mappers.h"
#include "nesfile.h"
#include "../cartridge.h"
#include "../eventmanager.h"

////////////////////////////////////////
#include "../expansion_audio/vrc6.h"

////////////////////////////////////////
#include "vrcirq.h"

////////////////////////////////////////
#include "000.h"
#include "001.h"
#include "002.h"
#include "003.h"
#include "007.h"
#include "024.h"
#include "069.h"

namespace schcore
{
    namespace mpr
    {

        namespace
        {
            std::unique_ptr<Cartridge> inesMapperLoad(NesFile& file)
            {
#define MPR(n)  return std::make_unique<n>
                switch(file.inesMapperNumber)
                {
                case   0:   MPR(Mpr_000)();
                case   1:   MPR(Mpr_001)();
                case   2:   MPR(Mpr_002)();
                case   3:   MPR(Mpr_003)();
                case   7:   MPR(Mpr_007)();
                case  24:   MPR(Mpr_024)(false);
                case  26:   MPR(Mpr_024)(true);
                case  69:   MPR(Mpr_069)();
                }
#undef MPR

                return nullptr;
            }
        }


        std::unique_ptr<Cartridge> buildCartridgeFromFile(NesFile& file)
        {
            switch(file.mapper)
            {
            case MapperId::INES_NUMBER:
                return inesMapperLoad(file);

            default:
            case MapperId::NSF:
                return nullptr;
            }
        }
    }

}