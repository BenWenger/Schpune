
#ifndef SCHPUNE_NESCORE_PPU_H_INCLUDED
#define SCHPUNE_NESCORE_PPU_H_INCLUDED

#include "schpunetypes.h"
#include "subsystem.h"


namespace schcore
{
    class ResetInfo;
    class CpuBus;
    class PpuBus;
    class EventManager;

    class Ppu : public SubSystem
    {
    public:
        //////////////////////////////////////////////////
        //  Resetting
        void                reset(const ResetInfo& info);

        //////////////////////////////////////////////////
        //  Detecting the length of the frame we just ran (for timestamp adjustment)
        timestamp_t         getMaxFrameLength() const;
        timestamp_t         finalizeFrame();                        // returns the number of cycles actually executed in the frame (to adjust all timestamps)

        //////////////////////////////////////////////////
        //  Running
        virtual void        run(timestamp_t runto) override;

        const u16*          getVideo() const            { return outputBuffer;      }

    private:
        void                predictNextEvent();
        
        void                onWrite(u16 a, u8 v);
        void                onRead(u16 a, u8& v);
        
        ///////////////////////////////////////
        //  scanline -1 is the pre-render scanline
        //  lines 0-239 are rendered lines
        //  line 240 is the post-render 'idle' scanline
        //  line 241 = "VBlank", scanCyc is ignored, and the VBlank timers are used instead

        static const int    line_pre = -1;
        static const int    line_post = 240;
        static const int    line_vbl = 241;

        u8                  regBus;

        int                 scanline;
        timestamp_t         scanCyc;
        timestamp_t         vblankCycles;   // total number of cycles in vblank
        timestamp_t         safeOamCycles;  // total number of cycles in vblank where writing to OAM is safe (must be <= vblankCycles)
        bool                oddFrame;
        bool                oddCycSkipped;

        bool                nmiEnabled;
        int                 spriteSize;     // 8 or 16
        u16                 bgPage;         // 0x0000 or 0x1000
        u16                 spPage;         // 0x0000 or 0x1000
        int                 addrInc;        // 1 or 32

        u16                 emphasis;       // emphasis bits in bits 6-8
        int                 bgClip;         // 0, 8, or 256
        int                 spClip;         // 0, 8, or 256
        u8                  pltMask;        // 0x30 for monochrome, 0x3F for color
        bool                renderOn;

        u8                  statusByte;     // $2002 status
        bool                suppressNmi;    // edge case NMI suppression

        u8                  oamAddr;        // $2003
        u8                  oam[0x100];

        u8                  readBuffer;

        bool                regToggle;
        u16                 addrTemp;
        u16                 addr;
        u8                  fineX;

        u8                  palette[0x20];
        
        CpuBus*             cpuBus = nullptr;
        PpuBus*             ppuBus = nullptr;
        EventManager*       eventManager = nullptr;

        u16                 outputBuffer[240 * 256];
        u16*                pixel;

        //  output shifters
        u16                 chrLoShift;
        u16                 chrHiShift;

        bool                spr0Hit;
        u8                  ntFetch;
        u8                  atFetch;
        u8                  chrLoFetch;
        u8                  chrHiFetch;
        
        void                incX();
        void                incY();
        void                resetX();

        ///////////////////////////////////////////////////////
        //  running!
        timestamp_t         run_vblank(timestamp_t ticks);
        timestamp_t         run_postRenderLine(timestamp_t ticks);
        timestamp_t         run_preRenderLine_Off(timestamp_t ticks);
        timestamp_t         run_line_On(timestamp_t ticks);
        timestamp_t         run_renderLine_Off(timestamp_t ticks);
    };


}

#endif