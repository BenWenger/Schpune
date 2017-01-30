
#include "sunsoft.h"
#include "../resetinfo.h"
#include "../apu.h"
#include "../cpubus.h"

namespace schcore
{

    void SunsoftAudio::reset(const ResetInfo& info)
    {
        if(info.hardReset)
        {
            addr = 0;
            chans[0].hardReset();
            chans[1].hardReset();
            chans[2].hardReset();
            
            setApuObj(info.apu);
            addChannel( ChannelId::sunsoft_chan0, &chans[0] );
            addChannel( ChannelId::sunsoft_chan1, &chans[1] );
            addChannel( ChannelId::sunsoft_chan2, &chans[2] );
            
            info.cpuBus->addWriter(0xC, 0xF, this, &SunsoftAudio::onWrite);
        }
    }

    void SunsoftAudio::onWrite(u16 a, u8 v)
    {
        if((a & 0xE000) == 0xC000)
            addr = (v & 0x0F);
        else
        {
            catchUp();
            switch(addr)
            {
            case 0x0:   chans[0].freqTimer = (chans[0].freqTimer & 0x0F00) | v;                 break;
            case 0x1:   chans[0].freqTimer = (chans[0].freqTimer & 0x00FF) | ((v & 0x0F) << 8); break;
            case 0x2:   chans[1].freqTimer = (chans[1].freqTimer & 0x0F00) | v;                 break;
            case 0x3:   chans[1].freqTimer = (chans[1].freqTimer & 0x00FF) | ((v & 0x0F) << 8); break;
            case 0x4:   chans[2].freqTimer = (chans[2].freqTimer & 0x0F00) | v;                 break;
            case 0x5:   chans[2].freqTimer = (chans[2].freqTimer & 0x00FF) | ((v & 0x0F) << 8); break;

            case 0x7:
                chans[0].enabled = !(v & 0x01);
                chans[1].enabled = !(v & 0x02);
                chans[2].enabled = !(v & 0x04);
                break;
                
            case 0x8:   chans[0].volume = (v & 0x0F);               break;
            case 0x9:   chans[1].volume = (v & 0x0F);               break;
            case 0xA:   chans[2].volume = (v & 0x0F);               break;
            }
        }
    }

    void SunsoftAudio::Tone::hardReset()
    {
        channelHardReset();
        freqTimer       = 0x0FFF;
        volume          = 0;
        enabled         = false;
        freqCounter     = 0x0FFF;
        dutyPhase       = 0;
    }

    int SunsoftAudio::Tone::doTicks(timestamp_t ticks, bool doaudio, bool docpu)
    {
        if(!doaudio)    return false;

        if(!enabled)    return 0;

        freqCounter -= ticks;
        while(freqCounter <= 0)
        {
            freqCounter += freqTimer + 1;
            dutyPhase = (dutyPhase + 1) & 0x1F;
        }

        if(dutyPhase & 0x10)
            return volume;
        return 0;
    }

    timestamp_t SunsoftAudio::Tone::clocksToNextUpdate()
    {
        if(!enabled || !volume)
            return Time::Never;

        return freqCounter;
    }

    void SunsoftAudio::Tone::recalcOutputLevels(const AudioSettings& settings, ChannelId chanid, std::vector<float> (&levels)[2])
    {
        static const float chanbasevol = 0.0045f;

        auto mul = getVolMultipliers( settings, chanid );
        levels[0].resize(0x10);
        levels[1].resize(0x10);

        float scale = settings.masterVol;

        float vol = chanbasevol;
        for(int i = 0; i < 0x10; ++i)
        {
            levels[0][i] = scale * vol * mul.first;
            levels[1][i] = scale * vol * mul.second;

            vol *= 1.4125f;
        }
    }

}