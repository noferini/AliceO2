// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.
#include <TTree.h>
#include <cassert>

#include "FairLogger.h"
#include "TOFBase/Geo.h"

#include <TFile.h>
#include "DataFormatsParameters/GRPObject.h"
#include "ReconstructionDataFormats/PID.h"

#include "GlobalTracking/CalibTOF.h"

#include "CommonConstants/LHCConstants.h"

#include "TMath.h"

using namespace o2::globaltracking;

ClassImp(CalibTOF);

//______________________________________________
CalibTOF::CalibTOF(){

  // constructor needed to instantiate the pointers of the class (histos + array)

  for(int ipad=0; ipad < NPADSPERSTEP; ipad++){
  }
  mHistoChTimeSlewingAll = new TH2F("hTOFchTimeSlewingAll", ";tot (ns);t - t_{exp} - t_{offset} (ps)", 5000, 0., 250., 1000, -24400., 24400.);
}
//______________________________________________
CalibTOF::~CalibTOF(){

  // destructor

  if (mHistoLHCphase) delete mHistoLHCphase;
  delete mHistoChTimeSlewingAll;
  
}  
//______________________________________________
void CalibTOF::attachInputTrees()
{
  ///< attaching the input tree

  if (!mTreeCollectedCalibInfoTOF) {
    LOG(FATAL) << "Input tree with collected TOF calib infos is not set";
  }

  if (!mTreeCollectedCalibInfoTOF->GetBranch(mCollectedCalibInfoTOFBranchName.data())) {
    LOG(FATAL) << "Did not find collected TOF calib info branch " << mCollectedCalibInfoTOFBranchName << " in the input tree";
  }
  /*
  LOG(INFO) << "Attached tracksTOF calib info " << mCollectedCalibInfoTOFBranchName << " branch with " << mTreeCollectedCalibInfoTOF->GetEntries()
            << " entries";
  */
}

//______________________________________________
void CalibTOF::init()
{
  ///< initizalizations

  if (mInitDone) {
    LOG(ERROR) << "Initialization was already done";
    return;
  }

  TH1::AddDirectory(0); // needed because we have the LHCPhase created here, while in the macro we might have the output file open
                        // (we don't want to bind the histogram to the file, or teh destructor will complain)

  attachInputTrees();

  std::fill_n(mCalibChannelOffset, o2::tof::Geo::NCHANNELS, 0);
  std::fill_n(mCalibChannelOffsetErr, o2::tof::Geo::NCHANNELS, -1);
  std::fill_n(mInitialCalibChannelOffset, o2::tof::Geo::NCHANNELS, 0);
  
  // load better knoldge of channel offset (from CCDB?)
  // to be done

  // create output branch with output -- for now this is empty
  if (mOutputTree) {
    mLHCphaseObj = new o2::dataformats::CalibLHCphaseTOF();
    mTimeSlewingObj = new o2::dataformats::CalibTimeSlewingParamTOF();
    mOutputTree->Branch("mLHCphaseObj", &mLHCphaseObj);
    mOutputTree->Branch("mTimeSlewingObj", &mTimeSlewingObj);
    mOutputTree->Branch("LHCphaseMeasurementInterval", &mNLHCphaseIntervals, "LHCphaseMeasurementInterval/I");
    mOutputTree->Branch("LHCphase", mLHCphase, "LHCphase[LHCphaseMeasurementInterval]/F");
    mOutputTree->Branch("LHCphaseErr", mLHCphaseErr, "LHCphaseErr[LHCphaseMeasurementInterval]/F");
    mOutputTree->Branch("LHCphaseStartInterval", mLHCphaseStartInterval, "LHCphaseStartInterval[LHCphaseMeasurementInterval]/I");
    mOutputTree->Branch("LHCphaseEndInterval", mLHCphaseEndInterval, "LHCphaseEndInterval[LHCphaseMeasurementInterval]/I");
    mOutputTree->Branch("nChannels", &mNChannels, "nChannels/I");
    mOutputTree->Branch("ChannelOffset", mCalibChannelOffset, "ChannelOffset[nChannels]/F");
    mOutputTree->Branch("ChannelOffsetErr", mCalibChannelOffsetErr, "ChannelOffsetErr[nChannels]/F");
    //    LOG(INFO) << "Matched tracks will be stored in " << mOutputBranchName << " branch of tree "
    //              << mOutputTree->GetName();
  } else {
    LOG(ERROR) << "Output tree is not attached, matched tracks will not be stored";
  }

  // booking the histogram of the LHCphase
  int nbinsLHCphase = TMath::Min(1000, int((mMaxTimestamp - mMinTimestamp)/300)+1);
  if (nbinsLHCphase < 1000) mMaxTimestamp = mMinTimestamp + mNLHCphaseIntervals*300; // we want that the last bin of the histogram is also large 300s; this we need to do only when we have less than 1000 bins, because in this case we will integrate over intervals that are larger than 300s anyway
  mHistoLHCphase = new TH2F("hLHCphase", ";clock offset (ps); timestamp (s)", 1000, -24400, 24400, nbinsLHCphase, mMinTimestamp, mMaxTimestamp);

  mInitDone = true;

  print();
}
//______________________________________________
void CalibTOF::run(int flag, int sector)
{
  ///< running the matching

  TTree *localTree = nullptr;
  Int_t   currTOFInfoTreeEntry = -1;

  std::vector<o2::dataformats::CalibInfoTOFshort>* localCalibInfoTOF = nullptr;
  TFile fOpenLocally(mTreeCollectedCalibInfoTOF->GetCurrentFile()->GetName());
  localTree = (TTree *) fOpenLocally.Get(mTreeCollectedCalibInfoTOF->GetName());

  if(! localTree){
    LOG(FATAL) << "tree " << mTreeCollectedCalibInfoTOF->GetName() << " not found in " << mTreeCollectedCalibInfoTOF->GetCurrentFile()->GetName();    
  }

  localTree->SetBranchAddress(mCollectedCalibInfoTOFBranchName.data(), &localCalibInfoTOF);
  
  if (!mInitDone) {
    LOG(FATAL) << "init() was not done yet";
  }

  TStopwatch timerTot;
  timerTot.Start();

  if (flag & kLHCphase) { // LHC phase --> we will use all the entries in the tree
    while(loadTOFCollectedCalibInfo(localTree, currTOFInfoTreeEntry)){ // fill here all histos you need 
      fillLHCphaseCalibInput(localCalibInfoTOF); // we will fill the input for the LHC phase calibration
    }
    doLHCPhaseCalib();
  }
  // channel offset + problematic (flag = 2), or time slewing (flag = 4)
  if ((flag & kChannelOffset) || (flag & kChannelTimeSlewing)) { // for the moment compute everything idependetly of the flag
    TH1F* histoChOffsetTemp[NPADSPERSTEP];
    std::vector<o2::dataformats::CalibInfoTOFshort>* calibTimePad[NPADSPERSTEP];
    for (int ipad = 0; ipad < NPADSPERSTEP; ipad++){
      histoChOffsetTemp[ipad] = new TH1F(Form("hLHCchOffsetTemp_Sec%02d_Pad%04d", sector, ipad), Form("Sector %02d (pad = %04d);channel offset (ps)", sector, ipad), 1000, -24400, 24400);
      if (flag & kChannelTimeSlewing) calibTimePad[ipad] = new std::vector<o2::dataformats::CalibInfoTOFshort>; // temporary array containing [time, tot] for every pad that we process; this will be the input for the 2D histo for timeSlewing calibration (to be filled after we get the channel offset)
      else calibTimePad[ipad] = nullptr;
    }

    TF1* funcChOffset = new TF1(Form("fTOFchOffset_%02d", sector), "gaus");

    TH2F* histoChTimeSlewingTemp = new TH2F(Form("hTOFchTimeSlewingTemp_%02d", sector), Form("Sector %02d;tot (ns);t - t_{exp} - t_{offset} (ps)", sector), 5000, 0., 250., 1000, -24400., 24400.);

    int startLoop = 0; // first pad that we will process in this process (we are processing a sector, unless sector = -1)
    int endLoop = o2::tof::Geo::NCHANNELS; // last pad that we will process in this process (we are processing a sector)
    if (sector > -1) {
      startLoop = sector * o2::tof::Geo::NPADSXSECTOR; // first pad that we will process in this process (we are processing a sector, unless sector = -1)
      endLoop = startLoop + o2::tof::Geo::NPADSXSECTOR; // last pad that we will process in this process (we are processing a sector, unless sector = -1)
    }
    for (int ich = startLoop; ich < endLoop; ich += NPADSPERSTEP){
      sector = ich/o2::tof::Geo::NPADSXSECTOR; // we change the value of sector which is needed when it is "-1" to put
                                               // in the output histograms a meaningful name; this is not needed in
                                               // case we run with sector != -1, but it will not hurt :) 
      resetChannelLevelHistos(histoChOffsetTemp, histoChTimeSlewingTemp, calibTimePad);
      printf("strip %i\n", ich/96);
      currTOFInfoTreeEntry = ich - 1;
      int ipad = 0;
      int entryNext = currTOFInfoTreeEntry + o2::tof::Geo::NCHANNELS;

      while (loadTOFCollectedCalibInfo(localTree, currTOFInfoTreeEntry)) { // fill here all histos you need 

	fillChannelCalibInput(localCalibInfoTOF, mInitialCalibChannelOffset[ich+ipad], ipad, histoChOffsetTemp[ipad], calibTimePad[ipad]); // we will fill the input for the channel-level calibration
	ipad++;

	if(ipad == NPADSPERSTEP){
	  ipad = 0;
	  currTOFInfoTreeEntry = entryNext;
	  entryNext += o2::tof::Geo::NCHANNELS;
	}
      }
      TFile * fout = nullptr;
      if (flag & kChannelTimeSlewing && mDebugMode) fout = new TFile(Form("timeslewingTOF%06i.root", ich/96),"RECREATE");
      
      for (ipad = 0; ipad < NPADSPERSTEP; ipad++){
	if (histoChOffsetTemp[ipad]->GetEntries() > 30){
	  int isProblematic = doChannelCalibration(ipad, histoChOffsetTemp[ipad], funcChOffset);
	  mCalibChannelOffset[ich+ipad] = funcChOffset->GetParameter(1) + mInitialCalibChannelOffset[ich+ipad];
	  if (!isProblematic)
	    mCalibChannelOffsetErr[ich+ipad] = abs(funcChOffset->GetParError(1));
	  else{ // problematic
	    mCalibChannelOffsetErr[ich+ipad] = -abs(funcChOffset->GetParError(1));
	    continue;
	  }
	  // now fill 2D histo for time-slewing using current channel offset
	  
	  if (flag & kChannelTimeSlewing) {
	    histoChTimeSlewingTemp->Reset();
	    fillChannelTimeSlewingCalib(mCalibChannelOffset[ich+ipad], ipad, histoChTimeSlewingTemp, calibTimePad[ipad]); // we will fill the input for the channel-time-slewing calibration
	    
	    histoChTimeSlewingTemp->SetName(Form("hTOFchTimeSlewing_Sec%02d_Pad%04d", sector, ipad));
	    histoChTimeSlewingTemp->SetTitle(Form("Sector %02d (pad = %04d)", sector, ipad));
	    TGraphErrors *gTimeVsTot = processSlewing(histoChTimeSlewingTemp, 1, funcChOffset);
	    
	    if (gTimeVsTot && gTimeVsTot->GetN()) {
	      for (int itot = 0; itot < gTimeVsTot->GetN(); itot++)
		mTimeSlewingObj->addTimeSlewingInfo(ich + ipad, gTimeVsTot->GetX()[itot], gTimeVsTot->GetY()[itot] + mCalibChannelOffset[ich + ipad]);
	    } else { // just add the channel offset
	      mTimeSlewingObj->addTimeSlewingInfo(ich + ipad, 0, mCalibChannelOffset[ich + ipad]);
	    }
	    
	    if (mDebugMode && gTimeVsTot && gTimeVsTot->GetN() && fout) {
	      fout->cd();
	      int istrip = ((ich + ipad) / o2::tof::Geo::NPADS) % o2::tof::Geo::NSTRIPXSECTOR;
	      gTimeVsTot->SetName(Form("pad_%02d_%02d_%02d", sector, istrip, ipad%o2::tof::Geo::NPADS));
	      gTimeVsTot->Write();
	      //	      histoChTimeSlewingTemp->Write(Form("histoChTimeSlewingTemp_%02d_%02d_%02d", sector, istrip, ipad%o2::tof::Geo::NPADS)); // no longer written since it produces a very large output
	    }
	  } else if (flag & kChannelOffset) {
	    mTimeSlewingObj->addTimeSlewingInfo(ich + ipad, 0, mCalibChannelOffset[ich + ipad]);
	  }
	}
      }
      if (fout) {
	fout->Close();
	delete fout;
      }
    }
    
    for(int ipad=0; ipad < NPADSPERSTEP; ipad++){
      delete histoChOffsetTemp[ipad];
      if (calibTimePad[ipad]) delete calibTimePad[ipad];
    }
    delete histoChTimeSlewingTemp;
    delete funcChOffset;
  }

  fOpenLocally.Close();

  timerTot.Stop();
  printf("Timing (%i):\n", sector);
  printf("Total:        ");
  timerTot.Print();
}

//______________________________________________
void CalibTOF::fillOutput(){
  mOutputTree->Fill();
}

//______________________________________________
void CalibTOF::print() const
{
  ///< print the settings

  LOG(INFO) << "****** component for calibration of TOF channels ******";
  if (!mInitDone) {
    LOG(INFO) << "init is not done yet - nothing to print";
    return;
  }

  LOG(INFO) << "**********************************************************************";
}

//______________________________________________
bool CalibTOF::loadTOFCollectedCalibInfo(TTree *localTree, int &currententry, int increment)
{
  ///< load next chunk of TOF infos
  //  printf("Loading TOF calib infos: number of entries in tree = %lld\n", mTreeCollectedCalibInfoTOF->GetEntries());

  currententry += increment;
  while (currententry < localTree->GetEntries()){
    //while (currententry < 800000){
    //    && currententry < o2::tof::Geo::NCHANNELS) {
    localTree->GetEntry(currententry);
    //LOG(INFO) << "Loading TOF calib info entry " << currententry << " -> " << mCalibInfoTOF->size()<< " infos";
        
    return true;
  }
  currententry -= increment;

  return false;
}

//______________________________________________

void CalibTOF::fillLHCphaseCalibInput(std::vector<o2::dataformats::CalibInfoTOFshort>* calibinfotof){
  
  // we will fill the input for the LHC phase calibration
  
  static double bc = 1.e13 / o2::constants::lhc::LHCRFFreq; // bunch crossing period (ps)
  static double bc_inv = 1./bc;
    
  for(auto infotof = calibinfotof->begin(); infotof != calibinfotof->end(); infotof++){
    double dtime = infotof->getDeltaTimePi();
    dtime -= int(dtime*bc_inv + 0.5)*bc;
    
    mHistoLHCphase->Fill(dtime, infotof->getTimestamp());
  }
  
}
//______________________________________________

void CalibTOF::doLHCPhaseCalib(){

  // calibrate with respect LHC phase

  if (!mFuncLHCphase){
    mFuncLHCphase = new TF1("fLHCphase", "gaus");
  }

  int ifit0 = 1;
  for (int ifit = ifit0; ifit <= mHistoLHCphase->GetNbinsY(); ifit++){
    TH1D* htemp = mHistoLHCphase->ProjectionX("htemp", ifit0, ifit);
    if (htemp->GetEntries() < 300) {
      // we cannot fit the histogram, we will merge with the next bin
      //      Printf("We don't have enough entries to fit");
      continue;
    }

    int res = FitPeak(mFuncLHCphase, htemp, 500., 3., 2.,"LHCphase");
    if(res) continue;

    mLHCphaseObj->addLHCphase(mHistoLHCphase->GetYaxis()->GetBinLowEdge(ifit0), mFuncLHCphase->GetParameter(1));
    mLHCphase[mNLHCphaseIntervals] = mFuncLHCphase->GetParameter(1);
    mLHCphaseErr[mNLHCphaseIntervals] = mFuncLHCphase->GetParError(1);
    mLHCphaseStartInterval[mNLHCphaseIntervals] = mHistoLHCphase->GetYaxis()->GetBinLowEdge(ifit0); // from when the interval 
    mLHCphaseEndInterval[mNLHCphaseIntervals] = mHistoLHCphase->GetYaxis()->GetBinUpEdge(ifit);
    ifit0 = ifit+1; // starting point for the next LHC interval
    mNLHCphaseIntervals++; // how many intervals we have calibrated so far
  }
}
//______________________________________________

void CalibTOF::fillChannelCalibInput(std::vector<o2::dataformats::CalibInfoTOFshort>* calibinfotof, float offset, int ipad, TH1F* histo, std::vector<o2::dataformats::CalibInfoTOFshort>* calibTimePad){
  
  // we will fill the input for the channel-level calibration

  static double bc = 1.e13 / o2::constants::lhc::LHCRFFreq; // bunch crossing period (ps)
  static double bc_inv = 1./bc;
    
  for(auto infotof = calibinfotof->begin(); infotof != calibinfotof->end(); infotof++){
    double dtime = infotof->getDeltaTimePi() - offset; // removing existing offset 
    dtime -= int(dtime * bc_inv + 0.5) * bc;
    
    histo->Fill(dtime);
    if (calibTimePad) calibTimePad->push_back(*infotof);
  }
}
//______________________________________________

void CalibTOF::fillChannelTimeSlewingCalib(float offset, int ipad, TH2F* histo, std::vector<o2::dataformats::CalibInfoTOFshort>* calibTimePad){
  
// we will fill the input for the channel-time-slewing calibration

  static double bc = 1.e13 / o2::constants::lhc::LHCRFFreq; // bunch crossing period (ps)
  static double bc_inv = 1./bc;
    
  for(auto infotof = calibTimePad->begin(); infotof != calibTimePad->end(); infotof++){
    double dtime = infotof->getDeltaTimePi() - offset; // removing the already calculated offset; this is needed to
                                                       // fill the time slewing histogram in the correct range 
    dtime -= int(dtime * bc_inv + 0.5) * bc;
    
    histo->Fill(TMath::Min(double(infotof->getTot()), 249.9), dtime);
    mHistoChTimeSlewingAll->Fill(infotof->getTot(), dtime);
  }
}
//______________________________________________

int CalibTOF::doChannelCalibration(int ipad, TH1F* histo, TF1* funcChOffset){

  // calibrate single channel from histos - offsets

  FitPeak(funcChOffset, histo, 500., 3., 2., "ChannelOffset");

  return 0;
}
//______________________________________________

void CalibTOF::resetChannelLevelHistos(TH1F* histoOffset[NPADSPERSTEP], TH2F* histoTimeSlewing, std::vector<o2::dataformats::CalibInfoTOFshort>* calibTimePad[NPADSPERSTEP]){
  
  // reset single channel histos

  for(int ipad=0; ipad < NPADSPERSTEP; ipad++){
    histoOffset[ipad]->Reset();
    if (calibTimePad[ipad]) calibTimePad[ipad]->clear();
  }
  histoTimeSlewing->Reset();
}

//______________________________________________

TGraphErrors *CalibTOF::processSlewing(TH2F *histo, Bool_t forceZero,TF1 *fitFunc){  
  /* projection-x */
  TH1D *hpx = histo->ProjectionX("hpx");

  /* define mix and max TOT bin */
  Int_t minBin = hpx->FindFirstBinAbove(0);
  Int_t maxBin = hpx->FindLastBinAbove(0);
  Float_t minTOT = hpx->GetBinLowEdge(minBin);
  Float_t maxTOT = hpx->GetBinLowEdge(maxBin + 1);
  //  printf("min/max TOT defined: %f < TOT < %f ns [%d, %d]\n", minTOT, maxTOT, minBin, maxBin);

  /* loop over TOT bins */
  Int_t nPoints = 0;
  Float_t tot[10000], toterr[10000];
  Float_t mean[10000], meanerr[10000];
  Float_t sigma[10000], vertexSigmaerr[10000];
  for (Int_t ibin = minBin; ibin <= maxBin; ibin++) {

    /* define TOT window */
    Int_t startBin = ibin;
    Int_t endBin = ibin;
    while(hpx->Integral(startBin, endBin) < 300) {
      if (startBin == 1 && forceZero) break;
      if (endBin < maxBin) endBin++;
      else if (startBin > minBin) startBin--;
      else break;
    }
    if (hpx->Integral(startBin, endBin) <= 0) continue;
    //    printf("TOT window defined: %f < TOT < %f ns [%d, %d], %d tracks\n", hpx->GetBinLowEdge(startBin), hpx->GetBinLowEdge(endBin + 1), startBin, endBin, (Int_t)hpx->Integral(startBin, endBin));

    /* projection-y */
    TH1D *hpy = histo->ProjectionY("hpy", startBin, endBin);

    /* average TOT */
    hpx->GetXaxis()->SetRange(startBin, endBin);
    tot[nPoints] = hpx->GetMean();
    toterr[nPoints] = hpx->GetMeanError();
    
    /* fit peak in slices of tot */
    if (FitPeak(fitFunc, hpy, 500., 3., 2., "TimeSlewing", histo) != 0) {
      //      printf("troubles fitting time-zero TRACKS, skip\n");
      delete hpy;
      continue;
    }
    mean[nPoints] = fitFunc->GetParameter(1);
    meanerr[nPoints] = fitFunc->GetParError(1);

    /* delete projection-y */
    delete hpy;

    //    printf("meanerr = %f\n",meanerr[nPoints]);

    /* increment n points if good mean error */
    if (meanerr[nPoints] < 100.) {
      nPoints++;
    }

    /* set current bin */
    ibin = endBin;

  } /* end of loop over time bins */

  /* check points */
  if (nPoints <= 0) {
    //    printf("no measurement available, quit\n");
    delete hpx;
    return NULL;
  }
  
  /* create graph */
  TGraphErrors *gSlewing = new TGraphErrors(nPoints, tot, mean, toterr, meanerr);

  delete hpx;
  return gSlewing;
}
//______________________________________________

Int_t CalibTOF::FitPeak(TF1 *fitFunc, TH1 *h, Float_t startSigma, Float_t nSigmaMin, Float_t nSigmaMax, const char *debuginfo, TH2 *hdbg){
  /*
   * fit peak
   */

  static int nn=0;

  Double_t fitCent = h->GetBinCenter(h->GetMaximumBin());
  Double_t fitMin = fitCent - nSigmaMin * startSigma;
  Double_t fitMax = fitCent + nSigmaMax * startSigma;
  if (fitMin < h->GetXaxis()->GetXmin()) fitMin = h->GetXaxis()->GetXmin();
  if (fitMax > h->GetXaxis()->GetXmax()) fitMax = h->GetXaxis()->GetXmax();
  fitFunc->SetParameter(0, 100);
  fitFunc->SetParameter(1, fitCent);
  fitFunc->SetParameter(2, startSigma);
  Int_t fitres = h->Fit(fitFunc, "WWq0", "", fitMin, fitMax);
  if (fitres != 0) return fitres;
  /* refit with better range */
  for (Int_t i = 0; i < 3; i++) {
    fitCent = fitFunc->GetParameter(1);
    fitMin = fitCent - nSigmaMin * fitFunc->GetParameter(2);
    fitMax = fitCent + nSigmaMax * fitFunc->GetParameter(2);
    if (fitMin < h->GetXaxis()->GetXmin()) fitMin = h->GetXaxis()->GetXmin();
    if (fitMax > h->GetXaxis()->GetXmax()) fitMax = h->GetXaxis()->GetXmax();
    fitres = h->Fit(fitFunc, "q0", "", fitMin, fitMax);
    if (fitres != 0) return fitres;
  }

  if(mDebugMode > 1 && fitFunc->GetParError(1) > 100){
    char* filename = Form("ProblematicFit_%s_%d.root", h->GetName(),nn);
    if (hdbg)
      filename = Form("ProblematicFit_%s_%d.root", hdbg->GetName(),nn);
    //    printf("write %s\n", filename);
    TFile ff(filename, "RECREATE");
    h->Write();
    if (hdbg) hdbg->Write();
    ff.Close();
    nn++;
  }

  return fitres;
}
//______________________________________________

void CalibTOF::merge(const char* name)
{
  TFile* f = TFile::Open(name);
  if (!f) {
    LOG(ERROR) << "File " << name << "not found (merging skept)";
    return;
  }
  TTree* t = (TTree*)f->Get(mOutputTree->GetName());
  if (!t) {
    LOG(ERROR) << "Tree " << mOutputTree->GetName() << "not found in " << name << " (merging skept)";
    return;
  }
  t->ls();
  mOutputTree->ls();

  o2::dataformats::CalibLHCphaseTOF* LHCphaseObj = nullptr;

  o2::dataformats::CalibTimeSlewingParamTOF* timeSlewingObj = nullptr;

  t->SetBranchAddress("mLHCphaseObj", &LHCphaseObj);
  t->SetBranchAddress("mTimeSlewingObj", &timeSlewingObj);

  t->GetEvent(0);

  *mTimeSlewingObj += *timeSlewingObj;
  *mLHCphaseObj += *LHCphaseObj;
  f->Close();
}
