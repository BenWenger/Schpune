
#include <algorithm>
#include "ppu.h"
#include "ppubus.h"
#include "cpubus.h"
#include "eventmanager.h"
#include "resetinfo.h"
#include "cpu.h"
#include "cputracer.h"

// TODO PPU Warm up time?   https://wiki.nesdev.com/w/index.php/PPU_power_up_state

namespace schcore
{
    Ppu::Ppu()
    {
        nametables[0].mem = &rawNametables[0x0000];
        nametables[1].mem = &rawNametables[0x0400];
        
        nametables[0].readable = nametables[1].readable =
        nametables[0].writable = nametables[1].writable = &ChipPage::alwaysTrue;

        nametables[0].mask = nametables[1].mask = 0x03FF;
    }

    /////////////////////////////////////////////////////////////////////
    //  Resetting
    void Ppu::reset(const ResetInfo& info)
    {
        regBus =            0;
        scanline =          line_post;
        scanCyc =           0;
        oddFrame =          false;
        oddCycSkipped =     false;

        nmiEnabled =        false;
        spriteSize =        8;
        bgPage =            0;
        spPage =            0;
        addrInc =           1;

        emphasis =          0;
        bgClip =            256;
        spClip =            256;
        pltMask =           0x3F;
        renderOn =          false;

        suppressNmi =       false;
        readBuffer =        0;
        regToggle =         false;
        addr =              0;
        
        pixel =             outputBuffer;
        chrLoShift =        0;
        chrHiShift =        0;
        ntFetch =           0;
        atFetch =           0;
        chrLoFetch =        0;
        chrHiFetch =        0;

        if(info.hardReset)
        {
            subSystem_HardReset( info.cpu, info.region.ppuClockBase );

            statusByte =            0;
            oamAddr =               0;
            for(auto i : oam)       i = 0xFF;

            addrTemp =              0;
            fineX =                 0;
            for(auto i : palette)   i = 0x0F;

            cpuBus = info.cpuBus;
            ppuBus = info.ppuBus;
            eventManager = info.eventManager;

            for(auto i : outputBuffer)  i = 0x0F;
            for(auto i : sprPixels)     i = 0;
            
            cpuBus->addReader(0x2, 0x3, this, &Ppu::onRead);
            cpuBus->addWriter(0x2, 0x3, this, &Ppu::onWrite);
            
            vblankCycles = 20 * 341 * info.region.ppuClockBase;     // TODO - do this smarter
            safeOamCycles = 20 * 341 * info.region.ppuClockBase;
        }
    }

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
                addrTemp = (addrTemp & 0x7F00) | v;
                addr = addrTemp;
            }
            else
            {
                addrTemp = (addrTemp & 0x00FF) | ((v & 0x3F) << 8);
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
        if(scanCyc == 0)
        {
            if(!suppressNmi)
            {
                statusByte |= 0x80;
                if(nmiEnabled)
                    cpuBus->triggerNmi();
            }
            predictNextEvent();                 // predict the next NMI
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
        if(scanline == 0 && scanCyc == 0)
            statusByte = 0;

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

            if(scanCyc == 256)
            {
                buildSpritePixels();
            }

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

        timestamp_t cyclesRun = 0;        // ticks ran

        while((cyclesRun < ticks) && (scanline < line_post))
        {
            //////////////////////////////////////////
            // pre-render scanline stuff
            if(scanline == line_pre)
            {
                if(scanCyc >= 279 && scanCyc < 304)
                    addr = addrTemp;

                if(scanCyc == 340)
                {
                    oddCycSkipped = oddFrame;       // TODO - don't do this on PAL
                    if(oddCycSkipped)
                    {
                        ++scanline;
                        scanCyc = 0;
                    }
                }
            }
            switch(scanCyc)
            {
            case   0:
                if(scanline == 0)
                    statusByte = 0;
                break;

            case 256:
                buildSpritePixels();
                resetX();
                // no break
                      case   8: case  16: case  24: case  32: case  40: case  48: case  56:
            case  64: case  72: case  80: case  88: case  96: case 104: case 112: case 120:
            case 128: case 136: case 144: case 152: case 160: case 168: case 176: case 184:
            case 192: case 200: case 208: case 216: case 224: case 232: case 240: case 248:
            case 328: case 336:
                chrLoShift |= chrLoFetch;
                chrHiShift |= chrHiFetch;
                break;
                
            case   1: case   9: case  17: case  25: case  33: case  41: case  49: case  57:
            case  65: case  73: case  81: case  89: case  97: case 105: case 113: case 121:
            case 129: case 137: case 145: case 153: case 161: case 169: case 177: case 185:
            case 193: case 201: case 209: case 217: case 225: case 233: case 241: case 249:
            case 321: case 329:                                         case 337: case 339:
                ntFetch = ppuBus->read(0x2000 | (addr & 0x0FFF));
                break;
                
            case   3: case  11: case  19: case  27: case  35: case  43: case  51: case  59:
            case  67: case  75: case  83: case  91: case  99: case 107: case 115: case 123:
            case 131: case 139: case 147: case 155: case 163: case 171: case 179: case 187:
            case 195: case 203: case 211: case 219: case 227: case 235: case 243: case 251:
            case 323: case 331:
                atFetch = ppuBus->read(0);       // TODO -- do the actual attribute read!
                break;
                
            case   5: case  13: case  21: case  29: case  37: case  45: case  53: case  61:
            case  69: case  77: case  85: case  93: case 101: case 109: case 117: case 125:
            case 133: case 141: case 149: case 157: case 165: case 173: case 181: case 189:
            case 197: case 205: case 213: case 221: case 229: case 237: case 245: case 253:
            case 325: case 333:
                chrLoFetch = ppuBus->read( bgPage | (ntFetch<<4) | ((addr >> 12) & 7) );
                break;
                
            case   7: case  15: case  23: case  31: case  39: case  47: case  55: case  63:
            case  71: case  79: case  87: case  95: case 103: case 111: case 119: case 127:
            case 135: case 143: case 151: case 159: case 167: case 175: case 183: case 191:
            case 199: case 207: case 215: case 223: case 231: case 239: case 247: case 255:
            case 327: case 335:
                chrHiFetch = ppuBus->read( bgPage | (ntFetch<<4) | ((addr >> 12) & 7) | 0x0008 );
                incX();
                if(scanCyc == 255)      incY();
                break;

                // TODO sprite stuff
            }

            //////////////////////////////////////////
            // do a pixel for cycs 0-255
            if((scanCyc < 256) && (scanline >= 0))
            {
                u8 bgpix = ((chrLoShift >> (15-fineX)) & 0x01)
                         | ((chrHiShift >> (14-fineX)) & 0x02);
                if(scanCyc < bgClip)        bgpix = 0;

                // TODO - add attributes
                
                ////////////////////////////////////////
                // get sprite pixel
                u8 sprpix = sprPixels[scanCyc];
                if(scanCyc < spClip)        sprpix = 0;

                //sprite 0 hit?
                if(bgpix && (sprpix & 0x40) && scanCyc != 255)
                    statusByte |= 0x40;                             // TODO - this is delayed 1 cycle

                // which pixel to use?
                if(!bgpix || (sprpix & 0x80))
                    bgpix = sprpix & 0x1F;


                *pixel++ = emphasis | (palette[bgpix] & pltMask);
                chrLoShift <<= 1;
                chrHiShift <<= 1;
            }

            // shift out pixels (but don't render)
            //  for cycs 320-335
            if((scanCyc >= 320) && (scanCyc < 336))
            {
                chrLoShift <<= 1;
                chrHiShift <<= 1;
            }

            // count the cycle
            ++cyclesRun;
            if(++scanCyc >= 341)
            {
                scanCyc = 0;
                ++scanline;
            }
        }


        cyc(cyclesRun);
        ticks -= cyclesRun;

        return ticks;
    }

    //////////////////////////////////////////////
    //  Other stuff
    
    timestamp_t Ppu::finalizeFrame()
    {
        oddFrame = !oddFrame;
        pixel = outputBuffer;

        auto out = getMaxFrameLength();
        if(oddCycSkipped)
            out -= getClockBase();      // 1 cycle skipped

        return out;
    }

    timestamp_t Ppu::getMaxFrameLength() const
    {
        timestamp_t out = (341 * 242);      // 242 scanlines * 341 cycs per line
        out += vblankCycles;                // plus however much time for vblank
        out *= getClockBase();

        return out;
    }


    inline void Ppu::resetX()
    {
        addr &= ~0x041F;
        addr |= (0x041F & addrTemp);
    }

    inline void Ppu::incX()
    {
        if((addr & 0x001F) == 0x001F)       addr ^= 0x041F;
        else                                ++addr;
    }

    inline void Ppu::incY()
    {
        if((addr & 0x7000) == 0x7000)
        {
            addr &= 0x0FFF;
            if     ((addr & 0x03E0) == 0x03E0)  addr ^= 0x03E0;
            else if((addr & 0x03E0) == 0x03C0)  addr ^= 0x03C0 | 0x0800;
            else                                addr += 0x0020;
        }
        else
            addr += 0x1000;
    }

    void Ppu::predictNextEvent()
    {
        // Only event we're interested in is NMI
        if(nmiEnabled)
        {
            timestamp_t evt = 0;

            int ln = scanline;
            int cy = scanCyc;

            if(ln == line_vbl)      // any vblank time left?
            {
                evt = vblankCycles - cy;
                ln = line_pre;
                cy = 0;
            }
            if(ln == line_pre)      // pre-render line?
            {
                evt += 341-cy;
                ln = 0;
                cy = 0;
            }
            if(ln != line_post)     // rendering lines?
            {
                evt += 341-cy;
                evt += 341 * (line_post-ln-1);
                ln = line_post;
                cy = 0;
            }
            if(ln == line_post)     // any full rendering lines
            {
                evt += 341-cy;
            }
            //  at this point we've reached another vblank

            evt *= getClockBase();      // scale up to an actual timestamp
            evt += curCyc();

            ///////////////////////////
            // Add the event at this timestamp
            eventManager->addEvent( evt, EventType::evt_ppu );

            // and also at time-1 in case the odd cycle is skipped
            eventManager->addEvent( evt - getClockBase(), EventType::evt_ppu );                          // TODO don't do this on PAL since there are no skipped cycles?
        }
    }


    ///////////////////////////////////////////

    void Ppu::buildSpritePixels()
    {
        // TODO -- this entire routine is temporary.  Replace this with actual OAM evaluation logic
        for(auto& i : sprPixels)        i = 0;      // TODO -- this is temporary

        if(!renderOn) return;

        int spritesfound = 0;
        u16 a;
        u8 at;

        for(int i = 0; i < 256; i += 4)
        {
            int y = scanline - oam[i+0];
            if(y < 0)               continue;
            if(y >= spriteSize)     continue;

            ++spritesfound;
            if(spritesfound > 8)
            {
                statusByte |= 0x20;
                break;
            }

            // v-flip
            if(oam[i+2] & 0x80)     y ^= 0x0F;

            if(spriteSize == 16)
            {
                a  = (oam[i+1] & 0xFE) << 4;
                a |= (oam[i+1] & 0x01) << 12;
                if(y & 0x08)
                    a |= 0x0010;
            }
            else
            {
                a  = oam[i+1] << 4;
                a |= spPage;
            }
            a |= (y & 0x07);


            at  = (oam[i+2] << 2) & 0x8C;
            at ^= 0x80;                     // flip priority bit (makes drawing logic easier)
            at |= 0x10;                     // sprites always use the 1x palette
            if(!i)      at |= 0x40;         //sprite 0

            u8 lo = ppuBus->read(a);
            u8 hi = ppuBus->read(a | 8);

            // h-flip
            u8* dst = &sprPixels[ oam[i+3] ];
            if(oam[i+2] & 0x40)
            {
                if(!dst[0])     if(dst[0] = ((hi << 1) & 2) | ((lo     ) & 1))      dst[0] |= at;
                if(!dst[1])     if(dst[1] = ((hi     ) & 2) | ((lo >> 1) & 1))      dst[1] |= at;
                if(!dst[2])     if(dst[2] = ((hi >> 1) & 2) | ((lo >> 2) & 1))      dst[2] |= at;
                if(!dst[3])     if(dst[3] = ((hi >> 2) & 2) | ((lo >> 3) & 1))      dst[3] |= at;
                if(!dst[4])     if(dst[4] = ((hi >> 3) & 2) | ((lo >> 4) & 1))      dst[4] |= at;
                if(!dst[5])     if(dst[5] = ((hi >> 4) & 2) | ((lo >> 5) & 1))      dst[5] |= at;
                if(!dst[6])     if(dst[6] = ((hi >> 5) & 2) | ((lo >> 6) & 1))      dst[6] |= at;
                if(!dst[7])     if(dst[7] = ((hi >> 6) & 2) | ((lo >> 7) & 1))      dst[7] |= at;
            }
            else
            {
                if(!dst[0])     if(dst[0] = ((hi >> 6) & 2) | ((lo >> 7) & 1))      dst[0] |= at;
                if(!dst[1])     if(dst[1] = ((hi >> 5) & 2) | ((lo >> 6) & 1))      dst[1] |= at;
                if(!dst[2])     if(dst[2] = ((hi >> 4) & 2) | ((lo >> 5) & 1))      dst[2] |= at;
                if(!dst[3])     if(dst[3] = ((hi >> 3) & 2) | ((lo >> 4) & 1))      dst[3] |= at;
                if(!dst[4])     if(dst[4] = ((hi >> 2) & 2) | ((lo >> 3) & 1))      dst[4] |= at;
                if(!dst[5])     if(dst[5] = ((hi >> 1) & 2) | ((lo >> 2) & 1))      dst[5] |= at;
                if(!dst[6])     if(dst[6] = ((hi     ) & 2) | ((lo >> 1) & 1))      dst[6] |= at;
                if(!dst[7])     if(dst[7] = ((hi << 1) & 2) | ((lo     ) & 1))      dst[7] |= at;
            }
        }
    }
}