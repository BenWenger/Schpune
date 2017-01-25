
#ifndef SCHPUNE_NESCORE_INPUTDEVICE_H_INCLUDED
#define SCHPUNE_NESCORE_INPUTDEVICE_H_INCLUDED

#include "schpunetypes.h"

namespace schcore
{
    class Nes;
    namespace input
    {
        class InputDevice
        {
        public:
            virtual ~InputDevice() {}
        private:
            ///////////////////////////////////
            // interface for the Nes
            friend class Nes;
            virtual void            hardReset() = 0;
            virtual void            connect() = 0;
            virtual void            write(u8 v) = 0;
            virtual u8              read() = 0;
        };

        ///////////////////////////////////////////////////////////////
        ///////////////////////////////////////////////////////////////
        class Controller : public InputDevice
        {
        public:
            enum
            {
                Btn_A =         (1<<0),
                Btn_B =         (1<<1),
                Btn_Select =    (1<<2),
                Btn_Start =     (1<<3),
                Btn_Up =        (1<<4),
                Btn_Down =      (1<<5),
                Btn_Left =      (1<<6),
                Btn_Right =     (1<<7)
            };
            void            setState(int v, bool dpad_protect = true);

        private:
            virtual void            hardReset() override    { latch = 0; strobe_reload = false;     }
            virtual void            connect() override      { }
            virtual void            write(u8 v) override;
            virtual u8              read() override;

            u8              state = 0;
            u8              latch = 0;
            bool            strobe_reload = false;
        };
    }
}

#endif
