
#include <algorithm>
#include "apu.h"
#include "cpu.h"
#include "cpubus.h"
#include "audiobuilder.h"
#include "resetinfo.h"

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
            { FSQ,      FSH,        FSQ,    FrSeq_Irq,      FSA,        FrSeq_Irq | FrSeq_End       },
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
        // tnd.clockSeqHalf();      // TODO uncomment when TND is ready
    }

    void Apu::clockSeqQuarter()
    {
        pulses.clockSeqQuarter();
        // tnd.clockSeqQuarter();      // TODO uncomment when TND is ready
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
            //tnd.writeMain(a,v);       TODO uncomment once TND exists
            break;


        case 0x4015:
            catchUp();
            pulses.write4015(v);
            //tnd.write4015(v);       TODO uncomment once TND exists
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

            // TODO -- predict next IRQ

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
            //tnd.read4015(v);       TODO uncomment once TND exists


            if(frameIrqPending)
            {
                v |= 0x40;
                frameIrqPending = false;
                bus->acknowledgeIrq( frameIrqBit );
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
            // TODO -- add TND when ready
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
    
    void Apu::fabricateMoreAudio(int count_in_s16s)
    {
        auto ts = builder->timestampToProduceSamples( count_in_s16s );
    }


    ////////////////////////////////////////////////////////////
    //  Resetting

    void Apu::reset( const ResetInfo& info )
    {
        if(info.hardReset)
        {
            subSystem_HardReset(info.cpu, info.region.apuClockBase);
            // TODO - clear expansion audio list

            // capture the bus
            bus = info.cpuBus;
            bus->addReader(0x4,0x4,this,&Apu::onRead);
            bus->addWriter(0x4,0x4,this,&Apu::onWrite);
            frameIrqBit = bus->createIrqCode("APU Frame");

            // capture the builder
            builder = info.audioBuilder;
            builder->addTimestampHolder( this );
            builder->addTimestampHolder( &pulses );
            pulses.setBuilder(builder);
            // TODO TND

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
        }

        
        pulses.reset( info.hardReset );
        // TODO TND
    }

}