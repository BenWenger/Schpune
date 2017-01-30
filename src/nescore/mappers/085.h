namespace schcore{ namespace mpr {

    class Mpr_085 : public VrcIrq_Mapper
    {
    public:
        Mpr_085()
            : audio( std::make_unique<Vrc7Audio>() )
        {}
    protected:

        virtual void cartReset(const ResetInfo& info) override
        {
            VrcIrq_Mapper::cartReset(info);
            
            audio->reset(info);

            if(info.hardReset)
            {
                setDefaultPrgCallbacks();
                swapPrg_8k(0xE, ~0);
                for(auto& i : chr)      i = 0;
                for(auto& i : prg)      i = 0;
                mode = 0;
                syncAll();
                
                info.cpuBus->addWriter(0x8,0xF,this,&Mpr_085::onWrite);
            }
        }

    private:
        std::unique_ptr<Vrc7Audio>  audio;            // TODO
        u8                          prg[3];
        u8                          chr[8];
        u8                          mode;

        void onWrite(u16 a, u8 v)
        {
            if(a & 0x0008)      a |= 0x0010;

            switch(a & 0xF010)
            {
            case 0x8000:        prg[0] = v & 0x3F;      syncPrg();      break;
            case 0x8010:        prg[1] = v & 0x3F;      syncPrg();      break;
            case 0x9000:        prg[2] = v & 0x3F;      syncPrg();      break;
                
            case 0xA000:        chr[0] = v;             syncChr();      break;
            case 0xA010:        chr[1] = v;             syncChr();      break;
            case 0xB000:        chr[2] = v;             syncChr();      break;
            case 0xB010:        chr[3] = v;             syncChr();      break;
            case 0xC000:        chr[4] = v;             syncChr();      break;
            case 0xC010:        chr[5] = v;             syncChr();      break;
            case 0xD000:        chr[6] = v;             syncChr();      break;
            case 0xD010:        chr[7] = v;             syncChr();      break;

            case 0xE000:        mode = v;               syncMode();     break;

            case 0xE010:        writeIrqLatch(v);                       break;
            case 0xF000:        writeIrqControl(v);                     break;
            case 0xF010:        writeIrqAcknowledge();                  break;
            }
        }

        ////////////////////////////////////////////////
        void syncAll()
        {
            syncMode();
            syncPrg();
            syncChr();
        }

        void syncMode()
        {
            prgRamEnable( mode & 0x80 );
            switch(mode & 0x03)
            {
            case 0x00:  mir_vert();         break;
            case 0x01:  mir_horz();         break;
            case 0x02:  mir_1scr(false);    break;
            case 0x03:  mir_1scr(true);     break;
            }

            // TODO -- 0x40 bit silences audio channels
        }

        void syncPrg()
        {
            swapPrg_8k(0x8,prg[0]);
            swapPrg_8k(0xA,prg[1]);
            swapPrg_8k(0xC,prg[2]);
        }

        void syncChr()
        {
            swapChr_1k(0, chr[0]);
            swapChr_1k(1, chr[1]);
            swapChr_1k(2, chr[2]);
            swapChr_1k(3, chr[3]);
            swapChr_1k(4, chr[4]);
            swapChr_1k(5, chr[5]);
            swapChr_1k(6, chr[6]);
            swapChr_1k(7, chr[7]);
        }

    };

}}