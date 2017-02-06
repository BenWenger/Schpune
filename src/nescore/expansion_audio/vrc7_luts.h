
#ifndef SCHPUNE_VRC7LUTS_OKTOINCLUDE
#error This file should only be included from vrc7.cpp
#endif
namespace
{

    typedef std::unique_ptr<int[]>      lut_t;
    bool                                lutsBuilt = false;
    const double                        pi = 3.1415926535897932384626433832795;
    

    const int           egcBitWidth =           22;                 // bitwidth of envelope generator
    const int           maxAttenuation =        (1<<egcBitWidth)-1; // max attentuation (highest value possible in egc).  This value or higher results in output being 0
    const int           maxSlotOutput =         (1<<22)-1;          // maximum output for a slot (no attenuation)
    
    const int           phaseBitWidth =         20;                 // bitwidth of phase generator
    const int           phaseHighBit =          (1 << (phaseBitWidth-1));

    //////////////////////////////////////////////////////
    //  Converting linear <-> dB
    //
    //  All values scaled to [0..1] range.
    //  VRC7 has a threshold of 48 dB, so '1.0' as dB input/output in below functions is actually 48.0 dB


    double dbToLinear(double x)
    {
        if(x >= 1.0)    return 0.0;
        if(x <= 0.0)    return 1.0;

        x = std::pow( 10, (48.0 * x) / -20.0 );
        if(x >= 1.0)    return 1.0;
        if(x <= 0.0)    return 0.0;
        return x;
    }

    double linearToDb(double x)
    {
        if(x >= 1.0)    return 0.0;
        if(x <= 0.0)    return 1.0;

        x = std::log10(x) * -20.0 / 48.0;
        if(x >= 1.0)    return 1.0;
        if(x <= 0.0)    return 0.0;
        return x;
    }

    constexpr int dB(double db)
    {
        return static_cast<int>(db * maxAttenuation / 48.0);
    }

    //////////////////////////////////////////////////////
    //  dB to Linear table

    const int           linLutBitWidth =        10;                 // bitwidth of db->linear table
    const int           linLutSize =            (1<<linLutBitWidth);
    const int           linLutShift =           (egcBitWidth - linLutBitWidth);
    const int           linLutMask =            (linLutSize-1);
    lut_t               linLut;

    inline int      lut_linear(int x)           { return linLut[ (x >> linLutShift) & linLutMask ]; }
    void            buildLut_Linear()
    {
        linLut = lut_t( new int[linLutSize] );

        for(int i = 0; i < linLutSize; ++i)
        {
            linLut[i] = static_cast<int>( maxSlotOutput * dbToLinear( static_cast<double>(i) / (linLutSize-1) ) );
        }
    }

    //////////////////////////////////////////////////////
    //  Sine table (phase)
    //      This is the half-sine (one-pi) table.  Output can be negated to get the second half of
    //  the sine function.  Output is scaled up to 20 bits (not counting the negative bit)
    //
    //      The sine table is the sine wave directly, but rather is the level of attenuation needed
    //  for sine output.  Attenuation in dB can be calculated as follows:
    //

    const int           sinLutBitWidth =        10;
    const int           sinLutSize =            (1<<sinLutBitWidth);
    const int           sinLutShift =           (phaseBitWidth - 1 - sinLutBitWidth);       // -1 because we want to drop the high bit
    const int           sinLutMask =            sinLutSize-1;
    lut_t               sinLut;
    inline int      lut_sine(int x)             { return sinLut[ (x >> sinLutShift) & sinLutMask ]; }
    void            buildLut_Sine()
    {
        sinLut = lut_t( new int[sinLutSize] );

        for(int i = 0; i < sinLutSize; ++i)
        {
            sinLut[i] = static_cast<int>( maxAttenuation * linearToDb( std::sin( pi * i / sinLutSize ) ) );
        }
    }
    
    //////////////////////////////////////////////////////
    //  Attack Table
    //      Attack has logarithmic growth.
    //

    const int           atkLutBitWidth =        8;
    const int           atkLutSize =            (1<<atkLutBitWidth);
    const int           atkLutShift =           (egcBitWidth - atkLutBitWidth);
    const int           atkLutMask =            atkLutSize-1;
    lut_t               atkLut;
    inline int      lut_attack(int x)           { return atkLut[ (x >> atkLutShift) & atkLutMask ]; }
    void            buildLut_Attack()
    {
        atkLut = lut_t( new int[atkLutSize] );
        double mxlog = std::log(atkLutSize-1);

        for(int i = 0; i < atkLutSize; ++i)
        {
            atkLut[i] = static_cast<int>( maxAttenuation - (maxAttenuation * std::log(i) / mxlog) );
        }
    }



    ////////////////////////////////
    //  do all of them!
    void buildAllLuts()
    {
        if(lutsBuilt)       return;
        
        buildLut_Linear();
        buildLut_Sine();
        buildLut_Attack();

        lutsBuilt = true;
    }


    ////////////////////////////////
    //  Fixed luts that don't really need building at runtime
    const ChannelId chanIds[6] = {
        ChannelId::vrc7_0,
        ChannelId::vrc7_1,
        ChannelId::vrc7_2,
        ChannelId::vrc7_3,
        ChannelId::vrc7_4,
        ChannelId::vrc7_5
    };

    const u8 fixedInstruments[15][8] = {
        { 0x03,0x21,0x04,0x06,0x8D,0xF2,0x42,0x17 },
        { 0x13,0x41,0x05,0x0E,0x99,0x96,0x63,0x12 },
        { 0x31,0x11,0x10,0x0A,0xF0,0x9C,0x32,0x02 },
        { 0x21,0x61,0x1D,0x07,0x9F,0x64,0x20,0x27 },
        { 0x22,0x21,0x1E,0x06,0xF0,0x76,0x08,0x28 },
        { 0x02,0x01,0x06,0x00,0xF0,0xF2,0x03,0x95 },
        { 0x21,0x61,0x1C,0x07,0x82,0x81,0x16,0x07 },
        { 0x23,0x21,0x1A,0x17,0xEF,0x82,0x25,0x15 },
        { 0x25,0x11,0x1F,0x00,0x86,0x41,0x20,0x11 },
        { 0x85,0x01,0x1F,0x0F,0xE4,0xA2,0x11,0x12 },
        { 0x07,0xC1,0x2B,0x45,0xB4,0xF1,0x24,0xF4 },
        { 0x61,0x23,0x11,0x06,0x96,0x96,0x13,0x16 },
        { 0x01,0x02,0xD3,0x05,0x82,0xA2,0x31,0x51 },
        { 0x61,0x22,0x0D,0x02,0xC3,0x7F,0x24,0x05 },
        { 0x21,0x62,0x0E,0x00,0xA1,0xA0,0x44,0x17 }
    };

    const int multiLut[0x10] = { 1, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 20, 24, 24, 30, 30 };

    const int kslLut[0x10] = {
        dB( 0.00), dB(18.00), dB(24.00), dB(27.75), dB(30.00), dB(32.25), dB(33.75), dB(35.25),
        dB(36.00), dB(37.50), dB(38.25), dB(39.00), dB(39.75), dB(40.50), dB(41.25), dB(42.00) };
}