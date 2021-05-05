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

#include "DetectorsCalibration/TimeSlot.h"
#include <deque>
#include <gsl/gsl>
#include <limits>

namespace o2
{
namespace calibration
{

template <typename Input, typename Container>
class TimeSlotCalibration
{
  using Slot = TimeSlot<Container>;

 public:

  //  static constexpr int64_t INFINITE_TF = 0xfffffffffffffff;
  
  TimeSlotCalibration() = default;
  virtual ~TimeSlotCalibration() = default;
  uint64_t getMaxSlotsDelay() const { return mMaxSlotsDelay; }
  void setMaxSlotsDelay(uint64_t v) { mMaxSlotsDelay = v; }
  //void setMaxSlotsDelay(uint32_t v) { (mSlotLength == 1 && mMaxSlotsDelay == 0) ? mMaxSlotsDelay = 0 : mMaxSlotsDelay = v < 1 ? 1 : v; }
  //void setMaxSlotsDelay(uint32_t v) { mSlotLength == 1 ? mMaxSlotsDelay = 0 : mMaxSlotsDelay = v < 1 ? 1 : v; }

  uint64_t getSlotLength() const { return mSlotLength; }
  void setSlotLength(uint64_t v) { mSlotLength = v < 1 ? 1 : v; }

  uint64_t getUpdateInterval() const { return mUpdateInterval; }
  void setUpdateInterval(uint64_t v) { mUpdateInterval = v; }

  TFType getFirstTF() const { return mFirstTF; }
  void setFirstTF(TFType v) { mFirstTF = v; }

  void setUpdateAtTheEndOfRunOnly() { mUpdateAtTheEndOfRunOnly = kTRUE; }

  int getNSlots() const { return mSlots.size(); }
  Slot& getSlotForTF(TFType tf);
  Slot& getSlot(int i) { return (Slot&)mSlots.at(i); }
  const Slot& getSlot(int i) const { return (Slot&)mSlots.at(i); }
  const Slot& getLastSlot() const { return (Slot&)mSlots.back(); }
  const Slot& getFirstSlot() const { return (Slot&)mSlots.front(); }

  virtual bool process(TFType tf, const gsl::span<const Input> data);
  virtual void checkSlotsToFinalize(TFType tf, int maxDelay = 0);
  virtual void finalizeOldestSlot();

  // Methods to be implemented by the derived user class

  // implement and call this method te reset the output slots once they are not needed
  virtual void initOutput() = 0;
  // process the time slot container and add results to the output
  virtual void finalizeSlot(Slot& slot) = 0;
  // create new time slot in the beginning or the end of the slots pool
  virtual Slot& emplaceNewSlot(bool front, TFType tstart, TFType tend) = 0;
  // check if the slot has enough data to be finalized
  virtual bool hasEnoughData(const Slot& slot) const = 0;

  virtual void print() const;

 protected:
  auto& getSlots() { return mSlots; }

 private:
  TFType tf2SlotMin(TFType tf) const;

  std::deque<Slot> mSlots;

  TFType mLastClosedTF = 0;
  TFType mFirstTF = 0;
  TFType mMaxSeenTF = 0;   // largest TF processed 
  uint64_t mSlotLength = 1;
  uint64_t mMaxSlotsDelay = 3;
  bool mUpdateAtTheEndOfRunOnly = false;
  uint64_t mUpdateInterval = 1;  // will be used if the TF length is INFINITE_TF_int64 to decide
                                 // when to check if to call the finalize; otherwise it is called
                                 // at every new TF; note that this is an approximation,
                                 // since TFs come in async order
  
  ClassDef(TimeSlotCalibration, 1);
};

//_________________________________________________
template <typename Input, typename Container>
bool TimeSlotCalibration<Input, Container>::process(TFType tf, const gsl::span<const Input> data)
{

  // process current TF

  int maxDelay = mMaxSlotsDelay * mSlotLength;
  if (!mUpdateAtTheEndOfRunOnly) { // if you update at the end of run only, then you accept everything
    //  if (tf<mLastClosedTF || (!mSlots.empty() && getSlot(0).getTFStart() > tf + maxDelay)) { // ignore TF
    /*
    if ((maxDelay != 0 && (tf < mLastClosedTF || (!mSlots.empty() && getLastSlot().getTFStart() > tf + maxDelay))) ||
	(mSlots.size() == 1 && mSlots[0].getTFEnd() == (std::numeric_limits<long>::max() - 1) && tf < mLastClosedTF)) { // ignore TF; if you have only 1 timeslot
                                                                                            // which is INFINITE_TF wide, then maxDelay
                                                                                            // does not matter: you won't accept TFs from the past
											    */
    if (tf < mLastClosedTF || (!mSlots.empty() && getLastSlot().getTFStart() > tf + maxDelay)) { // ignore TF; note that if you have only 1 timeslot
                                                                                                  // which is INFINITE_TF wide, then maxDelay
                                                                                                  // does not matter: you won't accept TFs from the past,
	                                                                                          // so the first condition will be used
      LOG(INFO) << "Ignoring TF " << tf;
      LOG(INFO) << "tf = " << tf << ", mLastClosedTF = " << mLastClosedTF;
      if (!mSlots.empty()) {
	LOG(INFO) << "mSlots not empty: getLastSlot().getTFStart() = " << getLastSlot().getTFStart() << ", maxDelay = " << maxDelay;
      }
      else {
	LOG(INFO) << "mSlots empty";
      }
	  
      return false;
    }
  }

  auto& slotTF = getSlotForTF(tf);
  slotTF.getContainer()->fill(data);
  if (tf > mMaxSeenTF) mMaxSeenTF = tf;  // keep track of the most recent TF processed
  if (!mUpdateAtTheEndOfRunOnly) {  // if you update at the end of run only, you don't check at every TF which slots can be closed
    // check if some slots are done
    checkSlotsToFinalize(tf, maxDelay);
  }
  
  return true;
}

//_________________________________________________
template <typename Input, typename Container>
void TimeSlotCalibration<Input, Container>::checkSlotsToFinalize(TFType tf, int maxDelay)
{
  // Check which slots can be finalized, provided the newly arrived TF is tf

  constexpr uint64_t INFINITE_TF = 0xffffffffffffffff;
  constexpr int64_t INFINITE_TF_int64 = std::numeric_limits<long>::max();
 
  // if we have one slot only which is INFINITE_TF_int64 long, and we are not at the end of run (tf != INFINITE_TF),
  // we need to check if we got enough statistics, and if so, redefine the slot
  LOG(INFO) << "mMaxSeenTF = " << mMaxSeenTF << ", mSlots[0].getTFStart() = " << mSlots[0].getTFStart();
  if (mSlots.size() == 1 && mSlots[0].getTFEnd() == INFINITE_TF_int64 - 1 && (mMaxSeenTF >= mUpdateInterval + mSlots[0].getTFStart() || tf == INFINITE_TF)) {
    if (tf == INFINITE_TF) {
      LOG << "End of run reached, trying to calibrate what we have, if we have enough statistics";
    }
    else {
      LOG(INFO) << "Calibrating as soon as we have enough statistics:";
      LOG(INFO) << "Update interval passed, checking slot for " << mSlots[0].getTFStart() << " <= TF <= " << mSlots[0].getTFEnd();
    }
    if (hasEnoughData(mSlots[0])) {
      mSlots[0].setTFStart(mLastClosedTF);
      mSlots[0].setTFEnd(mMaxSeenTF); // to be defined
      LOG(INFO) << "Finalizing slot for " << mSlots[0].getTFStart() << " <= TF <= " << mSlots[0].getTFEnd();
      finalizeSlot(mSlots[0]); // will be removed after finalization
      mLastClosedTF = mSlots[0].getTFEnd() + 1; // will not accept any TF below this
      mSlots.erase(mSlots.begin());
      // creating a new slot if we are not at the end of run
      if (tf != INFINITE_TF) {
	LOG(INFO) << "Creating new slot for " << mLastClosedTF << " <= TF <= " << std::numeric_limits<long>::max() - 1;
	emplaceNewSlot(true, mLastClosedTF, INFINITE_TF_int64 - 1);
      }
    }
  }
  else {
    // check if some slots are done
    for (auto slot = mSlots.begin(); slot != mSlots.end(); slot++) {
      //if (maxDelay == 0 || (slot->getTFEnd() + maxDelay) < tf) {
      if ((slot->getTFEnd() + maxDelay) < tf) {
	if (hasEnoughData(*slot)) {
	  LOG(INFO) << "Finalizing slot for " << slot->getTFStart() << " <= TF <= " << slot->getTFEnd();
	  finalizeSlot(*slot); // will be removed after finalization
	} else if ((slot + 1) != mSlots.end()) {
	  LOG(INFO) << "Merging underpopulated slot " << slot->getTFStart() << " <= TF <= " << slot->getTFEnd()
		    << " to slot " << (slot + 1)->getTFStart() << " <= TF <= " << (slot + 1)->getTFEnd();
	  (slot + 1)->mergeToPrevious(*slot);
	} else {
	  LOG(INFO) << "Discard underpopulated slot " << slot->getTFStart() << " <= TF <= " << slot->getTFEnd();
	  break; // slot has no enough stat. and there is no other slot to merge it to
	}
	mLastClosedTF = slot->getTFEnd() + 1; // will not accept any TF below this
	LOG(INFO) << "closing slot " << slot->getTFStart() << " <= TF <= " << slot->getTFEnd();
	mSlots.erase(slot);
      } else {
	break;
      }
      if (mSlots.empty()) { // since erasing the very last entry may invalidate mSlots.end()
	break;
      }
    }
  }
}

//_________________________________________________
template <typename Input, typename Container>
void TimeSlotCalibration<Input, Container>::finalizeOldestSlot()
{
  // Enforce finalization and removal of the oldest slot
  if (mSlots.empty()) {
    LOG(WARNING) << "There are no slots defined";
    return;
  }
  finalizeSlot(mSlots.front());
  mLastClosedTF = mSlots.front().getTFEnd() + 1; // do not accept any TF below this
  mSlots.erase(mSlots.begin());
}

//________________________________________
template <typename Input, typename Container>
inline TFType TimeSlotCalibration<Input, Container>::tf2SlotMin(TFType tf) const
{
  
  // returns the min TF of the slot to which "tf" belongs
  
  if (tf < mFirstTF) {
    throw std::runtime_error("invalide TF");
  }
  if (mUpdateAtTheEndOfRunOnly) {
    return mFirstTF;
  }
  return TFType((tf - mFirstTF) / mSlotLength) * mSlotLength + mFirstTF;
}

//_________________________________________________
template <typename Input, typename Container>
TimeSlot<Container>& TimeSlotCalibration<Input, Container>::getSlotForTF(TFType tf)
{

  LOG(INFO) << "Getting slot for TF " << tf;

  if (mUpdateAtTheEndOfRunOnly) {
    LOG(INFO) << "Updating only at the end of run";
    if (!mSlots.empty() && mSlots.back().getTFEnd() < tf) {
      LOG(INFO) << "Update end time of current slot: was = " << mSlots.back().getTFEnd() << ", will be = " << tf; 
      mSlots.back().setTFEnd(tf);
    } else if (mSlots.empty()) {
      LOG(INFO) << "Emplacing first slot";
      emplaceNewSlot(true, mFirstTF, tf);
    }
    return mSlots.back();
  }

  if (!mSlots.empty() && mSlots.front().getTFStart() > tf) { // we need to add a slot to the beginning
    LOG(INFO) << "Checking if we need a new slot";
    auto tfmn = tf2SlotMin(mSlots.front().getTFStart() - 1); // min TF of the slot corresponding to a TF smaller than the first seen
    auto tftgt = tf2SlotMin(tf);  // min TF of the slot to which the TF "tf" would belong
    LOG(INFO) << "tfmn = " << tfmn << ", tftgt = " << tftgt;
    while (tfmn >= tftgt) {
      LOG(INFO) << "Adding new slot for " << tfmn << " <= TF <= " << tfmn + mSlotLength - 1;
      emplaceNewSlot(true, tfmn, tfmn + mSlotLength - 1);
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
    LOG(INFO) << "Adding new slot for " << tfmn << " <= TF <= " << tfmn + mSlotLength - 1;
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
