namespace schcore{ namespace mpr {

    class Mmc2Latch : public Cartridge
    {
    protected:
        virtual void cartReset(const ResetInfo& info) override
        {
            if(info.hardReset)
            {
                chr[0][0] = 0;
                chr[0][1] = 0;
                chr[1][0] = 0;
                chr[1][1] = 0;
                latches[0] = 0;
                latches[1] = 0;
                lastAccess = 0;
            }
        }

        void setChrReg(int lohi, int latchval, u8 v)
        {
            chr[lohi][latchval] = v;
            syncChr();
        }

        void syncChr()
        {
            swapChr_4k( 0, chr[0][latches[0]] );
            swapChr_4k( 4, chr[1][latches[1]] );
        }

    private:
        u8      chr[2][2];      // chr[0000 or 1000][FD or FE]
        int     latches[2];
        u16     lastAccess;

        virtual void onPpuRead(u16 a, u8& v) override
        {
            if(lastAccess)
            {
                if((a & 0x3FF0) != lastAccess)
                {
                    latches[(lastAccess >> 12) & 1] = ((lastAccess >> 4) & 0xFF) - 0xFD;
                    lastAccess = 0;
                }
                syncChr();
            }

            if( ((a+0x10) & 0x2FE0) == 0x0FE0 )     lastAccess = a & 0x3FF0;

            Cartridge::onPpuRead(a, v);
        }
    };

}}