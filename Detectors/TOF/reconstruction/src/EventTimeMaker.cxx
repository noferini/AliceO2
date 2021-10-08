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

eventTimeContainer computeEvTime(const std::vector<eventTimeTrack>& tracks)
{
  const int maxNumberOfSets = 100;

  int ntracks = tracks.size();

  if (ntracks < 2) { // at least 2 tracks required
    return eventTimeContainer{0.f, 0.f};
  }

  int nmaxtracksinset = ntracks > 22 ? 6 : 10; // max number of tracks in a set for event time computation
  int ntracksinset = std::min(ntracks, nmaxtracksinset);

  Int_t nset = ((ntracks - 1) / ntracksinset) + 1;
  int ntrackUsed = ntracks;

  if (nset > maxNumberOfSets) {
    nset = maxNumberOfSets;
    ntrackUsed = nmaxtracksinset * nset;
  }

  // list of tracks in set
  std::vector<int> trackInSet[maxNumberOfSets];

  for (int i = 0; i < ntrackUsed; i++) {
    int iset = i % nset;

    trackInSet[iset].push_back(i);
  }

  // compute event time for each set
  for (int i = 0; i < nset; i++) {
  }

  // do average amonf all sets

  return eventTimeContainer{0.f, 0.f};
}

void generateEvTimeTracks(std::vector<eventTimeTrackTest>& tracks, int ntracks, float evTime)
{
  eventTimeTrackTest track;
  constexpr float masses[3] = {0.13957000, 0.49367700, 0.93827200};
  constexpr float kCSPEED = TMath::C() * 1.0e2f * 1.0e-12f; /// Speed of light in TOF units (cm/ps)
  float energy = 0.f;
  float betas[3] = {0.f};

  float pMismatch = ntracks * 0.00005;

  for (int i = 0; i < ntracks; i++) {
    track.mTOFChi2 = 1.f;
    track.mP = gRandom->Exp(1);
    track.mPt = track.mP;
    track.mLength = 400.;
    track.mHypo = gRandom->Exp(1);
    if (track.mHypo > 2) {
      track.mHypo = 2;
    }
    for (int j = 0; j < 3; j++) {
      energy = sqrt(masses[j] * masses[j] + track.mP * track.mP);
      betas[j] = track.mP / energy;
      track.expTimes[j] = track.mLength / (betas[j] * kCSPEED);
      track.expSigma[j] = 100.f;
      if (j == track.mHypo) {
        track.mSignal = track.expTimes[j] + gRandom->Gaus(0.f, track.expSigma[j]);

        if (gRandom->Rndm() < pMismatch) { // assign time from a different particle
          float p = gRandom->Exp(1);
          float l = 400;
          int hypo = gRandom->Exp(1);
          if (hypo > 2) {
            hypo = 2;
          }
          energy = sqrt(masses[hypo] * masses[hypo] + p * p);
          float beta = p / energy;
          track.mSignal = l / (beta * kCSPEED) + gRandom->Gaus(0.f, track.expSigma[j]);
        }
      }
    }
    tracks.push_back(track);
  }
}

} // namespace tof
} // namespace o2
