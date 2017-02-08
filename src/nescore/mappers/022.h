namespace schcore{ namespace mpr {

    class Mpr_022 : public Vrc2
    {
    public:
        Mpr_022() : Vrc2(true)
        {
            addLineConfiguration( 0x0002, 0x0001 );
        }
    };

}}