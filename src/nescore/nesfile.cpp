
#include <stdexcept>
#include <fstream>
#include "nesfile.h"

namespace
{
    void err(const char* msg)   { throw std::runtime_error(msg);    }
}

namespace schcore
{
    std::string NesFile::loadFile(const char* filename)
    {
        try
        {
            std::ifstream file( filename, std::ifstream::binary );
            if(!file.good())
                return "Unable to open file for reading";

            return loadFile(file);
        }
        catch(std::exception& e)
        {
            return e.what();
        }

        return "";
    }

    std::string NesFile::loadFile(std::istream& file)
    {
        try
        {
            NesFile temp;
            temp.internal_loadFile(file);
            *this = std::move(temp);
        }
        catch(std::exception& e)
        {
            return e.what();
        }

        return "";
    }
    
    ////////////////////////////////////////////////
    ////////////////////////////////////////////////
    void NesFile::internal_loadFile(std::istream& file)
    {
        u8 hdr[4];      // read the 4 byte header to determine the file type
        file.read( reinterpret_cast<char*>(hdr), 4 );
        if(file.gcount() != 4)      err( "Unable to read file header.  File might be too small." );
        
        if(!std::memcmp(hdr,"NES\x1A",4))       internal_loadFile_ines(file, 4);
        if(!std::memcmp(hdr,"NESM",4))          internal_loadFile_nsf(file, 4);
    }
    
    ////////////////////////////////////////////////
    //  iNES
    void NesFile::internal_loadFile_ines(std::istream& file, int bytes_skipped)
    {
        fileType = FileType::ROM;

        if(bytes_skipped > 4)
            err( "Internal erorr:  too many bytes skipped when reading header in NesFile::internal_loadFile" );

        u8 hdr[0x10];
        file.read( reinterpret_cast<char*>(hdr + bytes_skipped), 0x10 - bytes_skipped );

        //TODO
    }
    
    ////////////////////////////////////////////////
    //  NSFs
    void NesFile::internal_loadFile_nsf(std::istream& file, int bytes_skipped)
    {
        fileType = FileType::NSF;

        if(bytes_skipped > 6)
            err( "Internal erorr:  too many bytes skipped when reading header in NesFile::internal_loadFile" );
        u8 hdr[0x80];
        file.read( reinterpret_cast<char*>(hdr + bytes_skipped), 0x80 - bytes_skipped );

        if(file.gcount() != (0x80 - bytes_skipped))
            err( "Unable to read file header.  File might be too small." );

        ////////////
        trackCount = hdr[0x06];     if(trackCount == 0)     err( "Nsf seems to contain no songs" );
        startTrack = hdr[0x07];     if(startTrack < 1)      startTrack = 1;

        u16 loadaddr = hdr[0x08] | (hdr[0x09] << 8);
        initAddr = hdr[0x0A] | (hdr[0x0B] << 8);
        playAddr = hdr[0x0C] | (hdr[0x0D] << 8);

        // TODO - strings from NSF if anyone cares
        /*
            $00E    32  STRING  The name of the song, null terminated
            $02E    32  STRING  The artist, if known, null terminated
            $04E    32  STRING  The copyright holder, null terminated
        */

        hasBankswitching = false;
        for(int i = 0; i < 8; ++i)
        {
            bankswitchBytes[i] = hdr[i + 0x70];
            if(bankswitchBytes[i] != 0)     hasBankswitching = true;
        }

        // pal/NTSC bits
        if( hdr[0x7A] & 0x02 )      region = Region::Either;
        else if( hdr[0x7A] & 0x01 ) region = Region::PAL;
        else                        region = Region::NTSC;

        extraAudio =                hdr[0x7B] & 0x3F;


        ////////////////////////////////////
        //  read the PRG data.

        // Pad the PRG to the appropriate load addr
        if(hasBankswitching)
        {
            prgRom.resize( loadaddr & 0x0FFF );
        }
        else if( extraAudio & Audio_Fds )
        {
            if( loadaddr < 0x6000 )     err( "Invalid load address specified in the header" );
            prgRom.resize( loadaddr - 0x6000 );
        }
        else
        {
            if( loadaddr < 0x8000 )     err( "Invalid load address specified in the header" );
            prgRom.resize( loadaddr - 0x8000 );
        }

        // Reading directly into the PRG vector is actually kind of a pain in the arse
        while(true)
        {
            static const std::size_t blocksize = 0x2000;
            auto cursize = prgRom.size();
            auto nextsize = cursize + blocksize;

            prgRom.resize( nextsize );
            file.read(reinterpret_cast<char*>(&prgRom[cursize]), blocksize);
            if(file.fail())     err( "Error occurred when reading the file" );
            if(file.eof())
            {
                nextsize = static_cast<decltype(nextsize)>(cursize + file.gcount());
                prgRom.resize(nextsize);
                break;
            }
        }

        // After we have PRG, pad it to the appropriate boundary
        auto siz = prgRom.size();
        if( siz & 0x0FFF )                                      // should be on 4K bounary
            prgRom.resize( siz + 0x1000 - (siz & 0x0FFF) );

        if( !hasBankswitching )
        {
            // no bankswitching means the PRG size must be large enough to fill PRG addressing space
            std::size_t minsize = (extraAudio & Audio_Fds) ? 0xA000 : 0x8000;

            if(prgRom.size() < minsize)
                prgRom.resize( minsize );
        }
    }
}