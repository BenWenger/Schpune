
#include "cartridge.h"
#include "cpu.h"
#include "cpubus.h"
#include "ppubus.h"
#include "apu.h"
#include "ppu.h"

namespace schcore
{
    void Cartridge::reset(const ResetInfo& info)
    {
        cpuBus = info.cpuBus;
        ppuBus = info.ppuBus;
        subSystem_HardReset( info.cpu, info.region.cpuClockBase );

        cartReset(info);
    }

    void Cartridge::setPrgCallbacks(int readablestart, int readablestop, int writablestart, int writablestop, bool addprgram)
    {
        cpuBus->addPeeker( readablestart, readablestop, this, &Cartridge::onPeekPrg );
        cpuBus->addReader( readablestart, readablestop, this, &Cartridge::onReadPrg );
        cpuBus->addWriter( writablestart, writablestop, this, &Cartridge::onWritePrg );

        if(addprgram && !loadedFile->prgRamChips.empty())
            swapPrg_8k( 6, 0, true );
    }

    void Cartridge::clearPrgRam(u8 v)
    {
        for(auto& chip : loadedFile->prgRamChips)
        {
            if(chip.hasBattery)     continue;       // don't wipe battery chips
            chip.clear(v);
        }
    }

    void Cartridge::syncApu()
    {
        apu->catchUp();
    }
    
    void Cartridge::syncPpu()
    {
        ppu->catchUp();
    }

    void Cartridge::swapPrg_4k(int slot, int page, bool ram)
    {
        if(ram && !loadedFile->prgRamChips.empty())
            prgPages[slot & 0x0F] = loadedFile->prgRamChips.front().get4kPage(page);
        else
            prgPages[slot & 0x0F] = loadedFile->prgRomChips.front().get4kPage(page);
    }

    void Cartridge::swapPrg_8k(int slot, int page, bool ram)
    {
        page <<= 1;
        if(ram && !loadedFile->prgRamChips.empty())
        {
            prgPages[ slot    & 0x0F] = loadedFile->prgRamChips.front().get4kPage(page);
            prgPages[(slot+1) & 0x0F] = loadedFile->prgRamChips.front().get4kPage(page+1);
        }
        else
        {
            prgPages[ slot    & 0x0F] = loadedFile->prgRomChips.front().get4kPage(page);
            prgPages[(slot+1) & 0x0F] = loadedFile->prgRomChips.front().get4kPage(page+1);
        }
    }

    
    void Cartridge::onReadPrg(u16 a, u8& v)     { prgPages[a>>12].memRead(a,v);         }
    void Cartridge::onWritePrg(u16 a, u8 v)     { prgPages[a>>12].memWrite(a,v);        }
    int  Cartridge::onPeekPrg(u16 a) const      { return prgPages[a>>12].memPeek(a);    }
}

