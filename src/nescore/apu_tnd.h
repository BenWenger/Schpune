
#ifndef SCHPUNE_NESCORE_APU_TND_H_INCLUDED
#define SCHPUNE_NESCORE_APU_TND_H_INCLUDED

#include "schpunetypes.h"
#include "apu_support.h"
#include "audiochannel.h"
#include "dmc_supplier.h"


namespace schcore
{
    /////////////////////////////////////
    //   This class is for Triangle/Noise/DMC (hence 'TND')
    //  Since the output of one impacts the output of the others
    class ResetInfo;
    class EventManager;
    class Apu;

    class Apu_Tnd : public AudioChannel
    {
    public:

        void                    writeMain(u16 a, u8 v);
        void                    write4015(u8 v);
        void                    read4015(u8& v);

        void                    reset(const ResetInfo& info);
        void                    clockSeqHalf();
        void                    clockSeqQuarter();

    protected:
        virtual int             doTicks(timestamp_t ticks, bool doaudio, bool docpu) override;
        virtual timestamp_t     clocksToNextUpdate() override;


    private:
        static const u16    noiseFreqLut[2][0x10];
        static const int    dmcFreqLut[2][0x10];

        //////////////////////////////////////
        //  Triangle
        struct
        {
            Apu_Length          length;
            Apu_Linear          linear;
            u16                 freqTimer;
            int                 freqCounter;
            u8                  triStep;
        } tri;
        
        //////////////////////////////////////
        //  Noise
        struct
        {
            Apu_Length          length;
            Apu_Decay           decay;
            u16                 freqTimer;
            int                 freqCounter;
            u16                 shifter;
            int                 shiftMode;          // 14 for normal mode, 9 for alt mode
        } nse;
        
        //////////////////////////////////////
        //  Dmc
        Dmc_PeekSampleBuffer    dmcPeekSampleBuffer;
        CpuBus*                 cpuBus;
        EventManager*           eventManager;
        Apu*                    apuHost;
        int                     region = 0;
        u8                      dmcOut;
        int                     dmcFreqTimer;
        u16                     dmcAddrLoad;
        int                     dmcLenLoad;
        bool                    dmcIrqPending;
        bool                    dmcIrqEnabled;
        bool                    dmcLoop;
        irqsource_t             dmcIrqBit;
        struct DmcData
        {
            DmcSupplier*        supplier;
            int                 freqCounter;
            int                 len;
            u16                 addr;
            u8                  outputUnit;
            int                 bitsRemaining;
            bool                audible;
        };

        DmcData                 dmcpu;
        DmcData                 dmcaud;
        
        void                    tryToSyncDmc();
        void                    startDmcClip(DmcData& dat);
        void                    doDmcFetch(DmcData& dat, bool isdmcpu);
        void                    runDmc(DmcData& dat, timestamp_t ticks, bool isdmcpu);

        void                    predictNextEvent();
    };


}

#endif