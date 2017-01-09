
#ifndef SCHPUNE_NESCORE_CPUSTATE_H_INCLUDED
#define SCHPUNE_NESCORE_CPUSTATE_H_INCLUDED

#include "schpunetypes.h"


namespace schcore
{
    class CpuState
    {
    public:
        u8          A;
        u8          X;
        u8          Y;
        u8          SP;
        u16         PC;

        u8          getStatus(bool sw) const;
        void        setStatus(u8 v);
        
        bool        getC() const    { return fC;                    }
        bool        getZ() const    { return (fNZ & 0x0FF) == 0;    }
        bool        getI() const    { return fI;                    }
        bool        getD() const    { return fD;                    }
        bool        getV() const    { return fV;                    }
        bool        getN() const    { return (fNZ & 0x180) != 0;    }

        // nonzero = flag set, zero=flag cleared
        void        setC(int v)     { fC = (v != 0);                }
        void        setZ(int v);
        void        setI(int v)     { fI = (v != 0);                }
        void        setD(int v)     { fD = (v != 0);                }
        void        setV(int v)     { fV = (v != 0);                }
        void        setN(int v);

        // sets N/Z appropriately given a byte (ex: $80 sets N and clears Z)
        void        NZ(u8 v)        { fNZ = v;                      }

    private:
        static const u8             C_FLAG = 0x01;
        static const u8             Z_FLAG = 0x02;
        static const u8             I_FLAG = 0x04;
        static const u8             D_FLAG = 0x08;
        static const u8             B_FLAG = 0x10;
        static const u8             R_FLAG = 0x20;
        static const u8             V_FLAG = 0x40;
        static const u8             N_FLAG = 0x80;

        int         fNZ;
        bool        fC;
        bool        fI;
        bool        fD;
        bool        fV;
    };
    
    //////////////////////////////////////////////////
    //////////////////////////////////////////////////
    //////////////////////////////////////////////////
    inline u8 CpuState::getStatus(bool sw) const
    {
        u8 out = R_FLAG;
        if(getC())      out |= C_FLAG;
        if(getZ())      out |= Z_FLAG;
        if(getI())      out |= I_FLAG;
        if(getD())      out |= D_FLAG;
        if(sw)          out |= B_FLAG;
        if(getV())      out |= V_FLAG;
        if(getN())      out |= N_FLAG;
        return out;
    }

    inline void CpuState::setStatus(u8 v)
    {
        setC( v & C_FLAG );
        setI( v & I_FLAG );
        setD( v & D_FLAG );
        setV( v & V_FLAG );

        fNZ = !(v & Z_FLAG);
        if(v & N_FLAG)
            fNZ |= 0x100;
    }

    inline void CpuState::setZ(int v)
    {
        if(!v)              fNZ |= 1;
        else if(getN())     fNZ  = 0x100;
        else                fNZ  = 0;
    }

    inline void CpuState::setN(int v)
    {
        if(v)               fNZ |= 0x100;
        else                fNZ  = !getZ();
    }
}

#endif