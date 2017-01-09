
#ifndef SCHPUNE_NESCORE_APU_H_INCLUDED
#define SCHPUNE_NESCORE_APU_H_INCLUDED

#include "schpunetypes.h"
#include "subsystem.h"
#include "apu_support.h"


namespace schcore
{
    struct ResetInfo;
    class CpuBus;

    class Apu : public SubSystem
    {
    public:
        //////////////////////////////////////////////////
        //  Resetting
        void                reset(const ResetInfo& info);

        //////////////////////////////////////////////////
        //  Running
        virtual void        run(timestamp_t runto) override;

    private:
    };


}

#endif