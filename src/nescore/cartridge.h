
#ifndef SCHPUNE_NESCORE_CARTRIDGE_H_INCLUDED
#define SCHPUNE_NESCORE_CARTRIDGE_H_INCLUDED

#include "subsystem.h"
#include "resetinfo.h"
#include "nesfile.h"
#include "error.h"
#include "ppubus.h"
#include "cpubus.h"

namespace schcore
{
    class Apu;
    class Ppu;
    class CpuBus;
    class Cartridge : public SubSystem, public PpuIo
    {
    public:
                Cartridge() {}
        virtual ~Cartridge() { }

        virtual void run(timestamp_t runto) {}          // can override
        virtual void reset(const ResetInfo& info);      // should not override -- override cartReset instead
        void load(NesFile& file)                        // should not override -- override cartLoad instead
        {
            loadedFile = &file;
            cartLoad(file);
        }
        
        // PpuIo stuff -- can override
        virtual     void onPpuWrite(u16 a, u8 v) override;
        virtual     void onPpuRead(u16 a, u8& v) override;
        
    protected:
        NesFile*        loadedFile;
        virtual void    cartLoad(NesFile& file) {}
        virtual void    cartReset(const ResetInfo& info) = 0;

        ////////////////////////////////////////////////
        //  Useful on reset
        void            setDefaultPrgCallbacks();
        void            setPrgReaders(int start, int stop);
        void            setPrgWriters(int start, int stop);
        void            clearPrgRam(u8 v = 0);

        ////////////////////////////////////////////////
        //  Syncing other subsystems
        void            syncApu();
        void            syncPpu();

        ////////////////////////////////////////////////
        //  Bankswapping
        void            swapPrg_4k(int slot, int page, bool ram = false);
        void            swapPrg_8k(int slot, int page, bool ram = false);
        void            swapPrg_16k(int slot, int page, bool ram = false);
        void            swapPrg_32k(int slot, int page, bool ram = false);
        
        void            swapChr_1k(int slot, int page, bool ram = false);
        void            swapChr_2k(int slot, int page, bool ram = false);
        void            swapChr_4k(int slot, int page, bool ram = false);
        void            swapChr_8k(int slot, int page, bool ram = false);
        
        void            mir_horz();
        void            mir_vert();
        void            mir_hdr();
        void            mir_1scr(int scr);

        void            mir_chrPage(int slot, int page, bool ram = false);

        ////////////////////////////////////////////////
        //  Other
        u8              busConflict(u16 a, u8 v);
        void            prgRamEnable(int en);



    private:
        void            onReadPrg(u16 a, u8& v);
        int             onPeekPrg(u16 a) const;
        void            onWritePrg(u16 a, u8 v);
        CpuBus*         cpuBus;
        Apu*            apu;
        Ppu*            ppu;
        ChipPage        prgPages[0x10];
        ChipPage        chrPages[0x08];
        ChipPage        ntPages[0x04];

        MemoryChip      dummyChip;

        MemoryChip*     getFirstPrgChip(bool preferram);
        MemoryChip*     getFirstChrChip(bool preferram);
    };

}

#endif
