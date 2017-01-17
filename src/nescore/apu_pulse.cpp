
#include "apu_pulse.h"
#include <algorithm>

namespace schcore
{
    
    const bool Apu_Pulse::Data::dutyLut[4][8] = 
    {
        {false,true ,false,false,false,false,false,false},
        {false,true ,true ,false,false,false,false,false},
        {false,true ,true ,true ,true ,false,false,false},
        {true ,false,false,true ,true ,true ,true ,true }
    };

    void Apu_Pulse::writeMain(u16 a, u8 v)
    {
        auto& ch = dat[(a >> 2) & 1];

        switch(a & 3)
        {
        case 0: ch.decay.write(v);
                ch.dutyMode = (v >> 6);
                ch.length.writeHalt(v & 0x20);
                break;

        case 1: ch.sweep.write(v);
                break;

        case 2: ch.sweep.writeFLo(v);
                break;

        case 3: ch.sweep.writeFHi(v);
                ch.length.writeLoad(v);
                ch.decay.writeHigh();
                ch.dutyPhase = 0;
                break;
        }
    }

    void Apu_Pulse::write4015(u8 v)
    {
        dat[0].length.writeEnable(v & 0x01);
        dat[1].length.writeEnable(v & 0x02);
    }

    void Apu_Pulse::read4015(u8& v)
    {
        if(dat[0].length.isAudible())   v |= 0x01;
        if(dat[1].length.isAudible())   v |= 0x02;
    }
    
    int Apu_Pulse::doTicks(timestamp_t ticks, bool doaudio, bool docpu)
    {
        if(!doaudio)
            return 0;

        return dat[0].clock(ticks) | (dat[1].clock(ticks) << 4);
    }

    void Apu_Pulse::clockSeqHalf()
    {
        for(auto& ch : dat)
        {
            ch.length.clock();
            ch.sweep.clock();
        }
    }
    
    void Apu_Pulse::clockSeqQuarter()
    {
        for(auto& ch : dat)
        {
            ch.decay.clock();
        }
    }

    timestamp_t Apu_Pulse::clocksToNextUpdate()
    {
        timestamp_t next = Time::Never;

        if(dat[0].isAudible())      next = dat[0].freqCounter;
        if(dat[1].isAudible())      next = std::min(next, dat[1].freqCounter);

        return next;
    }

    void Apu_Pulse::reset(bool hard)
    {
        if(hard)
        {
            channelHardReset();

            dat[0].sweep.hardReset(false);
            dat[1].sweep.hardReset(true );

            for(auto& i : dat)
            {
                i.decay.hardReset();
                i.length.hardReset();
                i.dutyMode = 0;
                i.dutyPhase = 0;
                i.freqCounter = 1;
            }
        }
        else
        {
            dat[0].length.writeEnable(0);
            dat[1].length.writeEnable(0);
        }
    }
    
    void Apu_Pulse::recalcOutputLevels(const AudioSettings& settings, ChannelId chanid, std::vector<float> (&levels)[2])
    {
        levels[0].resize(0x100);
        levels[1].resize(0x100);

        /*
                          95.88
pulse_out = ------------------------------------
             (8128 / (pulse1 + pulse2)) + 100

             at max output, pulse_out = 0.25848310567936736161035226455787
        */

        static const float normalMaxOutput = 0.25848310567936736161035226455787f;
        float mvol = static_cast<float>( settings.masterVol * baseNativeOutputLevel );
        
        auto p_0 = getVolMultipliers( settings, ChannelId::pulse0 );
        auto p_1 = getVolMultipliers( settings, ChannelId::pulse1 );
        
        float t;

        if(settings.nonLinearPulse)
        {
            //  Non-linear pulse is more accurate, but has aliasing
            for(int i = 0; i < 0x100; ++i)
            {
                // L
                t       = (i & 0x0F) * p_0.first;
                t      += ( i >> 4 ) * p_1.first;
                if(t != 0)
                {
                    t = (8128 / t) + 100;
                    t = mvol * 95.88f / t;
                }
                levels[0][i] = t;
                
                // R
                t       = (i & 0x0F) * p_0.second;
                t      += ( i >> 4 ) * p_1.second;
                if(t != 0)
                {
                    t = (8128 / t) + 100;
                    t = mvol * 95.88f / t;
                }
                levels[1][i] = t;
            }
        }
        else
        {
            // Linear pulse removes aliasing, but not accurate (aliasing existed on real system)
            for(int i = 0; i < 0x100; ++i)
            {
                // L
                t       = (i & 0x0F) * p_0.first / 15.0f;
                t      += ( i >> 4 ) * p_1.first / 15.0f;
                levels[0][i] = t * normalMaxOutput * mvol;
                
                // R
                t       = (i & 0x0F) * p_0.second / 15.0f;
                t      += ( i >> 4 ) * p_1.second / 15.0f;
                levels[1][i] = t * normalMaxOutput * mvol;
            }
        }
    }
}
