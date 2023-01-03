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

/// @file   TOFMatchableWriterSpec.cxx

#include "TOFWorkflowIO/TOFMatchableWriterSpec.h"
#include "Framework/ConfigParamRegistry.h"
#include "Framework/ControlService.h"
#include "DPLUtils/MakeRootTreeWriterSpec.h"
#include "Headers/DataHeader.h"
#include "TTree.h"
#include "TBranch.h"
#include "TFile.h"
#include "ReconstructionDataFormats/MatchInfoTOFReco.h"

using namespace o2::framework;

namespace o2
{
namespace tof
{

template <typename T>
using BranchDefinition = MakeRootTreeWriterSpec::BranchDefinition<T>;
using MatchableType = std::vector<o2::dataformats::MatchInfoTOFReco>;

DataProcessorSpec getTOFMatchableWriterSpec(const char* outdef)
{
  // A spectator for logging
  auto logger = [](MatchableType const& indata) {
    LOG(debug) << "RECEIVED MATCHABLE SIZE " << indata.size();
  };
  o2::header::DataDescription ddMatchable{"MATCHABLES"};
  return MakeRootTreeWriterSpec("TOFMatchableWriter",
                                outdef,
                                "matchableTOF",
                                BranchDefinition<MatchableType>{InputSpec{"input", o2::header::gDataOriginTOF, ddMatchable, 0},
                                                                 "TOFMatchableInfo",
                                                                 "matchableinfo-branch-name",
                                                                 1,
                                                                 logger})();
}

} // namespace tof
} // namespace o2
