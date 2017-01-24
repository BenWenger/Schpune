
#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS
#define NOMINMAX
#include <Windows.h>
#include <algorithm>
#include "nes.h"
#include "display.h"

Display::Display()
{
    BITMAPINFO bi = {};
    bi.bmiHeader.biBitCount =   32;
    bi.bmiHeader.biWidth =      512;
    bi.bmiHeader.biHeight =     -480;
    bi.bmiHeader.biPlanes =     1;
    bi.bmiHeader.biSize =       sizeof( bi.bmiHeader );

    dc = CreateCompatibleDC(nullptr);
    bmp = CreateDIBSection( dc, &bi, DIB_RGB_COLORS, reinterpret_cast<void**>(&pixels), nullptr, 0 );
    bmpold = (HBITMAP)SelectObject(dc, bmp);

    buildPalette();

    refresh_rom(nullptr);
}

Display::~Display()
{
    SelectObject(dc, bmpold);
    DeleteObject(bmp);
    DeleteDC(dc);
}

void Display::blit(HDC target)
{
    BitBlt( target, 0, 0, 512, 480, dc, 0, 0, SRCCOPY );
}

void Display::refresh_rom(const u16* src)
{
    if(!src)
        std::memset( pixels, 0, sizeof(u32) * 512 * 480 );
    else
    {
        u32* dst = pixels;

        for(int y = 0; y < 240; ++y)
        {
            for(int x = 0; x < 256; ++x)
            {
                dst[0] = dst[1] = dst[512] = dst[513] = palette[ src[0] & 0x01FF ];
                dst += 2;
                src += 1;
            }
            dst += 512;
        }
    }
}

namespace
{
    void textOut(HDC dc, int x, int y, const char* str)
    {
        TextOutA( dc, x, y, str, strlen(str) );
    }
}

void Display::refresh_nsf(int track, int count)
{
    static const RECT full = {0, 0, 512, 480};
    char buffer[100];
    FillRect( dc, &full, (HBRUSH)GetStockObject(WHITE_BRUSH) );

    sprintf( buffer, "Track:  %d / %d", track, count );
    textOut( dc, 100, 100, buffer );
}

///////////////////////////////////////////////////////////////////////////////////////////
namespace
{
    const schcore::u8 rgbpalette[0x40 * 3] = {
         84, 84, 84,   0, 30,116,   8, 16,144,  48,  0,136,  68,  0,100,  92,  0, 48,  84,  4,  0,  60, 24,  0,  32, 42,  0,   8, 58,  0,   0, 64,  0,   0, 60,  0,   0, 50, 60,   0,  0,  0,   0,  0,  0,   0,  0,  0,
        152,150,152,   8, 76,196,  48, 50,236,  92, 30,228, 136, 20,176, 160, 20,100, 152, 34, 32, 120, 60,  0,  84, 90,  0,  40,114,  0,   8,124,  0,   0,118, 40,   0,102,120,   0,  0,  0,   0,  0,  0,   0,  0,  0,
        236,238,236,  76,154,236, 120,124,236, 176, 98,236, 228, 84,236, 236, 88,180, 236,106,100, 212,136, 32, 160,170,  0, 116,196,  0,  76,208, 32,  56,204,108,  56,180,204,  60, 60, 60,   0,  0,  0,   0,  0,  0,
        236,238,236, 168,204,236, 188,188,236, 212,178,236, 236,174,236, 236,174,212, 236,180,176, 228,196,144, 204,210,120, 180,222,120, 168,226,144, 152,226,180, 160,214,228, 160,162,160,   0,  0,  0,   0,  0,  0
    };

    const int emphasis[8 * 3] = {
        1000,1000,1000,
        1239, 915, 743,
         794,1086, 882,
        1019, 980, 653,
         905,1026,1277,
        1023, 908, 979,
         741, 987,1001,
         750, 750, 750
    };
}


void Display::buildPalette()
{
    int r, g, b;
    for( int i = 0; i < 0x200; ++i )
    {
        const auto* src = rgbpalette + ((i & 0x3F) * 3);
        const int* emph = emphasis + ((i >> 6) * 3);
        
        r = std::min(src[0] * emph[0] / 1000, 0xFF);
        g = std::min(src[1] * emph[1] / 1000, 0xFF);
        b = std::min(src[2] * emph[2] / 1000, 0xFF);

        palette[i] = (r <<  0) | (g <<  8) << (b << 16);
    }
}