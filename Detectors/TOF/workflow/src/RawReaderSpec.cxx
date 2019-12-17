// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// @file   RawReaderSpec.cxx

#include <vector>

#include "TTree.h"

#include "Framework/ControlService.h"
#include "Framework/ConfigParamRegistry.h"
#include "TOFWorkflow/RawReaderSpec.h"
#include "DataFormatsParameters/GRPObject.h"
#include <fstream> 

using namespace o2::framework;
using namespace o2::tof;

namespace o2
{
namespace tof
{

void RawReader::init(InitContext& ic)
{
  LOG(INFO) << "Init Raw reader!";
  mFilename = ic.options().get<std::string>("tof-raw-infile");
  std::ifstream f(mFilename.c_str(), std::ifstream::in);

  if(f.good()){
    mState = 1;
    LOG(INFO) << "TOF: TOF Raw file " << mFilename.c_str() << " found";
    f.close();
  }
  else{
    mState = 2;
    LOG(ERROR) << "TOF: TOF Raw file " << mFilename.c_str() << " not found";
  }
}

void RawReader::run(ProcessingContext& pc)
{
  if (mState != 1) {
    return;
  }

  mState = 2;

  o2::tof::compressed::Decoder decoder;

  decoder.open(mFilename.c_str());
  decoder.setVerbose(0);

  mDigits.clear();

  // decode raw to digit here
  std::vector<o2::tof::Digit> digitsTemp;
  int n_tof_window=0;
  int digit_size = 0;
  int end_of_file = 0;
  while(! end_of_file){
    digitsTemp.clear();
    
    end_of_file=decoder.decode(&digitsTemp);
    digit_size += digitsTemp.size();
    if(! end_of_file){
      mDigits.push_back(digitsTemp);
      n_tof_window++;
    }
  }

  LOG(INFO) << "TOF: N tof window decoded = " << n_tof_window << " with " << digit_size<< " digits";

  // add digits in the output snapshot
  pc.outputs().snapshot(Output{"TOF", "DIGITS", 0, Lifetime::Timeframe}, mDigits);

  static o2::parameters::GRPObject::ROMode roMode = o2::parameters::GRPObject::CONTINUOUS;

  LOG(INFO) << "TOF: Sending ROMode= " << roMode << " to GRPUpdater";
  pc.outputs().snapshot(Output{"TOF", "ROMode", 0, Lifetime::Timeframe}, roMode);

  //pc.services().get<ControlService>().readyToQuit(QuitRequest::Me);
  pc.services().get<ControlService>().endOfStream();
}

DataProcessorSpec getRawReaderSpec()
{
  std::vector<OutputSpec> outputs;
  outputs.emplace_back("TOF", "DIGITS", 0, Lifetime::Timeframe);
  outputs.emplace_back("TOF", "ROMode", 0, Lifetime::Timeframe);

  return DataProcessorSpec{
    "tof-raw-reader",
    Inputs{},
    outputs,
    AlgorithmSpec{adaptFromTask<RawReader>()},
    Options{{"tof-raw-infile", VariantType::String, "rawtof.bin", {"Name of the input file"}}}};
}

} // namespace tof
} // namespace o2
