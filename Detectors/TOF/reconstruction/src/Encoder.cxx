// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

#include "TOFReconstruction/Encoder.h"
#include "TOFReconstruction/Decoder.h"
#include <iostream>
#include <chrono>
#include <algorithm>
#include "CommonConstants/LHCConstants.h"
#include "CommonConstants/Triggers.h"
#include "TString.h"
#include "FairLogger.h"
#include <array>
#define VERBOSE

namespace o2
{
namespace tof
{
namespace raw
{

Encoder::Encoder()
{
  // intialize 72 buffers (one per crate)
  for(int i=0;i < 72;i++){
    mIntegratedBytes[i] = 0;
    mBuffer[i] = nullptr;
    mUnion[i] = nullptr;
    mStart[i] = nullptr;
    mNRDH[i] = 0;
  }
}

void Encoder::nextWord(int icrate){
  // mUnion[icrate]++;
  // mUnion[icrate]->data = 0;
  if(mNextWordStatus[icrate]){
    mUnion[icrate]++;
    mUnion[icrate]->data = 0;
    mUnion[icrate]++;
    mUnion[icrate]->data = 0;
  }
  mUnion[icrate]++;
  mNextWordStatus[icrate] = !mNextWordStatus[icrate];
}

bool Encoder::open(std::string name)
{
  bool status = false;

  for(int i=0;i < NCRU;i++){
    std::string nametmp;
    nametmp.append(Form("cru%02d",i));
    nametmp.append(name);
    printf("TOF Raw encoder: create stream to CRU: %s\n",nametmp.c_str());
    if (mFileCRU[i].is_open()) {
      std::cout << "Warning: a file (" << i  << ") was already open, closing" << std::endl;
      mFileCRU[i].close();
    }
    mFileCRU[i].open(nametmp.c_str(), std::fstream::out | std::fstream::binary);
    if (!mFileCRU[i].is_open()) {
      std::cerr << "Cannot open " << nametmp << std::endl;
      status = true;
    }

  }

  for(int i=0;i < 72;i++){
    std::string nametmp;
    nametmp.append(Form("%02d",i));
    nametmp.append(name);
    printf("TOF Raw encoder: create stream to %s\n",nametmp.c_str());
    if (mFile[i].is_open()) {
      std::cout << "Warning: a file (" << i  << ") was already open, closing" << std::endl;
      mFile[i].close();
    }
    mFile[i].open(nametmp.c_str(), std::fstream::out | std::fstream::binary);
    if (!mFile[i].is_open()) {
      std::cerr << "Cannot open " << nametmp << std::endl;
      status = true;
    }
  }
  return status;
}

bool Encoder::flush(int icrate){
  if(mIntegratedBytes[icrate]){
    mFile[icrate].write(mBuffer[icrate], mIntegratedBytes[icrate]);
    mIntegratedAllBytes += mIntegratedBytes[icrate];
    mIntegratedBytes[icrate] = 0;
    mUnion[icrate] = mStart[icrate];
  }
}

bool Encoder::flush()
{
  bool allempty=true;
  for(int i=0;i < 72;i++)
    if(mIntegratedBytes[i]) allempty = false;

  if(allempty) return true;

  // write superpages
  for(int i=0;i < 72;i++){
    int icru = i/NLINKSPERCRU;
    mFileCRU[icru].write(mBuffer[i], mIntegratedBytes[i]);
  }

  for(int i=0;i < 72;i++)
    flush(i);
  
  memset(mBuffer[0], 0, mSize*72);

  for(int i=0;i < 72;i++){
    mIntegratedBytes[i] = 0;
    mUnion[i] = mStart[i];
  }

  return false;
}

bool Encoder::close()
{
  for(int i=0;i < 72;i++)
    if (mFile[i].is_open())
      mFile[i].close();

  for(int i=0;i < NCRU;i++)
    if (mFileCRU[i].is_open())
      mFileCRU[i].close();
  return false;
}

bool Encoder::alloc(long size)
{
  if(size < 500000) size = 500000;

  mSize = size;

  mBufferLocal.resize(size*72);

  mBuffer[0] = mBufferLocal.data();
  memset(mBuffer[0], 0, mSize*72);
  mStart[0] = reinterpret_cast<Union_t*>(mBuffer[0]);
  mUnion[0] = mStart[0]; // rewind

  for(int i=1;i < 72;i++){
    mBuffer[i] = mBuffer[i-1]+size;
    mStart[i] = reinterpret_cast<Union_t*>(mBuffer[i]);
    mUnion[i] = mStart[i]; // rewind
  }
  return false;
}

void Encoder::encodeTRM(const std::vector<Digit>& summary, Int_t icrate, Int_t itrm, int& istart) // encode one TRM assuming digit vector sorted by electronic index
// return next TRM index (-1 if not in the same crate)
// start to convert digiti from istart --> then update istart to the starting position of the new TRM
{
  if(mVerbose) printf("Crate %d: encode TRM %d \n",icrate,itrm);

  // TRM HEADER
  mUnion[icrate]->trmGlobalHeader.slotID = itrm;
  mUnion[icrate]->trmGlobalHeader.eventWords = 0; // to be filled at the end
  mUnion[icrate]->trmGlobalHeader.eventNumber = mEventCounter;
  mUnion[icrate]->trmGlobalHeader.eBit = 0;
  mUnion[icrate]->trmGlobalHeader.wordType = 4;
  nextWord(icrate);

  // LOOP OVER CHAINS
  for(int ichain=0;ichain < 2; ichain++){
    // CHAIN HEADER
    mUnion[icrate]->trmChainHeader.slotID = itrm;
    mUnion[icrate]->trmChainHeader.bunchID = mIR.bc;
    mUnion[icrate]->trmChainHeader.pb24Temp = 0;
    mUnion[icrate]->trmChainHeader.pb24ID = 0;
    mUnion[icrate]->trmChainHeader.tsBit = 0;
    mUnion[icrate]->trmChainHeader.wordType = 2*ichain;
    nextWord(icrate);
    
    while(istart < summary.size()){ // fill hits
      /** loop over hits **/
      int whatChain = summary[istart].getElChainIndex();
      if (whatChain != ichain) break;
      int whatTRM = summary[istart].getElTRMIndex();
      if (whatTRM != itrm) break;
      int whatCrate = summary[istart].getElCrateIndex();
      if (whatCrate != icrate) break;
      
      int hittimeTDC = (summary[istart].getBC() - mEventCounter * Geo::BC_IN_WINDOW) * 1024 + summary[istart].getTDC(); // time in TDC bin within the TOF WINDOW
      
      // leading time
      mUnion[icrate]->tdcUnpackedHit.hitTime = hittimeTDC;
      mUnion[icrate]->tdcUnpackedHit.chan = summary[istart].getElChIndex();
      mUnion[icrate]->tdcUnpackedHit.tdcID = summary[istart].getElTDCIndex();
      mUnion[icrate]->tdcUnpackedHit.eBit = 0;
      mUnion[icrate]->tdcUnpackedHit.psBits = 1;
      mUnion[icrate]->tdcUnpackedHit.mustBeOne = 1;
      nextWord(icrate);
      
      // trailing time
      mUnion[icrate]->tdcUnpackedHit.hitTime = hittimeTDC + summary[istart].getTOT()*Geo::RATIO_TOT_TDC_BIN;
      mUnion[icrate]->tdcUnpackedHit.chan = summary[istart].getElChIndex();
      mUnion[icrate]->tdcUnpackedHit.tdcID = summary[istart].getElTDCIndex();
      mUnion[icrate]->tdcUnpackedHit.eBit = 0;
      mUnion[icrate]->tdcUnpackedHit.psBits = 2;
      mUnion[icrate]->tdcUnpackedHit.mustBeOne = 1;
      nextWord(icrate);
      
      istart++;
    }

    // CHAIN TRAILER
    mUnion[icrate]->trmChainTrailer.status = 0;
    mUnion[icrate]->trmChainTrailer.mustBeZero = 0;
    mUnion[icrate]->trmChainTrailer.eventCounter = mEventCounter;
    mUnion[icrate]->trmChainTrailer.wordType = 1 + 2*ichain;
    nextWord(icrate);
  }

  // TRM TRAILER
  mUnion[icrate]->trmGlobalTrailer.mustBeThree = 3;
  mUnion[icrate]->trmGlobalTrailer.eventCRC = 0; // to be implemented
  mUnion[icrate]->trmGlobalTrailer.temp = 0;
  mUnion[icrate]->trmGlobalTrailer.sendAd = 0;
  mUnion[icrate]->trmGlobalTrailer.chain = 0;
  mUnion[icrate]->trmGlobalTrailer.tsBit = 0;
  mUnion[icrate]->trmGlobalTrailer.lBit = 0;
  mUnion[icrate]->trmGlobalTrailer.wordType = 5;
  nextWord(icrate);
}

bool Encoder::encode(std::vector<std::vector<o2::tof::Digit>> digitWindow, int tofwindow) // pass a vector of digits in a TOF readout window, tof window is the entry in the vector-of-vector of digits needed to extract bunch id and orbit
{
  if(digitWindow.size() != Geo::NWINDOW_IN_ORBIT){
    printf("Something went wrong in encoding: digitWindow=%d different from %d\n",digitWindow.size(),Geo::NWINDOW_IN_ORBIT);
    return 999;
  }

  for(int i=0;i < 72;i++){
    if(mIntegratedBytes[i] + 100000 > mSize)
      flush();
  }

  for (int iwin=0; iwin < Geo::NWINDOW_IN_ORBIT; iwin++) {
    std::vector<o2::tof::Digit> &summary = digitWindow.at(iwin);
    // caching electronic indexes in digit array
    for (auto dig = summary.begin(); dig != summary.end(); dig++) {
      int digitchannel = dig->getChannel();
      dig->setElectronicIndex(Geo::getECHFromCH(digitchannel));
    }
    
    // sorting by electroni indexes
    std::sort(summary.begin(), summary.end(),
	      [](Digit a, Digit b) { return a.getElectronicIndex() < b.getElectronicIndex(); });
  }
  
#ifdef VERBOSE
  if (mVerbose)
    std::cout << "-------- START ENCODE EVENT ----------------------------------------" << std::endl;
#endif
  auto start = std::chrono::high_resolution_clock::now();

    mEventCounter = tofwindow; // tof window index
    mIR.orbit = mEventCounter / Geo::NWINDOW_IN_ORBIT; // since 3 tof window = 1 orbit

  for(int i=0;i < 72;i++){
    // add RDH open
    mRDH[i] = reinterpret_cast<o2::header::RAWDataHeader*>(mUnion[i]);
    openRDH(i);
  }

  // encode all windows
  for (int iwin=0; iwin < Geo::NWINDOW_IN_ORBIT; iwin++) {
    mEventCounter = tofwindow+iwin;                                                                       // tof window index

    std::vector<o2::tof::Digit> &summary = digitWindow.at(iwin);

    mIR.bc = ((mEventCounter % Geo::NWINDOW_IN_ORBIT) * Geo::BC_IN_ORBIT) / Geo::NWINDOW_IN_ORBIT; // bunch crossing in the current orbit at the beginning of the window

    int icurrentdigit = 0;
    // TOF data header
    for(int i=0;i < 72;i++){
      mDRMCommonHeader[i] = reinterpret_cast<DRMCommonHeader_t*>(mUnion[i]);
      mDRMCommonHeader[i]->payload=0; // event length in byte (to be filled later)
      mDRMCommonHeader[i]->wordType=4;
      nextWord(i);

      mUnion[i]->drmOrbitHeader.orbit = mIR.orbit;
      nextWord(i);

      mDRMGlobalHeader[i] = reinterpret_cast<DRMGlobalHeader_t*>(mUnion[i]);
      mDRMGlobalHeader[i]->slotID =1;
      mDRMGlobalHeader[i]->eventWords =0; // event length in word (to be filled later) --> word = byte/4
      mDRMGlobalHeader[i]->drmID =i;
      mDRMGlobalHeader[i]->wordType =4;
      nextWord(i);

      mUnion[i]->drmStatusHeader1.slotID = 1;
      mUnion[i]->drmStatusHeader1.participatingSlotID = (i % 2 == 0 ? 0x7fc : 0x7fe);
      mUnion[i]->drmStatusHeader1.cBit = 0;
      mUnion[i]->drmStatusHeader1.versID = 0x11;
      mUnion[i]->drmStatusHeader1.drmHSize = 5;
      mUnion[i]->drmStatusHeader1.undefined = 0;
      mUnion[i]->drmStatusHeader1.wordType = 4;
      nextWord(i);

      mUnion[i]->drmStatusHeader2.slotID = 1;
      mUnion[i]->drmStatusHeader2.slotEnableMask =  (i % 2 == 0 ? 0x7fc : 0x7fe);
      mUnion[i]->drmStatusHeader2.mustBeZero = 0;
      mUnion[i]->drmStatusHeader2.faultID = 0;
      mUnion[i]->drmStatusHeader2.rtoBit = 0;
      mUnion[i]->drmStatusHeader2.wordType = 4;
      nextWord(i);

      mUnion[i]->drmStatusHeader3.slotID = 1;
      mUnion[i]->drmStatusHeader3.l0BunchID = mIR.bc;
      mUnion[i]->drmStatusHeader3.runTimeInfo = 0;
      mUnion[i]->drmStatusHeader3.wordType = 4;
      nextWord(i);

      mUnion[i]->drmStatusHeader4.slotID = 1;
      mUnion[i]->drmStatusHeader4.temperature = 0;
      mUnion[i]->drmStatusHeader4.mustBeZero1 = 0;
      mUnion[i]->drmStatusHeader4.ackBit = 0;
      mUnion[i]->drmStatusHeader4.sensAD = 0;
      mUnion[i]->drmStatusHeader4.mustBeZero2 = 0;
      mUnion[i]->drmStatusHeader4.undefined= 0;
      mUnion[i]->drmStatusHeader4.wordType = 4;
      nextWord(i);

      mUnion[i]->drmStatusHeader5.unknown = 0;
      nextWord(i);

      int trm0 = 4 - (i%2);
      for(int itrm=trm0;itrm < 13;itrm++){
	encodeTRM(summary,i,itrm,icurrentdigit);
      }
      
      mUnion[i]->drmGlobalTrailer.slotID = 1;
      mUnion[i]->drmGlobalTrailer.localEventCounter = mEventCounter;
      mUnion[i]->drmGlobalTrailer.undefined = 0;
      mUnion[i]->drmGlobalTrailer.wordType = 5;
      nextWord(i);
      mUnion[i]->data = 0x70000000;
      nextWord(i);

      mDRMCommonHeader[i]->payload = getSize(mDRMCommonHeader[i],mUnion[i]);
      mDRMGlobalHeader[i]->eventWords = mDRMCommonHeader[i]->payload / 4;
    }
    
    // check that all digits were used
    if(icurrentdigit < summary.size()){
      LOG(ERROR) << "Not all digits are been used : only " << icurrentdigit << " of " << summary.size();
    }
  }

  for(int i=0;i < 72;i++){
    // adjust RDH open with the size
    mRDH[i]->memorySize = getSize(mRDH[i],mUnion[i]);
    mRDH[icrate]->offsetToNext = mRDH[icrate]->memorySize;
    mIntegratedBytes[i] += mRDH[icrate]->offsetToNext;

    // add RDH close
    mRDH[i] = reinterpret_cast<o2::header::RAWDataHeader*>(nextPage(mRDH[i],mRDH[icrate]->offsetToNext));
    closeRDH(i);
    mIntegratedBytes[i] += mRDH[icrate]->offsetToNext;
    mUnion[i] = reinterpret_cast<Union_t *>(nextPage(mRDH[i],mRDH[icrate]->offsetToNext));
  }

  auto finish = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> elapsed = finish - start;
  mIntegratedTime = elapsed.count();

#ifdef VERBOSE
  int allBytes = mIntegratedAllBytes;
  for(int i=0;i < 72;i++)
    allBytes += mIntegratedBytes[i];
  if (mVerbose && mIntegratedTime)
    std::cout << "-------- END ENCODE EVENT ------------------------------------------"
              << " " << allBytes << " words"
              << " | " << 1.e3 * mIntegratedTime << " ms"
              << " | " << 1.e-6 * allBytes / mIntegratedTime << " MB/s (average)"
              << std::endl;
#endif

  return false;
}

void Encoder::openRDH(int icrate){
  mNextWordStatus[icrate] = false;
    
  *mRDH[icrate] = mHBFSampler.createRDH<o2::header::RAWDataHeader>(mIR);

  // word1
  mRDH[icrate]->memorySize = mRDH[icrate]->headerSize;
  mRDH[icrate]->offsetToNext = mRDH[icrate]->memorySize;
  mRDH[icrate]->linkID = icrate%NLINKSPERCRU;
  mRDH[icrate]->packetCounter = mNRDH[icrate];
  mRDH[icrate]->cruID = icrate/NCRU;
  mRDH[icrate]->feeId = icrate;

  // word2
  mRDH[icrate]->triggerOrbit = 0; // to be checked
  mRDH[icrate]->heartbeatOrbit = mIR.orbit;
  
  // word4
  mRDH[icrate]->triggerBC = 0; // to be checked (it should be constant)
  mRDH[icrate]->heartbeatBC = 0;
  mRDH[icrate]->triggerType = o2::trigger::HB|o2::trigger::ORBIT;
  if(mNRDH[icrate]==0){
    //    mRDH[icrate]->triggerType |= mIsContinuous ? o2::trigger::SOC : o2::trigger::SOT; 
    mRDH[icrate]->triggerType |= o2::trigger::SOT; 
  }
  if(!(mIR.orbit%256))
    mRDH[icrate]->triggerType |= o2::trigger::TF; 

  // word6
  mRDH[icrate]->pageCnt = 2*mEventCounter; // check vs packetCounter

  char* shift = reinterpret_cast<char*>(mRDH[icrate]);

  mUnion[icrate] = reinterpret_cast<Union_t*>(shift + mRDH[icrate]->headerSize);
  mNRDH[icrate]++;
}

void Encoder::closeRDH(int icrate){
  *mRDH[icrate] = mHBFSampler.createRDH<o2::header::RAWDataHeader>(mIR);

  mRDH[icrate]->stop = 0x1; 

   // word1
  mRDH[icrate]->memorySize = mRDH[icrate]->headerSize;
  mRDH[icrate]->offsetToNext = mRDH[icrate]->memorySize;
  mRDH[icrate]->linkID = icrate%NLINKSPERCRU;
  mRDH[icrate]->packetCounter = mNRDH[icrate];
  mRDH[icrate]->cruID = icrate/NCRU;
  mRDH[icrate]->feeId = icrate;

  // word2
  mRDH[icrate]->triggerOrbit = 0; // to be checked
  mRDH[icrate]->heartbeatOrbit = mIR.orbit;
  
  // word4
  mRDH[icrate]->triggerBC = 0; // to be checked (it should be constant)
  mRDH[icrate]->heartbeatBC = 0;
  mRDH[icrate]->triggerType = o2::trigger::HB|o2::trigger::ORBIT;
  if(mNRDH[icrate]==1){
    //    mRDH[icrate]->triggerType |= mIsContinuous ? o2::trigger::SOC : o2::trigger::SOT; 
    mRDH[icrate]->triggerType |= o2::trigger::SOT; 
  }
  if(!(mIR.orbit%256))
    mRDH[icrate]->triggerType |= o2::trigger::TF; 

  // word6
  mRDH[icrate]->pageCnt = 2*mEventCounter + 1; // check vs packetCounter
  mNRDH[icrate]++;
}

char *Encoder::nextPage(void *current,int step){
  char *point = reinterpret_cast<char *>(current);
  point += step;

  return point;
}

int Encoder::getSize(void *first,void *last){
  char *in = reinterpret_cast<char *>(first);
  char *out = reinterpret_cast<char *>(last);

  return int(out - in);
}

} // namespace compressed
} // namespace tof
} // namespace o2
