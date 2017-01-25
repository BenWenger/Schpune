namespace schcore{ namespace mpr {

    class Mpr_003 : public Cartridge
    {
    protected:
        virtual void cartReset(const ResetInfo& info) override
        {
            if(info.hardReset)
            {
                setPrgCallbacks(0x8,0xF,-1,-1,false);
                swapPrg_32k(8, ~0);
                swapChr_8k(0,0);
                page = 0;

                info.cpuBus->addWriter(0x8,0xF,this,&Mpr_003::onWrite);
            }
        }

    private:
        u8      page;
        void onWrite(u16 a, u8 v)
        {
            page = busConflict(a,v);
            swapChr_8k(0, page);
        }
    };

}}