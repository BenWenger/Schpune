
#include <algorithm>
#include "audiochannel.h"
#include "audiobuilder.h"

namespace schcore
{    
    void AudioChannel::run(timestamp_t runto, bool doaudio, bool docpu)
    {
        // add clockRate-1 to dif to round up
        timestamp_t ticksToRun = (runto - timestamp + (clockRate - 1)) / clockRate;
        timestamp_t thistime = timestamp;


        //  Start by doing an update of 0 steps, since output may have an immediate
        //    change due to a register write
        timestamp_t step = 0;

        while(ticksToRun > 0)
        {
            int out = doTicks( step, doaudio, docpu );
            thistime += (step * clockRate);

            if(out != prevOut)
            {
                builder->addTransition( thistime, outputLevels[0][out] - outputLevels[0][prevOut],
                                                  outputLevels[1][out] - outputLevels[1][prevOut] );
                prevOut = out;
            }

            ticksToRun -= step;
            step = std::min( ticksToRun, clocksToNextUpdate() );
        }

        // if this was a CPU run, take the timestamp
        timestamp = thistime;
    }


}