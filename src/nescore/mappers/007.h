namespace schcore{ namespace mpr {

    // TODO -- some boards have bus conflicts

    class Mpr_007 : public Cartridge
    {
    protected:
        virtual void cartReset(const ResetInfo& info) override
        {
            if(info.hardReset)
            {
                setDefaultPrgCallbacks();
                reg = 0;
                swapChr_8k(0,0);
                syncAll();

                info.cpuBus->addWriter(0x8,0xF,this,&Mpr_007::onWrite);
            }
        }

    private:
        u8      reg;
        void onWrite(u16 a, u8 v)
        {
            reg = v;
            syncAll();
        }

        void syncAll()
        {
            swapPrg_32k( 8, reg & 7 );
            mir_1scr(reg & 0x10);
        }
    };

}}