
//#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS
#define NOMINMAX
#include <Windows.h>
#include <soundout.h>
#include <fstream>

#include "tempfront.h"

void textOut(HDC dc, int x, int y, const char* str)
{
    TextOutA(dc, x, y, str, std::strlen(str));
}

const char* const           classname = "Temp NSF Player";
schcore::TempFront          nsf;
SoundOut                    snd(48000, false, 200);
bool                        runApp = true;
bool                        loaded = false;
std::ofstream               traceFile;
bool                        tracing = false;
const char* const           traceFileName = "tempnsf_trace.txt";

void drawWindow(HWND wnd)
{
    PAINTSTRUCT ps;
    HDC dc = BeginPaint(wnd,&ps);
    
    textOut(dc, 20, 20, "Press Escape to load an NSF" );
    textOut(dc, 20, 40, "Press Left/Right arrows to switch tracks" );

    if(!loaded)
    {
        textOut( dc, 40, 80, "No file loaded" );
    }
    else
    {
        char buf[80];
        sprintf( buf, "Track %d of %d", nsf.getTrack(), nsf.getTrackCount() );
        textOut( dc, 40, 80, buf );
    }

    if(tracing)
    {
        textOut( dc, 60, 200, " ** TRACING **" );
    }

    EndPaint(wnd, &ps);
}

void toggleTrace()
{
    if(!tracing)
    {
        traceFile.open(traceFileName);
        if(traceFile.good())
        {
            tracing = true;
            nsf.setTracer( &traceFile );
        }
    }
    else
    {
        tracing = false;
        nsf.setTracer(nullptr);
    }
}

void fillAudio()
{
    {
        using schcore::s16;

        auto lk = snd.lock();
        int written = nsf.getSamples( lk.getBuffer<s16>(0), lk.getSize(0) / sizeof(s16), lk.getBuffer<s16>(1), lk.getSize(1) / sizeof(s16) );
        lk.setWritten(written * sizeof(s16));
    }
    nsf.doFrame();
}

void loadFile(HWND wnd)
{
    snd.stop(false);
    
    char filebuf[500] = "";

    OPENFILENAMEA ofn = {0};
    ofn.hwndOwner =         wnd;
    ofn.Flags =             OFN_HIDEREADONLY | OFN_FILEMUSTEXIST;
    ofn.lpstrFilter =       "NSF Files\0*.nsf\0All Files\0*\0\0";
    ofn.nFilterIndex =      1;
    ofn.lStructSize =       sizeof(ofn);
    ofn.lpstrFile =         filebuf;
    ofn.nMaxFile =          500;

    if(GetOpenFileNameA(&ofn))
        loaded = nsf.load(filebuf);

    InvalidateRect(wnd,NULL,true);

    if(loaded)
    {
        snd.stop(true);
        nsf.doFrame();
        fillAudio();
        snd.play();
    }
}

LRESULT CALLBACK WndProc(HWND wnd, UINT msg, WPARAM w, LPARAM l)
{
    switch(msg)
    {
    case WM_DESTROY:
        runApp = false;
        break;

    case WM_PAINT:
        drawWindow(wnd);
        break;

    case WM_KEYDOWN:
        if(w == VK_ESCAPE)
            loadFile(wnd);
        if(w == VK_TAB)
        {
            toggleTrace();
            InvalidateRect(wnd,nullptr,true);
        }
        break;
    }

    return DefWindowProc(wnd,msg,w,l);
}

int WINAPI WinMain(HINSTANCE inst, HINSTANCE prev, LPSTR cmd, int show)
{
    WNDCLASSEXA wc = {0};
    wc.cbSize = sizeof(wc);
    wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wc.hCursor = LoadCursorA(NULL, IDC_ARROW);
    wc.hInstance = inst;
    wc.lpfnWndProc = &WndProc;
    wc.lpszClassName = classname;

    if(!RegisterClassExA(&wc))
        return 1;

    HWND wnd = CreateWindowExA(0, classname, classname, WS_OVERLAPPEDWINDOW | WS_VISIBLE, 100, 100, 800, 600, NULL, NULL, inst, NULL );
    if(!wnd)
        return 2;

    MSG msg;
    runApp = true;

    while(runApp)
    {
        while(PeekMessage(&msg, wnd, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        if(!runApp)
            break;

        if(loaded && (snd.canWrite()*2) >= nsf.availableSamples())
        {
            fillAudio();
        }
        else
            Sleep(10);
    }

    return 0;
}