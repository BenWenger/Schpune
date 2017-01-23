
#ifndef SCHPUNE_NESCORE_EXAUDIO_H_INCLUDED
#define SCHPUNE_NESCORE_EXAUDIO_H_INCLUDED

namespace schcore
{
    class ResetInfo;

    /////////////////////
    //  There isn't very much to this class
    //  This is abstracted mostly for NSFs

    class ExAudio
    {
    public:
        virtual ~ExAudio() = default;
        virtual void reset(const ResetInfo& info) = 0;
    };
}


#endif
