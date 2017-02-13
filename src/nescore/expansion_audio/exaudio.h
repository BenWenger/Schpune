
#ifndef SCHPUNE_NESCORE_EXAUDIO_H_INCLUDED
#define SCHPUNE_NESCORE_EXAUDIO_H_INCLUDED

#include "../apu.h"

namespace schcore
{
    class ResetInfo;

    /////////////////////
    //  There isn't very much to this class
    //  This is abstracted mostly for NSFs

    class ExAudio : public SubSystem
    {
    public:
        virtual ~ExAudio() = default;
        virtual void reset(const ResetInfo& info) = 0;

        // Stuff only needed for things which have a master clock (MMC5's frame sequencer, VRC7's AM/FM)
        virtual timestamp_t audMaster_clocksToNextUpdate()          { return Time::Never;  }
        virtual void        audMaster_doTicks(timestamp_t ticks)    { }

        
        virtual void        run(timestamp_t runto) override
        {
            auto ticks = unitsToTimestamp(runto);
            cyc(ticks);
            audMaster_doTicks(ticks);
        }

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
