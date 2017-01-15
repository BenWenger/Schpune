#include "cpubus.h"
#include "resetinfo.h"
#include "dmc_supplier.h"


namespace schcore
{
    void Dmc_PeekSampleBuffer::reset(const ResetInfo& info)
    {
        val = 0;
        hasVal = false;

        if(info.hardReset)
            bus = info.cpuBus;
    }

    void Dmc_PeekSampleBuffer::triggerFetch(u16 addr)
    {
        hasVal = true;

        int t = bus->peek(addr);
        if(t < 0)           // hopefully this won't happen.  If we're unable to peek, we have to assume open bus, let's
            val = static_cast<u8>(addr >> 8);       // just assume high byte of addr would be on bus
        else
            val = static_cast<u8>(t);
    }

    void Dmc_PeekSampleBuffer::getFetchedByte(u8& byte, bool& hasval)
    {
        byte = val;
        hasval = hasVal;

        hasVal = false;
    }
}