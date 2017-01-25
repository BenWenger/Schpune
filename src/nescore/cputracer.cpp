
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
            Rl          // Relative
        };

        const int AddrModes[0x100] = {
    /*      x0 x1 x2 x3  x4 x5 x6 x7   x8 x9 xA xB  xC xD xE xF       */
    /* 0x */Im,Ix,Ip,Ix, Zp,Zp,Zp,Zp,  Ip,Im,Ac,Im, Ab,Ab,Ab,Ab,/* 0x */
    /* 1x */Rl,Iy,Ip,Iy, Zx,Zx,Zx,Zx,  Ip,Ay,Ip,Ay, Ax,Ax,Ax,Ax,/* 1x */
    /* 2x */Ab,Ix,Ip,Ix, Zp,Zp,Zp,Zp,  Ip,Im,Ac,Im, Ab,Ab,Ab,Ab,/* 2x */
    /* 3x */Rl,Iy,Ip,Iy, Zx,Zx,Zx,Zx,  Ip,Ay,Ip,Ay, Ax,Ax,Ax,Ax,/* 3x */
    /* 4x */Ip,Ix,Ip,Ix, Zp,Zp,Zp,Zp,  Ip,Im,Ac,Im, Ab,Ab,Ab,Ab,/* 4x */
    /* 5x */Rl,Iy,Ip,Iy, Zx,Zx,Zx,Zx,  Ip,Ay,Ip,Ay, Ax,Ax,Ax,Ax,/* 5x */
    /* 6x */Ip,Ix,Ip,Ix, Zp,Zp,Zp,Zp,  Ip,Im,Ac,Im, In,Ab,Ab,Ab,/* 6x */
    /* 7x */Rl,Iy,Ip,Iy, Zx,Zx,Zx,Zx,  Ip,Ay,Ip,Ay, Ax,Ax,Ax,Ax,/* 7x */
    /*      x0 x1 x2 x3  x4 x5 x6 x7   x8 x9 xA xB  xC xD xE xF       */
    /* 8x */Im,Ix,Im,Ix, Zp,Zp,Zp,Zp,  Ip,Im,Ip,Im, Ab,Ab,Ab,Ab,/* 8x */
    /* 9x */Rl,Iy,Ip,Iy, Zx,Zx,Zy,Zy,  Ip,Ay,Ip,Ay, Ax,Ax,Ay,Ay,/* 9x */
    /* Ax */Im,Ix,Im,Ix, Zp,Zp,Zp,Zp,  Ip,Im,Ip,Im, Ab,Ab,Ab,Ab,/* Ax */
    /* Bx */Rl,Iy,Ip,Iy, Zx,Zx,Zy,Zy,  Ip,Ay,Ip,Ay, Ax,Ax,Ay,Ay,/* Bx */
    /* Cx */Im,Ix,Im,Ix, Zp,Zp,Zp,Zp,  Ip,Im,Ip,Im, Ab,Ab,Ab,Ab,/* Cx */
    /* Dx */Rl,Iy,Ip,Iy, Zx,Zx,Zx,Zx,  Ip,Ay,Ip,Ay, Ax,Ax,Ax,Ax,/* Dx */
    /* Ex */Im,Ix,Im,Ix, Zp,Zp,Zp,Zp,  Ip,Im,Ip,Im, Ab,Ab,Ab,Ab,/* Ex */
    /* Fx */Rl,Iy,Ip,Iy, Zx,Zx,Zx,Zx,  Ip,Ay,Ip,Ay, Ax,Ax,Ax,Ax /* Fx */
    /*      x0 x1 x2 x3  x4 x5 x6 x7   x8 x9 xA xB  xC xD xE xF       */
        };

        const char* const InstNames[0x100] = {
/*        x0      x1      x2      x3      x4      x5      x6      x7        x8      x9      xA      xB      xC      xD      xE      xF          */
/* 0x */" BRK "," ORA ","{STP}","{SLO}","{NOP}"," ORA "," ASL ","{SLO}",  " PHP "," ORA "," ASL ","{ANC}","{NOP}"," ORA "," ASL ","{SLO}",/* 0x */
/* 1x */" BPL "," ORA ","{STP}","{SLO}","{NOP}"," ORA "," ASL ","{SLO}",  " CLC "," ORA ","{NOP}","{SLO}","{NOP}"," ORA "," ASL ","{SLO}",/* 1x */
/* 2x */" JSR "," AND ","{STP}","{RLA}"," BIT "," AND "," ROL ","{RLA}",  " PLP "," AND "," ROL ","{ANC}"," BIT "," AND "," ROL ","{RLA}",/* 2x */
/* 3x */" BMI "," AND ","{STP}","{RLA}","{NOP}"," AND "," ROL ","{RLA}",  " SEC "," AND ","{NOP}","{RLA}","{NOP}"," AND "," ROL ","{RLA}",/* 3x */
/* 4x */" RTI "," EOR ","{STP}","{SRE}","{NOP}"," EOR "," LSR ","{SRE}",  " PHA "," EOR "," LSR ","{ALR}"," JMP "," EOR "," LSR ","{SRE}",/* 4x */
/* 5x */" BVC "," EOR ","{STP}","{SRE}","{NOP}"," EOR "," LSR ","{SRE}",  " CLI "," EOR ","{NOP}","{SRE}","{NOP}"," EOR "," LSR ","{SRE}",/* 5x */
/* 6x */" RTS "," ADC ","{STP}","{RRA}","{NOP}"," ADC "," ROR ","{RRA}",  " PLA "," ADC "," ROR ","{ARR}"," JMP "," ADC "," ROR ","{RRA}",/* 6x */
/* 7x */" BVS "," ADC ","{STP}","{RRA}","{NOP}"," ADC "," ROR ","{RRA}",  " SEI "," ADC ","{NOP}","{RRA}","{NOP}"," ADC "," ROR ","{RRA}",/* 7x */
/*        x0      x1      x2      x3      x4      x5      x6      x7        x8      x9      xA      xB      xC      xD      xE      xF          */
/* 8x */"{NOP}"," STA ","{NOP}","{SAX}"," STY "," STA "," STX ","{SAX}",  " DEY ","{NOP}"," TXA ","{XAA}"," STY "," STA "," STX ","{SAX}",/* 8x */
/* 9x */" BCC "," STA ","{STP}","{AHX}"," STY "," STA "," STX ","{SAX}",  " TYA "," STA "," TXS ","{TAS}","{SHY}"," STA ","{SHX}","{AHX}",/* 9x */
/* Ax */" LDY "," LDA "," LDX ","{LAX}"," LDY "," LDA "," LDX ","{LAX}",  " TAY "," LDA "," TAX ","{LAX}"," LDY "," LDA "," LDX ","{LAX}",/* Ax */
/* Bx */" BCS "," LDA ","{STP}","{LAX}"," LDY "," LDA "," LDX ","{LAX}",  " CLV "," LDA "," TSX ","{LAS}"," LDY "," LDA "," LDX ","{LAX}",/* Bx */
/* Cx */" CPY "," CMP ","{NOP}","{DCP}"," CPY "," CMP "," DEC ","{DCP}",  " INY "," CMP "," DEX ","{AXS}"," CPY "," CMP "," DEC ","{DCP}",/* Cx */
/* Dx */" BNE "," CMP ","{STP}","{DCP}","{NOP}"," CMP "," DEC ","{DCP}",  " CLD "," CMP ","{NOP}","{DCP}","{NOP}"," CMP "," DEC ","{DCP}",/* Dx */
/* Ex */" CPX "," SBC ","{NOP}","{ISC}"," CPX "," SBC "," INC ","{ISC}",  " INX "," SBC "," NOP ","{SBC}"," CPX "," SBC "," INC ","{ISC}",/* Ex */
/* Fx */" BEQ "," SBC ","{STP}","{ISC}","{NOP}"," SBC "," INC ","{ISC}",  " SED "," SBC ","{NOP}","{ISC}","{NOP}"," SBC "," INC ","{ISC}" /* Fx */
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

    void CpuTracer::traceRawLine(const std::string& str)
    {
        if(isOn())
            (*streamOutput) << str << '\n';
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