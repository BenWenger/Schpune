
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
        enum class Adsr
        {
            Attack, Decay, Sustain, Release, Idle
        };

        struct Slot
        {
            u32             phase;
            int             output;

            /////////////////////////////////////
            u32             egc;            // envelope generation counter
            Adsr            adsr;
            u32             baseAtten;      // base attenuation

            bool            enableAm;
            bool            enableFm;
            bool            percussive;
            bool            ksr;            // key scale rate
            bool            rectifySine;
            int             multi;
            int             ksl;            // key scale level
            
            int             attackRate;
            int             decayRate;
            u32             sustainLevel;
            int             releaseRate;
        };

        class Channel : public AudioChannel
        {
        public:
            virtual void            makeSilent() override;
            void                    write_1(u8 v);
            void                    write_2(u8 v);
            void                    write_3(u8 v);
            void                    hardReset(Vrc7Audio* thehost);
            
        protected:
            //  To be implemented by derived classes
            virtual int             doTicks(timestamp_t ticks, bool doaudio, bool docpu) override { return 0; }
            virtual float           doTicks_raw(timestamp_t ticks, bool doaudio, bool docpu) override;
            virtual timestamp_t     clocksToNextUpdate() override;
            virtual void            recalcOutputLevels(const AudioSettings& settings, ChannelId chanid, std::vector<float> (&levels)[2]) override;

        private:
            friend class Vrc7Audio;
            Vrc7Audio*              host;
            void                    keyOn();
            void                    keyOff();
            int                     fNum;
            int                     block;
            bool                    slowRelease;
            Slot                    slots[2];       // [0]=modulator, [1]=carrier

            int                     instSelect;
            int                     feedback;

            void                    updateInstrumentInfo();
            void                    clockSlot( Slot& slt, int phaseadj );
            int                     getKeyScaleAttenuation( Slot& slt );
            int                     getEnvelope( Slot& slt );
        };

        /////////////////////////////////////////
        //  Host info
        friend class Channel;
        u8          addr;
        u8          customInstData[8];
        Channel     channels[6];
        u32         amPhase;
        u32         fmPhase;

        std::vector<int>        quarterSineLut;     // 0x80 entries -- use 7 bit lookups
        std::vector<int>        keyScaleLevelLut;

        void        onWrite(u16 a, u8 v);
    };
}

#endif
