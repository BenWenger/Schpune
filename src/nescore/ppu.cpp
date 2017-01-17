
#include "ppu.h"
#include "ppubus.h"

namespace schcore
{

    /////////////////////////////////////////////////////////////////////
    //  Register access

    void Ppu::onRead(u16 a, u8& v)
    {
        switch(a & 7)
        {
        case 2:     // $2002
            // TODO - edge case of missed frames
            catchUp();
            v &= ~0xE0;
            v |= status;
            regToggle = false;
            break;

        case 4:     // $2004
            // TODO do this right
            catchUp();
            v = oam[oamAddr];
            break;
            
        case 7:     // $2007
            // TODO do this right
            catchUp();
            v = readBuffer;
            if((addr & 0x3F00) == 0x3F00)
                v = palette[a & 0x1F] & pltMask;

            readBuffer = ppuBus->read(addr);
            addr = (addr + addrInc) & 0x7FFF;
            break;
        }
    }

    void Ppu::onWrite(u16 a, u8 v)
    {
        catchUp();

        switch(a & 7)
        {
        case 0:     // $2000
            nmiEnabled =        (v & 0x80) != 0;
            spriteSize =        (v & 0x20) ? 16 : 8;
            bgPage =            (v & 0x10) ? 0x1000 : 0x0000;
            spPage =            (v & 0x08) ? 0x1000 : 0x0000;
            addrInc =           (v & 0x04) ? 32 : 1;
            addrTemp =          (addrTemp & 0x73FF) | ((v & 0x03) << 10);
            if(nmiEnabled)
                predictNextEvent();
            break;

        case 1:     // $2001
            emphasis =          (v & 0xE0) << 1;
            if(v & 0x10)        spClip = (v & 0x04) ? 0 : 8;
            else                spClip = 256;
            if(v & 0x08)        bgClip = (v & 0x02) ? 0 : 8;
            else                bgClip = 256;
            renderOn =          (v & 0x18) != 0;
            pltMask =           (v & 0x01) ? 0x30 : 0x3F;
            break;

        case 3:     // $2003
            oamAddr = v;        // TODO -- do this right
            break;

        case 4:     // $2004
            if((oamAddr & 3) == 2)  oam[oamAddr] = v & 0xE3;
            else                    oam[oamAddr] = v;
            ++oamAddr;
            // TODO -- do this right
            break;

        case 5:     // $2005
            if(regToggle)
            {
                addrTemp &= ~0x73E0;
                addrTemp |= (v & 0x07) << 12;
                addrTemp |= (v & 0xF8) << 2;
            }
            else
            {
                addrTemp &= ~0x001F;
                addrTemp |= (v & 0xF8) >> 3;
                fineX = (v & 0x07);
            }
            regToggle = !regToggle;
            break;
            
        case 6:     // $2006
            if(regToggle)
            {
                addrTemp = (addrTemp & 0x00FF) | ((v & 0x7F) << 8);
                addr = addrTemp;
            }
            else
            {
                addrTemp = (addrTemp & 0x7F00) | v;
            }
            regToggle = !regToggle;
            break;

        case 7:     // $2007
            // TODO -- do this right
            if((addr & 0x3F00) == 0x3F00)
            {
                // palette write
                palette[addr & 0x1F] = v & 0x3F;
                if(!(addr & 3))         // weird palette mirroring
                    palette[(addr & 0x1F) ^ 0x10] = v & 0x3F; 
            }
            else
            {
                ppuBus->write(addr, v);
            }
            addr = (addr + addrInc) & 0x7FFF;
            break;

        }
    }

    ////////////////////////////////////////////////////////////////
    //  Running

    void Ppu::run(timestamp_t runto)
    {
        auto tick = unitsToTimestamp(runto);
        if(tick <= 0)   return;
        cyc( tick );

        if(scanline == vblank_line)
        {

        }
    }


}