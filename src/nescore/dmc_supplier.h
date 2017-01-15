
#ifndef SCHPUNE_NESCORE_DMC_SUPPLIER_H_INCLUDED
#define SCHPUNE_NESCORE_DMC_SUPPLIER_H_INCLUDED

#include "schpunetypes.h"


namespace schcore
{
    ////////////////////////////////////////////////
    //  There needs to be 2 different DMC suppliers -- one for the DMCPU and
    // one for the audible DMC.  The audible DMC will generally 'peek' to get sample bytes,
    // however the DMCPU will perform an actual DMA

    class DmcSupplier
    {
    public:
        virtual ~DmcSupplier() {}

        virtual bool    willBeAudible() const = 0;
        virtual void    triggerFetch(u16 addr) = 0;
        virtual void    getFetchedByte(u8& byte, bool& hasval) = 0;
    };



    ////////////////////////////////////////////////
    //  This is the audible DMC supplier
    class CpuBus;
    class ResetInfo;
    class Dmc_PeekSampleBuffer : public DmcSupplier
    {
    public:
        void            reset(const ResetInfo& info);
        
        virtual bool    willBeAudible() const override                  { return hasVal;        }
        virtual void    triggerFetch(u16 addr) override;
        virtual void    getFetchedByte(u8& byte, bool& hasval) override;

    private:
        const CpuBus*   bus = nullptr;
        u8              val = 0;
        bool            hasVal = false;
    };
}

#endif