
#ifndef SCHPUNE_NESCORE_PPU_H_INCLUDED
#define SCHPUNE_NESCORE_PPU_H_INCLUDED

#include "schpunetypes.h"
#include "subsystem.h"


namespace schcore
{
    class ResetInfo;
    class CpuBus;
    class PpuBus;

    class Ppu : public SubSystem
    {
    public:
        //////////////////////////////////////////////////
        //  Resetting
        void                reset(const ResetInfo& info);

        //////////////////////////////////////////////////
        //  Running
        virtual void        run(timestamp_t runto) override;

    private:
        void                predictNextEvent();
        
        void                onWrite(u16 a, u8 v);
        void                onRead(u16 a, u8& v);

        ///////////////////////////////////////
        //  scanline -1 is the pre-render scanline
        //  lines 0-239 are rendered lines
        //  line 240 is the post-render 'idle' scanline
        //  line 241 = "VBlank", scanCyc is ignored, and the VBlank timers are used instead

        static const int    pre_line = -1;
        static const int    post_line = 240;
        static const int    vblank_line = 241;

        int                 scanline;
        int                 scanCyc;
        timestamp_t         vblankCycles;
        timestamp_t         safeOamCycles;
            // when scanline == vblank_line
        timestamp_t         safeOamCounter;
        timestamp_t         vblankCounter;

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

        u8                  status;         // $2002 status

        u8                  oamAddr;        // $2003
        u8                  oam[0x100];

        u8                  readBuffer;

        bool                regToggle;
        u16                 addrTemp;
        u16                 addr;
        u8                  fineX;

        u8                  palette[0x20];

        PpuBus*             ppuBus = nullptr;
    };


}

#endif