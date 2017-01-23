
#ifndef SCHPUNE_NESCORE_NES_H_INCLUDED
#define SCHPUNE_NESCORE_NES_H_INCLUDED

#include <iostream>
#include <memory>
#include <vector>
#include "schpunetypes.h"
#include "nesfile.h"

namespace schcore
{
    //////////////////////////////////////////////
    //  used classes / components
    class Cpu;
    class CpuBus;
    class Ppu;
    class PpuBus;
    class Apu;
    class EventManager;
    class DmaUnit;
    class ResetInfo;
    class AudioBuilder;
    class NsfDriver;
    class Cartridge;
    class CpuTracer;

    ////////////////////////////////////////
    //  The NES!

    class Nes
    {
    public:
        enum class ResetType
        { none, soft, hard };

                        Nes();
                        ~Nes();
                        
        void            loadFile(NesFile&& file);           // takes ownership
        void            loadFile(const char* filename);
        void            loadFile(std::istream& file);

        void            copyAndLoadFile(NesFile file)       { loadFile(std::move(file));    }   // makes a copy, owns the copy
        void            hardReset()                         { reset(true);                  }
        void            softReset()                         { reset(false);                 }
        void            reset(bool hard);

        bool            isFileLoaded() const                { return loadedFile.fileType != NesFile::FileType::None;        }
        bool            isNsf() const                       { return loadedFile.fileType == NesFile::FileType::NSF;         }

        int             nsf_getTrack() const                { return curNsfTrack;                                           }
        int             nsf_getTrackCount() const           { return isNsf() ? loadedFile.trackCount : 0;                   }
        void            nsf_setTrack(int track);

        int             getApproxNaturalAudioSize() const;
        int             getAvailableAudioSize() const;

        void            doFrame();
        int             getAudio(void* bufa, int siza, void* bufb, int sizb);
        const u16*      getVideoBuffer();

        void            setTracer(std::ostream* stream);
        
        static const int    videoWidth = 256;
        static const int    videoHeight = 240;

    private:
        // subsystems
        std::unique_ptr<ResetInfo>      resetInfo;
        std::unique_ptr<Cpu>            cpu;
        std::unique_ptr<CpuBus>         cpuBus;
        std::unique_ptr<Ppu>            ppu;
        std::unique_ptr<PpuBus>         ppuBus;
        std::unique_ptr<Apu>            apu;
        std::unique_ptr<AudioBuilder>   audioBuilder;
        std::unique_ptr<DmaUnit>        dmaUnit;
        std::unique_ptr<CpuTracer>      cpuTracer;
        std::unique_ptr<EventManager>   eventManager;
        NesFile                         loadedFile;
        Cartridge*                      cartridge;

        std::unique_ptr<NsfDriver>      nsfDriver;

        // nsf specific stuff
        int                             curNsfTrack = 0;


        // other stuff on the system
        std::unique_ptr<u8[]>           systemRam;
        timestamp_t                     clocksPerFrame;

        /////////////////////////////////////////
        void            fillResetInfo();

        /////////////////////////////////////////
        //  callbacks
        int             onPeekRam(u16 a) const          { return systemRam[a & 0x07FF];                 }
        void            onReadRam(u16 a, u8& v)         { v = systemRam[a & 0x07FF];                    }
        void            onWriteRam(u16 a, u8 v)         { systemRam[a & 0x07FF] = v;                    }
    };
}

#endif