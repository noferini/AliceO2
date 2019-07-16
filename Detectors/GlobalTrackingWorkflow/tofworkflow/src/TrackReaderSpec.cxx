// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// @file   TrackReaderSpec.cxx

#include <vector>

#include "TTree.h"

#include "Framework/ControlService.h"
#include "TOFWorkflow/TrackReaderSpec.h"
#include "DataFormatsParameters/GRPObject.h"
#include "Framework/SerializationMethods.h"

using namespace o2::framework;
using namespace o2::tof;

namespace o2
{
namespace tof
{
void TrackReader::init(InitContext& ic)
{
  LOG(INFO) << "Init ITS-TPC Track reader!";
  auto filename = ic.options().get<std::string>("itstpc-track-infile");
  mFile = std::make_unique<TFile>(filename.c_str(), "OLD");
  if (!mFile->IsOpen()) {
    LOG(ERROR) << "Cannot open the " << filename.c_str() << " file !";
    mState = 0;
    return;
  }
  mState = 1;
}

void TrackReader::run(ProcessingContext& pc)
{
  if (mState != 1) {
    return;
  }

  mFile->ls();

  std::unique_ptr<TTree> treeTrack((TTree*)mFile->Get("matchTPCITS"));

  if (treeTrack) {
    treeTrack->SetBranchAddress("TPCITS", &mPtracks);

    if (mUseMC) {
      treeTrack->SetBranchAddress("MatchITSMCTruth", &mPITSLabels);
      treeTrack->SetBranchAddress("MatchTPCMCTruth", &mPTPCLabels);
    }

    treeTrack->GetEntry(0);

    printf("N ITS-TPC tracks = %d\n",mTracks.size());

    // add digits loaded in the output snapshot
    pc.outputs().snapshot(Output{ "GLO", "TPCITS", 0, Lifetime::Timeframe }, mTracks);
    if (mUseMC) {
      pc.outputs().snapshot(Output{ "GLO", "TPCITS_ITSMC", 0, Lifetime::Timeframe }, mITSLabels);
      pc.outputs().snapshot(Output{ "GLO", "TPCITS_TPCMC", 0, Lifetime::Timeframe }, mTPCLabels);
    }
  } else {
    LOG(ERROR) << "Cannot read the ITS-TPC tracks !";
    return;
  }

  mState = 2;
//  pc.services().get<ControlService>().readyToQuit(false);
}

DataProcessorSpec getTrackReaderSpec(bool useMC)
{
  std::vector<OutputSpec> outputs;
  outputs.emplace_back("GLO", "TPCITS", 0, Lifetime::Timeframe);
  if (useMC) {
    outputs.emplace_back("GLO", "TPCITS_ITSMC", 0, Lifetime::Timeframe);
    outputs.emplace_back("GLO", "TPCITS_TPCMC", 0, Lifetime::Timeframe);
  }

  return DataProcessorSpec{
    "itstpc-track-reader",
     Inputs{},
     outputs,
     AlgorithmSpec{ adaptFromTask<TrackReader>(useMC) },
     Options{
       { "itstpc-track-infile", VariantType::String, "o2match_itstpc.root", { "Name of the input file" } } }
   };
}

} // namespace tof
} // namespace o2
