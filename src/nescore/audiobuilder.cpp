
#include <algorithm>
#include "audiobuilder.h"
#include "audiotimestampholder.h"

namespace schcore
{
    AudioBuilder::AudioBuilder()
    {
        sampleRate = 48000;
        stereo = false;

        clocksPerSecond = 0;
        clocksPerFrame = 0;
        bufferSizeInElements = 0;

        timeScalar = 0;
        timeOverflow = 0;
        timeShift = 0;

        outSample[0] = outSample[1] = 0;
    }

    
    void AudioBuilder::hardReset()
    {
        audioTimestampHolders.clear();
        flushTransitionBuffers();
    }

    ////////////////////////////////////////////////////
    //  Generate audio samples!!!

    int AudioBuilder::generateSamples(int startpos, s16* audio, int count_in_s16s)
    {
        int maxtopull = bufferSizeInElements - startpos;
        int desired = stereo ? (count_in_s16s / 2) : count_in_s16s;
        int actual = std::min(maxtopull, desired);

        if(stereo)
        {
            // TODO

            return actual * 2;
        }
        else
        {
            for(int i = 0; i < actual; ++i)
            {
                outSample[0] += transitionBuffer[0][startpos + i];

                // TODO -- add REAL filters here.. this is temporary!
                s16 v;
                if     (outSample[0] < -0x7FFF)     v = -0x7FFF;
                else if(outSample[0] >  0x7FFF)     v =  0x7FFF;
                else                                v = static_cast<s16>( outSample[0] );

                v -= v >> 10;

                audio[i] = v;
            }

            return actual;
        }

        // should never reach here
        return 0;
    }
    
    ////////////////////////////////////////////////////
    //  Wipe Samples from the transition buffer
    void AudioBuilder::wipeSamples(int count_in_s16s)
    {
        int count = count_in_s16s;
        if(stereo)
            count /= 2;

        if(count <= 0)      return;     // This should never happen

        for(int chan = 0; chan < (stereo ? 2 : 1); ++chan)
        {
            for(int i = count; i < bufferSizeInElements; ++i)
                transitionBuffer[chan][i - count] = transitionBuffer[chan][i];
            for(int i = bufferSizeInElements - count; i < bufferSizeInElements; ++i)
                transitionBuffer[chan][i] = 0;
        }

        ///////////////////////////////////////
        //  we need to adjust timeOverflow to account for the wiped samples,
        //    and we need to adjust all channels audio timestamps to rebase them

        // cut however many samples we just wiped
        timeOverflow -= (count << (timeShift+5));

        // if overflow is less than 0 (likely at this point), change it to be >= 0 so our
        //   max timestamp is relevant
        if(timeOverflow < 0)
        {
            timestamp_t flip = -timeOverflow;
            flip += timeScalar - 1;         // round up
            flip /= timeScalar;

            for(auto& tsh : audioTimestampHolders)
                tsh->subtractFromAudioTimestamp( flip );

            timeOverflow += (flip * timeScalar);
        }
    }
    
    int AudioBuilder::samplesAvailableAtTimestamp( timestamp_t time )
    {
        auto x = ((time * timeScalar) + timeOverflow) >> (timeShift + 5);

        return static_cast<int>(x) - 1;
    }

    timestamp_t AudioBuilder::timestampToProduceSamples( int s16s )
    {
        timestamp_t samps = s16s;
        if(stereo)      samps /= 2;
        ++samps;

        return ((samps << (timeShift + 5)) - timeOverflow) / timeScalar;
    }
    
    ////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////
    
    void AudioBuilder::setFormat( int set_samplerate, bool set_stereo )
    {
        // no change, just exit
        if(sampleRate == set_samplerate && stereo == set_stereo)
            return;

        // otherwise, adapt changes
        sampleRate = set_samplerate;
        stereo = set_stereo;

        recalc();
    }
    
    void AudioBuilder::setClockRates( timestamp_t clocks_per_second, timestamp_t clocks_per_frame )
    {
        // no change?  just exit
        if( clocksPerSecond == clocks_per_second && clocksPerFrame == clocks_per_frame )
            return;
        
        clocksPerSecond = clocks_per_second;
        clocksPerFrame = clocks_per_frame;

        recalc();
    }

    void AudioBuilder::recalc()
    {
        if(clocksPerFrame <= 0 || clocksPerSecond <= 0 || sampleRate <= 0 || (clocksPerFrame > clocksPerSecond) )
        {
            timeOverflow = 0;
            timeShift = 0;
            timeScalar = 0;
            bufferSizeInElements = 0;
            return;
        }

        static constexpr timestamp_t stopvalue = (Time::Never >> 2);

        // get as many bits of fraction as possible
        timeOverflow    = 0;
        timeShift       = -1;
        do
        {
            ++timeShift;

            // do these next calculations in floating point to avoid wrapping errors
            double c = 1 << (timeShift+5);
            c /= clocksPerSecond;
            c *= sampleRate;
            timeScalar = static_cast<timestamp_t>(c);
        } while( (clocksPerFrame * timeScalar) < stopvalue );

        //////////////////////
        // how big do we need to make our buffer
        bufferSizeInElements  = ((clocksPerFrame * timeScalar) >> (timeShift+5));
        bufferSizeInElements += 15;         // a little padding for the transitions

        flushTransitionBuffers();
    }

    void AudioBuilder::flushTransitionBuffers()
    {
        outSample[0] = 0;
        outSample[1] = 0;
        
        transitionBuffer[0].clear();
        transitionBuffer[1].clear();

        transitionBuffer[0].resize( bufferSizeInElements, 0.0f );
        if(stereo)
            transitionBuffer[1].resize( bufferSizeInElements, 0.0f );

        timeOverflow = 0;
    }

    namespace
    {
        ////////////////////////////////////////////////////////////////////
        //  There are 32 "sets".
        //      Set 0 is the transition happening exactly on a sample.
        //      Each following set is the transition happening progressively further between two samples, with set 16 being
        //   exactly halfway.
        //
        //      The sum of all transitions in a set is equal to 1.0, resulting in a full transition.
        //
        //      These sets make up the "ripples" that happen near the transitions of audio changes in BL-synth
        //
        const float sincTable[0x20][13] = {
            { -0.01881860f,  0.04358196f, -0.05657481f,  0.06979057f, -0.08697524f,  0.15081083f,  0.79637059f,  0.15081083f, -0.08697524f,  0.06979057f, -0.05657481f,  0.04358196f, -0.01881860f },
            { -0.01945354f,  0.04421701f, -0.05558047f,  0.06619458f, -0.07859476f,  0.12590771f,  0.79545561f,  0.17645174f, -0.09493902f,  0.07291968f, -0.05714877f,  0.04260481f, -0.01803459f },
            { -0.01993629f,  0.04450949f, -0.05417985f,  0.06216846f, -0.06987282f,  0.10182403f,  0.79271466f,  0.20274391f, -0.10241105f,  0.07554779f, -0.05729117f,  0.04128859f, -0.01710575f },
            { -0.02026493f,  0.04446150f, -0.05238979f,  0.05775124f, -0.06088443f,  0.07863619f,  0.78815967f,  0.22959627f, -0.10931694f,  0.07764365f, -0.05699384f,  0.03963890f, -0.01603749f },
            { -0.02043870f,  0.04407763f, -0.05022974f,  0.05298396f, -0.05170392f,  0.05641500f,  0.78181042f,  0.25691367f, -0.11558341f,  0.07917924f, -0.05625177f,  0.03766392f, -0.01483631f },
            { -0.02045799f,  0.04336488f, -0.04772156f,  0.04790931f, -0.04240451f,  0.03522540f,  0.77369452f,  0.28459733f, -0.12113872f,  0.08012994f, -0.05506319f,  0.03537439f, -0.01350981f },
            { -0.02032436f,  0.04233264f, -0.04488933f,  0.04257131f, -0.03305792f,  0.01512620f,  0.76384715f,  0.31254524f, -0.12591316f,  0.08047487f, -0.05342957f,  0.03278354f, -0.01206660f },
            { -0.02004048f,  0.04099251f, -0.04175915f,  0.03701497f, -0.02373397f, -0.00383014f,  0.75231099f,  0.34065273f, -0.12983946f,  0.08019705f, -0.05135578f,  0.02990701f, -0.01051629f },
            { -0.01961011f,  0.03935828f, -0.03835890f,  0.03128599f, -0.01450018f, -0.02159770f,  0.73913589f,  0.36881291f, -0.13285325f,  0.07928364f, -0.04885000f,  0.02676282f, -0.00886939f },
            { -0.01903807f,  0.03744573f, -0.03471806f,  0.02543039f, -0.00542145f, -0.03813724f,  0.72437869f,  0.39691721f, -0.13489347f,  0.07772608f, -0.04592376f,  0.02337123f, -0.00713728f },
            { -0.01833017f,  0.03527250f, -0.03086739f,  0.01949419f,  0.00344031f, -0.05341630f,  0.70810289f,  0.42485595f, -0.13590283f,  0.07552031f, -0.04259195f,  0.01975461f, -0.00533211f },
            { -0.01749319f,  0.03285800f, -0.02683879f,  0.01352309f,  0.01202646f, -0.06740929f,  0.69037830f,  0.45251880f, -0.13582813f,  0.07266685f, -0.03887270f,  0.01593733f, -0.00346673f },
            { -0.01653478f,  0.03022315f, -0.02266498f,  0.00756219f,  0.02028195f, -0.08009752f,  0.67128071f,  0.47979541f, -0.13462073f,  0.06917092f, -0.03478733f,  0.01194559f, -0.00155458f },
            { -0.01546344f,  0.02739030f, -0.01837926f,  0.00165561f,  0.02815554f, -0.09146916f,  0.65089145f,  0.50657594f, -0.13223683f,  0.06504252f, -0.03036028f,  0.00780727f,  0.00039036f },
            { -0.01428841f,  0.02438300f, -0.01401530f, -0.00415373f,  0.03560005f, -0.10151924f,  0.62929702f,  0.53275158f, -0.12863788f,  0.06029644f, -0.02561893f,  0.00355171f,  0.00235370f },
            { -0.01301960f,  0.02122580f, -0.00960685f, -0.00982445f,  0.04257257f, -0.11024953f,  0.60658861f,  0.55821515f, -0.12379082f,  0.05495233f, -0.02059351f, -0.00079045f,  0.00432075f },
            { -0.01166752f,  0.01794411f, -0.00518749f, -0.01531690f,  0.04903463f, -0.11766843f,  0.58286161f,  0.58286161f, -0.11766843f,  0.04903463f, -0.01531690f, -0.00518749f,  0.00627659f },
            { -0.01024320f,  0.01456395f, -0.00079045f, -0.02059351f,  0.05495233f, -0.12379082f,  0.55821515f,  0.60658861f, -0.11024953f,  0.04257257f, -0.00982445f, -0.00960685f,  0.00820620f },
            { -0.00875809f,  0.01111180f,  0.00355171f, -0.02561893f,  0.06029644f, -0.12863788f,  0.53275158f,  0.62929702f, -0.10151924f,  0.03560005f, -0.00415373f, -0.01401530f,  0.01009459f },
            { -0.00722398f,  0.00761434f,  0.00780727f, -0.03036028f,  0.06504252f, -0.13223683f,  0.50657594f,  0.65089145f, -0.09146916f,  0.02815554f,  0.00165561f, -0.01837926f,  0.01192686f },
            { -0.00565291f,  0.00409833f,  0.01194559f, -0.03478733f,  0.06917092f, -0.13462073f,  0.47979541f,  0.67128071f, -0.08009752f,  0.02028195f,  0.00756219f, -0.02266498f,  0.01368837f },
            { -0.00405706f,  0.00059033f,  0.01593733f, -0.03887270f,  0.07266685f, -0.13582813f,  0.45251880f,  0.69037830f, -0.06740929f,  0.01202646f,  0.01352309f, -0.02683879f,  0.01536481f },
            { -0.00244868f, -0.00288344f,  0.01975461f, -0.04259195f,  0.07552031f, -0.13590283f,  0.42485595f,  0.70810289f, -0.05341630f,  0.00344031f,  0.01949419f, -0.03086739f,  0.01694234f },
            { -0.00083999f, -0.00629729f,  0.02337123f, -0.04592376f,  0.07772608f, -0.13489347f,  0.39691721f,  0.72437869f, -0.03813724f, -0.00542145f,  0.02543039f, -0.03471806f,  0.01840766f },
            {  0.00075691f, -0.00962630f,  0.02676282f, -0.04885000f,  0.07928364f, -0.13285325f,  0.36881291f,  0.73913589f, -0.02159770f, -0.01450018f,  0.03128599f, -0.03835890f,  0.01974817f },
            {  0.00233012f, -0.01284641f,  0.02990701f, -0.05135578f,  0.08019705f, -0.12983946f,  0.34065273f,  0.75231099f, -0.00383014f, -0.02373397f,  0.03701497f, -0.04175915f,  0.02095204f },
            {  0.00386806f, -0.01593466f,  0.03278354f, -0.05342957f,  0.08047487f, -0.12591316f,  0.31254524f,  0.76384715f,  0.01512620f, -0.03305792f,  0.04257131f, -0.04488933f,  0.02200828f },
            {  0.00535949f, -0.01886930f,  0.03537439f, -0.05506319f,  0.08012994f, -0.12113872f,  0.28459733f,  0.77369452f,  0.03522540f, -0.04240451f,  0.04790931f, -0.04772156f,  0.02290689f },
            {  0.00679366f, -0.02162997f,  0.03766392f, -0.05625177f,  0.07917924f, -0.11558341f,  0.25691367f,  0.78181042f,  0.05641500f, -0.05170392f,  0.05298396f, -0.05022974f,  0.02363893f },
            {  0.00816034f, -0.02419782f,  0.03963890f, -0.05699384f,  0.07764365f, -0.10931694f,  0.22959627f,  0.78815967f,  0.07863619f, -0.06088443f,  0.05775124f, -0.05238979f,  0.02419657f },
            {  0.00944989f, -0.02655564f,  0.04128859f, -0.05729117f,  0.07554779f, -0.10241105f,  0.20274391f,  0.79271466f,  0.10182403f, -0.06987282f,  0.06216846f, -0.05417985f,  0.02457320f },
            {  0.01065338f, -0.02868796f,  0.04260481f, -0.05714877f,  0.07291968f, -0.09493902f,  0.17645174f,  0.79545561f,  0.12590771f, -0.07859476f,  0.06619458f, -0.05558047f,  0.02476347f } 
        };

    }

    void AudioBuilder::addTransition( timestamp_t clocktime, float l, float r )
    {
        // Convert the given clock time to a sample time
        timestamp_t sampletime = ((clocktime * timeScalar) + timeOverflow) >> timeShift;
        const float* set = sincTable[sampletime & 0x1F];
        sampletime >>= 5;

        //  This should never happen... but just in case....
        if(sampletime >= bufferSizeInElements)      return;     // transition is too far out... abort!

        if(stereo)
        {
            for(int i = 0; i < 13; ++i)
            {
                transitionBuffer[0][ sampletime + i ] += l * set[i];
                transitionBuffer[1][ sampletime + i ] += r * set[i];
            }
        }
        else
        {
            for(int i = 0; i < 13; ++i)
            {
                transitionBuffer[0][ sampletime + i ] += l * set[i];
            }
        }
    }

}
