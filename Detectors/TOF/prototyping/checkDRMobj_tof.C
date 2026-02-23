#if !defined(__CLING__) || defined(__ROOTCLING__)
#include "TFile.h"
#include "TH2F.h"
#include "TOFBase/CalibTOFapi.h"
#endif

void checkDRMobj_tof(const char* fname = "ccdb.root")
{
  TFile* f = new TFile(fname);

  TH2F* hErrors = new TH2F("hDRMerrors", ";error code; frequency", 30, 0, 30, 72, 0, 72);

  o2::tof::Diagnostic* drmDia = (o2::tof::Diagnostic*)f->Get("ccdb_object");

  for (int j = 1; j <= 72; j++) {
    uint32_t patternRDH = o2::tof::Diagnostic::getDRMKey(j - 1);
    for (int i = 1; i <= hErrors->GetXaxis()->GetNbins(); i++) {
      uint32_t pattern = o2::tof::Diagnostic::getDRMerrorKey(j - 1, i - 1);
      if (drmDia->getFrequency(patternRDH)) {
        hErrors->SetBinContent(i, j, drmDia->getFrequency(pattern) * 1. / drmDia->getFrequency(patternRDH));
      }
    }
  }

  TCanvas* c = new TCanvas();
  c->cd(1);
  hErrors->Draw("colz");

  drmDia->print(true);
}
