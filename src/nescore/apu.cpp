
#include "apu.h"

namespace schcore
{

    //////////////////////////////////////////////////////////
    //   Reg access
    void Apu::onWrite(u16 a, u8 v)
    {
        switch(a)
        {
        case 0x4000: case 0x4001: case 0x4002: case 0x4003:
        case 0x4004: case 0x4005: case 0x4006: case 0x4007:
            catchUp();
            pulses.writeMain(a,v);
            break;
            
        case 0x4008: case 0x4009: case 0x400A: case 0x400B:
        case 0x400C: case 0x400D: case 0x400E: case 0x400F:
        case 0x4010: case 0x4011: case 0x4012: case 0x4013:
            catchUp();
            //tnd.writeMain(a,v);       TODO uncomment once TND exists
            break;


        case 0x4015:
            catchUp();
            pulses.write4015(v);
            //tnd.write4015(v);       TODO uncomment once TND exists
            break;
        }
    }

    void Apu::onRead(u16 a, u8& v)
    {
        if(a == 0x4015)
        {
            v &= ~0x20;
            pulses.read4015(v);
            //tnd.read4015(v);       TODO uncomment once TND exists
        }
    }
    
    //////////////////////////////////////////////////////////
    //   Running



}