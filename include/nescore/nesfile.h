
#ifndef SCHPUNE_NESCORE_NESFILE_H_INCLUDED
#define SCHPUNE_NESCORE_NESFILE_H_INCLUDED

#include <vector>
#include <iostream>
#include <string>
#include "schpunetypes.h"

namespace schcore
{
    class NesFile
    {
    public:
        // PRG and CHR
        std::vector<u8>                         prgRom;
        std::vector<u8>                         chrRom;
        unsigned                                prgRamSize = 0;
        unsigned                                chrRamSize = 0;

        // Mirroring
        enum class Mirror
        {   Vert, Horz, FourScr  }              headerMirroring = Mirror::Vert;
        bool                                    hasBattery = false;

        // Mapper
        int                                     inesMapperNumber = 0;

        enum class Region
        {   NTSC, PAL, Either, Unknown  }       region = Region::Unknown;

        // File Type
        enum class FileType
        {   None, ROM, NSF            }         fileType = FileType::None;

        // Expansion audio stuff (for NSFs)
        enum
        {
            Audio_Vrc6          = (1<<0),
            Audio_Vrc7          = (1<<1),
            Audio_Fds           = (1<<2),
            Audio_Mmc5          = (1<<3),
            Audio_Namco         = (1<<4),
            Audio_Sunsoft       = (1<<5)
        };
        int                                     extraAudio = 0;

        // other nsf stuff
        bool                                    hasBankswitching = false;
        u8                                      bankswitchBytes[8] = {};
        int                                     trackCount = 0;
        int                                     startTrack = 0;
        u16                                     initAddr = 0;
        u16                                     playAddr = 0;

        ////////////////////////////////////////
        //  File loading
        std::string         loadFile(const char* filename);
        std::string         loadFile(std::istream& file);

    private:
        void                internal_loadFile(std::istream& file);
        void                internal_loadFile_ines(std::istream& file, int bytes_skipped);
        void                internal_loadFile_nsf(std::istream& file, int bytes_skipped);
    };
}

#endif