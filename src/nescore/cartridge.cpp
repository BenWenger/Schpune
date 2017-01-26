
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
        apu = info.apu;
        ppu = info.ppu;
        cpuBus = info.cpuBus;
        subSystem_HardReset( info.cpu, info.region.cpuClockBase );
        if(info.hardReset)
        {
            info.ppuBus->setIoDevice(this);
            mir_hdr();
        }

        cartReset(info);
    }

    u8 Cartridge::busConflict(u16 a, u8 v)
    {
        u8 out = v;
        onReadPrg(a, v);
        return out & v;
    }

    void Cartridge::setDefaultPrgCallbacks()
    {
        cpuBus->addPeeker( 0x6, 0xF, this, &Cartridge::onPeekPrg );
        cpuBus->addReader( 0x6, 0xF, this, &Cartridge::onReadPrg );
        cpuBus->addWriter( 0x6, 0x7, this, &Cartridge::onWritePrg );

        swapPrg_8k( 6, 0, true );       // TODO -- is this desired?
    }
    
    void Cartridge::setPrgReaders(int start, int stop)
    {
        cpuBus->addPeeker( start, stop, this, &Cartridge::onPeekPrg );
        cpuBus->addReader( start, stop, this, &Cartridge::onReadPrg );
    }

    void Cartridge::setPrgWriters(int start, int stop)
    {
        cpuBus->addWriter( start, stop, this, &Cartridge::onWritePrg );
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
    // Getting Prg/Chr chips for easy swapping
    MemoryChip* Cartridge::getFirstChrChip(bool preferram)
    {
        if(loadedFile->chrRomChips.empty())     preferram = true;
        if(loadedFile->chrRamChips.empty())     preferram = false;

        auto& chipset = preferram ? loadedFile->chrRamChips : loadedFile->chrRomChips;
        if(chipset.empty())     return &dummyChip;
        return &chipset.front();
    }

    MemoryChip* Cartridge::getFirstPrgChip(bool preferram)
    {
        if(loadedFile->prgRomChips.empty())     preferram = true;
        if(loadedFile->prgRamChips.empty())     preferram = false;

        auto& chipset = preferram ? loadedFile->prgRamChips : loadedFile->prgRomChips;
        if(chipset.empty())     return &dummyChip;
        return &chipset.front();
    }

    /////////////////////////////////////////////
    // PRG
    void Cartridge::swapPrg_4k(int slot, int page, bool ram)
    {
        apu->catchUp();

        auto chip = getFirstPrgChip(ram);
        prgPages[slot & 0x0F] = chip->get4kPage(page);
    }
    void Cartridge::swapPrg_8k(int slot, int page, bool ram)
    {
        apu->catchUp();
        page <<= 1;

        auto chip = getFirstPrgChip(ram);
        prgPages[ slot    & 0x0F] = chip->get4kPage(page);
        prgPages[(slot+1) & 0x0F] = chip->get4kPage(page+1);
    }
    void Cartridge::swapPrg_16k(int slot, int page, bool ram)
    {
        apu->catchUp();
        page <<= 2;

        auto chip = getFirstPrgChip(ram);
        prgPages[ slot    & 0x0F] = chip->get4kPage(page);
        prgPages[(slot+1) & 0x0F] = chip->get4kPage(page+1);
        prgPages[(slot+2) & 0x0F] = chip->get4kPage(page+2);
        prgPages[(slot+3) & 0x0F] = chip->get4kPage(page+3);
    }
    void Cartridge::swapPrg_32k(int slot, int page, bool ram)
    {
        apu->catchUp();
        page <<= 3;

        auto chip = getFirstPrgChip(ram);
        prgPages[ slot    & 0x0F] = chip->get4kPage(page);
        prgPages[(slot+1) & 0x0F] = chip->get4kPage(page+1);
        prgPages[(slot+2) & 0x0F] = chip->get4kPage(page+2);
        prgPages[(slot+3) & 0x0F] = chip->get4kPage(page+3);
        prgPages[(slot+4) & 0x0F] = chip->get4kPage(page+4);
        prgPages[(slot+5) & 0x0F] = chip->get4kPage(page+5);
        prgPages[(slot+6) & 0x0F] = chip->get4kPage(page+6);
        prgPages[(slot+7) & 0x0F] = chip->get4kPage(page+7);
    }

    ///////////////////////////////////////
    //  CHR
    void Cartridge::swapChr_1k(int slot, int page, bool ram)
    {
        ppu->catchUp();
        
        auto chip = getFirstChrChip(ram);
        chrPages[slot & 0x07] = chip->get1kPage(page);
    }
    void Cartridge::swapChr_2k(int slot, int page, bool ram)
    {
        ppu->catchUp();
        page <<= 1;
        
        auto chip = getFirstChrChip(ram);
        chrPages[ slot    & 0x07] = chip->get1kPage(page);
        chrPages[(slot+1) & 0x07] = chip->get1kPage(page+1);
    }
    void Cartridge::swapChr_4k(int slot, int page, bool ram)
    {
        ppu->catchUp();
        page <<= 2;

        auto chip = getFirstChrChip(ram);
        chrPages[ slot    & 0x07] = chip->get1kPage(page);
        chrPages[(slot+1) & 0x07] = chip->get1kPage(page+1);
        chrPages[(slot+2) & 0x07] = chip->get1kPage(page+2);
        chrPages[(slot+3) & 0x07] = chip->get1kPage(page+3);
    }
    void Cartridge::swapChr_8k(int slot, int page, bool ram)
    {
        ppu->catchUp();
        page <<= 3;

        auto chip = getFirstChrChip(ram);
        chrPages[ slot    & 0x07] = chip->get1kPage(page);
        chrPages[(slot+1) & 0x07] = chip->get1kPage(page+1);
        chrPages[(slot+2) & 0x07] = chip->get1kPage(page+2);
        chrPages[(slot+3) & 0x07] = chip->get1kPage(page+3);
        chrPages[(slot+4) & 0x07] = chip->get1kPage(page+4);
        chrPages[(slot+5) & 0x07] = chip->get1kPage(page+5);
        chrPages[(slot+6) & 0x07] = chip->get1kPage(page+6);
        chrPages[(slot+7) & 0x07] = chip->get1kPage(page+7);
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
    void Cartridge::mir_1scr(int scr)
    {
        ppu->catchUp();
        ntPages[0] = ntPages[2] = 
        ntPages[1] = ntPages[3] = ppu->getNt(scr);
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
    
    void Cartridge::mir_chrPage(int slot, int page, bool ram)
    {
        if(loadedFile->chrRomChips.empty()) ram = true;
        if(loadedFile->chrRamChips.empty()) ram = false;
        auto& chip = (ram ? loadedFile->chrRamChips.front() : loadedFile->chrRomChips.front());

        ppu->catchUp();
        ntPages[slot & 0x03] = chip.get1kPage(page);
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


    void Cartridge::prgRamEnable(int v)
    {
        for(auto& i : loadedFile->prgRamChips)
        {
            i.readable = i.writable = (v != 0);
        }
    }
}

