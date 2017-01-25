
#include "nes.h"
#include "cpu.h"
#include "cpubus.h"
#include "ppu.h"
#include "ppubus.h"
#include "apu.h"
#include "eventmanager.h"
#include "dmaunit.h"
#include "resetinfo.h"
#include "audiobuilder.h"
#include "nsfdriver.h"
#include "cputracer.h"
#include "mappers/mappers.h"


//////////////////////////////////////////////////////////////
namespace schcore
{
    Nes::Nes()
        : resetInfo( new ResetInfo )
        , cpu( new Cpu )
        , cpuBus( new CpuBus )
        , ppu( new Ppu )
        , ppuBus( new PpuBus )
        , apu( new Apu )
        , audioBuilder( new AudioBuilder )
        , dmaUnit( new DmaUnit )
        , cpuTracer( new CpuTracer )
        , eventManager( new EventManager )
        , nsfDriver( new NsfDriver )
        , systemRam( new u8[0x0800] )
    {
        cpuTracer->enable();            // leave it enabled -- whether or not it has a stream will be whether it's enabled
        cpuTracer->supplyBus( cpuBus.get() );
        inputDevices[0] = inputDevices[1] = nullptr;
    }

    Nes::~Nes()
    {
    }

    void Nes::setTracer(std::ostream* stream)
    {
        cpuTracer->setOutputStream(stream);
    }
    
    ////////////////////////////////////////////
    ////////////////////////////////////////////
    //  File loading!
    
    void Nes::loadFile(const char* filename)
    {
        NesFile f;
        auto str = f.loadFile(filename);
        if(!str.empty())        throw Error(str);
        loadFile(std::move(f));
    }

    void Nes::loadFile(std::istream& file)
    {
        NesFile f;
        auto str = f.loadFile(file);
        if(!str.empty())        throw Error(str);
        loadFile(std::move(f));
    }

    void Nes::loadFile(NesFile&& file)
    {
        // is the file even loaded?
        if(file.fileType == NesFile::FileType::None)    throw Error("Nes::loadFile: File provided was empty");

        // The file must have PRG
        if(file.prgRomChips.empty())    throw Error("Nes::loadFile: given file has no PRG");


        switch(file.fileType)
        {
        case NesFile::FileType::ROM:
            // ROM files have to have CHR
            if(file.chrRomChips.empty() && file.chrRamChips.empty() && file.fileType == NesFile::FileType::ROM)
                throw Error("Nes::loadFile: give file has no CHR");

            ownedCartridge = mpr::buildCartridgeFromFile(file);
            cartridge = ownedCartridge.get();
            break;

        case NesFile::FileType::NSF:
            cartridge = nsfDriver.get();
            curNsfTrack = file.startTrack;
            break;
        }

        loadedFile = std::move(file);
        cartridge->load(loadedFile);
        fillResetInfo();
        hardReset();
    }

    ///////////////////////////////////////////////////
    void Nes::fillResetInfo()
    {
        resetInfo->hardReset =          true;
        resetInfo->suppressIrqs =       isNsf();

        resetInfo->cpuTracer =          cpuTracer.get();
        resetInfo->cpu =                cpu.get();
        resetInfo->ppu =                ppu.get();
        resetInfo->apu =                apu.get();
        resetInfo->cpuBus =             cpuBus.get();
        resetInfo->ppuBus =             ppuBus.get();
        resetInfo->audioBuilder =       audioBuilder.get();
        resetInfo->dmaUnit =            dmaUnit.get();
        resetInfo->eventManager =       eventManager.get();

        // TODO -- chose the region somehow
        resetInfo->region.apuClockBase =            3;
        resetInfo->region.cpuClockBase =            3;
        resetInfo->region.ppuClockBase =            1;
        resetInfo->region.apuTables =               RegionInfo::ApuTables::ntsc;
        resetInfo->region.masterCyclesPerSecond =   static_cast<timestamp_t>(1789772.727272 * resetInfo->region.cpuClockBase);
        clocksPerFrame =                            341*262*resetInfo->region.ppuClockBase;


        // Give a little extra time in the cycles per frame to account for "spilling over".
        //    Between NMI cycles, DMC stolen cycles, and worst case -- OAM DMA cycles, let's say
        //    we can spill 600 cpu cycles past the end of the frame in a single run
        resetInfo->region.masterCyclesPerFrame =    clocksPerFrame * resetInfo->region.cpuClockBase;
    }

    void Nes::reset(bool hard)
    {
        if(!isFileLoaded())     return;

        resetInfo->hardReset = hard;

        // reset the busses first
        cpuBus->reset( *resetInfo );
        ppuBus->reset( *resetInfo );

        // DMA unit next, as it needs to add its callbacks to the cpu bus first so that it can
        //   hijack reads
        dmaUnit->reset( *resetInfo );

        // Event manager should be next, so subsystems can add their events
        eventManager->reset( *resetInfo );

        // Cartridge should be last... but other than that, the rest of the order shouldn't matter
        if(hard)                audioBuilder->hardReset(resetInfo->region.masterCyclesPerSecond, resetInfo->region.masterCyclesPerFrame);
        cpu->reset( *resetInfo );
        ppu->reset( *resetInfo );
        apu->reset( *resetInfo );

        // Do cartridge last
        cartridge->reset( *resetInfo );

        // System memory gets wiped on hard reset (unless NSF -- since memory wiping will happen differently when we set the track)
        if(isNsf())
            nsf_setTrack( curNsfTrack );
        else
        {
            for(int i = 0; i < 0x0800; ++i)
                systemRam[i] = 0xFF;
        }

        ////////////////////////
        //  Add system memory read/writers
        if(hard)
        {
            cpuBus->addReader( 0, 1, this, &Nes::onReadRam );
            cpuBus->addWriter( 0, 1, this, &Nes::onWriteRam );
            cpuBus->addPeeker( 0, 1, this, &Nes::onPeekRam );
            if(!isNsf())                    // add controller access for non-nsfs
            {
                cpuBus->addReader( 4, 4, this, &Nes::onReadInput );
                cpuBus->addWriter( 4, 4, this, &Nes::onWriteInput );
                
                if(inputDevices[0]) inputDevices[0]->hardReset();
                if(inputDevices[1]) inputDevices[1]->hardReset();
            }
        }
    }
    
    void Nes::nsf_setTrack(int track)
    {
        if(!isNsf())        return;

        for(int i = 0; i < 0x0800; ++i)
            systemRam[i] = 0;

        curNsfTrack = track;
        cpu->primeNsf( static_cast<u8>(track-1),
                       resetInfo->region.apuTables == RegionInfo::ApuTables::pal,
                       nsfDriver->getTrackStartAddr()
                     );

        nsfDriver->changeTrack();
    }
    
    ///////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////

    int Nes::getApproxNaturalAudioSize() const
    {
        return audioBuilder->audioAvailableAtTimestamp( resetInfo->region.masterCyclesPerFrame );
    }

    int Nes::getAvailableAudioSize() const
    {
        return audioBuilder->audioAvailableAtTimestamp( apu->getAudTimestamp() );
    }

    int Nes::getAudio(void* bufa, int siza, void* bufb, int sizb)
    {
        int avail = getAvailableAudioSize();

        if(siza > avail)    siza = avail;
        audioBuilder->generateSamples(0, reinterpret_cast<s16*>(bufa), siza);
        avail -= siza;

        if(sizb > avail)    sizb = avail;
        audioBuilder->generateSamples(siza, reinterpret_cast<s16*>(bufb), sizb);

        //////////////////
        avail = siza + sizb;
        audioBuilder->wipeSamples(avail);

        return avail;
    }

    const u16* Nes::getVideoBuffer()
    {
        return ppu->getVideo();
    }
    ///////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////

    //  Input!
    void Nes::setInputDevice(int port, input::InputDevice* device)
    {
        inputDevices[port != 0] = device;
        if(device)
            device->hardReset();
    }
    
    void Nes::onReadInput(u16 a, u8& v)
    {
        if(a == 0x4016 || a == 0x4017)
        {
            a &= 1;
            v &= ~0x1F;

            if(inputDevices[a])     v |= inputDevices[a]->read() & 0x19;
                    //  TODO - expansion port / Famicom differences
        }
    }

    void Nes::onWriteInput(u16 a, u8 v)
    {
        if(a == 0x4016)
        {
            if(inputDevices[0])     inputDevices[0]->write(v & 0x01);
            if(inputDevices[1])     inputDevices[1]->write(v & 0x01);
                    // TODO - expansion port
        }
    }
    
    ///////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////

    void Nes::doFrame()
    {
        // TODO change behavior here for ROMs
        if(!isFileLoaded())     return;

        if(isNsf())
        {
            cpu->run( clocksPerFrame );
            apu->run( clocksPerFrame );
            cpu->unjam();
            
            cpu->endFrame( clocksPerFrame );
            apu->endFrame( clocksPerFrame );
        }
        else
        {
            timestamp_t run = clocksPerFrame;

            cpu->run( run );
            apu->run( run );
            ppu->run( run );
            cartridge->run( run );

            run = ppu->finalizeFrame();
            
            cpu->endFrame( run );
            apu->endFrame( run );
            ppu->endFrame( run );
            cartridge->endFrame( run );
            eventManager->subtractFromTimestamps( run );
        }
    }
}