// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// @file   TODRawWriterSpec.cxx

#include "TOFWorkflow/TOFRawWriterSpec.h"
#include "Framework/ControlService.h"
#include "Framework/ConfigParamRegistry.h"

using namespace o2::framework;

namespace o2
{
namespace tof
{
void RawWriter::init(InitContext& ic)
{
  // get the option from the init context
  mOutFileName = ic.options().get<std::string>("tof-raw-outfile");
  LOG(INFO) << "Raw output file: " << mOutFileName.c_str();
}

void RawWriter::run(ProcessingContext& pc)
{
  if (mFinished) {
    return;
  }

  auto digits = pc.inputs().get<std::vector<std::vector<o2::tof::Digit>>*>("tofdigits");
  int nwindow = digits->size();
  LOG(INFO) << "Encoding " << nwindow << " TOF readout windows";

  int cache = 100000000;
  int verbosity = 0;

  o2::tof::compressed::Encoder encoder;
  encoder.setVerbose(verbosity);

  encoder.open(mOutFileName.c_str());
  encoder.alloc(cache);

  for(int i=0;i < nwindow;i++){
    if(verbosity) printf("----------\nwindow = %d\n----------\n",i);
    encoder.encode(digits->at(i),i);
  }

  encoder.flush();
  encoder.close();

  mFinished = true;
  pc.services().get<ControlService>().readyToQuit(QuitRequest::Me);
}

DataProcessorSpec getTOFRawWriterSpec()
{
  std::vector<InputSpec> inputs;
  inputs.emplace_back("tofdigits", "TOF", "DIGITS", 0, Lifetime::Timeframe);

  return DataProcessorSpec{
    "TOFRawWriter",
    inputs,
    {}, // no output
    AlgorithmSpec{adaptFromTask<RawWriter>()},
    Options{
      {"tof-raw-outfile", VariantType::String, "rawtof.bin", {"Name of the input file"}},
    }};
}
} // namespace tof
} // namespace o2
