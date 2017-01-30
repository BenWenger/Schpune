
#ifndef SCHPUNE_NESCORE_VRC6AUDIO_H_INCLUDED
#define SCHPUNE_NESCORE_VRC6AUDIO_H_INCLUDED

#include "exaudio.h"
#include "../audiochannel.h"

namespace schcore
{
    class Apu;

    class Vrc6Audio : public ExAudio
    {
    public:
                        Vrc6Audio(bool swap_lines) : swapLines(swap_lines) {}
        virtual void    reset(const ResetInfo& info) override;
        

    private:
        class Pulse : public AudioChannel
        {
        public:
            virtual void            makeSilent() override;
            void                    write(u16 a, u8 v);
            void                    writeMaster(u8 v);
            void                    hardReset();

        protected:
            //  To be implemented by derived classes
            virtual int             doTicks(timestamp_t ticks, bool doaudio, bool docpu) override;
            virtual timestamp_t     clocksToNextUpdate() override;
            virtual void            recalcOutputLevels(const AudioSettings& settings, ChannelId chanid, std::vector<float> (&levels)[2]) override;

        private:
            bool                    mainDisable;
            int                     freqShifter;
            u8                      volume;
            int                     freqCounter;
            u16                     freqTimer;
            int                     dutyPhase;
            int                     dutyMode;
            bool                    enabled;
        };
        
        class Sawtooth : public AudioChannel
        {
        public:
            virtual void            makeSilent() override;
                        
            void                    write(u16 a, u8 v);
            void                    writeMaster(u8 v);
            void                    hardReset();

        protected:
            //  To be implemented by derived classes
            virtual int             doTicks(timestamp_t ticks, bool doaudio, bool docpu) override;
            virtual timestamp_t     clocksToNextUpdate() override;
            virtual void            recalcOutputLevels(const AudioSettings& settings, ChannelId chanid, std::vector<float> (&levels)[2]) override;

        private:
            bool                    mainDisable;
            int                     freqShifter;
            int                     freqCounter;
            u16                     freqTimer;
            int                     phase;
            u8                      accAdd;
            u8                      accumulator;
            bool                    enabled;
        };

        /////////////////////////////////////////
        //  Host info
        bool        swapLines;
        Pulse       pulse[2];
        Sawtooth    saw;
        void        onWrite(u16 a, u8 v);
    };
}

#endif
