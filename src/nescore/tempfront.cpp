
#include "tempfront.h"
#include "tempnsf.h"

namespace schcore
{
    TempFront::TempFront()  : nsf( new TempNsf() )      {}
    TempFront::~TempFront()                             {}
    
    bool TempFront::load(const char* filename, bool s)  { return nsf->load(filename, s);    }
    bool TempFront::loadTest(const char* filename)      { return nsf->loadTest(filename);   }
    int TempFront::getTrackCount()                      { return nsf->getTrackCount();      }
    int TempFront::getTrack()                           { return nsf->getTrack();           }
    void TempFront::setTrack(int track)                 { nsf->setTrack(track);             }
    
    int TempFront::doFrame()                            { return nsf->doFrame();            }
    int TempFront::availableAudio()                     { return nsf->availableAudio();     }
    int TempFront::getSamples(s16* bufa, int siza, s16* bufb, int sizb)
    {
        return nsf->getSamples(bufa,siza,bufb,sizb);
    }

    void TempFront::setTracer(std::ostream* strm)       { nsf->setTracer(strm);             }
}
