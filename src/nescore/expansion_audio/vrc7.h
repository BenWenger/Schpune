
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
        {   Attack, Decay, Sustain, Release, Idle   };

        struct Slot
        {
            Adsr            adsr;               // Current Adsr phase
            int             rawOut[2];          // raw output
            int             output;             // actual output (used outside this slot)

            int             phase;              // Current phase (position in sine)
            int             phaseAdd;           // Value to add to phase every clock
            int             multi;

            int             baseAtten;          // base attenuation level for this slot (volume or total level)
            int             kslAtten;           // Key-Scale level attenuation

            int             egc;                // Envelope level
            int             rateAttack;
            int             rateDecay;
            int             rateSustain;
            int             rateRelease;
            int             sustainLevel;

            int             kslBits;

            bool            rectify;            // false = normal sine, true = 2nd half of sine is zero'd
            bool            amEnabled;          // AM enabled
            bool            fmEnabled;          // FM enabled
            bool            percussive;
            bool            useKsr;



            int             update(int phaseadj);

        private:
            int             updateEnv();
        };

        class Channel : public AudioChannel
        {
        public:
            virtual void            makeSilent() override;
            bool                    isTrulySilent() const;

            Slot                    slot[2];        // [0]=modulator, [1]=carrier

            int                     fNum;           // fNumber for this channel
            int                     block;
            int                     feedbackLevel;
            int                     instId;
            u8                      inst[8];

            bool                    slowRelease;

            
            void                    keyOn();
            void                    keyOff();

        protected:
            virtual int             doTicks(timestamp_t ticks, bool doaudio, bool docpu) override;
            virtual timestamp_t     clocksToNextUpdate() override;
            virtual void            recalcOutputLevels(const AudioSettings& settings, ChannelId chanid, std::vector<float> (&levels)[2]) override;
        };

        Channel     ch[6];
        u8          customInstData[0x08];
        u8          vrc7Addr;
        void        onWrite(u16 a, u8 v);
        
        void        write_reg1(Channel& c, u8 v);
        void        write_reg2(Channel& c, u8 v);
        void        write_reg3(Channel& c, u8 v);
        
        void        updateInst(Channel& c, const u8* instdat);
        void        updateFreq(Channel& c);
        
        static int  calcAtkRate(int rks, int r);
        static int  calcStdRate(int rks, int r);
    };
}

#endif
