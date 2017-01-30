
#include <algorithm>
#include "audiochannel.h"
#include "audiobuilder.h"

namespace schcore
{
    const float AudioChannel::baseNativeOutputLevel = 1.3f;

    inline timestamp_t AudioChannel::calcTicksToRun( timestamp_t now, timestamp_t target ) const
    {
        if(now >= target)
            return 0;

        return (target - now + clockRate - 1) / clockRate;
    }

    void AudioChannel::run(timestamp_t cputarget, timestamp_t audiotarget)
    {
        timestamp_t cputick = calcTicksToRun(cpuTimestamp, cputarget);
        timestamp_t audtick = calcTicksToRun(audTimestamp, audiotarget);

        //  Start by doing an update of 0 steps, since output may have an immediate
        //    change due to a register write
        timestamp_t step = 0;

        int out;
        float outraw;

        while(audtick > 0)
        {
            if(useRawOutput)    outraw = doTicks_raw( step, true, (cputick > 0) );
            else                out = doTicks( step, true, (cputick > 0) );

            audTimestamp += (step * clockRate);
            if(cputick > 0)             cpuTimestamp += (step * clockRate);

            if(useRawOutput)
            {
                float dif = (outraw - prevRawOut);
                if(dif*dif > 0.000001f)
                {
                    builder->addTransition( audTimestamp, dif * outputLevels[0][0], dif * outputLevels[1][0] );
                    prevRawOut = outraw;
                }
            }
            else
            {
                if(out != prevOut)
                {
                    builder->addTransition( audTimestamp, outputLevels[0][out] - outputLevels[0][prevOut],
                                                          outputLevels[1][out] - outputLevels[1][prevOut] );
                    prevOut = out;
                }
            }

            audtick -= step;
            if(cputick > 0)             cputick -= step;

            step = std::min( audtick, clocksToNextUpdate() );

            if(cputick > 0)             step = std::min( step, cputick );
        }

        /////////////////////////////////
        // So audio has all been updated.  If there is CPU that still needs updating, we can run that all
        //   in one clump
        if(cputick > 0)
        {
            if(useRawOutput)    doTicks_raw( cputick, false, true );
            else                doTicks( cputick, false, true );
            cpuTimestamp += (cputick * clockRate);
        }
    }

    ////////////////////////////////////////////
    
    void AudioChannel::updateSettings(const AudioSettings& settings, ChannelId chanid)
    {
        recalcOutputLevels( settings, chanid, outputLevels );

        useRawOutput = outputLevels[0].size() == 1;
    }
    
    std::pair<float,float> AudioChannel::getVolMultipliers(const AudioSettings& settings, ChannelId chanid)
    {
        std::pair<float,float>  out;

        out.first = out.second = settings.chans[chanid].vol;

        if(settings.stereo)
        {
            float pan = settings.chans[chanid].pan;

            if(pan < 0)     out.second *= 1.0f + pan;
            if(pan > 0)     out.first  *= 1.0f - pan;
        }

        return out;
    }
    
    ////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////
    
    void AudioChannel::doLinearOutputLevels(const AudioSettings& settings, ChannelId chanid, std::vector<float> (&levels)[2], int maxstep, float baseoutput)
    {
        auto mul = getVolMultipliers( settings, chanid );
        float scale = baseoutput * settings.masterVol;
        
        if(maxstep <= 0)
        {
            levels[0].clear();      levels[0].push_back( mul.first  * scale );
            levels[1].clear();      levels[1].push_back( mul.second * scale );
        }
        else
        {
            levels[0].resize(maxstep+1);
            levels[1].resize(maxstep+1);

            for(int i = 0; i <= maxstep; ++i)
            {
                levels[0][i] = i * mul.first  * scale / maxstep;
                levels[1][i] = i * mul.second * scale / maxstep;
            }
        }
    }

}