
#include "tempfront.h"
#include "tempnsf.h"

namespace schcore
{
    TempFront::TempFront()  : nsf( new TempNsf() )      {}
    TempFront::~TempFront()                             {}

    bool TempFront::load(const char* filename)          { return nsf->load(filename);       }
    int TempFront::getTrackCount()                      { return nsf->getTrackCount();      }
    int TempFront::getTrack()                           { return nsf->getTrack();           }
    void TempFront::setTrack(int track)                 { nsf->setTrack(track);             }
    
    int TempFront::doFrame()                            { return nsf->doFrame();            }
    int TempFront::availableSamples()                   { return nsf->availableSamples();   }
    int TempFront::getSamples(s16* bufa, int siza, s16* bufb, int sizb)
    {
        return nsf->getSamples(bufa,siza,bufb,sizb);
    }

    void TempFront::setTracer(std::ostream* strm)       { nsf->setTracer(strm);             }
}
