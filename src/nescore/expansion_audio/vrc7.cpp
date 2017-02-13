
#include "vrc7.h"
#include "../resetinfo.h"
#include "../cpubus.h"
#include <cmath>
#include <algorithm>
#include <memory>

namespace schcore
{
    ////////////////////////////////////////////////////
    ////////////////////////////////////////////////////
    // FM needs a looooot of lookup tables
    //  put that stuff in another file....
    #define SCHPUNE_VRC7LUTS_OKTOINCLUDE
    #include "vrc7_luts.h"
    #undef SCHPUNE_VRC7LUTS_OKTOINCLUDE
    
    ////////////////////////////////////////////////////
    ////////////////////////////////////////////////////
    //  Construction and reset
    Vrc7Audio::Vrc7Audio()
    {
        buildAllLuts();
    }

    void Vrc7Audio::reset(const ResetInfo& info)
    {
        if(info.hardReset)
        {
            setClockBase( info.region.apuClockBase * 36 );  // TODO - load VRC7 clock from region
            info.apu->addExAudioMaster(this);

            setApuObj(info.apu);
            int index = 0;

            for(auto& c : ch)
            {
                c.fNum =                0;
                c.block =               0;
                c.feedbackLevel =       0;
                c.instId =              0;
                for(auto& i : c.inst)   i = 0;
                c.slowRelease =         false;

                for(auto& s : c.slot)
                {
                    s.adsr =            Adsr::Idle;
                    s.rawOut[0] =       0;
                    s.rawOut[1] =       0;
                    s.output =          0;
                    s.phase =           0;
                    s.phaseAdd =        0;
                    s.multi =           0;
                    s.baseAtten =       0;
                    s.kslAtten =        0;
                    s.egc =             maxAttenuation;
                    s.rateAttack =      0;
                    s.rateDecay =       0;
                    s.rateSustain =     0;
                    s.rateRelease =     0;
                    s.sustainLevel =    0;
                    s.kslBits =         0;
                    s.rectify =         false;
                    s.amEnabled =       false;
                    s.fmEnabled =       false;
                    s.percussive =      false;
                    s.useKsr =          false;
                }

                addChannel( chanIds[index++], &c, true );
                c.setClockRate( c.getClockRate() * 36 );            // divide clocks by 36 - TODO change this to a regional setting
            }
            
            for(auto& i : customInstData)       i = 0;
            vrc7Addr = 0;

            info.cpuBus->addWriter(0x9, 0x9, this, &Vrc7Audio::onWrite );
        }
        else
        {
            for(auto& c : ch)       c.makeSilent();
        }
    }
    
    ////////////////////////////////////////////////////
    ////////////////////////////////////////////////////
    //  Misc stuff

    timestamp_t Vrc7Audio::Channel::clocksToNextUpdate()
    {
        if( isTrulySilent() )       return Time::Never;
        return 1;
    }

    void Vrc7Audio::Channel::recalcOutputLevels(const AudioSettings& settings, ChannelId chanid, std::vector<float> (&levels)[2])
    {
        doLinearOutputLevels( settings, chanid, levels, 0, 0.3f / maxSlotOutput );
    }

    bool Vrc7Audio::Channel::isTrulySilent() const
    {
        if(slot[1].adsr != Adsr::Idle)      return false;
        if(slot[1].rawOut[0] != 0)          return false;
        if(slot[1].rawOut[1] != 0)          return false;
        return true;
    }
    
    void Vrc7Audio::Channel::makeSilent()
    {
        slot[1].adsr =          Adsr::Idle;
        slot[1].egc =           maxAttenuation;
        slot[1].rawOut[0] =     0;
        slot[1].rawOut[1] =     0;
    }
    
    void Vrc7Audio::Channel::keyOn()
    {
        if(slot[1].adsr == Adsr::Release || slot[1].adsr == Adsr::Idle)
        {
            for(auto& s : slot)
            {
                s.phase = 0;
                s.egc = 0;
                s.adsr = Adsr::Attack;
            }
        }
    }

    void Vrc7Audio::Channel::keyOff()
    {
        if(slot[1].adsr == Adsr::Idle)
            return;

        // apparently you only key off the carrier?
        if(slot[1].adsr == Adsr::Attack)
            slot[1].egc = lut_attack(slot[1].egc);

        slot[1].adsr = Adsr::Release;
    }
    
    int Vrc7Audio::calcAtkRate(int rks, int r)
    {
        if(!r)      return 0;
        if(r == 15) return maxAttenuation;

        int RL = (rks & 3);
        int RM = std::min( 15, r + (rks>>2) );
        return (3 * (RL+4)) << (RM+1);
    }

    int Vrc7Audio::calcStdRate(int rks, int r)
    {
        if(!r)      return 0;

        int RL = (rks & 3);
        int RM = std::min( 15, r + (rks>>2) );
        return (RL+4) << (RM-1);
    }
    
    ////////////////////////////////////////////////////
    ////////////////////////////////////////////////////
    //  VRC7 Clocking/Updating!
    
    int Vrc7Audio::Channel::doTicks(timestamp_t ticks, bool doaudio, bool docpu)
    {
        if(!doaudio)    return 0;
        if(ticks != 1)  return slot[1].output;  // only time we'd do more than 1 tick is if we're truly silent
        
        int fb = 0;
        if(feedbackLevel)
            fb = slot[0].output >> (8 - feedbackLevel);

        return slot[1].update( slot[0].update(fb) );
    }
    
    int Vrc7Audio::Slot::update(int phaseadj)
    {
        phase += phaseAdd;
        rawOut[1] = rawOut[0];

        int         pos = phase + phaseadj;                                     // TODO - add FM
        bool        rear = (pos & phaseHighBit) != 0;

        if(rectify && rear)
        {
            // 2nd half of rectified wave = flat
            updateEnv();
            rawOut[0] = 0;
        }
        else
        {
            rawOut[0] = updateEnv() + baseAtten + kslAtten + lut_sine(pos);     // TODO - add AM

            if(rawOut[0] >= maxAttenuation)     rawOut[0] = 0;
            else
            {
                rawOut[0] = lut_linear(rawOut[0]);
                if(rear)
                    rawOut[0] = -rawOut[0];
            }
        }

        return output = ( (rawOut[0] + rawOut[1]) >> 1 );
    }

    int Vrc7Audio::Slot::updateEnv()
    {
        switch(adsr)
        {
        case Adsr::Idle:
            return maxAttenuation;

        case Adsr::Attack:
            egc += rateAttack;
            if(egc >= maxAttenuation)
            {
                adsr = Adsr::Decay;
                return (egc = 0);
            }
            return lut_attack(egc);

        case Adsr::Decay:
            egc += rateDecay;
            if(egc >= sustainLevel)
            {
                egc = sustainLevel;
                adsr = Adsr::Sustain;
            }
            break;

        case Adsr::Sustain:         egc += rateSustain;         break;
        case Adsr::Release:         egc += rateRelease;         break;
        }

        if(egc >= maxAttenuation)
        {
            egc = maxAttenuation;
            adsr = Adsr::Idle;
        }
        return egc;
    }
    
    ////////////////////////////////////////////////////
    ////////////////////////////////////////////////////
    //  Register writes

    void Vrc7Audio::onWrite(u16 a, u8 v)
    {
        a &= 0x9030;
        if(a == 0x9010)
            vrc7Addr = (v & 0x3F);
        else if(a == 0x9030)
        {
            switch(vrc7Addr)
            {
            case 0x00: case 0x01: case 0x02: case 0x03:
            case 0x04: case 0x05: case 0x06: case 0x07:
                customInstData[vrc7Addr] = v;
                break;
                
            case 0x10: case 0x11: case 0x12: case 0x13: case 0x14: case 0x15:   catchUp(); write_reg1(ch[vrc7Addr & 0x07], v);  break;
            case 0x20: case 0x21: case 0x22: case 0x23: case 0x24: case 0x25:   catchUp(); write_reg2(ch[vrc7Addr & 0x07], v);  break;
            case 0x30: case 0x31: case 0x32: case 0x33: case 0x34: case 0x35:   catchUp(); write_reg3(ch[vrc7Addr & 0x07], v);  break;
            }
        }
    }
    
    void Vrc7Audio::write_reg1(Channel& c, u8 v)
    {
        c.fNum =        (c.fNum & 0x0100) | v;
        updateFreq(c);
    }

    void Vrc7Audio::write_reg2(Channel& c, u8 v)
    {
        c.fNum =        (c.fNum & 0x00FF) | ((v & 0x01) << 8);
        c.block =       (v >> 1) & 7;
        c.slowRelease = (v & 0x20) != 0;

        if(v & 0x10)
        {
            if(!c.instId)
                updateInst(c, customInstData);  // updating inst also updates freq
            else
                updateFreq(c);
            c.keyOn();
        }
        else
        {
            updateFreq(c);
            c.keyOff();
        }
    }

    void Vrc7Audio::write_reg3(Channel& c, u8 v)
    {
        // volume
        c.slot[1].baseAtten = (v & 0x0F) << (egcBitWidth - 4);

        // instrument
        v >>= 4;
        if(v != c.instId)
        {
            c.instId = v;
            updateInst(c, v ? fixedInstruments[v-1] : customInstData);
        }
    }
    
    ////////////////////////////////////////////////////
    ////////////////////////////////////////////////////
    //  Updating freq and instruments
    
    void Vrc7Audio::updateInst(Channel& c, const u8* instdat)
    {
        for(int i = 0; i < 8; ++i)  c.inst[i] = instdat[i];

        c.slot[0].multi =           multiLut[c.inst[0] & 0x0F];
        c.slot[0].useKsr =          (c.inst[0] & 0x10) != 0;
        c.slot[0].percussive =      (c.inst[0] & 0x20) == 0;
        c.slot[0].fmEnabled =       (c.inst[0] & 0x40) != 0;
        c.slot[0].amEnabled =       (c.inst[0] & 0x80) != 0;
        
        c.slot[1].multi =           multiLut[c.inst[1] & 0x0F];
        c.slot[1].useKsr =          (c.inst[1] & 0x10) != 0;
        c.slot[1].percussive =      (c.inst[1] & 0x20) == 0;
        c.slot[1].fmEnabled =       (c.inst[1] & 0x40) != 0;
        c.slot[1].amEnabled =       (c.inst[1] & 0x80) != 0;

        c.slot[0].kslBits =         (c.inst[2] >> 6);
        c.slot[0].baseAtten =       (c.inst[2] & 0x3F) << (egcBitWidth - 6);
        
        c.slot[1].kslBits =         (c.inst[3] >> 6);
        c.slot[1].rectify =         (c.inst[3] & 0x10) != 0;
        c.slot[0].rectify =         (c.inst[3] & 0x08) != 0;
        c.feedbackLevel =           c.inst[3] & 0x07;
        
        c.slot[0].sustainLevel =    (c.inst[6] >> 4) * dB(3.0);         // TODO -- sustain level of F is actually max attenuation?
        c.slot[1].sustainLevel =    (c.inst[7] >> 4) * dB(3.0);         // TODO -- sustain level of F is actually max attenuation?

        updateFreq(c);
    }

    void Vrc7Audio::updateFreq(Channel& c)
    {
        const u8* inst = c.inst;

        for(auto& s : c.slot)
        {
            // Overall freq
            s.phaseAdd = c.fNum * (1<<c.block) * s.multi;

            // Key Scale Level
            if(s.kslBits)
            {
                int a = kslLut[c.fNum >> 5] - dB(6) * (7 - c.block);
                if(a <= 0)      s.kslAtten = 0;
                else            s.kslAtten = a >> (3 - s.kslBits);
            }
            else
                s.kslAtten = 0;

            // ADSR rates
            int rks = (c.block << 1) | (c.fNum >> 8);
            if(!s.useKsr)           rks >>= 2;
            
            s.rateAttack =          calcAtkRate(rks, inst[4] >> 4);
            s.rateDecay =           calcStdRate(rks, inst[4] & 0x0F);

            if(s.percussive)        s.rateSustain = calcStdRate(rks, inst[6] & 0x0F);
            else                    s.rateSustain = 0;

            if(c.slowRelease)       s.rateRelease = calcStdRate(rks, 5);
            else if(!s.percussive)  s.rateRelease = calcStdRate(rks, inst[6] & 0x0F);
            else                    s.rateRelease = calcStdRate(rks, 7);

            ++inst;
        }
    }

    //////////////////////////////////////////////////
    //  AM/FM stuff
    
    timestamp_t Vrc7Audio::audMaster_clocksToNextUpdate()
    {
        return Time::Never;     // TODO - clock AM/FM
    }

    void Vrc7Audio::audMaster_doTicks(timestamp_t ticks)
    {
        // TODO AM/FM
    }
}