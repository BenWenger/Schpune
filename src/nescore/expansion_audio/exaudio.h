
#ifndef SCHPUNE_NESCORE_EXAUDIO_H_INCLUDED
#define SCHPUNE_NESCORE_EXAUDIO_H_INCLUDED

#include "../apu.h"

namespace schcore
{
    class ResetInfo;

    /////////////////////
    //  There isn't very much to this class
    //  This is abstracted mostly for NSFs

    class ExAudio
    {
    public:
        virtual ~ExAudio() = default;
        virtual void reset(const ResetInfo& info) = 0;

    protected:
        void setApuObj(Apu* apu_)      { apu = apu_;       }
        void catchUp()
        {
            apu->catchUp();
        }
        
        void addChannel(ChannelId id, AudioChannel* channel, bool apply_clock_rate = true)
        {
            apu->addExAudioChannel(id, channel, apply_clock_rate);
        }

    private:
        Apu* apu = nullptr;
    };
}


#endif
