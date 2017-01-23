
#ifndef SCHPUNE_NESCORE_NSFDRIVER_H_INCLUDED
#define SCHPUNE_NESCORE_NSFDRIVER_H_INCLUDED

#include <memory>
#include <vector>
#include "cartridge.h"

namespace schcore
{
    class ExAudio;

    class NsfDriver : public Cartridge
    {
    public:
                        NsfDriver();
                        ~NsfDriver();

        void            changeTrack();
        u16             getTrackStartAddr() const        { return driverCodeAddr;               }
        
    protected:
        virtual void            cartLoad(NesFile& file) override;
        virtual void            cartReset(const ResetInfo& info) override;

    private:
        static const int        driverCodeSize = 0x20;
        static const u16        driverCodeAddr = static_cast<u16>(0x4000 - driverCodeSize);
        u8                      driverCode[driverCodeSize];
        u8                      bankswappingValues[10];

        static const u8         driverCodePrimer[driverCodeSize];

        bool                    isFdsTune() const;

        // TODO expansion audio channels
        void            doNsfPrgSwap(int slot, u8 v);
        void            onReadNsfDriver(u16 a, u8& v);
        int             onPeekNsfDriver(u16 a) const;
        void            onWriteBankswap(u16 a, u8 v);

        Apu*                    apu;

        std::vector<std::unique_ptr<ExAudio>>   expansion;
    };

}

#endif
