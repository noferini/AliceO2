// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// @file   tof-reco-workflow.cxx
/// @author Francesco Noferini
/// @since  2019-05-22
/// @brief  Basic DPL workflow for TOF reconstruction starting from digits

#include "Framework/WorkflowSpec.h"
#include "Framework/ConfigParamSpec.h"
#include "TOFWorkflow/RecoWorkflowSpec.h"
#include "Algorithm/RangeTokenizer.h"

#include <string>
#include <stdexcept>
#include <unordered_map>

// add workflow options, note that customization needs to be declared before
// including Framework/runDataProcessing
void customize(std::vector<o2::framework::ConfigParamSpec>& workflowOptions)
{
  std::vector<o2::framework::ConfigParamSpec> options{
    { "input-type", o2::framework::VariantType::String, "digits", { "digits, raw, clusters" } },
    { "output-type", o2::framework::VariantType::String, "tracks", { "clusters, matchinginfo" } },
    { "disable-mc", o2::framework::VariantType::Bool, false, { "disable sending of MC information" } },
    { "tof-sectors", o2::framework::VariantType::String, "0-17", { "TOF sector range, e.g. 5-7,8,9" } },
    { "tof-lanes", o2::framework::VariantType::Int, 1, { "number of parallel lanes up to the matcher" } },
  };
  std::swap(workflowOptions, options);
}

#include "Framework/runDataProcessing.h" // the main driver

using namespace o2::framework;

/// The workflow executable for the stand alone TOF reconstruction workflow
/// The basic workflow for TOF reconstruction is defined in RecoWorkflow.cxx
/// and contains the following default processors
/// - digit reader
/// - clusterer
/// - cluster raw decoder
/// - track-TOF matcher
///
/// The default workflow can be customized by specifying input and output types
/// e.g. digits, raw, clusters.
///
/// MC info is processed by default, disabled by using command line option `--disable-mc`
///
/// This function hooks up the the workflow specifications into the DPL driver.
WorkflowSpec defineDataProcessing(ConfigContext const& cfgc)
{
  auto tofSectors = o2::RangeTokenizer::tokenize<int>(cfgc.options().get<std::string>("tof-sectors"));
  // the lane configuration defines the subspecification ids to be distributed among the lanes.
  std::vector<int> laneConfiguration;
  auto nLanes = cfgc.options().get<int>("tof-lanes");
  auto inputType = cfgc.options().get<std::string>("input-type");
  if (inputType == "digitizer") {
    // the digitizer is using a different lane setup so we have to force this for the moment
    laneConfiguration.resize(nLanes);
    std::iota(laneConfiguration.begin(), laneConfiguration.end(), 0);
  } else {
    laneConfiguration = tofSectors;
  }

  
  // return o2::TOF::RecoWorkflow::getWorkflow(tofSectors,                                    // sector configuration
  //                                           laneConfiguration,                             // lane configuration
  //                                           not cfgc.options().get<bool>("disable-mc"),    //
  //                                           nLanes,                                        //
  //                                           inputType,                                     //
  //                                           cfgc.options().get<std::string>("output-type") //
  // );
}
