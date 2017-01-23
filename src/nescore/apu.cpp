
#include <algorithm>
#include "apu.h"
#include "cpu.h"
#include "cpubus.h"
#include "audiobuilder.h"
#include "resetinfo.h"
#include "eventmanager.h"

namespace schcore
{
    ////////////////////////////////////////////////
    //  Lookup table for length counter
    const u8 Apu_Length::loadTable[0x20] = 
    {
        10,254, 20,  2, 40,  4, 80,  6, 160,  8, 60, 10, 14, 12, 26, 14,
        12, 16, 24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30
    };

    namespace
    {
        ////////////////////////////////////////////////
        //  frame sequencer actions
        enum
        {
            FrSeq_Half =    (1<<0),
            FrSeq_Quarter = (1<<1),
            FrSeq_Irq =     (1<<2),
            FrSeq_End =     (1<<3),

            // for FrSeqPhases table use only
            FSQ = FrSeq_Quarter,
            FSH = FrSeq_Quarter | FrSeq_Half,
            FSA = FrSeq_Quarter | FrSeq_Half | FrSeq_Irq

        };

        ////////////////////////////////////////////////
        //  frame sequencer phases
        const int FrSeqPhases[2][8] =
        {
            // 4-step mode
            { FSQ,      FSH,        FSQ,    FrSeq_Irq,      FSA,        FrSeq_Irq | FrSeq_End       },      // last entry must have FrSeq_Irq in it or predictNextEvent will break
            // 5-step mode
            { FSQ,      FSH,        FSQ,    FSH,            FrSeq_End                               }
        };
        
        ////////////////////////////////////////////////
        //  frame sequencer times
        const timestamp_t FrSeqTimes[2][8] =
        {
            // 4-step mode
            { 7457,     7456,       7458,   7457,           1,        1             },
            // 5-step mode
            { 7457,     7456,       7458,   14910,          1                       }
        };
    }
    
    inline timestamp_t Apu::calcTicksToRun( timestamp_t now, timestamp_t target ) const
    {
        if(now >= target)
            return 0;

        return (target - now + getClockBase() - 1) / getClockBase();
    }
    
    void Apu::clockSeqHalf()
    {
        pulses.clockSeqHalf();
        tnd.clockSeqHalf();
    }

    void Apu::clockSeqQuarter()
    {
        pulses.clockSeqQuarter();
        tnd.clockSeqQuarter();
    }
    
    void Apu::endFrame(timestamp_t sub)
    {
        SubSystem::endFrame(sub);
        pulses.subtractFromCpuTimestamp(sub);
        tnd.subtractFromCpuTimestamp(sub);
    }

    void Apu::silenceAllChannels()
    {
        pulses.makeSilent();
        tnd.makeSilent();

        // TODO - expansion channels
    }

    //////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////

    Apu::Apu()
    {
        builder                     = nullptr;
        audSettings.masterVol       = 1.0f;
        audSettings.sampleRate      = 48000;
        audSettings.stereo          = false;
        for(auto& i : audSettings.chans)
        {
            i.vol                   = 1.0f;
            i.pan                   = 0.0;
        }

        setAudioSettings(audSettings);
    }

    void Apu::setAudioSettings(const AudioSettings& settings)
    {
        audSettings = settings;

        // enforce that pan is within proper range
        for(auto& i : audSettings.chans)
        {
            if(i.pan < -1.0f)       i.pan = -1.0f;
            if(i.pan >  1.0f)       i.pan =  1.0f;
        }

        if(builder)
            builder->setFormat( audSettings.sampleRate, audSettings.stereo );
        
        pulses.updateSettings( audSettings, ChannelId::pulse0 );
        tnd.updateSettings( audSettings, ChannelId::triangle );

        // TODO - forward to expansion audio
    }

    //////////////////////////////////////////////////////////
    //   Reg access
    void Apu::onWrite(u16 a, u8 v)
    {
        switch(a)
        {
        case 0x4000: case 0x4001: case 0x4002: case 0x4003:
        case 0x4004: case 0x4005: case 0x4006: case 0x4007:
            catchUp();
            pulses.writeMain(a,v);
            break;
            
        case 0x4008: case 0x4009: case 0x400A: case 0x400B:
        case 0x400C: case 0x400D: case 0x400E: case 0x400F:
        case 0x4010: case 0x4011: case 0x4012: case 0x4013:
            catchUp();
            tnd.writeMain(a,v);
            break;


        case 0x4015:
            catchUp();
            pulses.write4015(v);
            tnd.write4015(v);
            break;

        case 0x4017:
            catchUp();

            frameIrqEnabled = !(v & 0x40);
            if(!frameIrqEnabled)
            {
                frameIrqPending = false;
                bus->acknowledgeIrq( frameIrqBit );
            }

            newSeqMode = v;
            modeResetCounter = (oddCycle ? 3 : 2);      // TODO -- might have these backwards!  verify!
            
            predictNextEvent();
            break;
        }
    }

    void Apu::onRead(u16 a, u8& v)
    {
        if(a == 0x4015)
        {
            catchUp();
            v &= ~0x20;
            pulses.read4015(v);
            tnd.read4015(v);


            if(frameIrqPending)
            {
                v |= 0x40;
                frameIrqPending = false;
                bus->acknowledgeIrq( frameIrqBit );
                predictNextEvent();
            }
        }
    }
    
    //////////////////////////////////////////////////////////
    //   Running
    
    void Apu::run(timestamp_t runto)
    {
        auto ticks = calcTicksToRun( curCyc(), runto );

        /////////////////////////////////////////
        while(ticks > 0)
        {
            timestamp_t step = std::min(seqCounter, ticks);
            if(modeResetCounter && (step > modeResetCounter))       step = modeResetCounter;

            // advance our aud/cpu timestamps
            ticks -= step;
            cyc(step);
            audTimestamp = std::min( audTimestamp + (step * getClockBase()),  builder->getMaxAllowedTimestamp() );

            // run all channels up to this point
            pulses.run(curCyc(), audTimestamp);
            tnd.run(curCyc(), audTimestamp);
            // TODO -- add expansion audio when ready
            
            ///////////////////////////////////////
            ///////////////////////////////////////
            // flip odd cycle
            if(step & 1)
                oddCycle = !oddCycle;

            // clock the frame sequencer
            seqCounter -= step;
            while(seqCounter <= 0)
            {
                int action = FrSeqPhases[seqMode][nextSeqPhase++];
                if(action & FrSeq_Quarter)                      clockSeqQuarter();
                if(action & FrSeq_Half)                         clockSeqHalf();
                if((action & FrSeq_Irq) && frameIrqEnabled)     { frameIrqPending = true; bus->triggerIrq( frameIrqBit );       }
                if(action & FrSeq_End)                          nextSeqPhase = 0;

                seqCounter += FrSeqTimes[seqMode][nextSeqPhase];
            }

            // clock the seq resetter
            if(modeResetCounter > 0)
            {
                modeResetCounter -= step;
                if(modeResetCounter <= 0)
                {
                    modeResetCounter = 0;
                    nextSeqPhase = 0;
                    if(newSeqMode & 0x80)
                    {
                        clockSeqHalf();
                        clockSeqQuarter();
                        seqMode = 1;
                    }
                    else
                        seqMode = 0;
                    seqCounter = FrSeqTimes[seqMode][nextSeqPhase];
                }
            }// end modeResetCounter
        }// end while loop
    }
    
    void Apu::fabricateMoreAudio(int bytes)
    {
        auto ts = builder->timestampToProduceBytes( bytes );
        
        pulses.run(Time::Now, ts);
        tnd.run(Time::Now, ts);

        // TODO - add expansion audio
    }


    ////////////////////////////////////////////////////////////
    //  Resetting

    void Apu::reset( const ResetInfo& info )
    {
        if(info.hardReset)
        {
            subSystem_HardReset(info.cpu, info.region.apuClockBase);
            // TODO - clear expansion audio list

            eventManager = info.eventManager;

            // capture the bus
            bus = info.cpuBus;
            bus->addReader(0x4,0x4,this,&Apu::onRead);
            bus->addWriter(0x4,0x4,this,&Apu::onWrite);
            frameIrqBit = bus->createIrqCode("APU Frame");

            // capture the builder
            builder = info.audioBuilder;
            builder->setFormat( audSettings.sampleRate, audSettings.stereo );
            builder->addTimestampHolder( this );
            builder->addTimestampHolder( &pulses );
            builder->addTimestampHolder( &tnd );
            pulses.setBuilder(builder);
            tnd.setBuilder(builder);

            oddCycle        = false;
            seqCounter      = 0;
            nextSeqPhase    = 0;
            seqMode         = 0;

            modeResetCounter = 0;
            newSeqMode      = 0;

            frameIrqEnabled = true;
            frameIrqPending = false;
            frameIrqBit     = bus->createIrqCode("APU Frame");

            audTimestamp    = 0;
            
            pulses.setClockRate( info.region.apuClockBase );
            tnd.setClockRate( info.region.apuClockBase );
        }

        
        pulses.reset( info.hardReset );
        tnd.reset( info );
        predictNextEvent();
    }

    ///////////////////////////////////////////////////
    //  IRQ prediction

    void Apu::predictNextEvent()
    {
        if(frameIrqPending)     return;     // if IRQ is already pending, no need to check for next one
        if(!frameIrqEnabled)    return;     // if IRQs aren't enabled, no need to check for them
        
        // The 'delay' counter after writing to $4017 complicates this a bit, because we
        //   have to check the current mode, then we have to check the mode we switch to

        // So first, record the current mode
        timestamp_t ticks = 0;
        timestamp_t cntr = seqCounter;
        int nextphase = nextSeqPhase;

        // If a reset is pending -- check the time before it resets
        if(modeResetCounter > 0)
        {
            // IRQs only exist in 4-step mode
            //   Also, we only have to check ahead one phase
            //   and only if that phase will trigger before the mode reset occurs
            if(!seqMode && (seqCounter <= modeResetCounter) && (FrSeqPhases[0][nextSeqPhase] & FrSeq_Irq))
            {
                // it'll happen in 'seqCounter' ticks!
                ticks = (seqCounter * getClockBase()) + curCyc();
                eventManager->addEvent(ticks, EventType::evt_apu);
                return;
            }

            // otherwise, it won't happen before the reset.  Switch the mode we're checking to the new mode
            if(newSeqMode & 0x80)       return;     // won't happen in 5-step mode

            ticks = modeResetCounter;
            cntr = FrSeqTimes[0][0];
            nextphase = 0;
        }

        // IRQ is for sure going to happen eventually (unless they get disabled or mode changes or whatever -- 
        //   but we don't care about that for this prediction)
        //
        // So start walking through all the phases until you find an IRQ
        while(true)
        {
            ticks += cntr;
            if(FrSeqPhases[0][nextphase] & FrSeq_Irq)       break;      // found it!
            ++nextphase;
            cntr = FrSeqTimes[0][nextphase];
        }

        // 'ticks' is the number of cycles that need to happen before the IRQ.  convert that to a timestamp
        ticks *= getClockBase();
        ticks += curCyc();

        eventManager->addEvent(ticks, EventType::evt_apu);
    }

}