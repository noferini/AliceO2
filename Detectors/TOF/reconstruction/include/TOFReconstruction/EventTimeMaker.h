// Copyright 2019-2020 CERN and copyright holders of ALICE O2.
// See https://alice-o2.web.cern.ch/copyright for details of the copyright holders.
// All rights not expressly granted are reserved.
//
// This software is distributed under the terms of the GNU General Public
// License v3 (GPL Version 3), copied verbatim in the file "COPYING".
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file EventTimeMaker.h
/// \brief Definition of the TOF event time maker

#ifndef ALICEO2_TOF_EVENTTIMEMAKER_H
#define ALICEO2_TOF_EVENTTIMEMAKER_H

#include "TRandom.h"
#include "TMath.h"

namespace o2
{

namespace tof
{

struct eventTimeContainer {
  eventTimeContainer(const float& e, const float& err) : eventTime{e}, eventTimeError{err} {};
  float eventTime = 0.f;
  float eventTimeError = 0.f;
};

struct eventTimeTrack {
  float tofSignal() { return mSignal; };
  float tofChi2() { return mTOFChi2; };
  float pt() { return mPt; };
  float p() { return mP; };
  float length() { return mLength; };
  float tofExpTimePi() { return expTimes[0]; };
  float tofExpTimeKa() { return expTimes[1]; };
  float tofExpTimePr() { return expTimes[2]; };
  float tofExpSigmaPi() { return expSigma[0]; };
  float tofExpSigmaKa() { return expSigma[1]; };
  float tofExpSigmaPr() { return expSigma[2]; };
  float mSignal = 0.f;
  float mTOFChi2 = -1.f;
  float mPt = 0.f;
  float mP = 0.f;
  float mLength = 0.f;
  float expTimes[3] = {0.f, 0.f, 0.f};
  float expSigma[3] = {999.f, 999.f, 999.f};
};

void generateEvTimeTracks(std::vector<eventTimeTrack>& tracks, int ntracks, float evTime = 0.f);

template <typename trackContainer>
eventTimeContainer evTimeMaker(const trackContainer& tracks)
{
  // Qui facciamo un pool di tracce buone per calcolare il T0
  for (auto track : tracks) {
    track.tofSignal();
    track.length();
  }
  return eventTimeContainer{0.f, 0.f};
}

} // namespace tof
} // namespace o2

#endif /* ALICEO2_TOF_EVENTTIMEMAKER_H */