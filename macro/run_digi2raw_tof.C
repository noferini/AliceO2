#if !defined(__CLING__) || defined(__ROOTCLING__)

#include <TTree.h>
#include <TFile.h>
#include <vector>
#include <string>

#endif

// example of TOF raw data encoding from digits

void run_digi2raw_tof(std::string outName = "rawtof.bin",     // name of the output binary file
                      std::string inpName = "tofdigits.root", // name of the input TOF digits
                      int verbosity = 0,                      // set verbosity
                      int cache = 100000000)                  // memory caching in Byte
{
  TFile* f = new TFile(inpName.c_str());
  TTree* t = (TTree*)f->Get("o2sim");

  std::vector<std::vector<o2::tof::Digit>> digits, *pDigits = &digits;

  t->SetBranchAddress("TOFDigit", &pDigits);
  t->GetEvent(0);

  int nwindow = digits.size();

  printf("Encoding %d tof window\n", nwindow);

  o2::tof::compressed::Encoder encoder;
  encoder.setVerbose(verbosity);

  encoder.open(outName.c_str());
  encoder.alloc(cache);

  for (int i = 0; i < nwindow; i++) {
    if (verbosity)
      printf("----------\nwindow = %d\n----------\n", i);
    encoder.encode(digits.at(i), i);
  }

  encoder.flush();
  encoder.close();
}
