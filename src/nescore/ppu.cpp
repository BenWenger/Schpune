
#include <algorithm>
#include "ppu.h"
#include "ppubus.h"
#include "cpubus.h"

namespace schcore
{

    /////////////////////////////////////////////////////////////////////
    //  Register access

    void Ppu::onRead(u16 a, u8& v)
    {
        switch(a & 7)
        {
        case 2:     // $2002
            catchUp();

            // edge case where NMIs being suppressed when this is read right as the flag is raised
            suppressNmi = (scanline == line_vbl) && (scanCyc == 0);

            v  = (regBus & ~0xE0) | statusByte;
            regToggle = false;
            statusByte &= ~0x80;
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

        default:
            v = regBus;
            break;
        }

        regBus = v; // <- not sure if this should be here...
    }

    void Ppu::onWrite(u16 a, u8 v)
    {
        catchUp();
        regBus = v;

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

        while(tick > 0)
        {
            switch(scanline)
            {
            case line_vbl:      tick = run_vblank(tick);                                                break;
            case line_post:     tick = run_postRenderLine(tick);                                        break;
            case line_pre:      tick = (renderOn ? run_line_On(tick) : run_preRenderLine_Off(tick));    break;
            default:            tick = (renderOn ? run_line_On(tick) : run_renderLine_Off(tick));       break;
            }
        }
    }

    //////////////////////////////////////
    //  VBlank
    timestamp_t Ppu::run_vblank(timestamp_t ticks)
    {
        // reset this here for good measure
        pixel = outputBuffer;

        ///////////////////////////////////
        // Only thing interesting that happens in VBlank is setting the VBlank flag and
        //  triggering NMI.  That happens on the very first cycle
        if((scanCyc == 0) && !suppressNmi)
        {
            statusByte |= 0x80;
            if(nmiEnabled)
                cpuBus->triggerNmi();
        }
        suppressNmi = false;

        // VBlank is long and uneventful, so we can "run ahead" for most of it.
        //  Aside from the NMI at the start, the only other interesting thing here is the
        //  end of safe OAM access

        timestamp_t target = ticks + scanCyc;               // the cycle we want to run to

        // if that target is after end of safe oam access, run through until just before end of vblank
        if(target >= safeOamCycles)
            target = std::max( vblankCycles-1, target );
        else
            target = safeOamCycles-1;       // otherwise, run up to just before end of safe oam access

        // are we trying to run past end of vblank?
        if(target >= vblankCycles)
        {
            auto go = vblankCycles - scanCyc;       // number of cycles to actually run
            scanCyc = 0;
            scanline = line_pre;
            cyc(go);
            return ticks-go;
        }
        else
        {
            // still in vblank
            auto go = target - scanCyc;
            scanCyc = target;
            cyc(go);
            return 0;
        }
    }
    
    //////////////////////////////////////
    //  Post-Render
    timestamp_t Ppu::run_postRenderLine(timestamp_t ticks)
    {
        // this line is even LESS interesting than VBlank
        //  absolutely nothing happens here.  Just jump to whatever target they want

        auto target = ticks + scanCyc;
        if(target >= 341)       // got to end of post-render line
        {
            auto go = 341 - scanCyc;
            scanline = line_vbl;
            scanCyc = 0;
            cyc(go);
            return ticks-go;
        }

        cyc(ticks);
        scanCyc += ticks;
        return 0;
    }
    
    //////////////////////////////////////
    //  Pre-Render  (Off)
    timestamp_t Ppu::run_preRenderLine_Off(timestamp_t ticks)
    {
        // nothing of note really happens when rendering is off
        //   Only interesting thing is that the odd-frame-skipped-cycle exists here
        //   and won't be skipped if rendering is off
        
        // this line is even LESS interesting than VBlank
        //  absolutely nothing happens here.  Just jump to whatever target they want

        auto target = ticks + scanCyc;

        if(target >= 341)       // got to end of pre-render line
        {
            oddCycSkipped = false;
            auto go = 341 - scanCyc;
            scanline = 0;
            scanCyc = 0;
            cyc(go);
            return ticks-go;
        }

        cyc(ticks);
        scanCyc += ticks;
        return 0;
    }
    
    //////////////////////////////////////
    //  Render  (Off)
    
    timestamp_t Ppu::run_renderLine_Off(timestamp_t ticks)
    {
        /////////////////////////////////////////////
        //  Only thing interesting that happens when rendering is disabled is the
        //    BG color output
        //  --- NOTE this is only for lines 0-239 ("render" lines).  pre-render should not call this function

        // When rendering is disabled, render lines will output the bg color, unless the
        //   ppu address is pointing to palette space -- in which case it will render
        //   whatever color it's pointing to
        u16 clrout = palette[0];
        if((addr & 0x3F00) == 0x3F00)   clrout = palette[addr & 0x001F];

        clrout = emphasis | (clrout & pltMask);     // emphasis and monochrome bits are still employed

        while((ticks > 0) && (scanline < line_post))        // TODO, could this be optimized?
        {
            if(scanCyc < 256)
                *pixel++ = clrout;

            cyc();
            --ticks;
            ++scanCyc;
            if(scanCyc >= 341)
            {
                scanCyc = 0;
                ++scanline;
            }
        }

        return ticks;
    }

    
    timestamp_t Ppu::run_line_On(timestamp_t ticks)
    {
        ////////////////////////////////////////////////////////////
        //  This is for all render lines as well as the pre-render line when
        //    rendering is enabled.
        //
        //  In other words -- ALL the rendering work is done here

        // TODO -- complete this!

        return ticks;
    }

}