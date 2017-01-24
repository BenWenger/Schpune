
//#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS
#define NOMINMAX
#include <Windows.h>
#include <soundout.h>
#include <fstream>
#include <cstdio>
#include <algorithm>

#include "nes.h"
#include "display.h"

namespace
{
    Display             display;
    schcore::Nes        nes;
    SoundOut            snd(48000, false, 100);
    bool                runApp = true;
    bool                loaded = false;
    bool                isnsf;
    HWND                wnd;
}

LRESULT CALLBACK wndProc(HWND wnd, UINT msg, WPARAM w, LPARAM l);

void makeWindow(HINSTANCE inst, const char* windowname)
{
    WNDCLASSEXA         wc = {};
    wc.cbSize =         sizeof(wc);
    wc.hCursor =        LoadCursorA(nullptr, IDC_ARROW);
    wc.hInstance =      inst;
    wc.lpfnWndProc =    &wndProc;
    wc.lpszClassName =  windowname;
    RegisterClassExA(&wc);

    static const int targetWd = 512;
    static const int targetHt = 480;

    wnd = CreateWindowExA( 0, windowname, windowname, WS_BORDER | WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU | WS_OVERLAPPED, 300, 300, targetWd, targetHt, nullptr, nullptr, inst, nullptr );

    RECT rc;
    GetClientRect(wnd,&rc);
    rc.right = (targetWd * 2) - rc.right;
    rc.bottom = (targetHt * 2) - rc.bottom;

    MoveWindow(wnd, 300, 300, rc.right, rc.bottom, false);
    ShowWindow(wnd, SW_SHOW);
}

void handlePaint()
{
    PAINTSTRUCT ps;
    HDC dc = BeginPaint(wnd,&ps);
    display.blit(dc);
    EndPaint(wnd,&ps);
}

bool readyForNextFrame()
{
    if(!loaded)     return false;

    return snd.canWrite() >= nes.getApproxNaturalAudioSize();
}

void fillAudio()
{
    using schcore::s16;
    auto lk = snd.lock();

    int written = nes.getAudio( lk.getBuffer(0), lk.getSize(0), lk.getBuffer(1), lk.getSize(1) );

    lk.setWritten(written);
}

void doFrame()
{
    nes.doFrame();
    fillAudio();
    if(!isnsf)
    {
        display.refresh_rom( nes.getVideoBuffer() );
        HDC dc = GetDC(wnd);
        display.blit(dc);
        ReleaseDC(wnd, dc);
    }
}

void changeNsfTrack(int change)
{
    if(!isnsf || !loaded)
        return;

    int cur = nes.nsf_getTrack();
    int next = cur + change;
    int count = nes.nsf_getTrackCount();
    
    if(next > count)
        next = count;
    if(next < 1)
        next = 1;

    if(cur != next)
    {
        nes.nsf_setTrack(next);
        display.refresh_nsf(next, count);
        InvalidateRect(wnd,nullptr,false);
    }
}

void loadFile()
{
    snd.stop(false);
    
    char filebuf[500] = "";

    OPENFILENAMEA ofn = {0};
    ofn.hwndOwner =         wnd;
    ofn.Flags =             OFN_HIDEREADONLY | OFN_FILEMUSTEXIST;
    ofn.lpstrFilter =       "Supported Files\0*.nsf;*.nes\0All Files\0*\0\0";
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
            MessageBoxA(wnd, e.what(), "File load error", MB_OK);
            loaded = false;
        }
    }
    else        // hit 'cancel' to get out of dialog -- no change
    {
        if(loaded)
            snd.play();
        return;
    }
    
    if(loaded)
    {
        isnsf = nes.isNsf();
        if(isnsf)
        {
            display.refresh_nsf( nes.nsf_getTrack(), nes.nsf_getTrackCount() );
            InvalidateRect(wnd,nullptr,false);
        }

        snd.stop(true);
        doFrame();
        snd.play();
    }
}

LRESULT CALLBACK wndProc(HWND wnd, UINT msg, WPARAM w, LPARAM l)
{
    switch(msg)
    {
    case WM_DESTROY:
        runApp = false;
        break;

    case WM_PAINT:
        handlePaint();
        break;

    case WM_KEYDOWN:
        {
            switch(w)
            {
            case VK_ESCAPE:     loadFile();             break;
            case VK_LEFT:       changeNsfTrack( -1);    break;
            case VK_RIGHT:      changeNsfTrack(  1);    break;
            case VK_UP:         changeNsfTrack( 10);    break;
            case VK_DOWN:       changeNsfTrack(-10);    break;
            }
        }
        break;
    }

    return DefWindowProc(wnd,msg,w,l);
}

int WINAPI WinMain(HINSTANCE inst, HINSTANCE prev, LPSTR cmd, int show)
{
    makeWindow(inst,"Schpune");
    runApp = true;

    MSG msg;
    while(runApp)
    {
        while(PeekMessage(&msg, wnd, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        if(!runApp)
            break;

        if(readyForNextFrame())     doFrame();
        else                        Sleep(1);
    }

    return 0;
}