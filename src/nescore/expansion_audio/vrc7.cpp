
#include "vrc7.h"
#include "../resetinfo.h"
#include "../cpubus.h"
#include <cmath>
#include <algorithm>
#include <memory>

//#define DISABLE_MODULATOR           // for debugging only -- be sure to uncomment!

namespace schcore
{
    namespace
    {
        //////////////////////////////////////////////////////
        //////////////////////////////////////////////////////
        //  Lookup Tables and constants
        typedef std::unique_ptr<int[]>  lut_t;

        const int max_dB =              48;                         // max attenuation is 48 dB
        const int maxAttenuationBits =  23;
        const int maxAttenuation =      (1<<maxAttenuationBits);    // This value effectively represents max_dB in "internal units"

        constexpr int dB(double db)             // handy function to provide a given dB level in internal units
        {
            return static_cast<int>( (db / max_dB) * maxAttenuation );
        }

        // dB to Linear conversion lut
        const int lut_LinearMaxOut =    (1<<20)-1;                  // slot output is 20 bits wide
        const int lut_LinearBits =      10;                         // number of relevant bits for lookup
        const int lut_LinearShift =     (maxAttenuationBits - lut_LinearBits);
        const int lut_LinearSize =      (1<<lut_LinearBits);
        const int lut_LinearMask =      lut_LinearSize - 1;
        lut_t       lut_Linear;

        // sine lut
        const int lut_SineBits =        8;
        const int lut_SineShift =       (18 - lut_SineBits);        // slot freq counter is 18 bits wide
        const int lut_SineSize =        (1<<lut_SineBits);
        const int lut_SineMask =        (1 << (lut_SineBits+1) )-1; // extra bit for the XOR bit
        const int lut_SineXorBit =      lut_SineSize;
        const int lut_SineRectifyBit =  lut_SineSize<<1;
        lut_t       lut_Sine;
        
        // Attack lut
        const int lut_AttackBits =      10;
        const int lut_AttackShift =     (23 - lut_AttackBits);      // EGC is 23 bits wide
        const int lut_AttackSize =      (1<<lut_AttackBits);
        const int lut_AttackMask =      (lut_AttackSize-1);
        lut_t       lut_Attack;
        
        // AM lut
        const int lut_AmBits =          10;
        const int lut_AmShift =         (20 - lut_AmBits);          // AM/FM counter is 20 bits wide
        const int lut_AmSize =          (1<<lut_AmBits);
        const int lut_AmMask =          (lut_AmSize-1);
        lut_t       lut_Am;
        
        // FM lut
        const int lut_FmBits =          10;
        const int lut_FmShift =         (20 - lut_FmBits);          // AM/FM counter is 20 bits wide
        const int lut_FmSize =          (1<<lut_FmBits);
        const int lut_FmMask =          (lut_FmSize-1);
        lut_t       lut_Fm;

        ///////////////////////////////////////////////////////
        ///////////////////////////////////////////////////////
        const double        pi = 3.1415926535897932384626433832795;

        bool        tablesBuilt = false;
        void        buildCommonTables()
        {
            // did we build these already?
            if(tablesBuilt)     return;

            ////////////////////////////////////////////
            //  dB to Linear output conversion
            {
                // Linear = 10 ^ (dB / -20)
                //      In our units, 'maxAttenuation' represents 'max_dB' decibels.
                //      Further, we want dB=maxAttenuation -> Linear=0,
                //        and dB=0 -> Linear=lut_LinearMaxOut
                //
                // So we want to scale this output
            
                lut_Linear = lut_t( new int[lut_LinearSize] );
                for(int i = 0; i < lut_LinearSize; ++i)
                {
                    double dB = i << lut_LinearShift;           // 'dB' is the dB level in our internal units (maxAttenuation represents max_dB decibels)
                    dB        = (dB / maxAttenuation) * max_dB; // convert that to actual decibels.  No longer in internal units

                    double linear = std::pow( 10.0, dB / -20.0 );   // do the conversion
                    linear *= lut_LinearMaxOut;                     // scale up to our desired output

                    lut_Linear[i] = static_cast<int>(linear);
                }
            }
                        
            ////////////////////////////////////////////
            //  sine table
            {
                // This is sort of the inverse of the Linear table.  We want to convert linear units (sine function output)
                //   to decibels:
                //       dB     = -20 * log10( Linear )
                //
                // But again we want this to work with our units.  'Linear' in this case is going to be bound [0,1] because it's
                //   the output of sine, so we want Linear=1 -> dB=0 and Linear=0 -> dB=maxAttenuation
                //
                // This sine table is also just the quarter sine table... so it only covers up through sin(pi/2)
                
                lut_Sine = lut_t( new int[lut_SineSize] );
                for(int i = 0; i < lut_SineSize; ++i)
                {
                    double sinx = std::sin( (pi / 2.0) * i / lut_SineSize );
                    if(sinx == 0)           lut_Sine[i] = maxAttenuation;
                    else
                    {
                        double dB = -20.0 * std::log10( sinx );         // dB is now the number of decibels
                        if(dB >= max_dB)    lut_Sine[i] = maxAttenuation;
                        else                lut_Sine[i] = static_cast<int>( dB / max_dB * maxAttenuation ); // scale up to desired base
                    }
                }
            }

            ////////////////////////////////////////////
            //  attack table
            {
                // Attack table determines the output attenuation when the envelope generator is in attack
                //   mode.
                //
                // attack_output = 48 dB - (48 dB * ln(EGC) / ln(maxAttenuation))
                //
                // note that '48 dB' is effectively 'maxAttenuation'
                
                double lnfull = std::log( static_cast<double>(maxAttenuation) );

                lut_Attack = lut_t( new int[lut_AttackSize] );
                for(int i = 0; i < lut_AttackSize; ++i)
                {
                    double egc = i << lut_AttackShift;
                    lut_Attack[i] = static_cast<int>( maxAttenuation - (maxAttenuation * std::log(egc) / lnfull) );
                }
            }
            
            ////////////////////////////////////////////
            //  AM table
            {
                // AM table = the output of the AM unit
                //
                //    sinx = sin(2 * pi * counter / (1<<20))
                //    AM_output = (1.0 + sinx) * 0.6 dB
                
                lut_Am = lut_t( new int[lut_AmSize] );
                for(int i = 0; i < lut_AmSize; ++i)
                {
                    int ctr = (i << lut_AmShift);
                    double sinx = std::sin( 2 * pi * ctr / (1<<20) );
                    lut_Am[i] = static_cast<int>( (1.0 + sinx) * dB(0.6) );
                }
            }
            
            ////////////////////////////////////////////
            //  FM table
            {
                // FM table = the output of the FM unit
                //
                //    sinx = sin(2 * pi * counter / (1<<20))
                //    FM_output = 2 ^ (13.75 / 1200 * sinx)
                
                lut_Fm = lut_t( new int[lut_FmSize] );
                for(int i = 0; i < lut_FmSize; ++i)
                {
                    int ctr = (i << lut_FmShift);
                    double sinx = std::sin( 2 * pi * ctr / (1<<20) );
                    lut_Fm[i] = static_cast<int>( std::pow(2, 13.75 / 1200 * sinx) );
                }
            }
        }   // End buildCommonTables

        const u8 fixedInstData[0xF][8] = {
            { 0x03, 0x21, 0x05, 0x06, 0xB8, 0x82, 0x42, 0x27 },
            { 0x13, 0x41, 0x13, 0x0D, 0xD8, 0xD6, 0x23, 0x12 },
            { 0x31, 0x11, 0x08, 0x08, 0xFA, 0x9A, 0x22, 0x02 },
            { 0x31, 0x61, 0x18, 0x07, 0x78, 0x64, 0x30, 0x27 },
            { 0x22, 0x21, 0x1E, 0x06, 0xF0, 0x76, 0x08, 0x28 },
            { 0x02, 0x01, 0x06, 0x00, 0xF0, 0xF2, 0x03, 0xF5 },
            { 0x21, 0x61, 0x1D, 0x07, 0x82, 0x81, 0x16, 0x07 },
            { 0x23, 0x21, 0x1A, 0x17, 0xCF, 0x72, 0x25, 0x17 },
            { 0x15, 0x11, 0x25, 0x00, 0x4F, 0x71, 0x00, 0x11 },
            { 0x85, 0x01, 0x12, 0x0F, 0x99, 0xA2, 0x40, 0x02 },
            { 0x07, 0xC1, 0x69, 0x07, 0xF3, 0xF5, 0xA7, 0x12 },
            { 0x71, 0x23, 0x0D, 0x06, 0x66, 0x75, 0x23, 0x16 },
            { 0x01, 0x02, 0xD3, 0x05, 0xA3, 0x92, 0xF7, 0x52 },
            { 0x61, 0x63, 0x0C, 0x00, 0x94, 0xAF, 0x34, 0x06 },
            { 0x21, 0x62, 0x0D, 0x00, 0xB1, 0xA0, 0x54, 0x17 }
        };

        const int lut_Multi[0x10] = {
            1, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 20, 24, 24, 30, 30
        };

        const int lut_Ksl[0x10] = {
            dB( 0.00),  dB(18.00),  dB(24.00),  dB(27.75),
            dB(30.00),  dB(32.25),  dB(33.75),  dB(35.25),
            dB(36.00),  dB(37.50),  dB(38.25),  dB(39.00),
            dB(39.75),  dB(40.50),  dB(41.25),  dB(42.00)
        };

        // A small little class to help with KSR calculations
        class KsrAid
        {
        public:
            KsrAid(int kb) : KB(kb) {}

            void        setR(int r)
            {
                int rks = r*4 + KB;
                RH = rks >> 2;
                RL = rks & 0x03;

                if(RH > 15)
                    RH = 15;
            }
            
            int         getRH()     { return RH;                }
            int         getRH_1()   { return RH ? (RH-1) : 0;   }
            int         getRL()     { return RL;                }

        private:
            int KB = 0;
            int RH = 0;
            int RL = 0;
        };
    }   // End anonymous namespace

    
    ///////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////

    Vrc7Audio::Vrc7Audio()
    {
        buildCommonTables();
    }

    void Vrc7Audio::reset(const ResetInfo& info)
    {
        if(info.hardReset)
        {
            setApuObj(info.apu);

            vrc7Addr = 0;
            for(auto& i : customInstData)   i = 0;
            for(int i = 0; i < 6; ++i)
            {
                auto& c = channels[i];

                c.fNum =        0;
                c.block =       0;
                c.feedback =    0;
                c.instId =      0;
                c.customInstData = customInstData;
                c.sustainOn =   false;
                for(auto& i : c.instData)   i = 0;
                for(auto& s : c.slot)
                {
                    s.phase =               0;
                    s.phaseRate =           0;
                    s.adsr =                Adsr::Idle;
                    s.egc =                 maxAttenuation;
                    s.attackRate =          0;
                    s.decayRate =           0;
                    s.sustainRate =         0;
                    s.releaseRate =         0;
                    s.sustainLevel =        0;
                    s.baseAtten =           0;
                    s.kslAtten =            0;
                    s.rectifyMultipler =    -1;
                    s.amEnabled =           false;
                    s.fmEnabled =           false;
                }

                addChannel( static_cast<ChannelId>(ChannelId::vrc7_0 + i), &c, true );
                c.setClockRate( c.getClockRate() * 36 );                        // TODO - pull this from region settings instead
            }

            info.cpuBus->addWriter(0x9, 0x9, this, &Vrc7Audio::onWrite);
        }
    }

    void Vrc7Audio::onWrite(u16 a, u8 v)
    {
        a &= 0x9030;
        if(a == 0x9010)             vrc7Addr = v & 0x3F;
        else if(a == 0x9030)
        {
            switch(vrc7Addr)
            {
            case 0x00: case 0x01: case 0x02: case 0x03:
            case 0x04: case 0x05: case 0x06: case 0x07:
                customInstData[vrc7Addr] = v;
                break;
                
            case 0x10: case 0x11: case 0x12: case 0x13: case 0x14: case 0x15:
            case 0x20: case 0x21: case 0x22: case 0x23: case 0x24: case 0x25:
            case 0x30: case 0x31: case 0x32: case 0x33: case 0x34: case 0x35:
                channels[vrc7Addr & 7].write(vrc7Addr >> 4, v);
                break;
            }
        }
    }

    // TODO - Vrc7Audio needs a way to have a main clock to drive the AM/FM units.
    //   MMC5 will need this too for its frame counter
    
    ///////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////

    void Vrc7Audio::Channel::makeSilent()
    {
        slot[0].egc =  slot[1].egc =  maxAttenuation;
        slot[0].adsr = slot[1].adsr = Adsr::Idle;
    }

    void Vrc7Audio::Channel::recalcOutputLevels(const AudioSettings& settings, ChannelId chanid, std::vector<float> (&levels)[2])
    {
        doLinearOutputLevels( settings, chanid, levels, 0, 0.2f / (1<<20) );
    }

    bool Vrc7Audio::Channel::isTrulyIdle() const
    {
        if(slot[0].adsr != Adsr::Idle)      return false;
        if(slot[1].adsr != Adsr::Idle)      return false;
        if(slot[0].output)                  return false;
        if(slot[1].output)                  return false;
        return true;
    }
    
    timestamp_t Vrc7Audio::Channel::clocksToNextUpdate()
    {
        if(isTrulyIdle())               return Time::Never;
        return 1;
    }
    
    int Vrc7Audio::Channel::doTicks(timestamp_t ticks, bool doaudio, bool docpu)
    {
        if(!doaudio)        return 0;
        if(isTrulyIdle())   return 0;

        while(ticks-- > 0)
        {
            int fb = 0;
            if(feedback)        fb = slot[0].output >> (8 - feedback);

#ifdef DISABLE_MODULATOR
            slot[1].update( 0 );
#else
            slot[0].update(       fb       );
            slot[1].update( slot[0].output * 4 );
#endif
        }
        return slot[1].output;
    }

    void Vrc7Audio::Channel::write(u8 a, u8 v)
    {
        switch(a)
        {
        case 1:
            fNum = (fNum & 0x0100) | v;
            updateFreq();
            break;

        case 2:
            fNum =      (fNum & 0x00FF) | ((v & 0x01) << 8);
            block =     (v >> 1) & 0x07;
            sustainOn = (v & 0x20) != 0;

            if(v & 0x10)        updateFreq_and_keyOn();
            else                updateFreq_and_keyOff();
            break;

        case 3:
            slot[1].baseAtten =     dB(3.00) * (v & 0x0F);
            if(instId != (v>>4))
                captureInstrument( v>>4 );      // this might not take effect until next key on? ???
            break;
        }
    }
    
    void Vrc7Audio::Channel::updateFreq()
    {
        // unique stuff rom inst[2]/inst[3]
        slot[0].baseAtten =         dB(0.75) * (instData[2] & 0x3F);
        slot[0].rectifyMultipler =  (instData[3] & 0x08) ? 0 : -1;
        slot[1].rectifyMultipler =  (instData[3] & 0x10) ? 0 : -1;
        feedback =                  (instData[3] & 0x07);

        for(int i = 0; i < 2; ++i)
        {
            auto& slt = slot[i];

            // Base phase additive
            slt.phaseRate  = fNum * (1 << block);
            slt.phaseRate *= lut_Multi[ instData[i+0] & 0x0F ];     // * the multiplier for the current slot (based on instrument settings)

                // other data from inst[0]/inst[1]
            bool ksrbit =       (instData[i+0] & 0x10) != 0;
            bool percussive =   (instData[i+0] & 0x20) == 0;
            slt.fmEnabled =     (instData[i+0] & 0x40) != 0;
            slt.amEnabled =     (instData[i+0] & 0x80) != 0;

                // stuff from inst[2]/inst[3]
            int  ksl =          (instData[i+2] >> 6);
            if(!ksl)
                slt.kslAtten = 0;
            else
            {
                int a =     lut_Ksl[ fNum >> 5 ] - (6 * (7-block));
                if(a < 0)       slt.kslAtten = 0;
                else            slt.kslAtten = a >> (3-ksl);
            }

                // ADSR rates (inst[4] - inst[7])
            int kb = (block << 1) | (fNum >> 8);
            if(!ksrbit)     kb >>= 2;
            KsrAid  ksr(kb);

                // -- attack rate
            ksr.setR( instData[i + 4] >> 4 );
            slt.attackRate =    (12 * (ksr.getRL()+4)) << ksr.getRH();

                // -- decay rate
            ksr.setR( instData[i+4] & 0x0F );
            slt.decayRate =     (ksr.getRL()+4) << ksr.getRH_1();
            
                // -- sustain rate, level
            if(percussive)      ksr.setR( instData[i+6] & 0x0F );
            else                ksr.setR( 0 );
            slt.sustainRate =   (ksr.getRL()+4) << ksr.getRH_1();
            slt.sustainLevel =  dB(3.0) * (instData[i+6] >> 4);

                // -- release rate
            if(sustainOn)       ksr.setR( 5 );
            else if(percussive) ksr.setR( instData[i+6] & 0x0F );
            else                ksr.setR( 7 );
            slt.releaseRate =   (ksr.getRL()+4) << ksr.getRH_1();
        }
    }

    void Vrc7Audio::Channel::captureInstrument(int inst)
    {
        const u8* src = customInstData;
        if(inst)        src = fixedInstData[inst-1];
        for(int i = 0; i < 8; ++i)      instData[i] = src[i];
        instId = inst;

        updateFreq();
    }

    void Vrc7Audio::Channel::updateFreq_and_keyOn()
    {
        for(auto& s : slot)
        {
            // Can we key on?
            if(s.adsr == Adsr::Release || s.adsr == Adsr::Idle)
            {
                s.adsr = Adsr::Attack;
                s.egc = 0;
                s.phase = 0;
            }
        }

        // If we have instrument 0, reload it to capture any written changes
        if(!instId)         captureInstrument( 0 );
        else                updateFreq();               // otherwise, just update the freq
    }

    void Vrc7Audio::Channel::updateFreq_and_keyOff()
    {
        updateFreq();
        for(auto& s : slot)
        {
            if(s.adsr == Adsr::Attack)
                s.egc = s.getEnvelope();
            if(s.adsr != Adsr::Idle)
                s.adsr = Adsr::Release;
        }
    }
    
    ///////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////

    void Vrc7Audio::Slot::update(int phaseadj)
    {
        // Update envelope
        updateEnvelope();

        // Update the sine phase
        int fm = 1;                     // TODO - get FM output if enabled
        phase += phaseRate * fm / 1;        // TODO is this supposed to be div 4 ???  Or should the clock speed be halved?  ???

        int useph = ((phase + phaseadj) >> lut_SineShift);
        bool rectified = (useph & lut_SineRectifyBit) != 0;
        useph &= lut_SineMask;
        if(useph & lut_SineXorBit)      useph ^= lut_SineMask;
        
        // calc new output
        int am = 0;                     // TODO - get AM output if enabled
        int next = baseAtten + lut_Sine[useph] + getEnvelope() + kslAtten + am;
        

        // make 'next' linear
        if(next >= maxAttenuation)  next  = 0;
        else                        next  = lut_Linear[  (next>>lut_LinearShift) & lut_LinearMask  ];
        

        if(rectified)               next *= rectifyMultipler;

        output = (next + output) / 2;
    }
    
    void Vrc7Audio::Slot::updateEnvelope()
    {
        switch(adsr)
        {
        case Adsr::Attack:
            egc += attackRate;
            if(egc >= maxAttenuation)           {   egc = 0;                adsr = Adsr::Decay;     }
            break;
        case Adsr::Decay:
            egc += decayRate;
            if(egc >= sustainLevel)             {  adsr = Adsr::Sustain;   }
            break;
        case Adsr::Sustain:
            egc += sustainRate;
            break;
        case Adsr::Release:
            egc += releaseRate;
            break;
        }

        if(egc >= maxAttenuation)
        {
            egc = maxAttenuation;
            adsr = Adsr::Idle;
        }
    }
    
    int Vrc7Audio::Slot::getEnvelope()
    {
        if(adsr == Adsr::Attack)        return lut_Attack[ (egc >> lut_AttackShift) & lut_AttackMask ];
        else                            return egc;
    }
}