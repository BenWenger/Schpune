
#ifndef SCHPUNE_NESCORE_MEMORYCHIP_H_INCLUDED
#define SCHPUNE_NESCORE_MEMORYCHIP_H_INCLUDED

#include <vector>
#include "schpunetypes.h"
#include "error.h"

namespace schcore
{
    struct ChipPage
    {
        static const bool alwaysTrue  = true;
        static const bool alwaysFalse = false;

        u8*             mem =       nullptr;
        const bool*     readable =  &alwaysFalse;
        const bool*     writable =  &alwaysFalse;
        std::size_t     mask =      0;
        
        void            memRead(u16 a, u8& v)       { if(*readable) v = mem[a & mask];          }
        void            memWrite(u16 a, u8 v)       { if(*writable) mem[a & mask] = v;          }
        int             memPeek(u16 a) const        { return (*readable ? mem[a & mask] : -1);  }
    };

    class MemoryChip
    {
    public:
        bool                readable;
        bool                writable;
        bool                hasBattery;

        /////////////////////////////////////////////////
        MemoryChip(std::vector<u8>&& d, bool writ = false)
        {
            adoptDataFromVector( std::move(d) );
            readable = true;
            writable = writ;
            hasBattery = false;
        }

        MemoryChip(std::size_t size, bool writ = true, bool bat = false)
        {
            setSize(size);
            readable = true;
            writable = writ;
            hasBattery = bat;
        }

        std::size_t         getSize() const         { return data.size();       }
        const u8*           getData() const         { return &data[0];          }
        u8*                 getData()               { return &data[0];          }

        void                adoptDataFromVector(std::vector<u8>&& d)
        {
            data = std::move(d);
            setSize(data.size());
        }

        void                setSize(std::size_t siz)
        {
            if(siz > 0x10000000)        throw Error("Internal error:  chip size is way too high");
            if(siz < 1)                 throw Error("Internal error:  chip size must be at least 1 byte");
            
            std::size_t actualsize;

            // enforce a power-of-two size
            actualsize = 1;
            while(actualsize < siz)
                actualsize <<= 1;
            mask = actualsize - 1;

            data.resize(actualsize);
        }

        ChipPage    get4kPage(unsigned page)
        {
            ChipPage pg;
            pg.readable =   &readable;
            pg.writable =   &writable;
            pg.mask =       0x0FFF & mask;
            pg.mem =        &data[(page << 12) & mask];
            return pg;
        }
        

        ChipPage    get1kPage(unsigned page)
        {
            ChipPage pg;
            pg.readable =   &readable;
            pg.writable =   &writable;
            pg.mask =       0x03FF & mask;
            pg.mem =        &data[(page << 10) & mask];
            return pg;
        }

        void        clear(u8 v = 0)
        {
            for(auto&i : data)      i = v;
        }

    private:
        std::vector<u8>     data;
        std::size_t         mask;
    };
}

#endif
