
#ifndef SCHPUNE_NESCORE_TEMPFRONT_H_INCLUDED
#define SCHPUNE_NESCORE_TEMPFRONT_H_INCLUDED

// this file is temporary -- it's just so I can quickly get NSF files playing!
//
//  Do this in a smarter way using an abstracted cartridge interface
//  And using a System main control thing

#include <memory>
#include <iostream>
#include "schpunetypes.h"
#include "regioninfo.h"

namespace schcore
{
    class TempNsf;

    class TempFront
    {
    public:
                        TempFront();
                        ~TempFront();
        bool            load(const char* filename, bool stereo);
        bool            loadTest(const char* filename);
        int             getTrackCount();
        int             getTrack();
        void            setTrack(int track);

        int             doFrame();                                                  // returns number of bytes of available audio
        int             availableAudio();                                           //   ditto
        int             getSamples(s16* bufa, int siza, s16* bufb, int sizb);

        void            setTracer(std::ostream* strm);

    private:
        std::unique_ptr<TempNsf>        nsf;
    };
}

#endif