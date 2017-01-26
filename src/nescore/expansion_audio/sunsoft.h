
#ifndef SCHPUNE_NESCORE_SUNSOFTAUDIO_H_INCLUDED
#define SCHPUNE_NESCORE_SUNSOFTAUDIO_H_INCLUDED

#include "exaudio.h"
#include "../audiochannel.h"

///////////////////////////////////////////
//  This code is minimal.  I'm only interested in getting Gimmick! working, so noise/envelope are not
//   implemented.  TODO:  add this eventually

namespace schcore
{
    class Apu;

    class SunsoftAudio : public ExAudio
    {
    public:
                        SunsoftAudio() {}
        virtual void    reset(const ResetInfo& info) override;
        

    private:
        class Tone : public AudioChannel
        {
        public:
            u16                     freqTimer;
            u8                      volume;
            bool                    enabled;

            virtual void            makeSilent() override   { enabled = false;  }
            void                    hardReset();

        protected:
            //  To be implemented by derived classes
            virtual int             doTicks(timestamp_t ticks, bool doaudio, bool docpu) override;
            virtual timestamp_t     clocksToNextUpdate() override;
            virtual void            recalcOutputLevels(const AudioSettings& settings, ChannelId chanid, std::vector<float> (&levels)[2]) override;

        private:
            int                     freqCounter;
            int                     dutyPhase;
        };
        
        /////////////////////////////////////////
        //  Host info
        u8          addr;
        Apu*        apu;                        // TODO -- move this to ExAudio
        Tone        chans[3];
        void        onWrite(u16 a, u8 v);
        void        catchUp();
    };
}

#endif
