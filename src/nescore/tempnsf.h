
#ifndef SCHPUNE_NESCORE_TEMPNSF_H_INCLUDED
#define SCHPUNE_NESCORE_TEMPNSF_H_INCLUDED

// this file is temporary -- it's just so I can quickly get NSF files playing!
//
//  Do this in a smarter way using an abstracted cartridge interface
//  And using a System main control thing

#include "schpunetypes.h"
#include <vector>
#include "apu.h"
#include "cpu.h"
#include "cpubus.h"
#include "audiobuilder.h"
#include "cputracer.h"

namespace schcore
{
    class TempNsf
    {
    public:
                        TempNsf();
        bool            load(const char* filename);
        bool            loadTest(const char* filename);
        int             getTrackCount()                         { return rawPrg.empty() ? 0 : totalTracks;  }
        int             getTrack()                              { return currentTrack;                      }
        void            setTrack(int track);

        int             doFrame();                                                  // returns number of available samples (in s16s)
        int             availableSamples();
        int             getSamples(s16* bufa, int siza, s16* bufb, int sizb);

        void            setTracer(std::ostream* stream);

    private:
        void            internalReset(bool hard);
        void            onRead_RAM(u16 a, u8& v);
        void            onRead_BIOS(u16 a, u8& v);
        void            onRead_SRAM(u16 a, u8& v);
        void            onRead_ROM(u16 a, u8& v);
        
        void            onWrite_RAM(u16 a, u8 v);
        void            onWrite_SRAM(u16 a, u8 v);
        void            onWrite_Swap(u16 a, u8 v);
        
        int             onPeek_RAM(u16 a) const;
        int             onPeek_BIOS(u16 a) const;
        int             onPeek_SRAM(u16 a) const;
        int             onPeek_ROM(u16 a) const;


        std::vector<u8>     rawPrg;                     // size=0 when not loaded
        u8                  ram [0x0800];
        u8                  sram[0x2000];
        const u8*           prg[8];
        u8                  bankswapInit[8];
        bool                hasBankswap;
        int                 numBanks;

        Cpu                 cpu;
        Apu                 apu;
        CpuBus              bus;
        AudioBuilder        builder;
        CpuTracer           tracer;

        int                 totalTracks;
        int                 currentTrack;
        u8                  bios[0x10];

        
        static const timestamp_t        clock_cycsPerSecond = 1789773;
        static const timestamp_t        clock_cycsPerFrame  = 29780;
    };
}

#endif