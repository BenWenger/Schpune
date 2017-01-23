
//#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS
#define NOMINMAX
#include <Windows.h>
#include <soundout.h>
#include <fstream>
#include <cstdio>
#include <algorithm>

#include "nes.h"

void textOut(HDC dc, int x, int y, const char* str)
{
    TextOutA(dc, x, y, str, std::strlen(str));
}

const bool inStereo =       false;

const char* const           classname = "Temp NSF Player";
//schcore::TempFront          nsf;
schcore::Nes                nes;
SoundOut                    snd(48000, inStereo, 200);
bool                        runApp = true;
bool                        loaded = false;
std::ofstream               traceFile;
bool                        tracing = false;
const char* const           traceFileName = "tempnsf_trace.txt";
const char* const           dumpFileName = "tempnsf_dump.bin";

unsigned char               tempdumpbuf[10000];


FILE*                       dumpfile = nullptr;
bool                        dumping = false;


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
        //sprintf( buf, "Track %d of %d", nsf.getTrack(), nsf.getTrackCount() );
        sprintf( buf, "Track %d of %d", nes.nsf_getTrack(), nes.nsf_getTrackCount() );
        textOut( dc, 40, 80, buf );
    }

    if(tracing)
    {
        textOut( dc, 60, 200, " ** TRACING **" );
    }
    if(dumping)
    {
        textOut( dc, 60, 250, " ** DUMPING **" );
    }

    EndPaint(wnd, &ps);
}

void toggleDump()
{
    if(!dumping)
    {
        if(!dumpfile)
            dumpfile = fopen(dumpFileName, "wb");
        if(dumpfile)
        {
            dumping = true;
        }
    }
    else
    {
        dumping = false;
        fclose(dumpfile);
        dumpfile = nullptr;
    }
}

void toggleTrace()
{
    if(!tracing)
    {
        if(!traceFile.is_open())
            traceFile.open(traceFileName);
        if(traceFile.is_open() && traceFile.good())
        {
            tracing = true;
            //nsf.setTracer( &traceFile );   TODO -- implement this
            nes.setTracer( &traceFile );
        }
    }
    else
    {
        tracing = false;
        //nsf.setTracer(nullptr);
        nes.setTracer( nullptr );
    }
}

void fillAudio()
{
    {
        using schcore::s16;
        auto lk = snd.lock();
        int written = 0;
        

        if(dumping)
        {
            //written = nsf.getSamples( reinterpret_cast<s16*>(tempdumpbuf), lk.getSize(0) + lk.getSize(1), nullptr, 0 );
            written = nes.getAudio( tempdumpbuf, lk.getSize(0) + lk.getSize(1), nullptr, 0 );
            if(dumping)
                fwrite( tempdumpbuf, 1, written, dumpfile );
            
            int writa = std::min(lk.getSize(0), written);
            int writb = std::min(lk.getSize(1), written - writa);

            memcpy( lk.getBuffer(0), tempdumpbuf, writa );
            memcpy( lk.getBuffer(1), tempdumpbuf + writa, writb );
            written = 0;
        }
        else
        {
            //written = nsf.getSamples( lk.getBuffer<s16>(0), lk.getSize(0), lk.getBuffer<s16>(1), lk.getSize(1) );
            written = nes.getAudio( lk.getBuffer(0), lk.getSize(0), lk.getBuffer(1), lk.getSize(1) );
        }

        lk.setWritten(written);
    }
    //nsf.doFrame();
    nes.doFrame();
}

void changeTrack(int chg, HWND wnd)
{
    //int oldtrack = nsf.getTrack();
    int oldtrack = nes.nsf_getTrack();
    int newtrack = oldtrack + chg;
    if(newtrack < 1)                        newtrack = 1;
    if(newtrack > nes.nsf_getTrackCount())  newtrack = nes.nsf_getTrackCount();

    if(newtrack != oldtrack)
    {
        nes.nsf_setTrack(newtrack);
        InvalidateRect(wnd,nullptr,true);
    }
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
    {
        try
        {
            nes.loadFile(filebuf);
            loaded = true;
        }
        catch(std::exception& e)
        {
            loaded = false;
            MessageBoxA(wnd, e.what(), "File load error", MB_OK);
        }
    }

    InvalidateRect(wnd,NULL,true);

    if(loaded)
    {
        snd.stop(true);
        nes.doFrame();
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
        {
            switch(w)
            {
            case VK_ESCAPE:
                loadFile(wnd);
                break;
            case VK_TAB:
                toggleTrace();
                InvalidateRect(wnd,nullptr,true);
                break;
            case VK_BACK:
                toggleDump();
                InvalidateRect(wnd,nullptr,true);
                break;
            case VK_LEFT:       changeTrack( -1,wnd);   break;
            case VK_RIGHT:      changeTrack(  1,wnd);   break;
            case VK_UP:         changeTrack( 10,wnd);   break;
            case VK_DOWN:       changeTrack(-10,wnd);   break;
            }
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

        if(loaded && (snd.canWrite() >= nes.getAvailableAudioSize()))
        {
            fillAudio();
        }
        else
            Sleep(10);
    }

    if(dumping)
        toggleDump();

    return 0;
}