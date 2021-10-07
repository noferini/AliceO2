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

/// \file EventTimeMaker.cxx
/// \brief Implementation of the TOF event time maker

#include "TOFReconstruction/EventTimeMaker.h"

namespace o2
{

namespace tof
{

void generateEvTimeTracks(std::vector<eventTimeTrack>& tracks, int ntracks, float evTime)
{
  eventTimeTrack track;
  constexpr float masses[3] = {0.13957000, 0.49367700, 0.93827200};
  constexpr float kCSPEED = TMath::C() * 1.0e2f * 1.0e-12f; /// Speed of light in TOF units (cm/ps)
  float energy = 0.f;
  int hypo;
  float betas[3] = {0.f};
  for (int i = 0; i < ntracks; i++) {
    track.mTOFChi2 = 1.f;
    track.mP = gRandom->Exp(1);
    track.mPt = track.mP;
    hypo = gRandom->Exp(1);
    if (hypo > 2) {
      hypo = 2;
    }
    for (int j = 0; j < 3; j++) {
      energy = sqrt(masses[hypo] * masses[hypo] + track.mP * track.mP);
      betas[j] = track.mP / energy;
      track.expTimes[j] = track.mLength / (betas[j] * kCSPEED);
      track.expSigma[j] = 100.f;
      if (j == hypo) {
        track.mSignal = track.expTimes[j] + gRandom->Gaus(0.f, track.expSigma[j]);
      }
    }
    tracks.push_back(track);
  }
}

} // namespace tof
} // namespace o2