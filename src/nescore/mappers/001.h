namespace schcore{ namespace mpr {

    class Mpr_001 : public Cartridge
    {
    protected:
        virtual void cartReset(const ResetInfo& info) override
        {
            regs[0] = 0xC;
            if(info.hardReset)
            {
                swapPrg_8k(6,0,true);
                setDefaultPrgCallbacks();
                regs[1] = 0;
                regs[2] = 0;
                regs[3] = 0;
                tempReg = 0;
                nextBit = 0x01;

                cpuBus->addWriter(0x8,0xF,this,&Mpr_001::onWrite);
            }
            
            syncAll();
        }

    private:
        u8      regs[4];
        u8      tempReg;
        u8      nextBit;

        void onWrite(u16 a, u8 v)
        {
            // duplicate writes are ignored
            if(cpuBus->isDuplicateWrite(a))        return;

            // reg reset?
            if(v & 0x80)
            {
                regs[0] |= 0x0C;
                syncPrg();
            }
            else
            {
                if(v & 0x01)        tempReg |= nextBit;
                if(nextBit >= 0x10)
                {
                    regs[(a >> 13) & 3] = tempReg;
                    nextBit = 0x01;
                    tempReg = 0;
                    syncAll();
                }
                else
                    nextBit <<= 1;
            }
        }

        void syncAll()
        {
            syncPrg();

            if(regs[0] & 0x10)      // 4K CHR mode
            {
                swapChr_4k(0, regs[1]);
                swapChr_4k(4, regs[2]);
            }
            else                    // 8K CHR mode
            {
                swapChr_8k(0, regs[1] >> 1);
            }

            // mirroring
            switch(regs[0] & 0x03)
            {
            case 0:     mir_1scr(0);        break;
            case 1:     mir_1scr(1);        break;
            case 2:     mir_vert();         break;
            case 3:     mir_horz();         break;
            }
        }

        void syncPrg()
        {
            prgRamEnable( !(regs[3] & 0x80) );

            // PRG swapping
            switch(regs[0] & 0x0C)
            {
            case 0x00: case 0x04:
                swapPrg_32k(0x8, (regs[3] & 0x0F) >> 1);
                break;
            case 0x08:
                swapPrg_16k(0x8, 0);
                swapPrg_16k(0xC, (regs[3] & 0x0F));
                break;
            case 0x0C:
                swapPrg_16k(0x8, (regs[3] & 0x0F));
                swapPrg_16k(0xC, ~0);
                break;
            }
        }
    };

}}