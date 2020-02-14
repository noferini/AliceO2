// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

#ifndef DETECTOR_CALIB_TIMESLOTCALIB_H_
#define DETECTOR_CALIB_TIMESLOTCALIB_H_

/// @brief Processor for the multiple time slots calibration

#include "TimeSlot.h"
#include <deque>
#include <gsl/gsl>

namespace o2
{
namespace calibration
{

template <typename Input, typename Container>
class TimeSlotCalibration
{
  using Slot = TimeSlot<Container>;

 public:
  uint32_t getMaxSlotsDelay() const { return mMaxSlotsDelay; }
  void setMaxSlotsDelay(uint32_t v) { mMaxSlotsDelay = v < 1 ? 1 : v; }

  uint32_t getSlotLength() const { return mSlotLength; }
  void setSlotLength(uint32_t v) { mSlotLength = v < 1 ? 1 : v; }

  int getNSlots() const { return mSlots.size(); }
  Slot& getSlotForTF(uint32_t tf);
  Slot& getSlot(int i) { return (Slot&)mSlots.at(i); }
  const Slot& getSlot(int i) const { return (Slot&)mSlots.at(i); }
  const Slot& getLastSlot() const { return (Slot&)mSlots.back(); }
  const Slot& getFirstSlot() const { return (Slot&)mSlots.front(); }

  virtual bool process(uint32_t tf, const gsl::span<Input> data);

  virtual void finalizeSlot(Slot& slot) = 0;
  virtual Slot& emplaceNewSlot(bool front, uint32_t tstart, uint32_t tend) = 0;
  virtual bool hasEnoughData(const Slot& slot) const = 0;

  virtual void print() const;

 protected:
  auto& getSlots() { return mSlots; }

 private:
  uint32_t tf2SlotMin(uint32_t tf) const;

  std::deque<Slot> mSlots;

  uint32_t mLastClosedTF = 0;
  uint32_t mTFStart = 0;
  uint32_t mSlotLength = 1;
  uint32_t mMaxSlotsDelay = 3;

  ClassDef(TimeSlotCalibration, 1);
};

//_________________________________________________
template <typename Input, typename Container>
bool TimeSlotCalibration<Input, Container>::process(uint32_t tf, const gsl::span<Input> data)
{
  int maxDelay = mMaxSlotsDelay * mSlotLength;
  //  if (tf<mLastClosedTF || (!mSlots.empty() && getSlot(0).getTFStart() > tf + maxDelay)) { // ignore TF
  if (tf < mLastClosedTF || (!mSlots.empty() && getLastSlot().getTFStart() > tf + maxDelay)) { // ignore TF

    LOG(INFO) << "Ignoring TF" << tf;
    return false;
  }

  // check if some slots are done
  for (auto slot = mSlots.begin(); slot != mSlots.end(); slot++) {
    if ((slot->getTFEnd() + maxDelay) < tf) {
      if (hasEnoughData(*slot)) {
        finalizeSlot(*slot); // will be removed after finalization
      } else if ((slot + 1) != mSlots.end()) {
        (slot + 1)->mergeToPrevious(*slot);
      } else {
        break; // slot has no enough stat. and there is no other slot to merge it to
      }
      mLastClosedTF = slot->getTFEnd() + 1; // do not accept any TF below this
      mSlots.erase(slot);
    } else {
      break;
    }
    if (mSlots.empty()) { // since erasing the very last entry may invalidate mSlots.end()
      break;
    }
  }
  // process current TF
  auto& slotTF = getSlotForTF(tf);
  slotTF.getContainer()->fill(data);
  return true;
}

//________________________________________
template <typename Input, typename Container>
inline uint32_t TimeSlotCalibration<Input, Container>::tf2SlotMin(uint32_t tf) const
{
  if (tf < mTFStart) {
    throw std::runtime_error("invalide TF");
  }
  return int((tf - mTFStart) / mSlotLength) * mSlotLength + mTFStart;
}

//_________________________________________________
template <typename Input, typename Container>
TimeSlot<Container>& TimeSlotCalibration<Input, Container>::getSlotForTF(uint32_t tf)
{
  if (!mSlots.empty() && mSlots.front().getTFStart() > tf) {
    auto tfmn = tf2SlotMin(mSlots.front().getTFStart() - 1);
    while (tfmn >= tf) {
      emplaceNewSlot(true, tfmn, tfmn + mSlotLength - 1);
      printf("addFront %d\n", tfmn);
      if (!tfmn) {
        break;
      }
      tfmn = tf2SlotMin(mSlots.front().getTFStart() - 1);
    }
    return mSlots[0];
  }
  for (auto it = mSlots.begin(); it != mSlots.end(); it++) {
    auto rel = (*it).relateToTF(tf);
    if (rel == 0) {
      return (*it);
    }
  }
  // need to add in the end
  auto tfmn = mSlots.empty() ? tf2SlotMin(tf) : tf2SlotMin(mSlots.back().getTFEnd() + 1);
  do {
    emplaceNewSlot(false, tfmn, tfmn + mSlotLength - 1);
    tfmn = tf2SlotMin(mSlots.back().getTFEnd() + 1);
  } while (tf > mSlots.back().getTFEnd());

  return mSlots.back();
}

//_________________________________________________
template <typename Input, typename Container>
void TimeSlotCalibration<Input, Container>::print() const
{
  for (int i = 0; i < getNSlots(); i++) {
    LOG(INFO) << "Slot #" << i << " of " << getNSlots();
    getSlot(i).print();
  }
}

} // namespace calibration
} // namespace o2

#endif
