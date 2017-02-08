namespace schcore{ namespace mpr {

    class Vrc2 : public Cartridge
    {
    public:
        Vrc2(bool shift_chr)
            : shiftChr(shift_chr ? 1 : 0)
        {}

    protected:
        void addLineConfiguration(u16 lobit, u16 hibit)
        {
            bitLines[0].push_back(lobit);
            bitLines[1].push_back(hibit);
        }

        virtual void cartReset(const ResetInfo& info) override
        {
            if(info.hardReset)
            {
                setDefaultPrgCallbacks();
                swapPrg_8k(0x6, 0, true);           // TODO - does this have RAM?
                swapPrg_8k(0xE, ~0);
                for(auto& i : chr)      i = 0;
                for(auto& i : prg)      i = 0;
                mir = 0;

                syncAll();
                
                info.cpuBus->addWriter(0x8,0xE,this,&Vrc2::onWrite);
            }
        }

    private:
        std::vector<u16>            bitLines[2];

        const int                   shiftChr;
        u8                          chr[8];
        u8                          prg[2];
        u8                          mir;

        void onWrite(u16 a, u8 v)
        {
            u16 orig_a = a;
            a &= 0xF000;
            for(auto& i : bitLines[0])  { if(orig_a & i) { a |= 0x0001;  break; }}
            for(auto& i : bitLines[1])  { if(orig_a & i) { a |= 0x0002;  break; }}

            switch(a)
            {
            case 0x8000: case 0x8001: case 0x8002: case 0x8003:
                prg[0] = v & 0x0F;
                syncPrg();
                break;

            case 0x9000: case 0x9001: case 0x9002: case 0x9003:
                mir = v & 0x03;
                syncMir();
                break;
                
            case 0xA000: case 0xA001: case 0xA002: case 0xA003:
                prg[1] = v & 0x0F;
                syncPrg();
                break;
                
            case 0xB000:    chr[0] = (chr[0] & 0xF0) |  (v & 0x0F);         syncChr();  break;
            case 0xB001:    chr[0] = (chr[0] & 0x0F) | ((v & 0x0F) << 4);   syncChr();  break;
            case 0xB002:    chr[1] = (chr[1] & 0xF0) |  (v & 0x0F);         syncChr();  break;
            case 0xB003:    chr[1] = (chr[1] & 0x0F) | ((v & 0x0F) << 4);   syncChr();  break;
            case 0xC000:    chr[2] = (chr[2] & 0xF0) |  (v & 0x0F);         syncChr();  break;
            case 0xC001:    chr[2] = (chr[2] & 0x0F) | ((v & 0x0F) << 4);   syncChr();  break;
            case 0xC002:    chr[3] = (chr[3] & 0xF0) |  (v & 0x0F);         syncChr();  break;
            case 0xC003:    chr[3] = (chr[3] & 0x0F) | ((v & 0x0F) << 4);   syncChr();  break;
            case 0xD000:    chr[4] = (chr[4] & 0xF0) |  (v & 0x0F);         syncChr();  break;
            case 0xD001:    chr[4] = (chr[4] & 0x0F) | ((v & 0x0F) << 4);   syncChr();  break;
            case 0xD002:    chr[5] = (chr[5] & 0xF0) |  (v & 0x0F);         syncChr();  break;
            case 0xD003:    chr[5] = (chr[5] & 0x0F) | ((v & 0x0F) << 4);   syncChr();  break;
            case 0xE000:    chr[6] = (chr[6] & 0xF0) |  (v & 0x0F);         syncChr();  break;
            case 0xE001:    chr[6] = (chr[6] & 0x0F) | ((v & 0x0F) << 4);   syncChr();  break;
            case 0xE002:    chr[7] = (chr[7] & 0xF0) |  (v & 0x0F);         syncChr();  break;
            case 0xE003:    chr[7] = (chr[7] & 0x0F) | ((v & 0x0F) << 4);   syncChr();  break;
            }
        }

        ////////////////////////////////////////////////
        void syncAll()
        {
            syncMir();
            syncPrg();
            syncChr();
        }

        void syncPrg()
        {
            swapPrg_8k(0x8, prg[0]);
            swapPrg_8k(0xA, prg[1]);
            swapPrg_8k(0xC, -2);
        }

        void syncMir()
        {
            switch(mir & 0x03)
            {
            case 0x00:  mir_vert();     break;
            case 0x01:  mir_horz();     break;
            case 0x02:  mir_1scr(0);    break;
            case 0x03:  mir_1scr(1);    break;
            }
        }

        void syncChr()
        {
            for(int i = 0; i < 8; ++i)
            {
                swapChr_1k( i, chr[i] >> shiftChr );
            }
        }

    };

}}