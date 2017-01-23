
#include "cpu.h"
#include "cpubus.h"
#include "resetinfo.h"
#include "cputracer.h"
#include "eventmanager.h"

namespace schcore
{

    ////////////////////////////////////////////////////
    //  Memory Access
    inline u8   Cpu::rd(u16 a)          { return bus->read(a);                      }
    inline void Cpu::wr(u16 a, u8 v)    { bus->write(a,v);                          }
    inline u8   Cpu::pull()             { return bus->read( 0x0100 | ++cpu.SP );    }
    inline void Cpu::push(u8 v)         { return bus->write(0x0100 | cpu.SP--, v ); }


    inline void Cpu::pollInterrupt()
    {
        wantInterrupt = bus->isNmiPending() || ( !cpu.getI() && bus->isIrqPending() );
    }

    void Cpu::consumeCycle()
    {
        if(cpuJammed)       return;

        cyc();
        eventManager->check(curCyc());
    }

    void Cpu::primeNsf(u8 A, u8 X, u16 PC)
    {
        cpu.A = A;
        cpu.X = X;
        cpu.PC = PC;
        cpu.SP = 0xFF;
        wantReset = wantInterrupt = false;
        cpuJammed = false;
    }
    
    ////////////////////////////////////////////////////
    //  Reset
    void Cpu::reset(const ResetInfo& info)
    {
        cpuJammed = false;
        wantInterrupt = wantReset = true;

        if(info.hardReset)
        {
            tracer = info.cpuTracer;
            bus = info.cpuBus;
            eventManager = info.eventManager;

            subSystem_HardReset(info.cpu, info.region.cpuClockBase);
            cpu.A = 0;
            cpu.X = 0;
            cpu.Y = 0;
            cpu.SP = 0;
            cpu.PC = 0;         // this doesn't actually matter ... 'wantReset' will cause the reset vector to be loaded shortly
            cpu.setStatus( 0x34 );
        }

        // I flag will be set when the reset interrupt is performed.
    }

    ////////////////////////////////////////////////////
    //  Interrupts
    void Cpu::performInterrupt(bool sw)
    {
        u16 vector = 0xFFFE;        // default to IRQ/BRK vector

        if(sw)
        {
            // First cycle was opcode read -- we performed that already
            rd(cpu.PC++);
        }
        else
        {
            rd(cpu.PC);
            rd(cpu.PC);
        }

        if(wantReset)
        {
            // don't push anything, perform reads instead
            bus->read(0x0100 | cpu.SP--);       // PCH
            bus->read(0x0100 | cpu.SP--);       // PCL
            bus->read(0x0100 | cpu.SP--);       // Status
            vector = 0xFFFC;
            wantReset = false;
        }
        else
        {
            push( static_cast<u8>(cpu.PC >> 8) );
            push( static_cast<u8>(cpu.PC     ) );
            // hijacking happens at this point
            if(bus->isNmiPending())
            {
                vector = 0xFFFA;
                bus->acknowledgeNmi();
                sw = false;
            }
            else if(bus->isIrqPending())
            {
                sw = false;
            }
            push( cpu.getStatus(sw) );
        }

        // fetch vector
        cpu.PC  = rd(vector++);
        cpu.PC |= rd(vector) << 8;

        // set I flag, don't want another interrupt after this one
        wantInterrupt = false;
        cpu.setI(1);
    }
    
    ////////////////////////////////////////////////////
    //  Run!
    void Cpu::run(timestamp_t runto)
    {
        if(cpuJammed && (curCyc() < runto))
        {
            setMainTimestamp(runto);
        }

        while( curCyc() < runto )
        {
            /////////////////////////////////////
            // Are we to take an interrupt
            if(wantInterrupt)
            {
                performInterrupt(false);
                continue;
            }

            if( *tracer )
                tracer->traceCpuLine( cpu );

            /////////////////////////////////////
            // No interrupt, do an instruction
            u8 opcode = rd( cpu.PC++ );
            switch(opcode)
            {
                /* Branches */
            case 0x10:  adBranch( !cpu.getN() );        break;  /* BPL  */
            case 0x30:  adBranch(  cpu.getN() );        break;  /* BMI  */
            case 0x50:  adBranch( !cpu.getV() );        break;  /* BVC  */
            case 0x70:  adBranch(  cpu.getV() );        break;  /* BVS  */
            case 0x90:  adBranch( !cpu.getC() );        break;  /* BCC  */
            case 0xB0:  adBranch(  cpu.getC() );        break;  /* BCS  */
            case 0xD0:  adBranch( !cpu.getZ() );        break;  /* BNE  */
            case 0xF0:  adBranch(  cpu.getZ() );        break;  /* BEQ  */

                /* Flag flip-flop   */
            case 0x18:  adImplied(); cpu.setC(0);       break;  /* CLC  */
            case 0x38:  adImplied(); cpu.setC(1);       break;  /* SEC  */
            case 0x58:  adImplied(); cpu.setI(0);       break;  /* CLI  */
            case 0x78:  adImplied(); cpu.setI(1);       break;  /* SEI  */
            case 0xB8:  adImplied(); cpu.setV(0);       break;  /* CLV  */
            case 0xD8:  adImplied(); cpu.setD(0);       break;  /* CLD  */
            case 0xF8:  adImplied(); cpu.setD(1);       break;  /* SED  */

                /* Stack push/pull  */
            case 0x08:  adPush( cpu.getStatus(true) );  break;  /* PHP  */
            case 0x28:  cpu.setStatus( adPull() );      break;  /* PLP  */
            case 0x48:  adPush( cpu.A );                break;  /* PHA  */
            case 0x68:  cpu.NZ( cpu.A = adPull() );     break;  /* PLA  */

                /* Reg Xfer         */
            case 0xAA:  adImplied(); cpu.NZ( cpu.X = cpu.A );   break;  /* TAX  */
            case 0xA8:  adImplied(); cpu.NZ( cpu.Y = cpu.A );   break;  /* TAY  */
            case 0xBA:  adImplied(); cpu.NZ( cpu.X = cpu.SP );  break;  /* TSX  */
            case 0x8A:  adImplied(); cpu.NZ( cpu.A = cpu.X );   break;  /* TXA  */
            case 0x9A:  adImplied();        cpu.SP = cpu.X;     break;  /* TXS  */
            case 0x98:  adImplied(); cpu.NZ( cpu.A = cpu.Y );   break;  /* TYA  */

                /* Misc */
            case 0x00:  performInterrupt(true);         break;  /* BRK          */
            case 0x4C:  full_JMP();                     break;  /* JMP $xxxx    */
            case 0x6C:  full_JMP_Indirect();            break;  /* JMP ($xxxx)  */
            case 0x20:  full_JSR();                     break;  /* JSR $xxxx    */
            case 0xEA:  adImplied();                    break;  /* NOP          */
            case 0x40:  full_RTI();                     break;  /* RTI          */
            case 0x60:  full_RTS();                     break;  /* RTS          */

                /* ADC  */
            case 0x69:  ADC( adRdIm() );                break;
            case 0x65:  ADC( adRdZp() );                break;
            case 0x75:  ADC( adRdZx() );                break;
            case 0x6D:  ADC( adRdAb() );                break;
            case 0x7D:  ADC( adRdAx() );                break;
            case 0x79:  ADC( adRdAy() );                break;
            case 0x61:  ADC( adRdIx() );                break;
            case 0x71:  ADC( adRdIy() );                break;
                
                /* AND  */
            case 0x29:  AND( adRdIm() );                break;
            case 0x25:  AND( adRdZp() );                break;
            case 0x35:  AND( adRdZx() );                break;
            case 0x2D:  AND( adRdAb() );                break;
            case 0x3D:  AND( adRdAx() );                break;
            case 0x39:  AND( adRdAy() );                break;
            case 0x21:  AND( adRdIx() );                break;
            case 0x31:  AND( adRdIy() );                break;

                /* ASL  */
            case 0x0A:  adImplied();    ASL(cpu.A);     break;
            case 0x06:  adRWZp( &Cpu::ASL );            break;
            case 0x16:  adRWZx( &Cpu::ASL );            break;
            case 0x0E:  adRWAb( &Cpu::ASL );            break;
            case 0x1E:  adRWAx( &Cpu::ASL );            break;

                /* BIT  */
            case 0x24:  BIT( adRdZp() );                break;
            case 0x2C:  BIT( adRdAb() );                break;
                
                /* CMP  */
            case 0xC9:  CMP( adRdIm() );                break;
            case 0xC5:  CMP( adRdZp() );                break;
            case 0xD5:  CMP( adRdZx() );                break;
            case 0xCD:  CMP( adRdAb() );                break;
            case 0xDD:  CMP( adRdAx() );                break;
            case 0xD9:  CMP( adRdAy() );                break;
            case 0xC1:  CMP( adRdIx() );                break;
            case 0xD1:  CMP( adRdIy() );                break;
                
                /* CPX  */
            case 0xE0:  CPX( adRdIm() );                break;
            case 0xE4:  CPX( adRdZp() );                break;
            case 0xEC:  CPX( adRdAb() );                break;
                
                /* CPY  */
            case 0xC0:  CPY( adRdIm() );                break;
            case 0xC4:  CPY( adRdZp() );                break;
            case 0xCC:  CPY( adRdAb() );                break;
                
                /* DEC  */
            case 0xCA:  adImplied();    DEC(cpu.X);     break;  /* DEX  */
            case 0x88:  adImplied();    DEC(cpu.Y);     break;  /* DEY  */
            case 0xC6:  adRWZp( &Cpu::DEC );            break;
            case 0xD6:  adRWZx( &Cpu::DEC );            break;
            case 0xCE:  adRWAb( &Cpu::DEC );            break;
            case 0xDE:  adRWAx( &Cpu::DEC );            break;
                
                /* EOR  */
            case 0x49:  EOR( adRdIm() );                break;
            case 0x45:  EOR( adRdZp() );                break;
            case 0x55:  EOR( adRdZx() );                break;
            case 0x4D:  EOR( adRdAb() );                break;
            case 0x5D:  EOR( adRdAx() );                break;
            case 0x59:  EOR( adRdAy() );                break;
            case 0x41:  EOR( adRdIx() );                break;
            case 0x51:  EOR( adRdIy() );                break;
                
                /* INC  */
            case 0xE8:  adImplied();    INC(cpu.X);     break;  /* INX  */
            case 0xC8:  adImplied();    INC(cpu.Y);     break;  /* INY  */
            case 0xE6:  adRWZp( &Cpu::INC );            break;
            case 0xF6:  adRWZx( &Cpu::INC );            break;
            case 0xEE:  adRWAb( &Cpu::INC );            break;
            case 0xFE:  adRWAx( &Cpu::INC );            break;
                
                /* LDA  */
            case 0xA9:  cpu.NZ( cpu.A = adRdIm() );     break;
            case 0xA5:  cpu.NZ( cpu.A = adRdZp() );     break;
            case 0xB5:  cpu.NZ( cpu.A = adRdZx() );     break;
            case 0xAD:  cpu.NZ( cpu.A = adRdAb() );     break;
            case 0xBD:  cpu.NZ( cpu.A = adRdAx() );     break;
            case 0xB9:  cpu.NZ( cpu.A = adRdAy() );     break;
            case 0xA1:  cpu.NZ( cpu.A = adRdIx() );     break;
            case 0xB1:  cpu.NZ( cpu.A = adRdIy() );     break;
                
                /* LDX  */
            case 0xA2:  cpu.NZ( cpu.X = adRdIm() );     break;
            case 0xA6:  cpu.NZ( cpu.X = adRdZp() );     break;
            case 0xB6:  cpu.NZ( cpu.X = adRdZy() );     break;
            case 0xAE:  cpu.NZ( cpu.X = adRdAb() );     break;
            case 0xBE:  cpu.NZ( cpu.X = adRdAy() );     break;
                
                /* LDY  */
            case 0xA0:  cpu.NZ( cpu.Y = adRdIm() );     break;
            case 0xA4:  cpu.NZ( cpu.Y = adRdZp() );     break;
            case 0xB4:  cpu.NZ( cpu.Y = adRdZx() );     break;
            case 0xAC:  cpu.NZ( cpu.Y = adRdAb() );     break;
            case 0xBC:  cpu.NZ( cpu.Y = adRdAx() );     break;
                
                /* LSR  */
            case 0x4A:  adImplied();    LSR(cpu.A);     break;
            case 0x46:  adRWZp( &Cpu::LSR );            break;
            case 0x56:  adRWZx( &Cpu::LSR );            break;
            case 0x4E:  adRWAb( &Cpu::LSR );            break;
            case 0x5E:  adRWAx( &Cpu::LSR );            break;
                
                /* ORA  */
            case 0x09:  ORA( adRdIm() );                break;
            case 0x05:  ORA( adRdZp() );                break;
            case 0x15:  ORA( adRdZx() );                break;
            case 0x0D:  ORA( adRdAb() );                break;
            case 0x1D:  ORA( adRdAx() );                break;
            case 0x19:  ORA( adRdAy() );                break;
            case 0x01:  ORA( adRdIx() );                break;
            case 0x11:  ORA( adRdIy() );                break;

                /* ROL  */
            case 0x2A:  adImplied();    ROL(cpu.A);     break;
            case 0x26:  adRWZp( &Cpu::ROL );            break;
            case 0x36:  adRWZx( &Cpu::ROL );            break;
            case 0x2E:  adRWAb( &Cpu::ROL );            break;
            case 0x3E:  adRWAx( &Cpu::ROL );            break;

                /* ROR  */
            case 0x6A:  adImplied();    ROR(cpu.A);     break;
            case 0x66:  adRWZp( &Cpu::ROR );            break;
            case 0x76:  adRWZx( &Cpu::ROR );            break;
            case 0x6E:  adRWAb( &Cpu::ROR );            break;
            case 0x7E:  adRWAx( &Cpu::ROR );            break;
                
                /* SBC  */
            case 0xE9:  SBC( adRdIm() );                break;
            case 0xE5:  SBC( adRdZp() );                break;
            case 0xF5:  SBC( adRdZx() );                break;
            case 0xED:  SBC( adRdAb() );                break;
            case 0xFD:  SBC( adRdAx() );                break;
            case 0xF9:  SBC( adRdAy() );                break;
            case 0xE1:  SBC( adRdIx() );                break;
            case 0xF1:  SBC( adRdIy() );                break;
                
                /* STA  */
            case 0x85:  adWrZp( cpu.A );                break;
            case 0x95:  adWrZx( cpu.A );                break;
            case 0x8D:  adWrAb( cpu.A );                break;
            case 0x9D:  adWrAx( cpu.A );                break;
            case 0x99:  adWrAy( cpu.A );                break;
            case 0x81:  adWrIx( cpu.A );                break;
            case 0x91:  adWrIy( cpu.A );                break;
                
                /* STX  */
            case 0x86:  adWrZp( cpu.X );                break;
            case 0x96:  adWrZy( cpu.X );                break;
            case 0x8E:  adWrAb( cpu.X );                break;
                
                /* STY  */
            case 0x84:  adWrZp( cpu.Y );                break;
            case 0x94:  adWrZx( cpu.Y );                break;
            case 0x8C:  adWrAb( cpu.Y );                break;
                
                /////////////////////////////////////
                // Unofficial ops
                
                /* One offs */
 case 0x0B: case 0x2B:  ANC( adRdIm() );                break;  /* ANC  */
            case 0x4B:  ALR( adRdIm() );                break;  /* ALR  */
            case 0x6B:  ARR( adRdIm() );                break;  /* ARR  */
            case 0xCB:  AXS( adRdIm() );                break;  /* AXS  */
            case 0xBB:  LAS( adRdAy() );                break;  /* LAS  */
            case 0xEB:  SBC( adRdIm() );                break;  /* alternative SBC  */
            case 0x9E:  adWrAy_xxx( cpu.X );            break;  /* SHX  */
            case 0x9C:  adWrAx_xxx( cpu.Y );            break;  /* SHY  */
            case 0x8B:  XAA( adRdIm() );                break;  /* XAA  */

            case 0x9B:  cpu.SP = cpu.A & cpu.X; adWrAy_xxx( cpu.SP );     break;  /* TAS  */
                
                /* AHX  */
            case 0x9F:  adWrAy_xxx( cpu.A & cpu.X );    break;
            case 0x93:  adWrIy_xxx( cpu.A & cpu.X );    break;

                /* DCP  */
            case 0xC7:  adRWZp( &Cpu::DCP );            break;
            case 0xD7:  adRWZx( &Cpu::DCP );            break;
            case 0xCF:  adRWAb( &Cpu::DCP );            break;
            case 0xDF:  adRWAx( &Cpu::DCP );            break;
            case 0xDB:  adRWAy( &Cpu::DCP );            break;
            case 0xC3:  adRWIx( &Cpu::DCP );            break;
            case 0xD3:  adRWIy( &Cpu::DCP );            break;
                
                /* ISC  */
            case 0xE7:  adRWZp( &Cpu::ISC );            break;
            case 0xF7:  adRWZx( &Cpu::ISC );            break;
            case 0xEF:  adRWAb( &Cpu::ISC );            break;
            case 0xFF:  adRWAx( &Cpu::ISC );            break;
            case 0xFB:  adRWAy( &Cpu::ISC );            break;
            case 0xE3:  adRWIx( &Cpu::ISC );            break;
            case 0xF3:  adRWIy( &Cpu::ISC );            break;

                /* LAX  */
            case 0xAB:  LAX( adRdIm() );                break;
            case 0xA7:  LAX( adRdZp() );                break;
            case 0xB7:  LAX( adRdZy() );                break;
            case 0xAF:  LAX( adRdAb() );                break;
            case 0xBF:  LAX( adRdAy() );                break;
            case 0xA3:  LAX( adRdIx() );                break;
            case 0xB3:  LAX( adRdIy() );                break;

                /* NOP  */
            case 0x1A: case 0x3A: case 0x5A: case 0x7A: case 0xDA: case 0xFA:   adImplied();break;
            case 0x04: case 0x44: case 0x64:                                    adRdZp();   break;
            case 0x14: case 0x34: case 0x54: case 0x74: case 0xD4: case 0xF4:   adRdZx();   break;
            case 0x80: case 0x82: case 0x89: case 0xC2: case 0xE2:              adRdIm();   break;
            case 0x0C:                                                          adRdAb();   break;
            case 0x1C: case 0x3C: case 0x5C: case 0x7C: case 0xDC: case 0xFC:   adRdAx();   break;
                
                /* RLA  */
            case 0x27:  adRWZp( &Cpu::RLA );            break;
            case 0x37:  adRWZx( &Cpu::RLA );            break;
            case 0x2F:  adRWAb( &Cpu::RLA );            break;
            case 0x3F:  adRWAx( &Cpu::RLA );            break;
            case 0x3B:  adRWAy( &Cpu::RLA );            break;
            case 0x23:  adRWIx( &Cpu::RLA );            break;
            case 0x33:  adRWIy( &Cpu::RLA );            break;
                
                /* RRA  */
            case 0x67:  adRWZp( &Cpu::RRA );            break;
            case 0x77:  adRWZx( &Cpu::RRA );            break;
            case 0x6F:  adRWAb( &Cpu::RRA );            break;
            case 0x7F:  adRWAx( &Cpu::RRA );            break;
            case 0x7B:  adRWAy( &Cpu::RRA );            break;
            case 0x63:  adRWIx( &Cpu::RRA );            break;
            case 0x73:  adRWIy( &Cpu::RRA );            break;

                /* SAX  */
            case 0x87:  adWrZp( cpu.A & cpu.X );        break;
            case 0x97:  adWrZy( cpu.A & cpu.X );        break;
            case 0x8F:  adWrAb( cpu.A & cpu.X );        break;
            case 0x83:  adWrIx( cpu.A & cpu.X );        break;
                
                /* SLO  */
            case 0x07:  adRWZp( &Cpu::SLO );            break;
            case 0x17:  adRWZx( &Cpu::SLO );            break;
            case 0x0F:  adRWAb( &Cpu::SLO );            break;
            case 0x1F:  adRWAx( &Cpu::SLO );            break;
            case 0x1B:  adRWAy( &Cpu::SLO );            break;
            case 0x03:  adRWIx( &Cpu::SLO );            break;
            case 0x13:  adRWIy( &Cpu::SLO );            break;
                
                /* SRE  */
            case 0x47:  adRWZp( &Cpu::SRE );            break;
            case 0x57:  adRWZx( &Cpu::SRE );            break;
            case 0x4F:  adRWAb( &Cpu::SRE );            break;
            case 0x5F:  adRWAx( &Cpu::SRE );            break;
            case 0x5B:  adRWAy( &Cpu::SRE );            break;
            case 0x43:  adRWIx( &Cpu::SRE );            break;
            case 0x53:  adRWIy( &Cpu::SRE );            break;

                /* STP  */
            case 0x02: case 0x12: case 0x22: case 0x32: case 0x42: case 0x52:
            case 0x62: case 0x72: case 0x92: case 0xB2: case 0xD2: case 0xF2:
                cpuJammed = true;
                setMainTimestamp(runto);
                break;
            }
        }
    }

}