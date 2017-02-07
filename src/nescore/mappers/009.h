namespace schcore{ namespace mpr {

    class Mpr_009 : public Cartridge
    {
    protected:
        virtual void cartReset(const ResetInfo& info) override
        {
            if(info.hardReset)
            {
                lolatch = 0;
                hilatch = 0;
                swapPrg_8k(0xE,-1);
                swapPrg_8k(0xC,-2);
                swapPrg_8k(0xA,-3);
                setDefaultPrgCallbacks();
                prg = 0;
                chrLo[0] = chrLo[1] = 0;
                chrHi[0] = chrHi[1] = 0;
                mir = 0;

                info.cpuBus->addWriter(0xA,0xF,this,&Mpr_009::onWrite);
                bus = info.cpuBus;
            }
            
            syncAll();
        }

    private:
        u8      prg;
        u8      chrLo[2];
        u8      chrHi[2];
        u8      mir;
        int     lolatch;
        int     hilatch;
        CpuBus* bus;

        void onWrite(u16 a, u8 v)
        {
            switch(a & 0xF000)
            {
            case 0xA000: prg = v;       syncPrg();  break;
            case 0xB000: chrLo[0] = v;  syncChr();  break;
            case 0xC000: chrLo[1] = v;  syncChr();  break;
            case 0xD000: chrHi[0] = v;  syncChr();  break;
            case 0xE000: chrHi[1] = v;  syncChr();  break;
            case 0xF000: mir = v;       syncMir();  break;
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

        void syncChr()
        {
            swapChr_4k(0, chrLo[lolatch] & 0x1F);
            swapChr_4k(4, chrHi[hilatch] & 0x1F);
        }

        void syncMir()
        {
            if(mir & 0x01)  mir_horz();
            else            mir_vert();
        }

        virtual void onPpuRead(u16 a, u8& v) override
        {
            if     ((a & 0x3FF0) == 0x0FD0)  { lolatch = 0; syncChr();   }
            else if((a & 0x3FF0) == 0x0FE0)  { lolatch = 1; syncChr();   }
            else if((a & 0x3FF0) == 0x1FD0)  { hilatch = 0; syncChr();   }
            else if((a & 0x3FF0) == 0x1FE0)  { hilatch = 1; syncChr();   }

            Cartridge::onPpuRead(a, v);
        }
    };

}}