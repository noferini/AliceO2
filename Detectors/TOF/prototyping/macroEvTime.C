#include "TOFReconstruction/EventTimeMaker.h"

using namespace o2::tof;
void macroEvTime()
{
  Printf("Ciao");
  std::vector<eventTimeTrack> tracks;
  for (int i = 0; i < 10; i++) {
    tracks.clear();
    generateEvTimeTracks(tracks, 100);
    auto evtime = evTimeMaker(tracks);
    Printf("Ev time %f +-%f", evtime.eventTime, evtime.eventTimeError);
  }
}