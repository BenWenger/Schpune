
#ifndef SCHPUNE_NESCORE_CPUBUS_H_INCLUDED
#define SCHPUNE_NESCORE_CPUBUS_H_INCLUDED

#include <string>
#include <functional>
#include <vector>
#include "schpunetypes.h"


namespace schcore
{
    struct ResetInfo;
    class Cpu;

    class CpuBus
    {
    public:
        //////////////////////////////////////////////////
        //  Resetting
        void                reset(const ResetInfo& info);


        //////////////////////////////////////////////////
        //  Callback handler types
        typedef std::function<void(u16,u8&)>    rdproc_t;
        typedef std::function<void(u16,u8 )>    wrproc_t;
        typedef std::function<int (u16    )>    pkproc_t;

        //////////////////////////////////////////////////
        //  Primary interfacing with the bus
        u8                  read(u16 a);
        void                write(u16 a, u8 v);
        int                 peek(u16 a) const;  // 'peek' is effectively a consequence-free read (no side-effects).
                                                //   return value is < 0 if no value could be read
        
        //////////////////////////////////////////////////
        //  Interrupt detection
        irqsource_t         createIrqCode(const std::string& name);
        std::string         getIrqName(irqsource_t s) const;

        irqsource_t         isIrqPending() const            { return pendingIrq & allowedIrqs;  }
        bool                isNmiPending() const            { return pendingNmi;    }
        void                triggerIrq(irqsource_t i)       { pendingIrq |= i;      }
        void                acknowledgeIrq(irqsource_t i)   { pendingIrq &= ~i;     }
        void                triggerNmi()                    { pendingNmi = true;    }
        void                acknowledgeNmi()                { pendingNmi = false;   }

        //////////////////////////////////////////////////
        //  The below functions are not updated until AFTER a read/write fully completes.
        //      Therefore they can be called from read/write handlers to see what
        //  the 'previous' address/data was prior to the current read/write being processed.
        //  This makes detecting bus line changes easier.
        u16                 getALine() const        { return aLine;     }
        u8                  getDLine() const        { return dLine;     }
        bool                wasWriteLast() const    { return wLine;     }   // true if the last access was a write


        //////////////////////////////////////////////////
        //  Adding read/write/peek handlers
        void                addReader(int pagefirst, int pagelast, const rdproc_t& proc);
        void                addWriter(int pagefirst, int pagelast, const wrproc_t& proc);
        void                addPeeker(int pagefirst, int pagelast, const pkproc_t& proc);

        // simplified template interface
        template <typename C> void  addReader(int pagefirst, int pagelast, C* obj, void (C::*proc)(u16,u8&))
            { addReader(pagefirst, pagelast, std::bind(proc, obj, std::placeholders::_1, std::placeholders::_2) );      }
        template <typename C> void  addWriter(int pagefirst, int pagelast, C* obj, void (C::*proc)(u16,u8 ))
            { addWriter(pagefirst, pagelast, std::bind(proc, obj, std::placeholders::_1, std::placeholders::_2) );      }
        template <typename C> void  addPeeker(int pagefirst, int pagelast, const C* obj, void (C::*proc)(u16) const)
            { addPeeker(pagefirst, pagelast, std::bind(proc, obj, std::placeholders::_1) );                             }

    private:
        static const int            maxProcs = 6;
        rdproc_t                    readers[0x10][maxProcs];
        wrproc_t                    writers[0x10][maxProcs];
        pkproc_t                    peekers[0x10][maxProcs];

        Cpu*                        cpu;                // access to CPU for consuming cycles

        u16                         aLine;
        u8                          dLine;
        bool                        wLine;

        irqsource_t                 pendingIrq;
        irqsource_t                 allowedIrqs;
        bool                        pendingNmi;

        std::vector<std::string>    irqNames;
    };


}

#endif