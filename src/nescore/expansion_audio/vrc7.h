
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
            int         phase;                      // current phase (in the sine wave).  Effectively 18-bits wide
            int         phaseRate;                  // F num * (1 << block) * multiplier
            Adsr        adsr;                       // current adsr mode
            int         egc;                        // the envelope counter (effectively 23-bits wide)
            int         attackRate;                 // amount to add to egc every clock in attack mode
            int         decayRate;                  //  ... in decay
            int         sustainRate;                //  ...
            int         releaseRate;                //  ...
            int         sustainLevel;               // level at which you switch from Decay->Sustain
            int         baseAtten;                  // base attenuation (ie, volume for the carrier, level for the modulator)
            int         kslAtten;                   // attenuation for ksl (based on current F Number, ksl settings, etc)
            int         output;                     // previous output of this slot
            int         rectifyMultipler;           // -1 for normal sine waves, 0 for rectified sine waves
            
            bool        amEnabled;                  // AM enabled for this slot
            bool        fmEnabled;                  // FM enabled for this slot

            void        update(int phaseadj);
            int         getEnvelope();
        private:
            void        updateEnvelope();
        };

        class Channel : public AudioChannel
        {
        public:
            virtual void            makeSilent() override;

            bool                    isTrulyIdle() const;
            void                    write(u8 a, u8 v);

            Slot                    slot[2];            // 0=Modulator, 1=Carrier
            int                     fNum;               // the F number for this channel
            int                     block;              // the block (octave) as written
            int                     feedback;           // the feedback level (as written)
            int                     instId;             // ID of the selected instrument
            u8                      instData[8];        // Current 8-byte instrument data set
            const u8*               customInstData;     // pointer to where the custom instrument data set is stored
            bool                    sustainOn;          // 'Sustain On' bit (reg $2x)

        protected:
            virtual int             doTicks(timestamp_t ticks, bool doaudio, bool docpu) override;
            virtual timestamp_t     clocksToNextUpdate() override;
            virtual void            recalcOutputLevels(const AudioSettings& settings, ChannelId chanid, std::vector<float> (&levels)[2]) override;
            
            void                    updateFreq();
            void                    captureInstrument(int inst);
            void                    updateFreq_and_keyOn();
            void                    updateFreq_and_keyOff();
        };

        Channel     channels[6];
        u8          customInstData[0x08];
        u8          vrc7Addr;
        void        onWrite(u16 a, u8 v);
    };
}

#endif
