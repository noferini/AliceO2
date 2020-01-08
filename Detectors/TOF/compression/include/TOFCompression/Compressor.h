// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// @file   Compressor.h
/// @author Roberto Preghenella
/// @since  2019-12-18
/// @brief  TOF raw data compressor

#ifndef O2_TOF_COMPRESSOR
#define O2_TOF_COMPRESSOR

#include <fstream>
#include <string>
#include <cstdint>
#include "Headers/RAWDataHeader.h"
#include "DataFormatsTOF/RawDataFormat.h"
#include "DataFormatsTOF/CompressedDataFormat.h"

namespace o2
{
namespace tof
{

class Compressor
{

 public:
  Compressor();
  ~Compressor();

  inline bool run()
  {
    rewind();
    if (processRDH())
      return false;
    for (int i = 0; i < 3 && !processDRM(); ++i)
      ;
    return false;
  };

  bool init();
  bool open(std::string inFileName, std::string outFileName);
  bool close();
  inline bool read() { return decoderRead(); };
  inline void rewind()
  {
    decoderRewind();
    encoderRewind();
  };
  inline bool write() { return encoderWrite(); };

  bool processRDH();
  bool processDRM();
  void checkSummary();

  void setDecoderVerbose(bool val) { mDecoderVerbose = val; };
  void setEncoderVerbose(bool val) { mEncoderVerbose = val; };
  void setCheckerVerbose(bool val) { mCheckerVerbose = val; };

  void setDecoderBuffer(char* val) { mDecoderBuffer = val; };
  void setEncoderBuffer(char* val) { mEncoderBuffer = val; };
  void setDecoderBufferSize(long val) { mDecoderBufferSize = val; };
  void setEncoderBufferSize(long val) { mEncoderBufferSize = val; };

  inline uint32_t getDecoderByteCounter() const { return reinterpret_cast<char*>(mDecoderPointer) - mDecoderBuffer; };
  inline uint32_t getEncoderByteCounter() const { return reinterpret_cast<char*>(mEncoderPointer) - mEncoderBuffer; };

  // benchmarks
  double mIntegratedBytes = 0.;
  double mIntegratedTime = 0.;

 protected:
  /** decoder private functions and data members **/

  bool decoderInit();
  bool decoderOpen(std::string name);
  bool decoderRead();
  bool decoderClose();
  void decoderClear();
  inline void decoderRewind() { mDecoderPointer = reinterpret_cast<uint32_t*>(mDecoderBuffer); };
  inline void decoderNext()
  {
    mDecoderPointer += mDecoderNextWord;
    mDecoderNextWord = (mDecoderNextWord + 2) % 4;
  };

  std::ifstream mDecoderFile;
  char* mDecoderBuffer = nullptr;
  bool mOwnDecoderBuffer = false;
  long mDecoderBufferSize = 8192;
  uint32_t* mDecoderPointer = nullptr;
  uint32_t mDecoderNextWord = 1;
  o2::header::RAWDataHeader* mDecoderRDH;
  bool mDecoderVerbose = false;

  /** encoder private functions and data members **/

  bool encoderInit();
  bool encoderOpen(std::string name);
  bool encoderWrite();
  bool encoderClose();
  void encoderSPIDER();
  inline void encoderRewind() { mEncoderPointer = reinterpret_cast<uint32_t*>(mEncoderBuffer); };
  inline void encoderNext() { mEncoderPointer++; };

  std::ofstream mEncoderFile;
  char* mEncoderBuffer = nullptr;
  bool mOwnEncoderBuffer = false;
  long mEncoderBufferSize = 8192;
  uint32_t* mEncoderPointer = nullptr;
  uint32_t mEncoderNextWord = 1;
  o2::header::RAWDataHeader* mEncoderRDH;
  bool mEncoderVerbose = false;

  /** checker private functions and data members **/

  bool checkerCheck();

  uint32_t mCounter;
  bool mCheckerVerbose = false;

  struct DRMCounters_t {
    uint32_t Headers;
    uint32_t EventWordsMismatch;
    uint32_t CBit;
    uint32_t Fault;
    uint32_t RTOBit;
  } mDRMCounters = {0};

  struct TRMCounters_t {
    uint32_t Headers;
    uint32_t Empty;
    uint32_t EventCounterMismatch;
    uint32_t EventWordsMismatch;
    uint32_t EBit;
  } mTRMCounters[10] = {0};

  struct TRMChainCounters_t {
    uint32_t Headers;
    uint32_t EventCounterMismatch;
    uint32_t BadStatus;
    uint32_t BunchIDMismatch;
    uint32_t TDCerror;
  } mTRMChainCounters[10][2] = {0};

  /** summary data **/

  struct RawSummary_t {
    uint32_t DRMCommonHeader;
    uint32_t DRMOrbitHeader;
    uint32_t DRMGlobalHeader;
    uint32_t DRMStatusHeader1;
    uint32_t DRMStatusHeader2;
    uint32_t DRMStatusHeader3;
    uint32_t DRMStatusHeader4;
    uint32_t DRMStatusHeader5;
    uint32_t DRMGlobalTrailer;
    uint32_t TRMGlobalHeader[10];
    uint32_t TRMGlobalTrailer[10];
    uint32_t TRMChainHeader[10][2];
    uint32_t TRMChainTrailer[10][2];
    uint32_t TDCUnpackedHit[2][15][256];
    uint8_t nTDCUnpackedHits[2][15];

    uint32_t FramePackedHit[256][256];
    uint8_t nFramePackedHits[256];
    uint8_t FirstFilledFrame;
    uint8_t LastFilledFrame;

    // derived data
    bool HasHits[10];
    bool HasErrors[10][2];
    // status
    bool decodeError;

    // checker data

    uint32_t nDiagnosticWords;
    uint32_t DiagnosticWord[12];

    uint32_t faultFlags;
  } mRawSummary = {0};
};

} // namespace tof
} // namespace o2

#endif /** O2_TOF_COMPRESSOR **/
