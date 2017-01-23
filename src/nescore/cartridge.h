
#ifndef SCHPUNE_NESCORE_CARTRIDGE_H_INCLUDED
#define SCHPUNE_NESCORE_CARTRIDGE_H_INCLUDED

#include "subsystem.h"
#include "resetinfo.h"
#include "nesfile.h"
#include "error.h"

namespace schcore
{
    class Apu;
    class Ppu;
    class CpuBus;
    class PpuBus;
    class Cartridge : public SubSystem
    {
    public:
        virtual ~Cartridge() { }

        virtual void run(timestamp_t runto) {}          // can override
        virtual void reset(const ResetInfo& info);      // should not override -- override cartReset instead
        void load(NesFile& file)                        // should not override -- override cartLoad instead
        {
            loadedFile = &file;
            cartLoad(file);
        }
        
    protected:
        NesFile*        loadedFile;
        virtual void    cartLoad(NesFile& file) {}
        virtual void    cartReset(const ResetInfo& info) {}

        ////////////////////////////////////////////////
        //  Useful on reset
        void            setPrgCallbacks(int readablestart, int readablestop, int writablestart, int writablestop);
        void            clearPrgRam(u8 v = 0);

        ////////////////////////////////////////////////
        //  Syncing other subsystems
        void            syncApu();
        void            syncPpu();

        ////////////////////////////////////////////////
        //  Bankswapping
        void            swapPrg_4k(int slot, int page);



    private:
        void            onReadPrg(u16 a, u8& v);
        int             onPeekPrg(u16 a) const;
        void            onWritePrg(u16 a, u8 v);
        CpuBus*         cpuBus;
        PpuBus*         ppuBus;
        Apu*            apu;
        Ppu*            ppu;
        ChipPage        prgPages[0x10];
        ChipPage        chrPages[0x08];
        ChipPage        ntPages[0x04];
    };

}

#endif
