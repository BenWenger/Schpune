
#include "ppubus.h"
#include "resetinfo.h"

namespace schcore
{
    // TODO -- all of this
    void PpuBus::write(u16 a, u8 v)
    {
        ioDevice->onPpuWrite(a, v);
    }

    u8 PpuBus::read(u16 a)
    {
        // TODO simulate bus?
        u8 v = 0;
        ioDevice->onPpuRead(a, v);
        return v;
    }

    void PpuBus::reset(const ResetInfo& info)
    {
        if(info.hardReset)
        {
        }
    }

}