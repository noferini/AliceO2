// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.
#if !defined(__CLING__) || defined(__ROOTCLING__)

#include <TTree.h>

#include <TFile.h>
#include <vector>
#include <string>
#include "TOFReconstruction/Decoder.h"
#include "TOFBase/Digit.h"

#endif

// example of TOF raw data encoding from digits

void run_rawdecoding_tof(std::string outName = "tofdigitDecoded.root", // name of the output digit file
                         std::string inpName = "rawtof.bin",           // name of the input raw file
                         int verbosity = 0)                            // set verbosity
{
  TFile* f = new TFile(outName.c_str(), "RECREATE");
  TTree* t = new TTree("o2sim", "o2sim");

  o2::tof::compressed::Decoder decoder;
  decoder.open(inpName.c_str());
  decoder.setVerbose(verbosity);
  decoder.decode();

  std::vector<o2::tof::Digit> *alldigits = decoder.getDigitPerTimeFrame();
  std::vector<o2::tof::ReadoutWindowData> *row = decoder.getReadoutWindowData();


  printf("N tof window decoded= %d\n", row->size());

  t->Branch("TOFDigit", &alldigits);
  t->Branch("TOFReadoutWindow", &row);
  t->Fill();
  t->Write();
  f->Close();
}
