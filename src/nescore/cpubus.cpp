
#include "cpubus.h"
#include "resetinfo.h"
#include <stdexcept>
#include "cpu.h"

namespace schcore
{
    void CpuBus::reset(const ResetInfo& info)
    {
        // if IRQs are suppressed (NSFs), zero 'allowedIrqs'
        allowedIrqs = info.suppressIrqs ? 0 : ~0;
        
        // no interrupts after a reset
        pendingIrq = 0;
        pendingNmi = false;


        if(info.hardReset)
        {
            // reset all procs
            for(int i = 0; i < 0x10; ++i)
            {
                for(int j = 0; j < maxProcs; ++j)
                {
                    readers[i][j] = rdproc_t();
                    writers[i][j] = wrproc_t();
                    peekers[i][j] = pkproc_t();
                }
            }

            // reset lines
            aLine = 0;
            dLine = 0;
            wLine = false;

            // reset register IRQs
            irqNames.clear();
        }
    }

    ///////////////////////////////
    //  Memory accessing
    u8 CpuBus::read(u16 a)
    {
        cpu->consumeCycle();

        u8 v = dLine;
        for(auto& proc : readers[a>>12])
        {
            if(!proc)       break;
            proc(a, v);
        }
        wLine = false;
        aLine = a;
        return dLine = v;
    }

    void CpuBus::write(u16 a, u8 v)
    {
        cpu->consumeCycle();

        for(auto& proc : writers[a>>12])
        {
            if(!proc)       break;
            proc(a, v);
        }
        wLine = true;
        aLine = a;
        dLine = v;
    }

    int CpuBus::peek(u16 a) const
    {
        int v = -1;
        for(auto& proc : peekers[a>>12])
        {
            if(!proc)       break;
            v = proc(a);
            if(v >= 0)      break;
        }
        return v;
    }
    
    //////////////////////////////////////////////////
    //  Interrupt detection
    irqsource_t CpuBus::createIrqCode(const std::string& name)
    {
        irqsource_t out = (1 << irqNames.size());

        if(!out)
            throw std::runtime_error("Internal error:  Too many IRQ codes generated");

        irqNames.push_back(name);
        return out;
    }
    
    std::string CpuBus::getIrqName(irqsource_t s) const
    {
        std::string out;

        for(auto& n : irqNames)
        {
            if(s & 1)
            {
                if(!out.empty())
                    out += ", ";
                out += n;
            }
            s >>= 1;
            if(!s)
                break;
        }

        return out;
    }
    
    //////////////////////////////////////////////////
    //  Adding read/write/peek handlers
    namespace
    {
        template <typename T, int S>
        void addProc(int pagefirst, int pagelast, const T& proc, T (&lst)[0x10][S])
        {
            if(pagefirst < 0)           throw std::runtime_error("Internal error:  Bus proc being added with invalid page");
            if(pagelast > 0x0F)         throw std::runtime_error("Internal error:  Bus proc being added with invalid page");
            if(pagefirst > pagelast)    throw std::runtime_error("Internal error:  Bus proc being added with invalid page");

            for(; pagefirst <= pagelast; ++pagefirst)
            {
                bool added = false;
                for(auto& v : lst[pagefirst])
                {
                    if(v)   continue;
                    v = proc;
                    added = true;
                    break;
                }
                if(!added)
                    throw std::runtime_error("Internal error:  Too many procs being added to CpuBus");
            }
        }
    }
    
    void CpuBus::addReader(int pagefirst, int pagelast, const rdproc_t& proc)   {   addProc(pagefirst, pagelast, proc, readers);    }
    void CpuBus::addWriter(int pagefirst, int pagelast, const wrproc_t& proc)   {   addProc(pagefirst, pagelast, proc, writers);    }
    void CpuBus::addPeeker(int pagefirst, int pagelast, const pkproc_t& proc)   {   addProc(pagefirst, pagelast, proc, peekers);    }

}