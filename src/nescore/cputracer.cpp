
#define _CRT_SECURE_NO_WARNINGS
#include "cputracer.h"
#include <string>
#include "cpustate.h"
#include "cpubus.h"

namespace schcore
{

    namespace
    {
        //////////////////////////////////////////////
        //  Addressing modes
        enum
        {
            Ip,         // Implied
            Ac,         // Accumulator
            Im,         // Immediate
            Zp,         // Zero Page
            Zx,         // Zero Page X
            Zy,         // Zero Page Y
            Ab,         // Absolute
            Ax,         // Absolute, X
            Ay,         // Absolute, Y
            In,         // Indirect
            Ix,         // Indirect, X
            Iy,         // Indirect, Y
            Rl,         // Relative

            __ = Ip
        };

        const int AddrModes[0x100] = {
    /*      x0 x1 x2 x3  x4 x5 x6 x7   x8 x9 xA xB  xC xD xE xF       */
    /* 0x */Im,Ix,__,__, __,Zp,Zp,__,  Ip,Im,Ac,__, __,Ab,Ab,__,/* 0x */
    /* 1x */Rl,Iy,__,__, __,Zx,Zx,__,  Ip,Ay,__,__, __,Ax,Ax,__,/* 1x */
    /* 2x */Ab,Ix,__,__, Zp,Zp,Zp,__,  Ip,Im,Ac,__, Ab,Ab,Ab,__,/* 2x */
    /* 3x */Rl,Iy,__,__, __,Zx,Zx,__,  Ip,Ay,__,__, __,Ax,Ax,__,/* 3x */
    /* 4x */Ip,Ix,__,__, __,Zp,Zp,__,  Ip,Im,Ac,__, Ab,Ab,Ab,__,/* 4x */
    /* 5x */Rl,Iy,__,__, __,Zx,Zx,__,  Ip,Ay,__,__, __,Ax,Ax,__,/* 5x */
    /* 6x */Ip,Ix,__,__, __,Zp,Zp,__,  Ip,Im,Ac,__, In,Ab,Ab,__,/* 6x */
    /* 7x */Rl,Iy,__,__, __,Zx,Zx,__,  Ip,Ay,__,__, __,Ax,Ax,__,/* 7x */
    /*      x0 x1 x2 x3  x4 x5 x6 x7   x8 x9 xA xB  xC xD xE xF       */
    /* 8x */__,Ix,__,__, Zp,Zp,Zp,__,  Ip,__,Ip,__, Ab,Ab,Ab,__,/* 8x */
    /* 9x */Rl,Iy,__,__, Zx,Zx,Zy,__,  Ip,Ay,Ip,__, __,Ax,__,__,/* 9x */
    /* Ax */Im,Ix,Im,__, Zp,Zp,Zp,__,  Ip,Im,Ip,__, Ab,Ab,Ab,__,/* Ax */
    /* Bx */Rl,Iy,__,__, Zx,Zx,Zy,__,  Ip,Ay,Ip,__, Ax,Ax,Ay,__,/* Bx */
    /* Cx */Im,Ix,__,__, Zp,Zp,Zp,__,  Ip,Im,Ip,__, Ab,Ab,Ab,__,/* Cx */
    /* Dx */Rl,Iy,__,__, __,Zx,Zx,__,  Ip,Ay,__,__, __,Ax,Ax,__,/* Dx */
    /* Ex */Im,Ix,__,__, Zp,Zp,Zp,__,  Ip,Im,Ip,__, Ab,Ab,Ab,__,/* Ex */
    /* Fx */Rl,Iy,__,__, __,Zx,Zx,__,  Ip,Ay,__,__, __,Ax,Ax,__ /* Fx */
    /*      x0 x1 x2 x3  x4 x5 x6 x7   x8 x9 xA xB  xC xD xE xF       */
        };

        const char* const InstNames[0x100] = {
/*        x0      x1      x2      x3      x4      x5      x6      x7        x8      x9      xA      xB      xC      xD      xE      xF          */
/* 0x */" BRK "," ORA ","{ ? }","{ ? }","{ ? }"," ORA "," ASL ","{ ? }",  " PHP "," ORA "," ASL ","{ ? }","{ ? }"," ORA "," ASL ","{ ? }",/* 0x */
/* 1x */" BPL "," ORA ","{ ? }","{ ? }","{ ? }"," ORA "," ASL ","{ ? }",  " CLC "," ORA ","{ ? }","{ ? }","{ ? }"," ORA "," ASL ","{ ? }",/* 1x */
/* 2x */" JSR "," AND ","{ ? }","{ ? }"," BIT "," AND "," ROL ","{ ? }",  " PLP "," AND "," ROL ","{ ? }"," BIT "," AND "," ROL ","{ ? }",/* 2x */
/* 3x */" BMI "," AND ","{ ? }","{ ? }","{ ? }"," AND "," ROL ","{ ? }",  " SEC "," AND ","{ ? }","{ ? }","{ ? }"," AND "," ROL ","{ ? }",/* 3x */
/* 4x */" RTI "," EOR ","{ ? }","{ ? }","{ ? }"," EOR "," LSR ","{ ? }",  " PHA "," EOR "," LSR ","{ ? }"," JMP "," EOR "," LSR ","{ ? }",/* 4x */
/* 5x */" BVC "," EOR ","{ ? }","{ ? }","{ ? }"," EOR "," LSR ","{ ? }",  " CLI "," EOR ","{ ? }","{ ? }","{ ? }"," EOR "," LSR ","{ ? }",/* 5x */
/* 6x */" RTS "," ADC ","{ ? }","{ ? }","{ ? }"," ADC "," ROR ","{ ? }",  " PLA "," ADC "," ROR ","{ ? }"," JMP "," ADC "," ROR ","{ ? }",/* 6x */
/* 7x */" BVS "," ADC ","{ ? }","{ ? }","{ ? }"," ADC "," ROR ","{ ? }",  " SEI "," ADC ","{ ? }","{ ? }","{ ? }"," ADC "," ROR ","{ ? }",/* 7x */
/*        x0      x1      x2      x3      x4      x5      x6      x7        x8      x9      xA      xB      xC      xD      xE      xF          */
/* 8x */"{ ? }"," STA ","{ ? }","{ ? }"," STY "," STA "," STX ","{ ? }",  " DEY ","{ ? }"," TXA ","{ ? }"," STY "," STA "," STX ","{ ? }",/* 8x */
/* 9x */" BCC "," STA ","{ ? }","{ ? }"," STY "," STA "," STX ","{ ? }",  " TYA "," STA "," TXS ","{ ? }","{ ? }"," STA ","{ ? }","{ ? }",/* 9x */
/* Ax */" LDY "," LDA "," LDX ","{ ? }"," LDY "," LDA "," LDX ","{ ? }",  " TAY "," LDA "," TAX ","{ ? }"," LDY "," LDA "," LDX ","{ ? }",/* Ax */
/* Bx */" BCS "," LDA ","{ ? }","{ ? }"," LDY "," LDA "," LDX ","{ ? }",  " CLV "," LDA "," TSX ","{ ? }"," LDY "," LDA "," LDX ","{ ? }",/* Bx */
/* Cx */" CPY "," CMP ","{ ? }","{ ? }"," CPY "," CMP "," DEC ","{ ? }",  " INY "," CMP "," DEX ","{ ? }"," CPY "," CMP "," DEC ","{ ? }",/* Cx */
/* Dx */" BNE "," CMP ","{ ? }","{ ? }","{ ? }"," CMP "," DEC ","{ ? }",  " CLD "," CMP ","{ ? }","{ ? }","{ ? }"," CMP "," DEC ","{ ? }",/* Dx */
/* Ex */" CPX "," SBC ","{ ? }","{ ? }"," CPX "," SBC "," INC ","{ ? }",  " INX "," SBC "," NOP ","{ ? }"," CPX "," SBC "," INC ","{ ? }",/* Ex */
/* Fx */" BEQ "," SBC ","{STP}","{ ? }","{ ? }"," SBC "," INC ","{ ? }",  " SED "," SBC ","{ ? }","{ ? }","{ ? }"," SBC "," INC ","{ ? }" /* Fx */
/*        x0      x1      x2      x3      x4      x5      x6      x7        x8      x9      xA      xB      xC      xD      xE      xF          */
        };

        inline void n2(std::string& s, int v)
        {
            if(v < 0)   s += "??";
            else
            {
                char buf[3];
                sprintf(buf, "%02X", v & 0xFF);
                s += buf;
            }
        }

        inline void n4(std::string& s, int v)
        {
            if(v < 0)   s += "??" "??";
            else
            {
                char buf[5];
                sprintf(buf, "%04X", v & 0xFFFF);
                s += buf;
            }
        }
    }


    
    void CpuTracer::traceCpuLine(const CpuState& stt)
    {
        if(!isOn())
            return;

        std::string out;
        out.reserve(100);

        /*  Demo line:
PCPC:  OP OA OB    LDA  ($xx), Y  [ADDR=VL]   AA XX YY [NVIZC] SP
        */

        n4(out, stt.PC);    out += ":  ";

        int op = bus->peek(stt.PC);     n2(out, op);        out += ' ';
        if(op < 0)
            out += "        { ? }                       ";
        else
        {
            int opa = bus->peek((stt.PC + 1) & 0xFFFF);
            int opb = bus->peek((stt.PC + 2) & 0xFFFF);

            int mode = AddrModes[op];
            switch(mode)
            {
            default:
            case Ip: case Ac:
                out += "       ";
                break;
            case Im: case Zp: case Zx: case Zy:
            case Ix: case Iy: case Rl:
                n2(out, opa);   out += "     ";
                break;
            case Ab: case Ax: case Ay: case In:
                n2(out, opa);   out += ' '; n2(out, opb); out += "  ";
                break;
            }
            
            /////////////////////////////////////////
            out += InstNames[op];
            out += ' ';

            /////////////////////////////////////////
            int final = -1;
            bool hasfinal = true;
            switch(mode)
            {
                /*($xx), Y  [ADDR=VL]*/
            default: case Ip:
                out += "                   ";       hasfinal = false;
                break;
            case Ac:
                out += "A                  ";       hasfinal = false;
                break;
            case Im:
                out += "#$";
                n2(out, opa);
                out += "               ";       hasfinal = false;
                break;
            case Zp:
                out += '$';
                n2(out, opa);
                out += "       ";
                final = opa;
                break;
            case Zx:
                out += '$';
                n2(out, opa);
                out += ",X     ";
                if(opa >= 0)        final = (opa + stt.X) & 0xFF;
                break;
            case Zy:
                out += '$';
                n2(out, opa);
                out += ",Y     ";
                if(opa >= 0)        final = (opa + stt.Y) & 0xFF;
                break;
            case Ab:
                out += '$';
                n2(out, opb);   n2(out,opa);
                out += "     ";
                final = opa | (opb << 8);
                break;
            case Ax:
                out += '$';
                n2(out, opb);   n2(out,opa);
                out += ",X   ";
                if(opa >= 0 && opb >= 0)            final = ((opa | (opb << 8)) + stt.X) & 0xFFFF;
                break;
            case Ay:
                out += '$';
                n2(out, opb);   n2(out,opa);
                out += ",Y   ";
                if(opa >= 0 && opb >= 0)            final = ((opa | (opb << 8)) + stt.Y) & 0xFFFF;
                break;
            case In:
                out += "($";
                n2(out, opb);   n2(out,opa);
                out += ")   ";
                if(opa >= 0 && opb >= 0)
                {
                    opb <<= 8;
                    final = bus->peek(opb | opa);
                    final |= bus->peek(opb | ((opa+1) & 0xFF)) << 8;
                }
                break;
            case Ix:
                out += "($";
                n2(out,opa);
                out += ", X)  ";
                if(opa >= 0)
                {
                    opa = (opa + stt.X) & 0xFF;
                    final = bus->peek(opa);     opa = (opa+1) & 0xFF;
                    final |= bus->peek(opa) << 8;
                }
                break;
            case Iy:
                out += "($";
                n2(out, opa);
                out += "), Y  ";
                if(opa >= 0)
                {
                    final = bus->peek(opa);     opa = (opa+1) & 0xFF;
                    final |= bus->peek(opa) << 8;
                    if(final >= 0)
                        final = (final + stt.Y) & 0xFFFF;
                }
                break;
            case Rl:
                out += '$';
                n2(out, opa);
                out += "       ";
                if(opa >= 0)
                    final = (stt.PC + 2 + ((opa ^ 0x80) - 0x80)) & 0xFFFF;
                break;
            }   // end huge switch for addressing modes

            if(hasfinal)
            {
                /*[ADDR=VL]*/
                out += '[';
                n4(out, final);
                out += '=';
                n2(out, bus->peek(final));
                out += ']';
            }
        }


        //////////////////////////////
        /*   AA XX YY [NVIZC] SP*/
        out += "   ";
        n2( out, stt.A );
        out += ' ';
        n2( out, stt.X );
        out += ' ';
        n2( out, stt.Y );
        out += " [";
        out += stt.getN() ? 'N' : '.';
        out += stt.getV() ? 'V' : '.';
        out += stt.getI() ? 'I' : '.';
        out += stt.getZ() ? 'Z' : '.';
        out += stt.getC() ? 'C' : '.';
        out += "] ";
        n2( out, stt.SP );


        //////////////////////////////
        out += '\n';
        (*streamOutput) << out;

    }

}