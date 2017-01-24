#ifndef SCHPUNE_FRONT_DISPLAY_H_INCLUDED
#define SCHPUNE_FRONT_DISPLAY_H_INCLUDED

class Display
{
public:
    typedef schcore::u16        u16;
    typedef schcore::u32        u32;

    Display();
    ~Display();
    Display(const Display&) = delete;
    Display& operator = (const Display&) = delete;

    void        refresh_nsf(int track, int count);
    void        refresh_rom(const u16* src);

    void        blit(HDC target);

private:
    HDC         dc;
    HBITMAP     bmp;
    HBITMAP     bmpold;
    u32*        pixels;
    u32         palette[0x200];

    void        buildPalette();
};

#endif