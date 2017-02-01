
#ifndef SCHPUNE_NESCORE_VRC7AUDIO_H_INCLUDED
#define SCHPUNE_NESCORE_VRC7AUDIO_H_INCLUDED

#include "exaudio.h"
#include "../audiochannel.h"

namespace schcore
{
    class Apu;

    class Vrc7Audio : public ExAudio
    {
    public:
                        Vrc7Audio();
        virtual void    reset(const ResetInfo& info) override;
        

    private:
        void        onWrite(u16 a, u8 v);
    };
}

#endif
