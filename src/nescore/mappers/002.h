namespace schcore{ namespace mpr {

    class Mpr_002 : public Cartridge
    {
    protected:
        virtual void cartReset(const ResetInfo& info) override
        {
            if(info.hardReset)
            {
                page = 0;
                setDefaultPrgCallbacks();
                page = 0;
                swapPrg_16k(0xC, ~0);
                swapChr_8k(0,0);
                syncPrg();

                info.cpuBus->addWriter(0x8,0xF,this,&Mpr_002::onWrite);
            }
        }

    private:
        u8      page;
        void onWrite(u16 a, u8 v)
        {
            page = busConflict(a,v);
            syncPrg();
        }

        void syncPrg()
        {
            swapPrg_16k(0x8, page);
        }
    };

}}