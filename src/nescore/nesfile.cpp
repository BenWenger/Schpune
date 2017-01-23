
#include <stdexcept>
#include <fstream>
#include "nesfile.h"

namespace
{
    void err(const char* msg)   { throw std::runtime_error(msg);    }

    std::string readnsfstring(const schcore::u8* ptr)
    {
        std::string out;
        for(int i = 0; i < 32; ++i)
        {
            if(ptr[i])      out += static_cast<char>(ptr[i]);
            else            break;
        }

        if(out.empty())
            out = "<?>";
        return out;
    }
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
        if(file.fail())         err( "Unable to read file header" );
        if(file.eof())          err( "Unexpected EOF when reading file header" );

        // PRG/CHR sizes
        unsigned prgSize = hdr[0x04] * 0x4000;
        unsigned chrSize = hdr[0x05] * 0x2000;

        if(prgSize == 0)        err( "File has unrecognized PRG size" );

        // misc flags
        hasBattery =            (hdr[0x06] & 0x02) != 0;
        if(hdr[0x06] & 0x08)    headerMirroring = Mirror::FourScr;
        else                    headerMirroring = (hdr[0x06] & 0x01) ? Mirror::Vert : Mirror::Horz;

        int inesMapperNumber =  ((hdr[0x06] & 0xF0) >> 4) | (hdr[0x07] & 0xF0);
        mapper =                MapperId::NROM;     // TODO this is temporary

        ////////////////////////////////////
        //   Read the ROM data
        std::vector<u8> prg(prgSize);
        file.read( reinterpret_cast<char*>( &prg[0] ), prgSize );
        if(file.eof())          err( "Unexpected EOF when reading PRG ROM" );
        prgRomChips.emplace_back( std::move(prg) );
        
        if(chrSize > 0)
        {
            std::vector<u8> chr(chrSize);
            file.read( reinterpret_cast<char*>( &chr[0] ), chrSize );
            if(file.eof())      err( "Unexpected EOF when reading CHR ROM" );
            chrRomChips.emplace_back( std::move(chr) );
        }
        else
            chrRamChips.emplace_back( 0x2000 );

        if(file.fail())         err( "Unexpected error when reading file" );


        // TODO - actually do something with the mapper number and 'battery' bit
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

        loadAddr = hdr[0x08] | (hdr[0x09] << 8);
        initAddr = hdr[0x0A] | (hdr[0x0B] << 8);
        playAddr = hdr[0x0C] | (hdr[0x0D] << 8);
        
        nameOfGame = readnsfstring( &hdr[0x0E] );
        nameOfArtist = readnsfstring( &hdr[0x2E] );
        nameOfCopyright = readnsfstring( &hdr[0x4E] );


        nsf_hasBankswitching = false;
        for(int i = 0; i < 8; ++i)
        {
            nsf_bankswitchBytes[i] = hdr[i + 0x70];
            if(nsf_bankswitchBytes[i] != 0)     nsf_hasBankswitching = true;
        }

        // pal/NTSC bits
        if( hdr[0x7A] & 0x02 )      region = Region::Either;
        else if( hdr[0x7A] & 0x01 ) region = Region::PAL;
        else                        region = Region::NTSC;

        extraAudio =                hdr[0x7B] & 0x3F;


        ////////////////////////////////////
        //  read the PRG data.
        std::vector<u8>     prg;

        // Pad the PRG to the appropriate load addr
        if(nsf_hasBankswitching)
        {
            prg.resize( loadAddr & 0x0FFF );
        }
        else if( extraAudio & Audio_Fds )
        {
            if( loadAddr < 0x6000 )     err( "Invalid load address specified in the header" );
            prg.resize( loadAddr - 0x6000 );
        }
        else
        {
            if( loadAddr < 0x8000 )     err( "Invalid load address specified in the header" );
            prg.resize( loadAddr - 0x8000 );
        }

        // Reading directly into the PRG vector is actually kind of a pain in the arse
        while(true)
        {
            static const std::size_t blocksize = 0x2000;
            auto cursize = prg.size();
            auto nextsize = cursize + blocksize;

            prg.resize( nextsize );
            file.read(reinterpret_cast<char*>(&prg[cursize]), blocksize);
            if(file.eof())
            {
                nextsize = static_cast<decltype(nextsize)>(cursize + file.gcount());
                prg.resize(nextsize);
                break;
            }
            if(file.fail())     err( "Error occurred when reading the file" );
        }

        // After we have PRG, pad it to the appropriate minimum
        auto siz = prg.size();

        if( !nsf_hasBankswitching )
        {
            // no bankswitching means the PRG size must be large enough to fill PRG addressing space
            std::size_t minsize = (extraAudio & Audio_Fds) ? 0xA000 : 0x8000;
            if(prg.size() < minsize)
                prg.resize( minsize );
        }

        mapper = MapperId::NSF;
        prgRomChips.emplace_back( std::move(prg) );

        siz = (extraAudio & Audio_Fds) ? 0xA000 : 0x2000;
        prgRamChips.emplace_back( siz );
    }
}