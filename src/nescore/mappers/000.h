namespace schcore{ namespace mpr {

    class Mpr_000 : public Cartridge
    {
    protected:
        virtual void cartReset(const ResetInfo& info) override
        {
            if(info.hardReset)
            {
                setDefaultPrgCallbacks();
                swapPrg_32k(8, ~0);
                swapChr_8k(0,0);
            }
        }
    };

}}