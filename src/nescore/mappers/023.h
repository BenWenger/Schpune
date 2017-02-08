namespace schcore{ namespace mpr {

    class Mpr_023 : public Vrc4
    {
    public:
        Mpr_023()
        {
            addLineConfiguration( 0x0004, 0x0008 );
            addLineConfiguration( 0x0001, 0x0002 );
        }
    };

}}