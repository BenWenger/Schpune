
//#define INTELLISENSE_HELPING_HAND       // comment out before compiling -- this is just to help with intellisense

#ifndef INTELLISENSE_HELPING_HAND
    #ifndef SCHPUNE_NESCORE_CPU_H___SUBHEADERSOK
    #error This header must not be included by any file other than cpu.h
    #endif
#endif

#ifdef INTELLISENSE_HELPING_HAND
        #include "schpunetypes.h"
        #include "cpustate.h"

        namespace schcore
        {
        CpuState            cpu;
        
        u8                  rd(u16 a);
        void                wr(u16 a, u8 v);
        void                pollInterrupt();
        bool                wantInterrupt;

        void                push(u8 v);
        u8                  pull();
#endif

        
///////////////////////////////////////////
///////////////////////////////////////////
//  Read operators
void ADC(u8 v)
{
    int sum = cpu.A + v + cpu.getC();
    cpu.setV( (cpu.A ^ sum) & (v ^ sum) & 0x80 );
    cpu.setC( sum > 0xFF );
    cpu.NZ( cpu.A = static_cast<u8>(sum) );
}

void BIT(u8 v)
{
    cpu.NZ( v & cpu.A );
    cpu.setV( v & 0x40 );
    cpu.setN( v & 0x80 );
}

void CMR(u8 v, u8 r)
{
    int cmp = r - v;
    cpu.setC(cmp >= 0);
    cpu.NZ( static_cast<u8>(cmp) );
}

void AND(u8 v)  { cpu.NZ( cpu.A &= v );     }
void CMP(u8 v)  { CMR(v, cpu.A );           }
void CPX(u8 v)  { CMR(v, cpu.X );           }
void CPY(u8 v)  { CMR(v, cpu.Y );           }
void EOR(u8 v)  { cpu.NZ( cpu.A ^= v );     }
void LDA(u8 v)  { cpu.NZ( cpu.A  = v );     }
void LDX(u8 v)  { cpu.NZ( cpu.X  = v );     }
void LDY(u8 v)  { cpu.NZ( cpu.Y  = v );     }
void ORA(u8 v)  { cpu.NZ( cpu.A |= v );     }
void SBC(u8 v)  { ADC(~v);                  }


///////////////////////////////////////////
///////////////////////////////////////////
//  Read-Modify-Write operators

void ASL(u8& v)
{
    cpu.setC( v & 0x80 );
    cpu.NZ( v <<= 1 );
}

void LSR(u8& v)
{
    cpu.setC( v & 0x01 );
    cpu.NZ( v >>= 1 );
}

void ROL(u8& v)
{
    bool c = cpu.getC();
    cpu.setC( v & 0x80 );
    v <<= 1;
    if(c)       v |= 0x01;
    cpu.NZ( v );
}

void ROR(u8& v)
{
    bool c = cpu.getC();
    cpu.setC( v & 0x01 );
    v >>= 1;
    if(c)       v |= 0x80;
    cpu.NZ( v );
}

void DEC(u8& v) { cpu.NZ( --v );            }
void INC(u8& v) { cpu.NZ( ++v );            }


///////////////////////////////////////////
///////////////////////////////////////////
//  Full instructions (don't need any addressing mode... they ARE the addressing mode

void full_RTI()
{
    rd(cpu.PC);
    rd( 0x0100 | cpu.SP );
    cpu.setStatus( pull() );
    cpu.PC  = pull();
    cpu.PC |= pull() << 8;
}

void full_RTS()
{
    rd(cpu.PC);
    rd( 0x0100 | cpu.SP );
    cpu.PC  = pull();
    cpu.PC |= pull() << 8;
    rd( cpu.PC++ );
}

void full_JSR()
{
    u8 a = rd(cpu.PC++);
    rd( 0x0100 | cpu.SP );
    push( static_cast<u8>( cpu.PC >> 8 ) );
    push( static_cast<u8>( cpu.PC      ) );
    cpu.PC = a | (rd(cpu.PC) << 8);
}

void full_JMP()
{
    u8 a = rd(cpu.PC++);
    cpu.PC = a | (rd(cpu.PC) << 8);
}

void full_JMP_Indirect()
{
    u8 lo   = rd(cpu.PC++);
    u16 hi  = rd(cpu.PC) << 8;
    cpu.PC  = rd(hi | lo++);
    cpu.PC |= rd(hi | lo) << 8;
}


#ifdef INTELLISENSE_HELPING_HAND
        }
#endif