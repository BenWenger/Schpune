namespace schcore{ namespace mpr {

    class Mpr_009 : public Mmc2Latch
    {
    protected:
        virtual void cartReset(const ResetInfo& info) override
        {
            Mmc2Latch::cartReset(info);

            if(info.hardReset)
            {
                swapPrg_8k(0xE,-1);
                swapPrg_8k(0xC,-2);
                swapPrg_8k(0xA,-3);
                setDefaultPrgCallbacks();
                prg = 0;
                mir = 0;

                info.cpuBus->addWriter(0xA,0xF,this,&Mpr_009::onWrite);
            }
            
            syncAll();
        }

    private:
        u8      prg;
        u8      mir;

        void onWrite(u16 a, u8 v)
        {
            switch(a & 0xF000)
            {
            case 0xA000: prg = v;           syncPrg();  break;
            case 0xB000: setChrReg(0,0,v & 0x1F);       break;
            case 0xC000: setChrReg(0,1,v & 0x1F);       break;
            case 0xD000: setChrReg(1,0,v & 0x1F);       break;
            case 0xE000: setChrReg(1,1,v & 0x1F);       break;
            case 0xF000: mir = v;           syncMir();  break;
            }
        }

        void syncAll()
        {
            syncPrg();
            syncChr();
            syncMir();
        }

        void syncPrg()
        {
            swapPrg_8k(8, prg & 0x0F);
        }

        void syncMir()
        {
            if(mir & 0x01)  mir_horz();
            else            mir_vert();
        }
    };

}}