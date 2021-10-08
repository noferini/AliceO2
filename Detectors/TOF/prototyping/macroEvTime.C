#if !defined(__CLING__) || defined(__ROOTCLING__)
#include "TOFReconstruction/EventTimeMaker.h"
#endif

using namespace o2::tof;
void macroEvTime()
{
  std::vector<eventTimeTrackTest> tracks;
  for (int i = 0; i < 10; i++) {
    tracks.clear();
    generateEvTimeTracks(tracks, 100);
    auto evtime = evTimeMaker<eventTimeTrackTest, filterDummy>(tracks);
    Printf("Ev time %f +-%f", evtime.eventTime, evtime.eventTimeError);
  }
}
