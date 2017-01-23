
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
        subSystem_HardReset( info.cpu, info.region.cpuClockBase );
        if(info.hardReset)
        {
            info.ppuBus->setIoDevice(this);
            mir_hdr();
        }

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

    /////////////////////////////////////////////
    // PRG
    void Cartridge::swapPrg_4k(int slot, int page, bool ram)
    {
        apu->catchUp();
        if(ram && !loadedFile->prgRamChips.empty())
            prgPages[slot & 0x0F] = loadedFile->prgRamChips.front().get4kPage(page);
        else
            prgPages[slot & 0x0F] = loadedFile->prgRomChips.front().get4kPage(page);
    }
    void Cartridge::swapPrg_8k(int slot, int page, bool ram)
    {
        apu->catchUp();
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
    void Cartridge::swapPrg_16k(int slot, int page, bool ram)
    {
        apu->catchUp();
        page <<= 2;
        if(ram && !loadedFile->prgRamChips.empty())
        {
            prgPages[ slot    & 0x0F] = loadedFile->prgRamChips.front().get4kPage(page);
            prgPages[(slot+1) & 0x0F] = loadedFile->prgRamChips.front().get4kPage(page+1);
            prgPages[(slot+2) & 0x0F] = loadedFile->prgRamChips.front().get4kPage(page+2);
            prgPages[(slot+3) & 0x0F] = loadedFile->prgRamChips.front().get4kPage(page+3);
        }
        else
        {
            prgPages[ slot    & 0x0F] = loadedFile->prgRomChips.front().get4kPage(page);
            prgPages[(slot+1) & 0x0F] = loadedFile->prgRomChips.front().get4kPage(page+1);
            prgPages[(slot+2) & 0x0F] = loadedFile->prgRomChips.front().get4kPage(page+2);
            prgPages[(slot+3) & 0x0F] = loadedFile->prgRomChips.front().get4kPage(page+3);
        }
    }
    void Cartridge::swapPrg_32k(int slot, int page, bool ram)
    {
        apu->catchUp();
        page <<= 3;
        if(ram && !loadedFile->prgRamChips.empty())
        {
            prgPages[ slot    & 0x0F] = loadedFile->prgRamChips.front().get4kPage(page);
            prgPages[(slot+1) & 0x0F] = loadedFile->prgRamChips.front().get4kPage(page+1);
            prgPages[(slot+2) & 0x0F] = loadedFile->prgRamChips.front().get4kPage(page+2);
            prgPages[(slot+3) & 0x0F] = loadedFile->prgRamChips.front().get4kPage(page+3);
            prgPages[(slot+4) & 0x0F] = loadedFile->prgRamChips.front().get4kPage(page+4);
            prgPages[(slot+5) & 0x0F] = loadedFile->prgRamChips.front().get4kPage(page+5);
            prgPages[(slot+6) & 0x0F] = loadedFile->prgRamChips.front().get4kPage(page+6);
            prgPages[(slot+7) & 0x0F] = loadedFile->prgRamChips.front().get4kPage(page+7);
        }
        else
        {
            prgPages[ slot    & 0x0F] = loadedFile->prgRomChips.front().get4kPage(page);
            prgPages[(slot+1) & 0x0F] = loadedFile->prgRomChips.front().get4kPage(page+1);
            prgPages[(slot+2) & 0x0F] = loadedFile->prgRomChips.front().get4kPage(page+2);
            prgPages[(slot+3) & 0x0F] = loadedFile->prgRomChips.front().get4kPage(page+3);
            prgPages[(slot+4) & 0x0F] = loadedFile->prgRomChips.front().get4kPage(page+4);
            prgPages[(slot+5) & 0x0F] = loadedFile->prgRomChips.front().get4kPage(page+5);
            prgPages[(slot+6) & 0x0F] = loadedFile->prgRomChips.front().get4kPage(page+6);
            prgPages[(slot+7) & 0x0F] = loadedFile->prgRomChips.front().get4kPage(page+7);
        }
    }

    ///////////////////////////////////////
    //  CHR
    void Cartridge::swapChr_1k(int slot, int page, bool ram)
    {
        if(loadedFile->prgRomChips.empty()) ram = true;
        if(loadedFile->prgRamChips.empty()) ram = false;
        auto& chip = (ram ? loadedFile->chrRamChips.front() : loadedFile->chrRomChips.front());

        ppu->catchUp();
        chrPages[slot & 0x07] = chip.get1kPage(page);
    }
    void Cartridge::swapChr_2k(int slot, int page, bool ram)
    {
        if(loadedFile->prgRomChips.empty()) ram = true;
        if(loadedFile->prgRamChips.empty()) ram = false;
        auto& chip = (ram ? loadedFile->chrRamChips.front() : loadedFile->chrRomChips.front());

        ppu->catchUp();
        page <<= 1;
        chrPages[ slot    & 0x07] = chip.get1kPage(page);
        chrPages[(slot+1) & 0x07] = chip.get1kPage(page+1);
    }
    void Cartridge::swapChr_4k(int slot, int page, bool ram)
    {
        if(loadedFile->prgRomChips.empty()) ram = true;
        if(loadedFile->prgRamChips.empty()) ram = false;
        auto& chip = (ram ? loadedFile->chrRamChips.front() : loadedFile->chrRomChips.front());

        ppu->catchUp();
        page <<= 2;
        chrPages[ slot    & 0x07] = chip.get1kPage(page);
        chrPages[(slot+1) & 0x07] = chip.get1kPage(page+1);
        chrPages[(slot+2) & 0x07] = chip.get1kPage(page+2);
        chrPages[(slot+3) & 0x07] = chip.get1kPage(page+3);
    }
    void Cartridge::swapChr_8k(int slot, int page, bool ram)
    {
        if(loadedFile->prgRomChips.empty()) ram = true;
        if(loadedFile->prgRamChips.empty()) ram = false;
        auto& chip = (ram ? loadedFile->chrRamChips.front() : loadedFile->chrRomChips.front());

        ppu->catchUp();
        page <<= 2;
        chrPages[ slot    & 0x07] = chip.get1kPage(page);
        chrPages[(slot+1) & 0x07] = chip.get1kPage(page+1);
        chrPages[(slot+2) & 0x07] = chip.get1kPage(page+2);
        chrPages[(slot+3) & 0x07] = chip.get1kPage(page+3);
        chrPages[(slot+4) & 0x07] = chip.get1kPage(page+4);
        chrPages[(slot+5) & 0x07] = chip.get1kPage(page+5);
        chrPages[(slot+6) & 0x07] = chip.get1kPage(page+6);
        chrPages[(slot+7) & 0x07] = chip.get1kPage(page+7);
    }

    ////////////////////////////////////////////////
    //  Mirroring
    void Cartridge::mir_horz()
    {
        ppu->catchUp();
        ntPages[0] = ntPages[1] = ppu->getNt(0);
        ntPages[2] = ntPages[3] = ppu->getNt(1);
    }
    void Cartridge::mir_vert()
    {
        ppu->catchUp();
        ntPages[0] = ntPages[2] = ppu->getNt(0);
        ntPages[1] = ntPages[3] = ppu->getNt(1);
    }
    void Cartridge::mir_hdr()
    {
        switch(loadedFile->headerMirroring)
        {
        default:
        case NesFile::Mirror::Horz:     mir_horz();     break;
        case NesFile::Mirror::Vert:     mir_vert();     break;
        }
    }

    void Cartridge::onReadPrg(u16 a, u8& v)     { prgPages[a>>12].memRead(a,v);         }
    void Cartridge::onWritePrg(u16 a, u8 v)     { prgPages[a>>12].memWrite(a,v);        }
    int  Cartridge::onPeekPrg(u16 a) const      { return prgPages[a>>12].memPeek(a);    }
    
    void Cartridge::onPpuWrite(u16 a, u8 v)
    {
        if(a & 0x2000)      ntPages[(a >> 10) & 3].memWrite(a,v);
        else                chrPages[(a >> 10) & 7].memWrite(a,v);
    }
    void Cartridge::onPpuRead(u16 a, u8& v)
    {
        if(a & 0x2000)      ntPages[(a >> 10) & 3].memRead(a,v);
        else                chrPages[(a >> 10) & 7].memRead(a,v);
    }
}

