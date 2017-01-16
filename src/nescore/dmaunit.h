
#ifndef SCHPUNE_NESCORE_DMAUNIT_H_INCLUDED
#define SCHPUNE_NESCORE_DMAUNIT_H_INCLUDED

#include "schpunetypes.h"
#include "dmc_supplier.h"


namespace schcore
{
    class ResetInfo;
    class CpuBus;

    class DmaUnit : public DmcSupplier
    {
    public:
        void                reset(const ResetInfo& info);

        // DmcSupplier stuff
        virtual bool        willBeAudible() const override                  { return hasDmcVal;        }
        virtual void        triggerFetch(u16 addr) override;
        virtual void        getFetchedByte(u8& byte, bool& hasval) override;

    private:
        CpuBus*             bus;

        bool                wantOam;
        bool                wantDmc;
        bool                midDma;
        bool                hasDmcVal;
        bool                oddCycle;

        u16                 oamPage;
        u16                 dmcAddr;
        u8                  dmcVal;

        void                onRead(u16 a, u8& v);
        void                onWrite(u16 a, u8 v);

        void                performDma(u16 dummyaddr);
        
    };
}

#endif