
#include "vrc7.h"
#include "../resetinfo.h"
#include "../cpubus.h"
#include <cmath>
#include <algorithm>
#include <memory>

namespace schcore
{
    namespace
    {
        //////////////////////////////////////////////////////
        //////////////////////////////////////////////////////
        //  Lookup Tables and constants
        typedef std::unique_ptr<int[]>  lut_t;

        const int max_dB =              48;                         // max attenuation is 48 dB
        const int maxAttenuationBits =  23;
        const int maxAttenuation =      (1<<maxAttenuationBits);    // This value effectively represents max_dB in "internal units"

        constexpr int dB(double db)             // handy function to provide a given dB level in internal units
        {
            return static_cast<int>( (db / max_dB) * maxAttenuation );
        }

        // dB to Linear conversion lut
        const int lut_LinearMaxOut =    (1<<20)-1;                  // slot output is 20 bits wide
        const int lut_LinearBits =      10;                         // number of relevant bits for lookup
        const int lut_LinearShift =     (maxAttenuationBits - lut_LinearBits);
        const int lut_LinearSize =      (1<<lut_LinearBits);
        const int lut_LinearMask =      lut_LinearSize - 1;
        lut_t       lut_Linear;

        // sine lut
        const int lut_SineBits =        10;
        const int lut_SineShift =       (18 - lut_SineBits);        // slot freq counter is 18 bits wide
        const int lut_SineSize =        (1<<lut_SineBits);
        const int lut_SineMask =        (1 << (lut_SineBits+1) )-1; // extra bit for the XOR bit
        const int lut_SineXorBit =      lut_SineSize;
        const int lut_SineRectifyBit =  lut_SineSize<<1;
        lut_t       lut_Sine;
        
        // Attack lut
        const int lut_AttackBits =      10;
        const int lut_AttackShift =     (23 - lut_AttackBits);      // EGC is 23 bits wide
        const int lut_AttackSize =      (1<<lut_AttackBits);
        const int lut_AttackMask =      (lut_AttackSize-1);
        lut_t       lut_Attack;
        
        // AM lut
        const int lut_AmBits =          10;
        const int lut_AmShift =         (20 - lut_AmBits);          // AM/FM counter is 20 bits wide
        const int lut_AmSize =          (1<<lut_AmBits);
        const int lut_AmMask =          (lut_AmSize-1);
        lut_t       lut_Am;
        
        // FM lut
        const int lut_FmBits =          10;
        const int lut_FmShift =         (20 - lut_FmBits);          // AM/FM counter is 20 bits wide
        const int lut_FmSize =          (1<<lut_FmBits);
        const int lut_FmMask =          (lut_FmSize-1);
        lut_t       lut_Fm;

        ///////////////////////////////////////////////////////
        ///////////////////////////////////////////////////////
        const double        pi = 3.1415926535897932384626433832795;

        bool        tablesBuilt = false;
        void        buildCommonTables()
        {
            // did we build these already?
            if(tablesBuilt)     return;

            ////////////////////////////////////////////
            //  dB to Linear output conversion
            {
                // Linear = 10 ^ (dB / -20)
                //      In our units, 'maxAttenuation' represents 'max_dB' decibels.
                //      Further, we want dB=maxAttenuation -> Linear=0,
                //        and dB=0 -> Linear=lut_LinearMaxOut
                //
                // So we want to scale this output
            
                lut_Linear = lut_t( new int[lut_LinearSize] );
                for(int i = 0; i < lut_LinearSize; ++i)
                {
                    double dB = i << lut_LinearShift;           // 'dB' is the dB level in our internal units (maxAttenuation represents max_dB decibels)
                    dB        = (dB / maxAttenuation) * max_dB; // convert that to actual decibels.  No longer in internal units

                    double linear = std::pow( 10.0, dB / -20.0 );   // do the conversion
                    linear *= lut_LinearMaxOut;                     // scale up to our desired output

                    lut_Linear[i] = static_cast<int>(linear);
                }
            }
            
            ////////////////////////////////////////////
            //  sine table
            {
                // This is sort of the inverse of the Linear table.  We want to convert linear units (sine function output)
                //   to decibels:
                //       dB     = -20 * log10( Linear )
                //
                // But again we want this to work with our units.  'Linear' in this case is going to be bound [0,1] because it's
                //   the output of sine, so we want Linear=1 -> dB=0 and Linear=0 -> dB=maxAttenuation
                //
                // This sine table is also just the quarter sine table... so it only covers up through sin(pi/2)
                
                lut_Sine = lut_t( new int[lut_SineSize] );
                for(int i = 0; i < lut_SineSize; ++i)
                {
                    double sinx = std::sin( (pi / 2.0) * i / lut_SineSize );
                    if(sinx == 0)           lut_Sine[i] = maxAttenuation;
                    else
                    {
                        double dB = -20.0 * std::log10( sinx );         // dB is now the number of decibels
                        if(dB >= max_dB)    lut_Sine[i] = maxAttenuation;
                        else                lut_Sine[i] = static_cast<int>( dB / max_dB * maxAttenuation ); // scale up to desired base
                    }
                }
            }
            
            ////////////////////////////////////////////
            //  attack table
            {
                // Attack table determines the output attenuation when the envelope generator is in attack
                //   mode.
                //
                // attack_output = 48 dB - (48 dB * ln(EGC) / ln(maxAttenuation))
                //
                // note that '48 dB' is effectively 'maxAttenuation'
                
                double lnfull = std::log( static_cast<double>(maxAttenuation) );

                lut_Attack = lut_t( new int[lut_AttackSize] );
                for(int i = 0; i < lut_AttackSize; ++i)
                {
                    double egc = i << lut_AttackShift;
                    lut_Attack[i] = static_cast<int>( maxAttenuation - (maxAttenuation * std::log(egc) / lnfull) );
                }
            }
            
            ////////////////////////////////////////////
            //  AM table
            {
                // AM table = the output of the AM unit
                //
                //    sinx = sin(2 * pi * counter / (1<<20))
                //    AM_output = (1.0 + sinx) * 0.6 dB
                
                lut_Am = lut_t( new int[lut_AmSize] );
                for(int i = 0; i < lut_AmSize; ++i)
                {
                    int ctr = (i << lut_AmShift);
                    double sinx = std::sin( 2 * pi * ctr / (1<<20) );
                    lut_Am[i] = static_cast<int>( (1.0 + sinx) * dB(0.6) );
                }
            }
            
            ////////////////////////////////////////////
            //  FM table
            {
                // FM table = the output of the FM unit
                //
                //    sinx = sin(2 * pi * counter / (1<<20))
                //    FM_output = 2 ^ (13.75 / 1200 * sinx)
                
                lut_Fm = lut_t( new int[lut_FmSize] );
                for(int i = 0; i < lut_FmSize; ++i)
                {
                    int ctr = (i << lut_FmShift);
                    double sinx = std::sin( 2 * pi * ctr / (1<<20) );
                    lut_Fm[i] = std::pow(2, 13.75 / 1200 * sinx);
                }
            }
        }   // End buildCommonTables
    }   // End anonymous namespace

}