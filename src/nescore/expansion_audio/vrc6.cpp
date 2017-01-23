
#include "vrc6.h"
#include "../resetinfo.h"
#include "../apu.h"
#include "../cpubus.h"

namespace schcore
{
    //////////////////////////////////////////
    //////////////////////////////////////////
    //  Host

    void Vrc6Audio::reset(const ResetInfo& info)
    {
        if(info.hardReset)
        {
            pulse[0].hardReset();
            pulse[1].hardReset();
            saw.hardReset();

            apu = info.apu;
            apu->addExAudioChannel( ChannelId::vrc6_pulse0,  &pulse[0] );
            apu->addExAudioChannel( ChannelId::vrc6_pulse1, &pulse[1] );
            apu->addExAudioChannel( ChannelId::vrc6_saw, &saw );

            info.cpuBus->addWriter( 0x9, 0xB, this, &Vrc6Audio::onWrite );
        }
        else
        {
            // Are these disabled on soft reset?  Let's say yes.
            pulse[0].makeSilent();
            pulse[1].makeSilent();
            saw.makeSilent();
        }
    }

    inline void Vrc6Audio::catchUp()
    {
        apu->catchUp();
    }

    void Vrc6Audio::onWrite(u16 a, u8 v)
    {
        if(swapLines)
        {
            switch(a & 3)
            {
            case 1: case 2: a ^= 3;
            }
        }

        switch(a & 0xF003)
        {
        case 0x9000: case 0x9001: case 0x9002:  catchUp();  pulse[0].write(a & 3, v);       break;
        case 0xA000: case 0xA001: case 0xA002:  catchUp();  pulse[1].write(a & 3, v);       break;
        case 0xB000: case 0xB001: case 0xB002:  catchUp();       saw.write(a & 3, v);       break;

        case 0x9003:
            catchUp();
            pulse[0].writeMaster(v);
            pulse[1].writeMaster(v);
            saw.writeMaster(v);
            break;
        }
    }
    
    //////////////////////////////////////////
    //////////////////////////////////////////
    //  Pulse
    
    void Vrc6Audio::Pulse::makeSilent()
    {
        enabled = false;
        dutyPhase = 0;
    }
    timestamp_t Vrc6Audio::Pulse::clocksToNextUpdate()
    {
        if(mainDisable)     return Time::Never;
        if(!volume)         return Time::Never;
        if(!enabled)        return Time::Never;
        return freqCounter;
    }
    void Vrc6Audio::Pulse::write(u16 a, u8 v)
    {
        switch(a)
        {
        case 0:     volume = (v & 0x0F);
                    if(v & 0x80)        dutyMode = 15;
                    else                dutyMode = (v>>4) & 0x07;
                    break;
                  
        case 1:     freqTimer = (freqTimer & 0x0F00) | v;
                    break;

        case 2:     freqTimer = (freqTimer & 0x00FF) | ((v & 0x0F) << 8);
                    enabled = (v & 0x80) != 0;

                    if(!enabled)
                        dutyPhase = 0;
                    break;
        }
    }
    void Vrc6Audio::Pulse::writeMaster(u8 v)
    {
        mainDisable     =   (v & 0x01) != 0;
        if(v & 0x04)        freqShifter = 8;
        else if(v & 0x02)   freqShifter = 4;
        else                freqShifter = 0;
    }
    int Vrc6Audio::Pulse::doTicks(timestamp_t ticks, bool doaudio, bool docpu)
    {
        if(!doaudio)        return 0;
        if(mainDisable)     return 0;

        freqCounter -= ticks;
        while(freqCounter <= 0)
        {
            freqCounter += (freqTimer >> freqShifter) + 1;
            if(enabled)
                dutyPhase = (dutyPhase + 1) & 0x0F;
        }

        if(!enabled)                return 0;
        if(dutyPhase <= dutyMode)   return volume;
        return 0;
    }
    void Vrc6Audio::Pulse::recalcOutputLevels(const AudioSettings& settings, ChannelId chanid, std::vector<float> (&levels)[2])
    {
        doLinearOutputLevels(settings, chanid, levels, 0x0F, -0.25848310567936736161035226455787f);
    }
    void Vrc6Audio::Pulse::hardReset()
    {
        mainDisable = false;
        freqShifter = 0;
        volume = 0;
        freqCounter = 0;
        freqTimer = 0x0FFF;
        dutyPhase = dutyMode = 0;
        enabled = false;
    }
    
    //////////////////////////////////////////
    //////////////////////////////////////////
    //  Sawtooth

    void Vrc6Audio::Sawtooth::makeSilent()
    {
        enabled = false;
        accumulator = 0;
        phase = 0;
    }
    timestamp_t Vrc6Audio::Sawtooth::clocksToNextUpdate()
    {
        if(mainDisable)     return Time::Never;
        if(!accAdd)         return Time::Never;
        if(!enabled)        return Time::Never;
        return freqCounter;
    }
    void Vrc6Audio::Sawtooth::write(u16 a, u8 v)
    {
        switch(a)
        {
        case 0:     accAdd = (v & 0x3F);
                    break;
                  
        case 1:     freqTimer = (freqTimer & 0x0F00) | v;
                    break;

        case 2:     freqTimer = (freqTimer & 0x00FF) | ((v & 0x0F) << 8);
                    enabled = (v & 0x80) != 0;

                    if(!enabled)
                    {
                        accumulator = 0;
                        phase = 0;
                    }
                    break;
        }
    }
    void Vrc6Audio::Sawtooth::writeMaster(u8 v)
    {
        mainDisable     =   (v & 0x01) != 0;
        if(v & 0x04)        freqShifter = 8;
        else if(v & 0x02)   freqShifter = 4;
        else                freqShifter = 0;
    }
    int Vrc6Audio::Sawtooth::doTicks(timestamp_t ticks, bool doaudio, bool docpu)
    {
        if(!doaudio)        return 0;
        if(mainDisable)     return 0;

        freqCounter -= ticks;
        while(freqCounter <= 0)
        {
            freqCounter += (freqTimer >> freqShifter) + 1;
            if(enabled)
            {
                ++phase;
                if(phase >= 14)
                {
                    phase = 0;
                    accumulator = 0;
                }
                else if(!(phase & 1))
                    accumulator += accAdd;
            }
        }

        return accumulator >> 3;
    }
    void Vrc6Audio::Sawtooth::recalcOutputLevels(const AudioSettings& settings, ChannelId chanid, std::vector<float> (&levels)[2])
    {
        doLinearOutputLevels(settings, chanid, levels, 0x1F, -0.25848310567936736161035226455787f * 2);
    }
    void Vrc6Audio::Sawtooth::hardReset()
    {
        mainDisable = false;
        freqShifter = 0;
        freqCounter = 0;
        freqTimer = 0x0FFF;
        phase = 0;
        accAdd = 0;
        accumulator = 0;
        enabled = false;
    }

}