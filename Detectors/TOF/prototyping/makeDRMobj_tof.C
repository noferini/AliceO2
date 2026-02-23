#if !defined(__CLING__) || defined(__ROOTCLING__)
#include "TFile.h"
#include "TH2F.h"
#include "TOFBase/CalibTOFapi.h"
#endif

void makeDRMobj_tof(const char* inputfile = "TObject_1764607157510.root", bool dummy = false)
{
  TFile* f = new TFile(inputfile);
  TH2F* h = (TH2F*)f->Get("ccdb_object");

  o2::tof::Diagnostic drmDia;

  if (!dummy) {
    drmDia = o2::tof::CalibTOFapi::doDRMerrCalibFromQCHisto(h, "ccdb.root");
    return;
  }

  // continue if dummy
  for (int j = 1; j <= 72; j++) {
    drmDia.fill(o2::tof::Diagnostic::getDRMKey(j - 1));
  }

  TFile* fo = new TFile("ccdb.root", "RECREATE");
  fo->WriteObjectAny(&drmDia, drmDia.Class_Name(), "ccdb_object");
  fo->Close();
}
