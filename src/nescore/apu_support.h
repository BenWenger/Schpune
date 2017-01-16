
#ifndef SCHPUNE_NESCORE_APUSUPPORT_H_INCLUDED
#define SCHPUNE_NESCORE_APUSUPPORT_H_INCLUDED

#include "schpunetypes.h"



namespace schcore
{
    /////////////////////////////////////////////////
    //   APU Support systems


    /////////////////////////////////////////
    //  Length Counter (pulse, tri, noise)
    class Apu_Length
    {
    public:
        void                writeLoad(u8 v)     { if(enabled)     counter = loadTable[v >> 3];      }
        void                writeHalt(u8 v)     { halted = (v != 0);                                }
        void                writeEnable(u8 v)   { enabled = (v != 0);   if(!enabled) counter = 0;   }
        void                clock()             { if(counter && !halted)  --counter;                }
        
        bool                isAudible() const   { return counter != 0;                              }

        void                hardReset()         { counter = 0; halted = enabled = false;            }

    private:
        static const u8     loadTable[0x20];
        u8                  counter;
        bool                halted;
        bool                enabled;
    };
    
    /////////////////////////////////////////
    //  Linear Counter (tri)
    class Apu_Linear
    {
    public:
        void                writeLoad(u8 v)     { control = (v & 0x80) != 0;   load = v & 0x7F;     }
        void                writeHigh()         { reload = true;                                    }
        
        bool                isAudible() const   { return counter != 0;                              }

        void                hardReset()         { load = counter = 0; control = reload = false;     }
        
        void                clock()
        {
            if(reload)          counter = load;
            else if(counter)    --counter;

            if(!control)        reload = false;
        }

    private:
        u8                  load;
        u8                  counter;
        bool                control;
        bool                reload;
    };
    
    /////////////////////////////////////////
    //  Decay Unit (pulses, noise)
    class Apu_Decay
    {
    public:
        void                write(u8 v)
        {
            timer =      v & 0x0F;
            loop =      (v & 0x20) != 0;
            constVol =  (v & 0x10) != 0;
        }
        void                writeHigh()         { reload = true;                                    }
        u8                  getOutput() const   { return (constVol ? timer : volume);               }

        void                hardReset()
        { 
            timer = 0;
            counter = 0;
            volume = 0;
            loop = false;
            constVol = true;
            reload = false;
        }
        
        void                clock()
        {
            if(reload)
            {
                reload = false;
                volume = 0x0F;
                counter = timer;
            }
            else if(counter)
                --counter;
            else
            {
                counter = timer;
                if(volume)
                    --volume;
                else if(loop)
                    volume = 0x0F;
            }
        }

    private:
        u8                  timer;
        u8                  counter;
        u8                  volume;
        bool                loop;
        bool                constVol;
        bool                reload;
    };

    
    /////////////////////////////////////////
    //  Sweep Unit (pulses)
    class Apu_Sweep
    {
    public:
        int         getFreqTimer() const        { return (freqReg + 1) * 2;     }
        bool        isAudible() const           { return !forceSilence;         }
        void        write(u8 v)
        {
            reload = true;
            enabled = (v & 0x80) && (v & 0x07);
            negate =  (v & 0x08) != 0;
            shift =   (v & 0x07);
            timer =   (v & 0x70) >> 4;
            checkSilence();
        }
        
        void        writeFLo(u8 v)              { freqReg = (freqReg & 0x0700) | v;               checkSilence();     }
        void        writeFHi(u8 v)              { freqReg = (freqReg & 0x00FF) | ((v & 7) << 8);  checkSilence();     }

        void        clock()
        {
            if( counter )
                --counter;
            else
            {
                counter = timer;
                if(enabled && !forceSilence)
                {
                    if(negate)      freqReg -= (freqReg >> shift) + subOne;
                    else            freqReg += (freqReg >> shift);
                    checkSilence();
                }
            }

            if(reload)
            {
                reload = false;
                counter = timer;
            }
        }

        void checkSilence()
        {
            forceSilence = (freqReg < 8) || (!negate && ((freqReg + (freqReg >> shift)) > 0x7FF));
        }

        void hardReset(bool subone)
        {
            freqReg = 0x7FF;
            forceSilence = true;
            negate = false;
            enabled = false;
            reload = false;
            subOne = subone;
            shift = 0;
            timer = 0xF;
            counter = 0xF;
        }

    private:
        u16         freqReg;
        bool        forceSilence;
        bool        negate;
        bool        enabled;
        bool        reload;
        bool        subOne;

        int         shift;
        u8          timer;
        u8          counter;
    };
}

#endif