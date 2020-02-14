// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

#ifndef TOF_CALIBWORKFLOW_H_
#define TOF_CALIBWORKFLOW_H_

#include "DetectorsCalibration/TimeSlotCalibration.h"
#include "DetectorsCalibration/TimeSlot.h"
#include "DataFormatsTOF/CalibInfoTOF.h"

#include <array>

namespace o2
{
namespace tof
{

struct LHCClockDataHisto {
  float range = 24400;
  int nbins = 1000;
  float v2Bin = nbins / (2 * range);
  int entries = 0;
  std::vector<float> histo{0};

  LHCClockDataHisto();

  LHCClockDataHisto(int nb, float r) : nbins(nb), range(r), v2Bin(0)
  {
    if (r <= 0. || nb < 1) {
      throw std::runtime_error("Wrong initialization of the histogram");
    }
    v2Bin = nbins / (2 * range);
    histo.resize(nbins, 0.);
  }

  size_t getEntries() const { return entries; }
  void print() const;
  void fill(const gsl::span<o2::dataformats::CalibInfoTOF> data);
  void merge(const LHCClockDataHisto* prev);

  ClassDefNV(LHCClockDataHisto, 1);
};

class LHCClockCalibrator : public o2::calibration::TimeSlotCalibration<o2::dataformats::CalibInfoTOF, o2::tof::LHCClockDataHisto>
{
  using Slot = o2::calibration::TimeSlot<o2::tof::LHCClockDataHisto>;

 public:
  LHCClockCalibrator(int minEnt, int nb, float r) : mMinEntries(minEnt), mNBins(nb), mRange(r) {}

  bool hasEnoughData(const Slot& slot) const final { return slot.getContainer()->entries >= mMinEntries; }
  void finalizeSlot(Slot& slot) final;
  Slot& emplaceNewSlot(bool front, uint32_t tstart, uint32_t tend) final;

 private:
  int mMinEntries = 0;
  int mNBins = 0;
  float mRange = 0.;

  ClassDefOverride(LHCClockCalibrator, 1);
};

} // end namespace tof
} // end namespace o2

#endif /* TOF_CALIBWORKFLOW_H_ */
