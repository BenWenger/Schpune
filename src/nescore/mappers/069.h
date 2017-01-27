namespace schcore{ namespace mpr {

    class Mpr_069 : public Cartridge
    {

    protected:

        virtual void cartReset(const ResetInfo& info) override
        {
            audio->reset(info);

            if(info.hardReset)
            {
                setDefaultPrgCallbacks();

                bus = info.cpuBus;
                evt = info.eventManager;

                addr = 0;
                for(auto& i : chrRegs)      i = 0;
                for(auto& i : prgRegs)      i = 0;
                ntReg = 0;
                irqCtrl = 0;
                irqCounter = 0;
                irqBit = bus->createIrqCode("Mapper");
                irqPending = false;

                swapPrg_8k( 0xE, ~0 );
                syncAll();

                bus->addWriter(0x8, 0xB, this, &Mpr_069::onWrite);
            }
        }

    private:
        std::unique_ptr<SunsoftAudio>  audio = std::make_unique<SunsoftAudio>();
        u8              addr;
        u8              chrRegs[8];
        u8              prgRegs[4];
        u8              ntReg;
        u8              irqCtrl;
        u16             irqCounter;
        irqsource_t     irqBit;
        bool            irqPending;
        CpuBus*         bus;
        EventManager*   evt;

        void onWrite(u16 a, u8 v)
        {
            if((a & 0xE000) == 0x8000)
                addr = v & 0x0F;
            else
            {
                switch(addr)
                {
                case 0x0:   chrRegs[0] = v;     syncChr();      break;
                case 0x1:   chrRegs[1] = v;     syncChr();      break;
                case 0x2:   chrRegs[2] = v;     syncChr();      break;
                case 0x3:   chrRegs[3] = v;     syncChr();      break;
                case 0x4:   chrRegs[4] = v;     syncChr();      break;
                case 0x5:   chrRegs[5] = v;     syncChr();      break;
                case 0x6:   chrRegs[6] = v;     syncChr();      break;
                case 0x7:   chrRegs[7] = v;     syncChr();      break;
                case 0x8:   prgRegs[0] = v;     syncPrg();      break;
                case 0x9:   prgRegs[1] = v;     syncPrg();      break;
                case 0xA:   prgRegs[2] = v;     syncPrg();      break;
                case 0xB:   prgRegs[3] = v;     syncPrg();      break;
                case 0xC:   ntReg = v;          syncNt();       break;
                case 0xD:
                    catchUp();
                    irqCtrl = v;
                    ack();
                    predict();
                    break;
                case 0xE:
                    irqCounter = (irqCounter & 0xFF00) | v;
                    predict();
                    break;
                case 0xF:
                    irqCounter = (irqCounter & 0x00FF) | (v << 8);
                    predict();
                    break;
                }
            }
        }

        //////////////////////////////////////////////
        void syncAll()
        {
            syncPrg();
            syncChr();
            syncNt();
        }

        void syncPrg()
        {
            prgRamEnable( prgRegs[0] & 0x80 );
            swapPrg_8k(0x6, prgRegs[0] & 0x3F, (prgRegs[0] & 0x40) != 0);
            swapPrg_8k(0x8, prgRegs[1] & 0x3F);
            swapPrg_8k(0xA, prgRegs[2] & 0x3F);
            swapPrg_8k(0xC, prgRegs[3] & 0x3F);
        }

        void syncChr()
        {
            swapChr_1k(0, chrRegs[0]);
            swapChr_1k(1, chrRegs[1]);
            swapChr_1k(2, chrRegs[2]);
            swapChr_1k(3, chrRegs[3]);
            swapChr_1k(4, chrRegs[4]);
            swapChr_1k(5, chrRegs[5]);
            swapChr_1k(6, chrRegs[6]);
            swapChr_1k(7, chrRegs[7]);
        }

        void syncNt()
        {
            switch(ntReg & 0x03)
            {
            case 0x00:  mir_vert();     break;
            case 0x01:  mir_horz();     break;
            case 0x02:  mir_1scr(0);    break;
            case 0x03:  mir_1scr(1);    break;
            }
        }

        void trigger()
        {
            irqPending = true;
            bus->triggerIrq(irqBit);
        }

        void ack()
        {
            irqPending = false;
            bus->acknowledgeIrq(irqBit);
        }

        void predict()
        {
            if((irqCtrl & 0x81) != 0x81)
                return;

            timestamp_t ticks = irqCounter;

            ticks *= getClockBase();
            ticks += curCyc();
            evt->addEvent(ticks, EventType::evt_mpr);
        }

        virtual void run(timestamp_t runto) override
        {
            timestamp_t ticks = unitsToTimestamp(runto);
            if(ticks <= 0)          return;
            cyc(ticks);

            // if IRQ counter doesn't count, nothing to do here
            if(!(irqCtrl & 0x80))   return;

            // apply the ticks!!!
            ticks = irqCounter - ticks;
            if((ticks < 0) && (irqCtrl & 0x01))
                trigger();

            irqCounter = static_cast<u16>(ticks & 0xFFFF);
        }
    };

}}