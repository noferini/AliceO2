// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

#include "TOFCalibration/LHCClockCalibrator.h"
#include "Framework/Logger.h"

namespace o2
{
namespace tof
{

using Slot = o2::calibration::TimeSlot<o2::tof::LHCClockDataHisto>;

//_____________________________________________
LHCClockDataHisto::LHCClockDataHisto()
{
  LOG(INFO) << "Default c-tor, not to be used";
}

//_____________________________________________
void LHCClockDataHisto::fill(const gsl::span<o2::dataformats::CalibInfoTOF> data)
{
  // fill container
  for (int i = data.size(); i--;) {
    auto dt = data[i].getDeltaTimePi();
    dt += range;
    if (dt > 0 && dt < 2 * range) {
      histo[int(dt * v2Bin)]++;
      entries++;
    }
  }
}

//_____________________________________________
void LHCClockDataHisto::merge(const LHCClockDataHisto* prev)
{
  // merge data of 2 slots
  for (int i = histo.size(); i--;) {
    histo[i] += prev->histo[i];
  }
  entries += prev->entries;
}

//_____________________________________________
void LHCClockDataHisto::print() const
{
  LOG(INFO) << entries << " entries";
}

//===================================================================

//_____________________________________________
void LHCClockCalibrator::finalizeSlot(Slot& slot)
{
  LOG(INFO) << "Finalize";
  slot.print();
}

//_____________________________________________
Slot& LHCClockCalibrator::emplaceNewSlot(bool front, uint32_t tstart, uint32_t tend)
{
  auto& cont = getSlots();
  auto& slot = front ? cont.emplace_front(tstart, tend) : cont.emplace_back(tstart, tend);
  slot.setContainer(std::make_unique<LHCClockDataHisto>(mNBins, mRange));
}

} // end namespace tof
} // end namespace o2
