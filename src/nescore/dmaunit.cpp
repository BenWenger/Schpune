
#include "dmaunit.h"
#include "cpubus.h"
#include "resetinfo.h"

namespace schcore
{
    
    void DmaUnit::reset(const ResetInfo& info)
    {
        wantOam = false;
        midDma = false;

        if(info.hardReset)
        {
            bus = info.cpuBus;
            wantDmc = false;
            hasDmcVal = false;
            oddCycle = false;

            oamPage = 0;
            dmcAddr = 0;
            dmcVal = 0;
            
            bus->addReader(0x0, 0xF, this, &DmaUnit::onRead);
            bus->addWriter(0x0, 0xF, this, &DmaUnit::onWrite);
        }
    }

    
    void DmaUnit::triggerFetch(u16 addr)
    {
        wantDmc = true;
        dmcAddr = addr;
    }

    void DmaUnit::getFetchedByte(u8& byte, bool& hasval)
    {
        byte = dmcVal;
        hasval = hasDmcVal;
        hasDmcVal = false;
    }

    
    void DmaUnit::onRead(u16 a, u8& v)
    {
        if(!midDma && (wantDmc || wantOam))
        {
            midDma = true;  // prevent re-entrant
            performDma(a);
            midDma = false;
        }
        oddCycle = !oddCycle;
    }

    void DmaUnit::onWrite(u16 a, u8 v)
    {
        if(a == 0x4014)
        {
            wantOam = true;
            oamPage = (v << 8);
        }
        oddCycle = !oddCycle;
    }

    void DmaUnit::performDma(u16 dummyaddr)
    {
        if(wantDmc && !wantOam)         // DMC-only is nice and easy
        {
            bus->read(dummyaddr);                   // dummy for the halt
            bus->read(dummyaddr);                   // extra dummy for DMC-only
            if(oddCycle)    bus->read(dummyaddr);   // dummy for alignment

            hasDmcVal = true;
            wantDmc = false;
            dmcVal = bus->read(dmcAddr);

            return;
        }

        if(!wantOam)        // shouldn't be necessary, but just in case
            return;
        
        //////////////////////////////////
        //  OAM DMA is a bit tricker because DMC-DMA can overlap
        //     dmc needs to have a halt cycle, and an extra padding cycle before
        //   it can actually cut in
        bool hadhalt = false;
        bool hadpad = false;

        // do the initial halt cycle
        hadhalt = wantDmc;
        bus->read(dummyaddr);

        // do we need alignment for OAM?
        if(oddCycle)
        {
            hadpad = hadhalt;
            hadhalt = wantDmc;
            bus->read(dummyaddr);
        }

        // loop through 256 times (do the last iteration separately)
        for(u16 i = 0; i < 0x100; ++i)
        {
            // did DMC cut in?
            if(hadpad)
            {
                wantDmc = hadpad = hadhalt = false;
                hasDmcVal = true;
                dmcVal = bus->read(dmcAddr);
                bus->read(dummyaddr);               // <- realignment
            }

            // do an OAM read/write pair
            hadpad = hadhalt;
            hadhalt = wantDmc;
            u8 val = bus->read(i | oamPage);
            hadpad = hadhalt;
            hadhalt = wantDmc;
            bus->write(0x2004, val);
        }

        // did DMC cut in at the very end?
        if(hadpad)
        {
            hasDmcVal = true;
            dmcVal = bus->read(dmcAddr);
        }
        else
        {
            if(wantDmc && !hadhalt)     // want DMC, but didn't have the halt yet
            {
                hadhalt = true;
                bus->read(dummyaddr);   // do the halt cycle
            }
            if(hadhalt)
            {
                // had halt, but not pad -- add the padding
                bus->read(dummyaddr);                   // padding
                if(oddCycle) bus->read(dummyaddr);      // realignment if necessary
                hasDmcVal = true;
                dmcVal = bus->read(dmcAddr);
            }
        }

        ////////////////////////
        //  All DMA is finally done!
        wantOam = false;
        wantDmc = false;
    }

}
