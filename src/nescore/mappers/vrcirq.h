namespace schcore{ namespace mpr {

    /////////////////////////////////////////////
    //  Inheritance doesn't make much LOGICAL sense here -- composition would be more appropriate,
    //      but this really simplifies the code....
    //
    //  Mappers that use this IRQ counter can derive

    class VrcIrq_Mapper : public Cartridge
    {
    protected:
        virtual void cartReset(const ResetInfo& info) override
        {
            if(info.hardReset)
            {
                bus = info.cpuBus;
                evt = info.eventManager;
                irqBit = bus->createIrqCode("Mapper");

                latch = 0xFF;
                control = 0;
                pending = false;
                prescalar = 341;
                counter = 0;
            }
        }

        void    writeIrqLatch(u8 v)         { catchUp();    latch = v;                              }
        void    writeIrqLatch_lo(u8 v)      { catchUp();    latch = (latch & 0xF0) | (v & 0x0F);    }
        void    writeIrqLatch_hi(u8 v)      { catchUp();    latch = (latch & 0x0F) | (v & 0xF0);    }
        void    writeIrqControl(u8 v)       { catchUp();    control = v;      ack();  predict();    }
        void    writeIrqAcknowledge()
        {
            catchUp();
            if(control & 1)     control |= 2;
            else                control &= ~2;
            ack();
            predict();
        }

        virtual void run(timestamp_t runto) override
        {
            auto ticks = unitsToTimestamp(runto);
            if(ticks <= 0)      return;

            cyc(ticks);
            if(!(control & 0x02))       return;     // IRQs disabled ... counter doesn't count
            if(!(control & 0x04))
            {
                // scanline mode -- use the prescalar
                prescalar -= ticks*3;
                if(prescalar > 0)
                    ticks = 0;
                else
                {
                    ticks = (-prescalar / 341) + 1;
                    prescalar = 341 - (-prescalar % 341);
                }
            }

            // at this point, ticks is the number of times to clock the IRQ counter
            ticks += counter;
            if(ticks >= 0x100)
            {
                trigger();
                ticks = (ticks - 0x100) % (0x100 - latch);
            }
            counter = static_cast<u8>(ticks);
        }


    private:
        u8              latch;
        u8              control;
        irqsource_t     irqBit;
        bool            pending;
        CpuBus*         bus;
        EventManager*   evt;

        timestamp_t     prescalar;
        u8              counter;

        void ack()
        {
            pending = false;
            bus->acknowledgeIrq(irqBit);
        }

        void trigger()
        {
            pending = true;
            bus->triggerIrq(irqBit);
        }

        void predict()
        {
            if(pending)             return; // nothing to predict if it's already pending
            if(!(control & 0x02))   return; // nothing to predict if IRQs are disabled

            timestamp_t ticks;
            if(control & 0x04)
            {
                // cycle mode
                ticks = 0x100 - counter;
            }
            else
            {
                // scanline mode
                ticks = (0xFF - counter) * 341;
                ticks += prescalar;

                ticks = (ticks + 2) / 3;
            }
            --ticks;

            ticks *= getClockBase();
            ticks += curCyc();
            evt->addEvent(ticks, EventType::evt_mpr);
        }

    };

}}