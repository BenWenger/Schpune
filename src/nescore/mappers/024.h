namespace schcore{ namespace mpr {

    class Mpr_024 : public VrcIrq_Mapper
    {
    public:
        Mpr_024(bool swap_lines)
            : swapLines(swap_lines)
            , audio( std::make_unique<Vrc6Audio>( swap_lines ) )
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
                bigprg = 0;
                smlprg = 0;
                mode = 0;
                syncAll();
                
                info.cpuBus->addWriter(0x8,0x8,this,&Mpr_024::onWrite);
                info.cpuBus->addWriter(0xB,0xF,this,&Mpr_024::onWrite);
            }
        }

    private:
        bool                        swapLines;
        std::unique_ptr<Vrc6Audio>  audio;
        u8                          chr[8];
        u8                          bigprg;
        u8                          smlprg;
        u8                          mode;

        void onWrite(u16 a, u8 v)
        {
            if(swapLines)
            {
                switch(a & 3)
                {
                case 1: case 2: a ^= 3;
                }
            }

            switch(a & 0xF003)
            {
            case 0x8000: case 0x8001: case 0x8002: case 0x8003:
                bigprg = (v & 0x0F);
                syncPrg();
                break;
                
            case 0xC000: case 0xC001: case 0xC002: case 0xC003:
                smlprg = (v & 0x1F);
                syncPrg();
                break;

            case 0xB003:
                mode = v;
                syncChr();
                syncRam();
                break;
                
            case 0xD000:    chr[0] = v;     syncChr();          break;
            case 0xD001:    chr[1] = v;     syncChr();          break;
            case 0xD002:    chr[2] = v;     syncChr();          break;
            case 0xD003:    chr[3] = v;     syncChr();          break;
            case 0xE000:    chr[4] = v;     syncChr();          break;
            case 0xE001:    chr[5] = v;     syncChr();          break;
            case 0xE002:    chr[6] = v;     syncChr();          break;
            case 0xE003:    chr[7] = v;     syncChr();          break;

            case 0xF000:    writeIrqLatch(v);                   break;
            case 0xF001:    writeIrqControl(v);                 break;
            case 0xF002:    writeIrqAcknowledge();              break;
            }
        }

        ////////////////////////////////////////////////
        void syncAll()
        {
            syncRam();
            syncPrg();
            syncChr();
        }

        void syncRam()
        {
            prgRamEnable( mode & 0x80 );
        }

        void syncPrg()
        {
            swapPrg_16k(0x8,bigprg);
            swapPrg_8k(0xC,smlprg);
        }

        void syncChr()
        {
            // TODO -- mode & 0x20 does something but the wiki is very confusing.

            switch(mode & 0x03)
            {
            case 0:
                swapChr_1k(0, chr[0]);
                swapChr_1k(1, chr[1]);
                swapChr_1k(2, chr[2]);
                swapChr_1k(3, chr[3]);
                swapChr_1k(4, chr[4]);
                swapChr_1k(5, chr[5]);
                swapChr_1k(6, chr[6]);
                swapChr_1k(7, chr[7]);
                break;
            case 1:
                swapChr_2k(0, chr[0] >> 1);     // TODO - not sure if these are supposed to be shifted always, or only if 0x20 mode
                swapChr_2k(2, chr[1] >> 1);
                swapChr_2k(4, chr[2] >> 1);
                swapChr_2k(6, chr[3] >> 1);
            case 2: case 3:
                swapChr_1k(0, chr[0]);
                swapChr_1k(1, chr[1]);
                swapChr_1k(2, chr[2] >> 1);
                swapChr_1k(3, chr[3] >> 1);
                swapChr_2k(4, chr[4] >> 1);
                swapChr_2k(6, chr[5] >> 1);
                break;
            }

            // TODO - mode & 0x10 uses CHR as nametables... but screw it
            //   this mirroring code is temporary
            switch(mode & 0x0C)
            {
            case 0x00:  mir_vert();     break;
            case 0x04:  mir_horz();     break;
            case 0x08:  mir_1scr(0);    break;
            case 0x0C:  mir_1scr(1);    break;
            }
        }

    };

}}