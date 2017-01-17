
// this file is temporary -- it's just so I can quickly get NSF files playing!
//
//  Do this in a smarter way using an abstracted cartridge interface
//  And using a System main control thing

#define _CRT_SECURE_NO_WARNINGS
#include <cstdio>
#include "tempnsf.h"
#include "resetinfo.h"

namespace schcore
{
    namespace
    {
        const u8 biosPrime[0x10] = {
            0xA0, 0x0F,             // LDY #$0F
            0x8C, 0x15, 0x40,       // STY $4015
            0x20, 0, 0,             // JSR [init address]       // init addr to be inserted at [6],[7]
            0xF2,                   // STP
            0x20, 0, 0,             // JSR [play address]       // play addr to be inserted at [10],[11]
            0x4C, 0xF8, 0x3F        // JMP $3FF8 (the 'STP')
        };

    }

    TempNsf::TempNsf()
    {
    }

    bool TempNsf::load(const char* filename)
    {
        FILE* file = fopen(filename, "rb");
        if(!file)                                   return false;

        u8 hdr[0x80];
        if( fread(hdr, 1, 0x80, file) != 0x80 ) {   fclose(file);   return false;   }

        if( memcmp(hdr, "NESM\x1A", 5) )        {   fclose(file);   return false;   }

        totalTracks =           hdr[0x06];
        currentTrack =          hdr[0x07];

        int loadaddr =          hdr[0x08] | (hdr[0x09] << 8);

        memcpy( bios, biosPrime, 0x10 );
        bios[ 6] = hdr[0x0A];   // init addr
        bios[ 7] = hdr[0x0B];
        bios[10] = hdr[0x0C];   // play addr
        bios[11] = hdr[0x0D];

        // Bankswap shit
        hasBankswap = false;
        for(int i = 0; i < 8; ++i)
        {
            hasBankswap = (bankswapInit[i] = hdr[0x70+i]) || hasBankswap;
        }

        ////////////////
        // The actual PRG
        fseek(file, 0, SEEK_END);
        int prgsize = ftell(file) - 0x80;
        fseek(file, 0x80, SEEK_SET);
        if(prgsize < 1)                 {   fclose(file);   return false;   }

        if(hasBankswap)
        {
            numBanks = (((loadaddr & 0x0FFF) + prgsize - 1) >> 12) + 1;

            rawPrg.clear();
            rawPrg.resize( numBanks * 0x1000, 0 );

            fread(&rawPrg[loadaddr & 0x0FFF], 1, prgsize, file);
        }
        else
        {
            if(loadaddr < 0x8000)       {   fclose(file);   return false;   }
            if(prgsize + loadaddr > 0x10000)
                prgsize = 0x10000 - loadaddr;
            
            numBanks = 8;
            for(u8 i = 0; i < 8; ++i)
                bankswapInit[i] = i;

            rawPrg.clear();
            rawPrg.resize(0x8000, 0);

            fread(&rawPrg[loadaddr-0x8000], 1, prgsize, file);
        }
        fclose(file);

        ////////////////////////////////////////////
        // Do a hard reset
        internalReset(true);

        setTrack(currentTrack);
        return true;
    }

    
    bool TempNsf::loadTest(const char* filename)
    {
        FILE* file = fopen(filename, "rb");

        rawPrg.resize(0x4000);
        fseek(file, 0x10, SEEK_SET);
        fread(&rawPrg[0], 1, 0x4000, file);
        fclose(file);
        
        hasBankswap = false;
        numBanks = 4;
        bankswapInit[0] = bankswapInit[4] = 0;
        bankswapInit[1] = bankswapInit[5] = 1;
        bankswapInit[2] = bankswapInit[6] = 2;
        bankswapInit[3] = bankswapInit[7] = 3;

        internalReset(true);
        setTrack(0);
        return true;
    }

    void TempNsf::internalReset(bool hard)
    {
        tracer.supplyBus( &bus );

        ResetInfo info;
        info.audioBuilder =         &builder;
        info.cpuTracer =            &tracer;
        info.cpu =                  &cpu;
        info.apu =                  &apu;
        info.cpuBus =               &bus;
        info.dmaUnit =              &dmaUnit;
        info.eventManager =         &eventManager;
        info.hardReset =            hard;
        info.suppressIrqs =         true;
        
        info.region.ppuClockBase =  1;
        info.region.apuClockBase =  1;
        info.region.cpuClockBase =  1;
        info.region.masterCyclesPerSecond = clock_cycsPerSecond;
        info.region.masterCyclesPerFrame  = clock_cycsPerFrame;
        info.region.apuTables =     RegionInfo::ApuTables::ntsc;

        
        ///////////////////////////////////////////
        if(hard)
            builder.hardReset();
        builder.setClockRates( clock_cycsPerSecond, clock_cycsPerFrame );

        bus.reset( info );
        dmaUnit.reset( info );
        cpu.reset( info );
        apu.reset( info );
        eventManager.reset( info );

        if(hard)
        {
            bus.addPeeker( 0x0, 0x1, this, &TempNsf::onPeek_RAM );
            bus.addPeeker( 0x3, 0x3, this, &TempNsf::onPeek_BIOS );
            bus.addPeeker( 0x6, 0x7, this, &TempNsf::onPeek_SRAM );
            bus.addPeeker( 0x8, 0xF, this, &TempNsf::onPeek_ROM );

            bus.addReader( 0x0, 0x1, this, &TempNsf::onRead_RAM );
            bus.addReader( 0x3, 0x3, this, &TempNsf::onRead_BIOS );
            bus.addReader( 0x6, 0x7, this, &TempNsf::onRead_SRAM );
            bus.addReader( 0x8, 0xF, this, &TempNsf::onRead_ROM );
            
            bus.addWriter( 0x0, 0x1, this, &TempNsf::onWrite_RAM );
            bus.addWriter( 0x5, 0x5, this, &TempNsf::onWrite_Swap );
            if(hasBankswap)
                bus.addWriter( 0x6, 0x7, this, &TempNsf::onWrite_SRAM );
        }
    }

    void TempNsf::setTrack(int track)
    {
        currentTrack = track;
        
        memset(ram, 0, 0x0800);
        memset(sram, 0, 0x2000);

        for(int i = 0; i < 8; ++i)
        {
            prg[i] = &rawPrg[bankswapInit[i] * 0x1000];
        }

        //////////////////////////////////////////////
        internalReset(false);
        cpu.primeNsf( static_cast<u8>(currentTrack - 1), 0, 0x3FF0 );
    }


    /////////////////////////////////////////////////////////////////////
    
    int TempNsf::onPeek_RAM(u16 a) const            { return ram[a & 0x07FF];                                   }
    int TempNsf::onPeek_BIOS(u16 a) const           { if(a < 0x3FF0) return 0;      return bios[a & 0x000F];    }
    int TempNsf::onPeek_SRAM(u16 a) const           { return sram[a & 0x1FFF];                                  }
    int TempNsf::onPeek_ROM(u16 a) const            { return prg[(a >> 12)&7][a & 0x0FFF];                      }
    
    void TempNsf::onRead_RAM(u16 a, u8& v)          { v = ram[a & 0x07FF];                          }
    void TempNsf::onRead_BIOS(u16 a, u8& v)         { if(a >= 0x3FF0)   v = bios[a & 0x000F];       }
    void TempNsf::onRead_SRAM(u16 a, u8& v)         { v = sram[a & 0x1FFF];                         }
    void TempNsf::onRead_ROM(u16 a, u8& v)          { v = prg[(a>>12)&7][a & 0x0FFF];               }
    
    void TempNsf::onWrite_RAM(u16 a, u8 v)          { ram[a & 0x07FF] = v;                          }
    void TempNsf::onWrite_SRAM(u16 a, u8 v)         { sram[a & 0x1FFF] = v;                         }
    void TempNsf::onWrite_Swap(u16 a, u8 v)
    {
        if(a >= 0x5FF8)
            prg[a & 7] = &rawPrg[(v % numBanks) << 12];
    }

    int TempNsf::doFrame()
    {
        cpu.run( clock_cycsPerFrame );
        apu.run( clock_cycsPerFrame );
        
        cpu.subtractFromMainTimestamp( clock_cycsPerFrame );
        apu.subtractFromMainTimestamp( clock_cycsPerFrame );

        cpu.unjam();

        return availableSamples();
    }

    int TempNsf::availableSamples()
    {
        return builder.samplesAvailableAtTimestamp( apu.getAudTimestamp() );
    }
    
    int TempNsf::getSamples(s16* bufa, int siza, s16* bufb, int sizb)
    {
        int avail = availableSamples();

        if(siza > avail)    siza = avail;
        builder.generateSamples(0, bufa, siza);
        avail -= siza;

        if(sizb > avail)    sizb = avail;
        builder.generateSamples(siza, bufb, sizb);

        //////////////////
        avail = siza + sizb;
        builder.wipeSamples(avail);

        return avail;
    }

    void TempNsf::setTracer(std::ostream* stream)
    {
        tracer.enable();
        tracer.setOutputStream(stream);
    }
}
