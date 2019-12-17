// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

#include "TOFReconstruction/Decoder.h"
#include <iostream>
#include <chrono>
#include <algorithm>
#include "CommonConstants/LHCConstants.h"
#include "TString.h"
#include "FairLogger.h"
//#define VERBOSE

namespace o2
{
namespace tof
{
namespace compressed
{
Decoder::Decoder(){
  for(int i=0;i < 72;i++){
    mIntegratedBytes[i] = 0.;
    mBuffer[i] = nullptr;
    mCrateIn[i] = false;
  }
}

bool Decoder::open(std::string name)
{
  int nfileopened = 72;

  int fullsize = 0;
  for(int i=0;i < 72;i++){
    std::string nametmp;
    nametmp.append(Form("%02d",i));
    nametmp.append(name);

    if (mFile[i].is_open()) {
      std::cout << "Warning: a file (" << i  << ") was already open, closing" << std::endl;
      mFile[i].close();
    }
    mFile[i].open(nametmp.c_str(), std::fstream::in | std::fstream::binary);
    if (!mFile[i].is_open()) {
      std::cout << "Cannot open " << nametmp << std::endl;
      nfileopened--;
      mSize[i] = 0;
      mCrateIn[i] = false;
    }
    else{
      mFile[i].seekg(0, mFile[i].end);
      mSize[i] = mFile[i].tellg();
      mFile[i].seekg(0);

      fullsize += mSize[i];

      mCrateIn[i] = true;
    }
  }

  if(! nfileopened){
    std::cerr << "No streams available" << std::endl;
    return true;
  }

  mBufferLocal.resize(fullsize);

  printf("Full input buffer size = %d byte\n",fullsize);

  char* pos = mBufferLocal.data();
  for(int i=0;i < 72;i++){
    if(!mCrateIn[i]) continue;

    mBuffer[i] = pos;
      
    // read content of infile
    mFile[i].read(mBuffer[i], mSize[i]);
    mUnion[i] = reinterpret_cast<Union_t*>(mBuffer[i]);
    mUnionEnd[i] = reinterpret_cast<Union_t*>(mBuffer[i] + mSize[i] - 1);

    pos += mSize[i];
  }


  printf("Number of TOF compressed streamers = %d/72\n",nfileopened);

  close();

  return false;
}

bool Decoder::close()
{
  for(int i=0;i < 72;i++){
    if (mFile[i].is_open())
      mFile[i].close();
  }
  return false;
}

void Decoder::readTRM(std::vector<Digit>* digits, int icrate, int orbit, int bunchid)
{
  if (mVerbose)
    printTRMInfo(icrate);
  int nhits = mUnion[icrate]->frameHeader.numberOfHits;
  int time_ext = mUnion[icrate]->frameHeader.frameID << 13;
  int itrm = mUnion[icrate]->frameHeader.trmID;
  mUnion[icrate]++;
  mIntegratedBytes[icrate] += 4;

  // read hits
  Int_t channel, echannel;
  Int_t tdc;
  Int_t tot;
  Int_t bc;
  Int_t time;

  std::array<int, 4> digitInfo;

  for (int i = 0; i < nhits; i++) {
    fromRawHit2Digit(icrate, itrm, mUnion[icrate]->packedHit.tdcID, mUnion[icrate]->packedHit.chain, mUnion[icrate]->packedHit.channel, orbit, bunchid, time_ext + mUnion[icrate]->packedHit.time, mUnion[icrate]->packedHit.tot, digitInfo);

    if (mVerbose)
      printHitInfo(icrate);
    digits->emplace_back(digitInfo[0], digitInfo[1], digitInfo[2], digitInfo[3]);
    mUnion[icrate]++;
    mIntegratedBytes[icrate] += 4;
  }
}

void Decoder::fromRawHit2Digit(int icrate, int itrm, int itdc, int ichain, int channel, int orbit, int bunchid, int tdc, int tot, std::array<int, 4>& digitInfo)
{
  // convert raw info in digit info (channel, tdc, tot, bc)
  // tdc = packetHit.time + (frameHeader.frameID << 13)
  int echannel = Geo::getECHFromIndexes(icrate, itrm, ichain, itdc, channel);
  digitInfo[0] = Geo::getCHFromECH(echannel);
  digitInfo[2] = tot;

//  printf("crate=%d, trm=%d, chain=%d, tdc=%d, ch=%d ---- echannel = %d ---- channel = %d - tot=%d\n",icrate, itrm, ichain, itdc, channel,echannel,digitInfo[0],tot);

  digitInfo[3] = int(orbit * o2::tof::Geo::BC_IN_ORBIT);
  digitInfo[3] += bunchid;
  digitInfo[3] += tdc / 1024;
  digitInfo[1] = tdc % 1024;
}

void Decoder::loadDigits(int window, std::vector<Digit> *digits){
  if(window < Geo::NWINDOW_IN_ORBIT)
    *digits = mDigitWindow[window];
  else
    std::cout << "You tried to get more window (" << window<< ") than expected in one orbit (" << Geo::NWINDOW_IN_ORBIT << ")" << std::endl;
}

  char *Decoder::nextPage(void *current, int shift){
  char *point = reinterpret_cast<char *>(current);
  point += shift;

  return point;
}

bool Decoder::decode() // return a vector of digits in a TOF readout window
{ 
#ifdef VERBOSE
  if (mVerbose)
    std::cout << "-------- START DECODE EVENTS IN THE HB ----------------------------------------" << std::endl;
#endif
  auto start = std::chrono::high_resolution_clock::now();

  //  reset windows of digits
  for(int window = 0; window < Geo::NWINDOW_IN_ORBIT;window++)
    mDigitWindow[window].clear();

  int nread = 0;

  for (int icrate = 0; icrate < 72; icrate++) {
    if(! mCrateIn[icrate]) continue; // no data stream available for this crate/DRM
    // read open RDH
    mRDH = reinterpret_cast<o2::header::RAWDataHeader*>(mUnion[icrate]);
    if (mVerbose)
      printRDH();

    // move after RDH
    char* shift = reinterpret_cast<char*>(mRDH);
    mUnion[icrate] = reinterpret_cast<Union_t*>(shift + mRDH->headerSize);
    mIntegratedBytes[icrate] += mRDH->headerSize;

    if(mUnion[icrate] >= mUnionEnd[icrate]) continue; // end of data stream reached for this crate/DRM
//    printf("byte to be read = %d\n",int(mUnionEnd[icrate] - mUnion[icrate]));

    // number of crates/DRM read
    nread++;

    for(int window=0; window < Geo::NWINDOW_IN_ORBIT; window++){
      // read Crate Header
      int bunchid = mUnion[icrate]->crateHeader.bunchID;
      if (mVerbose)
	printCrateInfo(icrate);
      mUnion[icrate]++;
      mIntegratedBytes[icrate] += 4;

      //read Orbit
      int orbit = mUnion[icrate]->crateOrbit.orbitID;
      if (mVerbose)
	printf("orbit ID      = %d\n", orbit);
      mUnion[icrate]++;
      mIntegratedBytes[icrate] += 4;

      while (!mUnion[icrate]->frameHeader.mustBeZero) {
	readTRM(&(mDigitWindow[window]), icrate, orbit, bunchid);
      }

      // read Crate Trailer
      if (mVerbose)
	printCrateTrailerInfo(icrate);
      auto ndw = mUnion[icrate]->crateTrailer.numberOfDiagnostics;
      mUnion[icrate]++;
      mIntegratedBytes[icrate] += 4;

      // loop over number of diagnostic words
      for (int idw = 0; idw < ndw; ++idw){
	mUnion[icrate]++;
	mIntegratedBytes[icrate] += 4;
      }
    }

    // read close RDH
    mRDH = reinterpret_cast<o2::header::RAWDataHeader*>(nextPage(mRDH,mRDH->memorySize));
    if (mVerbose)
       printRDH();
    mIntegratedBytes[icrate] += mRDH->headerSize;

    // go to next page
    mUnion[icrate] = reinterpret_cast<Union_t*>(nextPage(mRDH,mRDH->memorySize));

  }

  auto finish = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> elapsed = finish - start;
  mIntegratedTime = elapsed.count();

#ifdef VERBOSE
  if (mVerbose && mIntegratedTime){
    double allbytes = 0;
    for(int i=0; i < 72; i++) allbytes += mIntegratedBytes[i];
    std::cout << "-------- END DECODE EVENT ------------------------------------------"
              << " " << allbytes/4 << " words"
              << " | " << 1.e3 * mIntegratedTime << " ms"
              << " | " << 1.e-6 * allbytes / mIntegratedTime << " MB/s (average)"
              << std::endl;
  }
#endif

  if(! nread) return 1; // all buffers exausted

  return 0;
}

void Decoder::printCrateInfo(int icrate) const
{
  printf("___CRATE HEADER____\n");
  printf("DRM ID           = %d\n", mUnion[icrate]->crateHeader.drmID);
  printf("Bunch ID         = %d\n", mUnion[icrate]->crateHeader.bunchID);
  printf("Slot enable mask = %d\n", mUnion[icrate]->crateHeader.slotEnableMask);
  printf("Must be ONE      = %d\n", mUnion[icrate]->crateHeader.mustBeOne);
  printf("___________________\n");
}

void Decoder::printCrateTrailerInfo(int icrate) const
{
  printf("___CRATE TRAILER___\n");
  printf("Event counter         = %d\n", mUnion[icrate]->crateTrailer.eventCounter);
  printf("Number of diagnostics = %d\n", mUnion[icrate]->crateTrailer.numberOfDiagnostics);
  printf("Must be ONE           = %d\n", mUnion[icrate]->crateTrailer.mustBeOne);
  printf("___________________\n");
}

void Decoder::printTRMInfo(int icrate) const
{
  printf("______TRM_INFO_____\n");
  printf("TRM ID        = %d\n", mUnion[icrate]->frameHeader.trmID);
  printf("Frame ID      = %d\n", mUnion[icrate]->frameHeader.frameID);
  printf("N. hits       = %d\n", mUnion[icrate]->frameHeader.numberOfHits);
  printf("DeltaBC       = %d\n", mUnion[icrate]->frameHeader.deltaBC);
  printf("Must be Zero  = %d\n", mUnion[icrate]->frameHeader.mustBeZero);
  printf("___________________\n");
}

void Decoder::printHitInfo(int icrate) const
{
  printf("______HIT_INFO_____\n");
  printf("TDC ID        = %d\n", mUnion[icrate]->packedHit.tdcID);
  printf("CHAIN ID      = %d\n", mUnion[icrate]->packedHit.chain);
  printf("CHANNEL ID    = %d\n", mUnion[icrate]->packedHit.channel);
  printf("TIME          = %d\n", mUnion[icrate]->packedHit.time);
  printf("TOT           = %d\n", mUnion[icrate]->packedHit.tot);
  printf("___________________\n");
}

void Decoder::printRDH() const{
  printf("______RDH_INFO_____\n");
  printf("VERSION       = %d\n", mRDH->version);
  printf("BLOCK LENGTH  = %d\n", mRDH->blockLength);
  printf("HEADER SIZE   = %d\n", mRDH->headerSize);
  printf("MEMORY SIZE   = %d\n", mRDH->memorySize);
  printf("PACKET COUNTER= %d\n", mRDH->packetCounter);
  printf("CRU ID        = %d\n", mRDH->cruID);
  printf("LINK ID       = %d\n", mRDH->linkID);
  printf("___________________\n");

}
} // namespace compressed
} // namespace tof
} // namespace o2
