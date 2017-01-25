
#include "inputdevice.h"

namespace schcore
{
    namespace input
    {
        //////////////////////////////////////////////
        //  Standard Controller
        void Controller::setState(int v, bool dpad_protect)
        {
            state = static_cast<u8>(v & 0xFF);

            if(dpad_protect)
            {
                static const int UD = Btn_Up | Btn_Down;
                static const int LR = Btn_Left | Btn_Right;
                
                if((state & UD) == UD)      state &= ~UD;
                if((state & LR) == LR)      state &= ~LR;
            }
        }

        void Controller::write(u8 v)
        {
            bool newstrobe = (v & 0x01) != 0;

            if(strobe_reload && !newstrobe)
                latch = state;

            strobe_reload = newstrobe;
        }

        u8 Controller::read()
        {
            if(strobe_reload)       return (state & 0x01);

            u8 out = (state & 0x01);
            state >>= 1;
            state |= 0x80;
            return out;
        }
    }
}