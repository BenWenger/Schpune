
#ifndef SCHPUNE_NESCORE_PPUBUS_H_INCLUDED
#define SCHPUNE_NESCORE_PPUBUS_H_INCLUDED

#include <string>
#include <functional>
#include <vector>
#include "schpunetypes.h"


namespace schcore
{
    class ResetInfo;

    class PpuIo
    {
    public:
        virtual     ~PpuIo() {}
        virtual     void onPpuWrite(u16 a, u8 v) = 0;
        virtual     void onPpuRead(u16 a, u8& v) = 0;
    };

    class PpuBus
    {
    public:
        void        write(u16 a, u8 v);
        u8          read(u16 a);

        void        reset(const ResetInfo& info);

        void        setIoDevice(PpuIo* device)      { ioDevice = device;        }

    private:
        PpuIo*      ioDevice;
    };


}

#endif