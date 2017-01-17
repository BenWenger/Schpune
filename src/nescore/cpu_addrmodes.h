
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
        u8                  rd(u16 a);
        void                wr(u16 a, u8 v);
        void                pollInterrupt();
        bool                wantInterrupt;

        void                push(u8 v);
        u8                  pull();

        CpuState            cpu;

        typedef int         rmw_op;

        #define CALLOP(v)
#else
        #define CALLOP(v)       (this->*op)(v)
#endif
        
///////////////////////////////////////////
///////////////////////////////////////////
//  Support
inline u8 badPageRead(u16 a, u16 t)
{
    return rd( (t & 0xFF00) | (a & 0x00FF) );
}

inline void conditionalBadPageRead(u16 a, u16 t)
{
    if((t ^ a) & 0xFF00)
        badPageRead(a,t);
}

///////////////////////////////////////////
///////////////////////////////////////////
//  Read-only addressing modes

// Immediate mode
u8      adRdIm()
{
    pollInterrupt(); return rd(cpu.PC++);
}

// Zero Page mode
u8      adRdZp()
{
    u8 a =                  rd(cpu.PC++);
    pollInterrupt(); return rd(a);
}

// Zero Page Indexed mode
u8      adRdZr(u8 r)
{
    u8 a =                  rd(cpu.PC++);
                            rd(a);              a += r;
    pollInterrupt(); return rd(a);
}
u8      adRdZx()        { return adRdZr(cpu.X);         }
u8      adRdZy()        { return adRdZr(cpu.Y);         }

// Absolute mode
u8      adRdAb()
{
    u16 a =                 rd(cpu.PC++);
    a |=                    rd(cpu.PC++) << 8;
    pollInterrupt(); return rd(a);
}

// Absolute Indexed mode
u8      adRdAr(u8 r)
{
    u16 t =                 rd(cpu.PC++);
    t |=                    rd(cpu.PC++) << 8;      u16 a = t + r;
    conditionalBadPageRead(a,t);
    pollInterrupt(); return rd(a);
}
u8      adRdAx()        { return adRdAr(cpu.X);         }
u8      adRdAy()        { return adRdAr(cpu.Y);         }

// Indirect X mode
u8      adRdIx()
{
    u8 t =                  rd(cpu.PC++);
                            rd(t);          t += cpu.X;
    u16 a =                 rd(t++);
    a |=                    rd(t) << 8;
    pollInterrupt(); return rd(a);
}

// Indirect Y mode
u8      adRdIy()
{
    u8 p =                  rd(cpu.PC++);
    u16 t =                 rd(p++);
    t |=                    rd(p) << 8;     u16 a = t + cpu.Y;
    conditionalBadPageRead(a,t);
    pollInterrupt(); return rd(a);
}


///////////////////////////////////////////
///////////////////////////////////////////
//  'Write' addressing modes

// Zero Page mode
void    adWrZp(u8 v)
{
    u8 a =                  rd(cpu.PC++);
    pollInterrupt();        wr(a,v);
}

// Zero Page Indexed mode
void    adWrZr(u8 v, u8 r)
{
    u8 a =                  rd(cpu.PC++);
                            rd(a);              a += r;
    pollInterrupt();        wr(a,v);
}
void    adWrZx(u8 v)        { adWrZr(v, cpu.X);             }
void    adWrZy(u8 v)        { adWrZr(v, cpu.Y);             }

// Absolute mode
void    adWrAb(u8 v)
{
    u16 a =                 rd(cpu.PC++);
    a |=                    rd(cpu.PC++) << 8;
    pollInterrupt();        wr(a,v);
}

// Absolute Indexed mode
void    adWrAr(u8 v, u8 r)
{
    u16 t =                 rd(cpu.PC++);
    t |=                    rd(cpu.PC++) << 8;      u16 a = t + r;
    badPageRead(a,t);
    pollInterrupt();        wr(a,v);
}
void    adWrAx(u8 v)        { adWrAr(v, cpu.X);             }
void    adWrAy(u8 v)        { adWrAr(v, cpu.Y);             }

// Absolute Indexed mode (with conflicts)
void    adWrAr_xxx(u8 v, u8 r)
{
    u16 t =                 rd(cpu.PC++);
    t |=                    rd(cpu.PC++) << 8;      u16 a = t + r;
    v &= badPageRead(a,t);
    pollInterrupt();        wr(a,v);
}
void    adWrAx_xxx(u8 v)    { adWrAr_xxx(v, cpu.X);         }
void    adWrAy_xxx(u8 v)    { adWrAr_xxx(v, cpu.Y);         }

// Indirect X mode
void    adWrIx(u8 v)
{
    u8 t =                  rd(cpu.PC++);
                            rd(t);          t += cpu.X;
    u16 a =                 rd(t++);
    a |=                    rd(t) << 8;
    pollInterrupt();        wr(a,v);
}

// Indirect Y mode
void    adWrIy(u8 v)
{
    u8 p =                  rd(cpu.PC++);
    u16 t =                 rd(p++);
    t |=                    rd(p) << 8;     u16 a = t + cpu.Y;
    badPageRead(a,t);
    pollInterrupt();        wr(a,v);
}

// Indirect Y mode (with conflicts)
void    adWrIy_xxx(u8 v)
{
    u8 p =                  rd(cpu.PC++);
    u16 t =                 rd(p++);
    t |=                    rd(p) << 8;     u16 a = t + cpu.Y;
    v &= badPageRead(a,t);
    pollInterrupt();        wr(a,v);
}


///////////////////////////////////////////
///////////////////////////////////////////
//  'Read-Modify-Write' addressing modes

// Zero Page mode
void    adRWZp(rmw_op op)
{
    u8 a =                  rd(cpu.PC++);
    u8 v =                  rd(a);
                            wr(a,v);    CALLOP(v);
    pollInterrupt();        wr(a,v);
}

// Zero Page Indexed mode
void    adRWZr(rmw_op op, u8 r)
{
    u8 a =                  rd(cpu.PC++);
                            rd(a);              a += r;
    u8 v =                  rd(a);
                            wr(a,v);    CALLOP(v);
    pollInterrupt();        wr(a,v);
}
void    adRWZx(rmw_op op)   { adRWZr(op, cpu.X);            }
void    adRWZy(rmw_op op)   { adRWZr(op, cpu.Y);            }

// Absolute mode
void    adRWAb(rmw_op op)
{
    u16 a =                 rd(cpu.PC++);
    a |=                    rd(cpu.PC++) << 8;
    u8 v =                  rd(a);
                            wr(a,v);    CALLOP(v);
    pollInterrupt();        wr(a,v);
}

// Absolute Indexed mode
void    adRWAr(rmw_op op, u8 r)
{
    u16 t =                 rd(cpu.PC++);
    t |=                    rd(cpu.PC++) << 8;      u16 a = t + r;
    badPageRead(a,t);
    u8 v =                  rd(a);
                            wr(a,v);    CALLOP(v);
    pollInterrupt();        wr(a,v);
}
void    adRWAx(rmw_op op)   { adRWAr(op, cpu.X);            }
void    adRWAy(rmw_op op)   { adRWAr(op, cpu.Y);            }

// Indirect X mode
void    adRWIx(rmw_op op)
{
    u8 t =                  rd(cpu.PC++);
                            rd(t);          t += cpu.X;
    u16 a =                 rd(t++);
    a |=                    rd(t) << 8;
    u8 v =                  rd(a);
                            wr(a,v);    CALLOP(v);
    pollInterrupt();        wr(a,v);
}

// Indirect Y mode
void    adRWIy(rmw_op op)
{
    u8 p =                  rd(cpu.PC++);
    u16 t =                 rd(p++);
    t |=                    rd(p) << 8;     u16 a = t + cpu.Y;
    badPageRead(a,t);
    u8 v =                  rd(a);
                            wr(a,v);    CALLOP(v);
    pollInterrupt();        wr(a,v);
}



///////////////////////////////////////////
///////////////////////////////////////////
//  Misc modes

void    adPush(u8 v)
{
                            rd(cpu.PC);
    pollInterrupt();        push(v);
}

u8      adPull()
{
                            rd(cpu.PC);
                            rd( 0x0100 | cpu.SP );
    pollInterrupt(); return pull();
}

void    adImplied()
{
    pollInterrupt();        rd(cpu.PC);
}

void    adBranch(bool cond)
{
    pollInterrupt();

    if(cond)
    {
        int adj =       (rd( cpu.PC++ ) ^ 0x80) - 0x80;
        u16 newpc =     static_cast<u16>( cpu.PC + adj );

        rd(cpu.PC);

        // subtle interrupt polling behavior here means we can't use conditionalBadPageRead... bummer
        if( (newpc ^ cpu.PC) & 0xFF00 )
        {
            if( !wantInterrupt )        pollInterrupt();
            badPageRead( newpc, cpu.PC );
        }

        cpu.PC = newpc;
    }
    else
        rd( cpu.PC++ );
}



#undef CALLOP

#ifdef INTELLISENSE_HELPING_HAND
        }
#endif