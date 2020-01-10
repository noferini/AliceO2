// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file Decoder.h
/// \brief Definition of the TOF encoder

#ifndef ALICEO2_TOF_DECODER_H
#define ALICEO2_TOF_DECODER_H

#include <fstream>
#include <string>
#include <cstdint>
#include "DataFormatsTOF/CompressedDataFormat.h"
#include "TOFBase/Geo.h"
#include "TOFBase/Digit.h"
#include <array>
#include "Headers/RAWDataHeader.h"

namespace o2
{
namespace tof
{
namespace compressed
{
/// \class Decoder
/// \brief Decoder class for TOF
///
class Decoder
{

 public:
  Decoder();
  ~Decoder() = default;

  bool open(std::string name);

  bool decode();
  void loadDigits(int window, std::vector<Digit> *digits);
  void readTRM(std::vector<Digit>* digits, int icrate, int orbit, int bunchid);

  bool close();
  void setVerbose(bool val) { mVerbose = val; };

  void printRDH() const;
  void printCrateInfo(int icrate) const;
  void printTRMInfo(int icrate) const;
  void printCrateTrailerInfo(int icrate) const;
  void printHitInfo(int icrate) const;

  static void fromRawHit2Digit(int icrate, int itrm, int itdc, int ichain, int channel, int orbit, int bunchid, int tdc, int tot, std::array<int, 4>& digitInfo); // convert raw info in digit info (channel, tdc, tot, bc), tdc = packetHit.time + (frameHeader.frameID << 13)

  char *nextPage(void *current,int shift=8192);

 protected:
  // benchmarks
  double mIntegratedBytes[72];
  double mIntegratedTime = 0.;

  std::ifstream mFile[72];
  bool mVerbose = false;
  bool mCrateIn[72];

  char* mBuffer[72];
  std::vector<char> mBufferLocal;
  long mSize[72];
  Union_t* mUnion[72];
  Union_t* mUnionEnd[72];

  o2::header::RAWDataHeader* mRDH;

  std::vector<Digit> mDigitWindow[Geo::NWINDOW_IN_ORBIT];
};

} // namespace compressed
} // namespace tof
} // namespace o2
#endif
