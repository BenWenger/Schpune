
#include "nsfdriver.h"
#include "cpubus.h"
#include "apu.h"
#include "expansion_audio/exaudio.h"
#include "expansion_audio/vrc6.h"

namespace schcore
{
    const u8 NsfDriver::driverCodePrimer[NsfDriver::driverCodeSize] = {
        0xA0, 0x0F,         // 00:  LDY #$0F
        0x8C, 0x15, 0x40,   // 02:  STY $4015
        0x20, 0,0,          // 05:  JSR initroutine
        0xF2,               // 08:  STP
        0x20, 0,0,          // 09:  JSR playroutine
        0x4C, 0,0           // 0C:  JMP the_stp_command
    };

    NsfDriver::NsfDriver()
    {
        for(int i = 0; i < driverCodeSize; ++i)
            driverCode[i] = driverCodePrimer[i];

        u16 stpaddr = driverCodeAddr + 0x08;
        driverCode[0x0D] = static_cast<u8>( stpaddr );
        driverCode[0x0E] = static_cast<u8>( stpaddr >> 8 );
    }

    NsfDriver::~NsfDriver()
    {
    }

    inline bool NsfDriver::isFdsTune() const
    {
        return (loadedFile->extraAudio & NesFile::Audio_Fds) != 0;
    }

    
    void NsfDriver::cartLoad(NesFile& file)
    {
        std::size_t minramsize = isFdsTune() ? 0xA000 : 0x2000;

        if(file.prgRamChips.empty())                                file.prgRamChips.emplace_back( minramsize );
        else if(file.prgRamChips.front().getSize() < minramsize)    file.prgRamChips.front().setSize( minramsize );

        // PRG ROM should be at least 4K
        if(file.prgRomChips.empty() ||
           file.prgRomChips.front().getSize() < 0x1000)
            throw Error("Internal error:  PRG ROM size needs to be at least 4K for NSF tunes.  Should have been expanded upon loading.");
        
        driverCode[0x06] = static_cast<u8>( file.initAddr );
        driverCode[0x07] = static_cast<u8>( file.initAddr >> 8 );
        driverCode[0x0A] = static_cast<u8>( file.playAddr );
        driverCode[0x0B] = static_cast<u8>( file.playAddr >> 8 );

        // Get the bankswapping values
        if(file.nsf_hasBankswitching)
        {
            for(int i = 0; i < 8; ++i)
                bankswappingValues[i+2] = file.nsf_bankswitchBytes[i];
            bankswappingValues[0] = bankswappingValues[8];
            bankswappingValues[1] = bankswappingValues[9];
        }
        else
        {
            int offset = (isFdsTune() ? 0 : -2);
            for(int i = 0; i < 10; ++i)
                bankswappingValues[i] = static_cast<u8>( i + offset );
        }

        // expansion audio?
        expansion.clear();
        if(file.extraAudio & NesFile::Audio_Vrc6)   expansion.emplace_back( std::make_unique<Vrc6Audio>(false) );
    }

    void NsfDriver::cartReset(const ResetInfo& info)
    {
        if(info.hardReset)
        {
            apu = info.apu;

            if(isFdsTune())
            {/*
                setPrgReaders(0x6,0xF);
                setPrgWriters(0x6,0xF);*/            // TODO this is all broken.  Figure I also have to swap PRG in for NSFs.  Figure this out
            }
            else
                setDefaultPrgCallbacks();
            
            info.cpuBus->addReader( 0x3, 0x3, this, &NsfDriver::onReadNsfDriver );
            info.cpuBus->addPeeker( 0x3, 0x3, this, &NsfDriver::onPeekNsfDriver );

            // only add bankswitching regs to the bus if this nsf has bankswitching
            if(loadedFile->nsf_hasBankswitching)
                info.cpuBus->addWriter( 0x5, 0x5, this, &NsfDriver::onWriteBankswap );
        }

        for(auto& i : expansion)
            i->reset(info);
    }

    void NsfDriver::doNsfPrgSwap(int slot, u8 v)
    {
        if( isFdsTune() )
        {
            syncApu();

            // FDS doesn't actually bankswap -- do a RAM copy
            auto src = loadedFile->prgRomChips.front().get4kPage(v);
            auto dst = loadedFile->prgRamChips.front().get4kPage(slot);
            for(int i = 0; i < 0x1000; ++i)     *dst.mem = *src.mem;
        }
        else if(slot >= 2)
            swapPrg_4k(slot + 6, v);
    }

    void NsfDriver::changeTrack()
    {
        // wipe PRG RAM only if this isn't an FDS tune (FDS PRG RAM will get wiped on bankswapping)
        if( !isFdsTune() )
            clearPrgRam();

        // silence all the channels
        apu->silenceAllChannels();

        // Do all the PRG swapping
        for(int i = 0; i < 10; ++i)
            doNsfPrgSwap(i, bankswappingValues[i]);
    }

    void NsfDriver::onWriteBankswap(u16 a, u8 v)
    {
        if( a >= 0x5FF6 )
        {
            if(a >= 0x5FF8 || isFdsTune())      // $5FF6,7 only count for FDS tunes
                doNsfPrgSwap( a-0x5FF6, v );
        }
    }
    
    void NsfDriver::onReadNsfDriver(u16 a, u8& v)
    {
        if(a >= driverCodeAddr)
            v = driverCode[a - driverCodeAddr];
    }

    int NsfDriver::onPeekNsfDriver(u16 a) const
    {
        if(a >= driverCodeAddr)
            return driverCode[a - driverCodeAddr];

        return -1;
    }
}