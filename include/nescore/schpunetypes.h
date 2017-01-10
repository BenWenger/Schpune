
#ifndef SCHPUNE_NESCORE_SCHPUNETYPES_H_INCLUDED
#define SCHPUNE_NESCORE_SCHPUNETYPES_H_INCLUDED

#include <cstdint>
#include <limits>

namespace schcore
{
    ///////////////////////////////////////////////
    // Fixed types
    typedef std::int8_t         s8;
    typedef std::int16_t        s16;
    typedef std::int32_t        s32;
    typedef std::int64_t        s64;
    typedef std::uint8_t        u8;
    typedef std::uint16_t       u16;
    typedef std::uint32_t       u32;
    typedef std::uint64_t       u64;

    ////////////////////////////////////////////////
    // Other common types
    typedef std::uint_fast16_t  irqsource_t;
    typedef std::int_fast32_t   timestamp_t;

    ////////////////////////////////////////////////
    // Special fixed timestamp values
    namespace Time
    {
        static const timestamp_t    Never = std::numeric_limits<timestamp_t>::max() - 10000;
        static const timestamp_t    Now = std::numeric_limits<timestamp_t>::min() + 10000;
    }
}

#endif