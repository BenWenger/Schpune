
#ifndef SCHPUNE_NESCORE_ERROR_H_INCLUDED
#define SCHPUNE_NESCORE_ERROR_H_INCLUDED

#include <stdexcept>

namespace schcore
{
    class Error : public std::runtime_error
    {
    public:
        Error(const char* s) : runtime_error(s) {}
        Error(const std::string& s) : runtime_error(s) {}
    };
}

#endif
