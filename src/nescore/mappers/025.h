namespace schcore{ namespace mpr {

    class Mpr_025 : public Vrc4
    {
    public:
        Mpr_025()
        {
            addLineConfiguration( 0x0002, 0x0001 );
            addLineConfiguration( 0x0008, 0x0004 );
        }
    };

}}