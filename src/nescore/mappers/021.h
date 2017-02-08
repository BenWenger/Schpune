namespace schcore{ namespace mpr {

    class Mpr_021 : public Vrc4
    {
    public:
        Mpr_021()
        {
            addLineConfiguration( 0x0002, 0x0004 );
            addLineConfiguration( 0x0040, 0x0080 );
        }
    };

}}