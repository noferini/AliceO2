#include "TOFCompression/Compressor.h"
#include "TOFBase/Geo.h"

#include <cstring>
#include <iostream>
#include <chrono>

#define DECODER_VERBOSE
#define ENCODER_VERBOSE

#ifdef DECODER_VERBOSE
#warning "Building code with DecoderVerbose option. This may limit the speed."
#endif
#ifdef ENCODER_VERBOSE
#warning "Building code with EncoderVerbose option. This may limit the speed."
#endif
#ifdef CHECKER_VERBOSE
#warning "Building code with CheckerVerbose option. This may limit the speed."
#endif

#define colorReset "\033[0m"
#define colorRed "\033[1;31m"
#define colorGreen "\033[1;32m"
#define colorYellow "\033[1;33m"
#define colorBlue "\033[1;34m"

namespace o2
{
namespace tof
{

Compressor::Compressor()
{
}

Compressor::~Compressor()
{
  if (mDecoderBuffer && mOwnDecoderBuffer)
    delete[] mDecoderBuffer;
  if (mEncoderBuffer && mOwnDecoderBuffer)
    delete[] mEncoderBuffer;
}

bool Compressor::init()
{
  if (decoderInit())
    return true;
  if (encoderInit())
    return true;
  return false;
}

bool Compressor::open(std::string inFileName, std::string outFileName)
{
  if (decoderOpen(inFileName))
    return true;
  if (encoderOpen(outFileName))
    return true;
  return false;
}

bool Compressor::close()
{
  if (decoderClose())
    return true;
  if (encoderClose())
    return true;
  return false;
}

bool Compressor::decoderInit()
{
#ifdef DECODER_VERBOSE
  if (mDecoderVerbose) {
    std::cout << colorBlue
              << "--- INITIALISE DECODER BUFFER: " << mDecoderBufferSize << " bytes"
              << colorReset
              << std::endl;
  }
#endif
  if (mDecoderBuffer && mOwnDecoderBuffer) {
    std::cout << colorYellow
              << "-W- a buffer was already allocated, cleaning"
              << colorReset
              << std::endl;
    delete[] mDecoderBuffer;
  }
  mDecoderBuffer = new char[mDecoderBufferSize];
  mOwnDecoderBuffer = true;
  return false;
}

bool Compressor::encoderInit()
{
#ifdef ENCODER_VERBOSE
  if (mEncoderVerbose) {
    std::cout << colorBlue
              << "--- INITIALISE ENCODER BUFFER: " << mEncoderBufferSize << " bytes"
              << colorReset
              << std::endl;
  }
#endif
  if (mEncoderBuffer && mOwnEncoderBuffer) {
    std::cout << colorYellow
              << "-W- a buffer was already allocated, cleaning"
              << colorReset
              << std::endl;
    delete[] mEncoderBuffer;
  }
  mEncoderBuffer = new char[mEncoderBufferSize];
  mOwnEncoderBuffer = true;
  encoderRewind();
  return false;
}

bool Compressor::decoderOpen(std::string name)
{
  if (mDecoderFile.is_open()) {
    std::cout << colorYellow
              << "-W- a file was already open, closing"
              << colorReset
              << std::endl;
    mDecoderFile.close();
  }
  mDecoderFile.open(name.c_str(), std::fstream::in | std::fstream::binary);
  if (!mDecoderFile.is_open()) {
    std::cerr << colorRed
              << "-E- Cannot open input file: " << name
              << colorReset
              << std::endl;
    return true;
  }
  return false;
}

bool Compressor::encoderOpen(std::string name)
{
  if (mEncoderFile.is_open()) {
    std::cout << colorYellow
              << "-W- a file was already open, closing"
              << colorReset
              << std::endl;
    mEncoderFile.close();
  }
  mEncoderFile.open(name.c_str(), std::fstream::out | std::fstream::binary);
  if (!mEncoderFile.is_open()) {
    std::cerr << colorRed << "-E- Cannot open output file: " << name
              << colorReset
              << std::endl;
    return true;
  }
  return false;
}

bool Compressor::decoderClose()
{
  if (mDecoderFile.is_open()) {
    mDecoderFile.close();
    return false;
  }
  return true;
}

bool Compressor::encoderClose()
{
  if (mEncoderFile.is_open())
    mEncoderFile.close();
  return false;
}

bool Compressor::decoderRead()
{
  if (!mDecoderFile.is_open()) {
    std::cout << colorRed << "-E- no input file is open"
              << colorReset
              << std::endl;
    return true;
  }
  mDecoderFile.read(mDecoderBuffer, mDecoderBufferSize);
  decoderRewind();
  if (!mDecoderFile) {
    std::cout << colorRed << "--- Nothing else to read"
              << colorReset
              << std::endl;
    return true;
  }
#ifdef DECODER_VERBOSE
  if (mDecoderVerbose) {
    std::cout << colorBlue
              << "--- DECODER READ PAGE: " << mDecoderBufferSize << " bytes"
              << colorReset
              << std::endl;
  }
#endif
  return false;
}

bool Compressor::encoderWrite()
{
#ifdef ENCODER_VERBOSE
  if (mEncoderVerbose) {
    std::cout << colorBlue
              << "--- ENCODER WRITE BUFFER: " << getEncoderByteCounter() << " bytes"
              << colorReset
              << std::endl;
  }
#endif
  mEncoderFile.write(mEncoderBuffer, getEncoderByteCounter());
  encoderRewind();
  return false;
}

void Compressor::decoderClear()
{
  mRawSummary.DRMCommonHeader = 0x0;
  mRawSummary.DRMOrbitHeader = 0x0;
  mRawSummary.DRMGlobalHeader = 0x0;
  mRawSummary.DRMStatusHeader1 = 0x0;
  mRawSummary.DRMStatusHeader2 = 0x0;
  mRawSummary.DRMStatusHeader3 = 0x0;
  mRawSummary.DRMStatusHeader4 = 0x0;
  mRawSummary.DRMStatusHeader5 = 0x0;
  mRawSummary.DRMGlobalTrailer = 0x0;
  for (int itrm = 0; itrm < 10; itrm++) {
    mRawSummary.TRMGlobalHeader[itrm] = 0x0;
    mRawSummary.TRMGlobalTrailer[itrm] = 0x0;
    mRawSummary.HasHits[itrm] = false;
    for (int ichain = 0; ichain < 2; ichain++) {
      mRawSummary.TRMChainHeader[itrm][ichain] = 0x0;
      mRawSummary.TRMChainTrailer[itrm][ichain] = 0x0;
      mRawSummary.HasErrors[itrm][ichain] = false;
    }
  }
}

bool Compressor::processRDH()
{

#ifdef DECODER_VERBOSE
  if (mDecoderVerbose) {
    std::cout << colorBlue
              << "--- DECODE RDH"
              << colorReset
              << std::endl;
  }
#endif

  mDecoderRDH = reinterpret_cast<o2::header::RAWDataHeader*>(mDecoderBuffer);
  mEncoderRDH = reinterpret_cast<o2::header::RAWDataHeader*>(mEncoderBuffer);
  uint64_t HeaderSize = mDecoderRDH->headerSize;
  uint64_t MemorySize = mDecoderRDH->memorySize;

  /** copy RDH to encoder buffer **/
  std::memcpy(mEncoderRDH, mDecoderRDH, HeaderSize);

  /** move pointers after RDH **/
  mDecoderPointer = reinterpret_cast<uint32_t*>(mDecoderBuffer + HeaderSize);
  mEncoderPointer = reinterpret_cast<uint32_t*>(mEncoderBuffer + HeaderSize);

#ifdef DECODER_VERBOSE
  if (mDecoderVerbose) {
    printf(" %016x RDH Word0      (HeaderSize=%d, BlockLength=%d) \n", mDecoderRDH->word0,
           mDecoderRDH->headerSize, mDecoderRDH->blockLength);

    printf(" %016x RDH Word1      (OffsetToNext=%d, MemorySize=%d, LinkID=%d, PacketCounter=%d) \n", mDecoderRDH->word1,
           mDecoderRDH->offsetToNext, mDecoderRDH->memorySize, mDecoderRDH->linkID, mDecoderRDH->packetCounter);

    printf(" %016x RDH Word2      (TriggerOrbit=%d, HeartbeatOrbit=%d) \n", mDecoderRDH->word2,
           mDecoderRDH->triggerOrbit, mDecoderRDH->heartbeatOrbit);

    printf(" %016x RDH Word3 \n", mDecoderRDH->word3);

    printf(" %016x RDH Word4      (TriggerBC=%d, HeartbeatBC=%d, TriggerType=%d) \n", mDecoderRDH->word4,
           mDecoderRDH->triggerBC, mDecoderRDH->heartbeatBC, mDecoderRDH->triggerType);

    printf(" %016x RDH Word5 \n", mDecoderRDH->word5);

    printf(" %016x RDH Word6 \n", mDecoderRDH->word6);

    printf(" %016x RDH Word7 \n", mDecoderRDH->word7);
  }
#endif

  if (MemorySize <= HeaderSize)
    return true;
  return false;
}

bool Compressor::processDRM()
{
  /** check if we have memory to decode **/
  if (getDecoderByteCounter() >= mDecoderRDH->memorySize) {
#ifdef DECODER_VERBOSE
    if (mDecoderVerbose) {
      std::cout << colorYellow
                << "-W- decode request exceeds memory size: "
                << (void*)mDecoderPointer << " | " << (void*)mDecoderBuffer << " | " << mDecoderRDH->memorySize
                << colorReset
                << std::endl;
    }
#endif
    return true;
  }

#ifdef DECODER_VERBOSE
  if (mDecoderVerbose) {
    std::cout << colorBlue << "--- DECODE EVENT"
              << colorReset
              << std::endl;
  }
#endif

  /** init decoder **/
  auto start = std::chrono::high_resolution_clock::now();
  mDecoderNextWord = 1;
  decoderClear();

  /** check DRM Common Header **/
  if (!IS_DRM_COMMON_HEADER(*mDecoderPointer)) {
#ifdef DECODER_VERBOSE
    printf("%s %08x [ERROR] fatal error %s \n", colorRed, *mDecoderPointer, colorReset);
#endif
    return true;
  }
  mRawSummary.DRMCommonHeader = *mDecoderPointer;
#ifdef DECODER_VERBOSE
  if (mDecoderVerbose) {
    auto DRMCommonHeader = reinterpret_cast<raw::DRMCommonHeader_t*>(mDecoderPointer);
    auto Payload = DRMCommonHeader->payload;
    printf(" %08x DRM Common Header     (Payload=%d) \n", *mDecoderPointer, Payload);
  }
#endif
  decoderNext();

  /** DRM Orbit Header **/
  mRawSummary.DRMOrbitHeader = *mDecoderPointer;
#ifdef DECODER_VERBOSE
  if (mDecoderVerbose) {
    auto DRMOrbitHeader = reinterpret_cast<raw::DRMOrbitHeader_t*>(mDecoderPointer);
    auto Orbit = DRMOrbitHeader->orbit;
    printf(" %08x DRM Orbit Header      (Orbit=%d) \n", *mDecoderPointer, Orbit);
  }
#endif
  decoderNext();
  /** check DRM Global Header **/
  if (!IS_DRM_GLOBAL_HEADER(*mDecoderPointer)) {
#ifdef DECODER_VERBOSE
    printf("%s %08x [ERROR] fatal error %s \n", colorRed, *mDecoderPointer, colorReset);
#endif
    return true;
  }
  mRawSummary.DRMGlobalHeader = *mDecoderPointer;
#ifdef DECODER_VERBOSE
  if (mDecoderVerbose) {
    auto DRMGlobalHeader = reinterpret_cast<raw::DRMGlobalHeader_t*>(mDecoderPointer);
    auto DRMID = DRMGlobalHeader->drmID;
    printf(" %08x DRM Global Header     (DRMID=%d) \n", *mDecoderPointer, DRMID);
  }
#endif
#ifdef ALLOW_DRMID
  if (mDRM != -1 && GET_DRMGLOBALHEADER_DRMID(*mDecoderPointer) != mDRM)
    return true;
#endif
  decoderNext();

  /** DRM Status Header 1 **/
  mRawSummary.DRMStatusHeader1 = *mDecoderPointer;
#ifdef DECODER_VERBOSE
  if (mDecoderVerbose) {
    auto DRMStatusHeader1 = reinterpret_cast<raw::DRMStatusHeader1_t*>(mDecoderPointer);
    auto ParticipatingSlotID = DRMStatusHeader1->participatingSlotID;
    auto CBit = DRMStatusHeader1->cBit;
    auto DRMhSize = DRMStatusHeader1->drmHSize;
    printf(" %08x DRM Status Header 1   (ParticipatingSlotID=0x%03x, CBit=%d, DRMhSize=%d) \n", *mDecoderPointer, ParticipatingSlotID, CBit, DRMhSize);
  }
#endif
  decoderNext();

  /** DRM Status Header 2 **/
  mRawSummary.DRMStatusHeader2 = *mDecoderPointer;
#ifdef DECODER_VERBOSE
  if (mDecoderVerbose) {
    auto DRMStatusHeader2 = reinterpret_cast<raw::DRMStatusHeader2_t*>(mDecoderPointer);
    auto SlotEnableMask = DRMStatusHeader2->slotEnableMask;
    auto FaultID = DRMStatusHeader2->faultID;
    auto RTOBit = DRMStatusHeader2->rtoBit;
    printf(" %08x DRM Status Header 2   (SlotEnableMask=0x%03x, FaultID=%d, RTOBit=%d) \n", *mDecoderPointer, SlotEnableMask, FaultID, RTOBit);
  }
#endif
  decoderNext();

  /** DRM Status Header 3 **/
  mRawSummary.DRMStatusHeader3 = *mDecoderPointer;
#ifdef DECODER_VERBOSE
  if (mDecoderVerbose) {
    auto DRMStatusHeader3 = reinterpret_cast<raw::DRMStatusHeader3_t*>(mDecoderPointer);
    auto L0BCID = DRMStatusHeader3->l0BunchID;
    auto RunTimeInfo = DRMStatusHeader3->runTimeInfo;
    printf(" %08x DRM Status Header 3   (L0BCID=%d, RunTimeInfo=0x%03x) \n", *mDecoderPointer, L0BCID, RunTimeInfo);
  }
#endif
  decoderNext();

  /** DRM Status Header 4 **/
  mRawSummary.DRMStatusHeader4 = *mDecoderPointer;
#ifdef DECODER_VERBOSE
  if (mDecoderVerbose) {
    printf(" %08x DRM Status Header 4 \n", *mDecoderPointer);
  }
#endif
  decoderNext();

  /** DRM Status Header 5 **/
  mRawSummary.DRMStatusHeader5 = *mDecoderPointer;
#ifdef DECODER_VERBOSE
  if (mDecoderVerbose) {
    printf(" %08x DRM Status Header 5 \n", *mDecoderPointer);
  }
#endif
  decoderNext();

  /** encode Crate Header **/
  *mEncoderPointer = 0x80000000;
  *mEncoderPointer |= GET_DRMSTATUSHEADER2_SLOTENABLEMASK(mRawSummary.DRMStatusHeader2) << 12;
  *mEncoderPointer |= GET_DRMGLOBALHEADER_DRMID(mRawSummary.DRMGlobalHeader) << 24;
  *mEncoderPointer |= GET_DRMSTATUSHEADER3_L0BCID(mRawSummary.DRMStatusHeader3);
#ifdef ENCODER_VERBOSE
  if (mEncoderVerbose) {
    auto CrateHeader = reinterpret_cast<compressed::CrateHeader_t*>(mEncoderPointer);
    auto BunchID = CrateHeader->bunchID;
    auto DRMID = CrateHeader->drmID;
    auto SlotEnableMask = CrateHeader->slotEnableMask;
    printf("%s %08x Crate header          (DRMID=%d, BunchID=%d, SlotEnableMask=0x%x) %s \n", colorGreen, *mEncoderPointer, DRMID, BunchID, SlotEnableMask, colorReset);
  }
#endif
  encoderNext();

  /** encode Crate Orbit **/
  *mEncoderPointer = mRawSummary.DRMOrbitHeader;
#ifdef ENCODER_VERBOSE
  if (mEncoderVerbose) {
    auto CrateOrbit = reinterpret_cast<compressed::CrateOrbit_t*>(mEncoderPointer);
    auto OrbitID = CrateOrbit->orbitID;
    printf("%s %08x Crate orbit           (OrbitID=%d) %s \n", colorGreen, *mEncoderPointer, OrbitID, colorReset);
  }
#endif
  encoderNext();

  /** loop over DRM payload **/
  while (true) {

    /** LTM global header detected **/
    if (IS_LTM_GLOBAL_HEADER(*mDecoderPointer)) {

#ifdef DECODER_VERBOSE
      if (mDecoderVerbose) {
        printf(" %08x LTM Global Header \n", *mDecoderPointer);
      }
#endif
      decoderNext();

      /** loop over LTM payload **/
      while (true) {

        /** LTM global trailer detected **/
        if (IS_LTM_GLOBAL_TRAILER(*mDecoderPointer)) {
#ifdef DECODER_VERBOSE
          if (mDecoderVerbose) {
            printf(" %08x LTM Global Trailer \n", *mDecoderPointer);
          }
#endif
          decoderNext();
          break;
        }

#ifdef DECODER_VERBOSE
        if (mDecoderVerbose) {
          printf(" %08x LTM data \n", *mDecoderPointer);
        }
#endif
        decoderNext();
      }
    }

    /** TRM global header detected **/
    if (IS_TRM_GLOBAL_HEADER(*mDecoderPointer) && GET_TRMGLOBALHEADER_SLOTID(*mDecoderPointer) > 2) {
      uint32_t SlotID = GET_TRMGLOBALHEADER_SLOTID(*mDecoderPointer);
      int itrm = SlotID - 3;
      mRawSummary.TRMGlobalHeader[itrm] = *mDecoderPointer;
#ifdef DECODER_VERBOSE
      if (mDecoderVerbose) {
        auto TRMGlobalHeader = reinterpret_cast<raw::TRMGlobalHeader_t*>(mDecoderPointer);
        auto EventWords = TRMGlobalHeader->eventWords;
        auto EventNumber = TRMGlobalHeader->eventNumber;
        auto EBit = TRMGlobalHeader->eBit;
        printf(" %08x TRM Global Header     (SlotID=%d, EventWords=%d, EventNumber=%d, EBit=%01x) \n", *mDecoderPointer, SlotID, EventWords, EventNumber, EBit);
      }
#endif
      decoderNext();

      /** loop over TRM payload **/
      while (true) {

        /** TRM chain-A header detected **/
        if (IS_TRM_CHAINA_HEADER(*mDecoderPointer) && GET_TRMCHAINHEADER_SLOTID(*mDecoderPointer) == SlotID) {
          int ichain = 0;
          mRawSummary.TRMChainHeader[itrm][ichain] = *mDecoderPointer;
#ifdef DECODER_VERBOSE
          if (mDecoderVerbose) {
            auto TRMChainHeader = reinterpret_cast<raw::TRMChainHeader_t*>(mDecoderPointer);
            auto BunchID = TRMChainHeader->bunchID;
            printf(" %08x TRM Chain-A Header    (SlotID=%d, BunchID=%d) \n", *mDecoderPointer, SlotID, BunchID);
          }
#endif
          decoderNext();

          /** loop over TRM chain-A payload **/
          while (true) {

            /** TDC hit detected **/
            if (IS_TDC_HIT(*mDecoderPointer)) {
              mRawSummary.HasHits[itrm] = true;
              auto itdc = GET_TDCHIT_TDCID(*mDecoderPointer);
              auto ihit = mRawSummary.nTDCUnpackedHits[ichain][itdc];
              mRawSummary.TDCUnpackedHit[ichain][itdc][ihit] = *mDecoderPointer;
              mRawSummary.nTDCUnpackedHits[ichain][itdc]++;
#ifdef DECODER_VERBOSE
              if (mDecoderVerbose) {
                auto TDCUnpackedHit = reinterpret_cast<raw::TDCUnpackedHit_t*>(mDecoderPointer);
                auto HitTime = TDCUnpackedHit->hitTime;
                auto Chan = TDCUnpackedHit->chan;
                auto TDCID = TDCUnpackedHit->tdcID;
                auto EBit = TDCUnpackedHit->eBit;
                auto PSBits = TDCUnpackedHit->psBits;
                printf(" %08x TDC Hit               (HitTime=%d, Chan=%d, TDCID=%d, EBit=%d, PSBits=%d \n", *mDecoderPointer, HitTime, Chan, TDCID, EBit, PSBits);
              }
#endif
              decoderNext();
              continue;
            }

            /** TDC error detected **/
            if (IS_TDC_ERROR(*mDecoderPointer)) {
              mRawSummary.HasErrors[itrm][ichain] = true;
#ifdef DECODER_VERBOSE
              if (mDecoderVerbose) {
                printf("%s %08x TDC error %s \n", colorRed, *mDecoderPointer, colorReset);
              }
#endif
              decoderNext();
              continue;
            }

            /** TRM chain-A trailer detected **/
            if (IS_TRM_CHAINA_TRAILER(*mDecoderPointer)) {
              mRawSummary.TRMChainTrailer[itrm][ichain] = *mDecoderPointer;
#ifdef DECODER_VERBOSE
              if (mDecoderVerbose) {
                auto TRMChainTrailer = reinterpret_cast<raw::TRMChainTrailer_t*>(mDecoderPointer);
                auto EventCounter = TRMChainTrailer->eventCounter;
                printf(" %08x TRM Chain-A Trailer   (SlotID=%d, EventCounter=%d) \n", *mDecoderPointer, SlotID, EventCounter);
              }
#endif
              decoderNext();
              break;
            }

#ifdef DECODER_VERBOSE
            if (mDecoderVerbose) {
              printf("%s %08x [ERROR] breaking TRM Chain-A decode stream %s \n", colorRed, *mDecoderPointer, colorReset);
            }
#endif
            decoderNext();
            break;
          }
        } /** end of loop over TRM chain-A payload **/

        /** TRM chain-B header detected **/
        if (IS_TRM_CHAINB_HEADER(*mDecoderPointer) && GET_TRMCHAINHEADER_SLOTID(*mDecoderPointer) == SlotID) {
          int ichain = 1;
          mRawSummary.TRMChainHeader[itrm][ichain] = *mDecoderPointer;
#ifdef DECODER_VERBOSE
          if (mDecoderVerbose) {
            auto TRMChainHeader = reinterpret_cast<raw::TRMChainHeader_t*>(mDecoderPointer);
            auto BunchID = TRMChainHeader->bunchID;
            printf(" %08x TRM Chain-B Header    (SlotID=%d, BunchID=%d) \n", *mDecoderPointer, SlotID, BunchID);
          }
#endif
          decoderNext();

          /** loop over TRM chain-B payload **/
          while (true) {

            /** TDC hit detected **/
            if (IS_TDC_HIT(*mDecoderPointer)) {
              mRawSummary.HasHits[itrm] = true;
              auto itdc = GET_TDCHIT_TDCID(*mDecoderPointer);
              auto ihit = mRawSummary.nTDCUnpackedHits[ichain][itdc];
              mRawSummary.TDCUnpackedHit[ichain][itdc][ihit] = *mDecoderPointer;
              mRawSummary.nTDCUnpackedHits[ichain][itdc]++;
#ifdef DECODER_VERBOSE
              if (mDecoderVerbose) {
                auto TDCUnpackedHit = reinterpret_cast<raw::TDCUnpackedHit_t*>(mDecoderPointer);
                auto HitTime = TDCUnpackedHit->hitTime;
                auto Chan = TDCUnpackedHit->chan;
                auto TDCID = TDCUnpackedHit->tdcID;
                auto EBit = TDCUnpackedHit->eBit;
                auto PSBits = TDCUnpackedHit->psBits;
                printf(" %08x TDC Hit               (HitTime=%d, Chan=%d, TDCID=%d, EBit=%d, PSBits=%d \n", *mDecoderPointer, HitTime, Chan, TDCID, EBit, PSBits);
              }
#endif
              decoderNext();
              continue;
            }

            /** TDC error detected **/
            if (IS_TDC_ERROR(*mDecoderPointer)) {
              mRawSummary.HasErrors[itrm][ichain] = true;
#ifdef DECODER_VERBOSE
              if (mDecoderVerbose) {
                printf("%s %08x TDC error %s \n", colorRed, *mDecoderPointer, colorReset);
              }
#endif
              decoderNext();
              continue;
            }

            /** TRM chain-B trailer detected **/
            if (IS_TRM_CHAINB_TRAILER(*mDecoderPointer)) {
              mRawSummary.TRMChainTrailer[itrm][ichain] = *mDecoderPointer;
#ifdef DECODER_VERBOSE
              if (mDecoderVerbose) {
                auto TRMChainTrailer = reinterpret_cast<raw::TRMChainTrailer_t*>(mDecoderPointer);
                auto EventCounter = TRMChainTrailer->eventCounter;
                printf(" %08x TRM Chain-B Trailer   (SlotID=%d, EventCounter=%d) \n", *mDecoderPointer, SlotID, EventCounter);
              }
#endif
              decoderNext();
              break;
            }

#ifdef DECODER_VERBOSE
            if (mDecoderVerbose) {
              printf("%s %08x [ERROR] breaking TRM Chain-B decode stream %s \n", colorRed, *mDecoderPointer, colorReset);
            }
#endif
            decoderNext();
            break;
          }
        } /** end of loop over TRM chain-A payload **/

        /** TRM global trailer detected **/
        if (IS_TRM_GLOBAL_TRAILER(*mDecoderPointer)) {
          mRawSummary.TRMGlobalTrailer[itrm] = *mDecoderPointer;
#ifdef DECODER_VERBOSE
          if (mDecoderVerbose) {
            auto TRMGlobalTrailer = reinterpret_cast<raw::TRMGlobalTrailer_t*>(mDecoderPointer);
            auto EventCRC = TRMGlobalTrailer->eventCRC;
            auto LBit = TRMGlobalTrailer->lBit;
            printf(" %08x TRM Global Trailer    (SlotID=%d, EventCRC=%d, LBit=%d) \n", *mDecoderPointer, SlotID, EventCRC, LBit);
          }
#endif
          decoderNext();

          /** encoder SPIDER **/
          if (mRawSummary.HasHits[itrm]) {

            encoderSPIDER();

            /** loop over frames **/
            for (int iframe = mRawSummary.FirstFilledFrame; iframe < mRawSummary.LastFilledFrame + 1; iframe++) {

              /** check if frame is empty **/
              if (mRawSummary.nFramePackedHits[iframe] == 0)
                continue;

              // encode Frame Header
              *mEncoderPointer = 0x00000000;
              *mEncoderPointer |= SlotID << 24;
              *mEncoderPointer |= iframe << 16;
              *mEncoderPointer |= mRawSummary.nFramePackedHits[iframe];
#ifdef ENCODER_VERBOSE
              if (mEncoderVerbose) {
                auto FrameHeader = reinterpret_cast<compressed::FrameHeader_t*>(mEncoderPointer);
                auto NumberOfHits = FrameHeader->numberOfHits;
                auto FrameID = FrameHeader->frameID;
                auto TRMID = FrameHeader->trmID;
                printf("%s %08x Frame header          (TRMID=%d, FrameID=%d, NumberOfHits=%d) %s \n", colorGreen, *mEncoderPointer, TRMID, FrameID, NumberOfHits, colorReset);
              }
#endif
              encoderNext();

              // packed hits
              for (int ihit = 0; ihit < mRawSummary.nFramePackedHits[iframe]; ++ihit) {
                *mEncoderPointer = mRawSummary.FramePackedHit[iframe][ihit];
#ifdef ENCODER_VERBOSE
                if (mEncoderVerbose) {
                  auto PackedHit = reinterpret_cast<compressed::PackedHit_t*>(mEncoderPointer);
                  auto Chain = PackedHit->chain;
                  auto TDCID = PackedHit->tdcID;
                  auto Channel = PackedHit->channel;
                  auto Time = PackedHit->time;
                  auto TOT = PackedHit->tot;
                  printf("%s %08x Packed hit            (Chain=%d, TDCID=%d, Channel=%d, Time=%d, TOT=%d) %s \n", colorGreen, *mEncoderPointer, Chain, TDCID, Channel, Time, TOT, colorReset);
                }
#endif
                encoderNext();
              }

              mRawSummary.nFramePackedHits[iframe] = 0;
            }
          }

          /** filler detected **/
          if (IS_FILLER(*mDecoderPointer)) {
#ifdef DECODER_VERBOSE
            if (mDecoderVerbose) {
              printf(" %08x Filler \n", *mDecoderPointer);
            }
#endif
            decoderNext();
          }

          break;
        }

#ifdef DECODER_VERBOSE
        if (mDecoderVerbose) {
          printf("%s %08x [ERROR] breaking TRM decode stream %s \n", colorRed, *mDecoderPointer, colorReset);
        }
#endif
        decoderNext();
        break;

      } /** end of loop over TRM payload **/

      continue;
    }

    /** DRM global trailer detected **/
    if (IS_DRM_GLOBAL_TRAILER(*mDecoderPointer)) {
      mRawSummary.DRMGlobalTrailer = *mDecoderPointer;
#ifdef DECODER_VERBOSE
      if (mDecoderVerbose) {
        auto DRMGlobalTrailer = reinterpret_cast<raw::DRMGlobalTrailer_t*>(mDecoderPointer);
        auto LocalEventCounter = DRMGlobalTrailer->localEventCounter;
        printf(" %08x DRM Global Trailer    (LocalEventCounter=%d) \n", *mDecoderPointer, LocalEventCounter);
      }
#endif
      decoderNext();

      /** filler detected **/
      if (IS_FILLER(*mDecoderPointer)) {
#ifdef DECODER_VERBOSE
        if (mDecoderVerbose) {
          printf(" %08x Filler \n", *mDecoderPointer);
        }
#endif
        decoderNext();
      }

      /** check event **/
      checkerCheck();

      /** encode Crate Trailer **/
      *mEncoderPointer = 0x80000000;
      *mEncoderPointer |= mRawSummary.nDiagnosticWords;
      *mEncoderPointer |= GET_DRMGLOBALTRAILER_LOCALEVENTCOUNTER(mRawSummary.DRMGlobalTrailer) << 4;
#ifdef ENCODER_VERBOSE
      if (mEncoderVerbose) {
        auto CrateTrailer = reinterpret_cast<compressed::CrateTrailer_t*>(mEncoderPointer);
        auto EventCounter = CrateTrailer->eventCounter;
        auto NumberOfDiagnostics = CrateTrailer->numberOfDiagnostics;
        printf("%s %08x Crate trailer         (EventCounter=%d, NumberOfDiagnostics=%d) %s \n", colorGreen, *mEncoderPointer, EventCounter, NumberOfDiagnostics, colorReset);
      }
#endif
      encoderNext();

      /** encode Diagnostic Words **/
      for (int iword = 0; iword < mRawSummary.nDiagnosticWords; ++iword) {
        *mEncoderPointer = mRawSummary.DiagnosticWord[iword];
#ifdef ENCODER_VERBOSE
        if (mEncoderVerbose) {
          auto Diagnostic = reinterpret_cast<compressed::Diagnostic_t*>(mEncoderPointer);
          auto SlotID = Diagnostic->slotID;
          auto FaultBits = Diagnostic->faultBits;
          printf("%s %08x Diagnostic            (SlotID=%d, FaultBits=0x%x) %s \n", colorGreen, *mEncoderPointer, SlotID, FaultBits, colorReset);
        }
#endif
        encoderNext();
      }

      mRawSummary.nDiagnosticWords = 0;

      break;
    }

#ifdef DECODER_VERBOSE
    if (mDecoderVerbose) {
      printf("%s %08x [ERROR] trying to recover DRM decode stream %s \n", colorRed, *mDecoderPointer, colorReset);
    }
#endif
    decoderNext();

  } /** end of loop over DRM payload **/

  auto finish = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> elapsed = finish - start;

  mIntegratedBytes += getDecoderByteCounter();
  mIntegratedTime += elapsed.count();

  /** updated encoder RDH **/
  mEncoderRDH->memorySize = getEncoderByteCounter();
  mEncoderRDH->offsetToNext = getEncoderByteCounter();

#ifdef DECODER_VERBOSE
  if (mDecoderVerbose) {
    std::cout << colorBlue
              << "--- END DECODE EVENT: " << getDecoderByteCounter() << " bytes"
              << colorReset
              << std::endl;
  }
#endif

  return false;
}

void Compressor::encoderSPIDER()
{
  /** reset packed hits counter **/
  mRawSummary.FirstFilledFrame = 255;
  mRawSummary.LastFilledFrame = 0;

  /** loop over TRM chains **/
  for (int ichain = 0; ichain < 2; ++ichain) {

    /** loop over TDCs **/
    for (int itdc = 0; itdc < 15; ++itdc) {

      auto nhits = mRawSummary.nTDCUnpackedHits[ichain][itdc];
      if (nhits == 0)
        continue;

      /** loop over hits **/
      for (int ihit = 0; ihit < nhits; ++ihit) {

        auto lhit = mRawSummary.TDCUnpackedHit[ichain][itdc][ihit];
        if (GET_TDCHIT_PSBITS(lhit) != 0x1)
          continue; // must be a leading hit

        auto Chan = GET_TDCHIT_CHAN(lhit);
        auto HitTime = GET_TDCHIT_HITTIME(lhit);
        auto EBit = GET_TDCHIT_EBIT(lhit);
        uint32_t TOTWidth = 0;

        // check next hits for packing
        for (int jhit = ihit + 1; jhit < nhits; ++jhit) {
          auto thit = mRawSummary.TDCUnpackedHit[ichain][itdc][jhit];
          if (GET_TDCHIT_PSBITS(thit) == 0x2 && GET_TDCHIT_CHAN(thit) == Chan) {    // must be a trailing hit from same channel
            TOTWidth = (GET_TDCHIT_HITTIME(thit) - HitTime)/Geo::RATIO_TOT_TDC_BIN; // compute TOT
            lhit = 0x0;                                                             // mark as used
            break;
          }
        }

        auto iframe = HitTime >> 13;
        auto phit = mRawSummary.nFramePackedHits[iframe];

        mRawSummary.FramePackedHit[iframe][phit] = 0x00000000;
        mRawSummary.FramePackedHit[iframe][phit] |= (TOTWidth & 0x7FF) << 0;
        mRawSummary.FramePackedHit[iframe][phit] |= (HitTime & 0x1FFF) << 11;
        mRawSummary.FramePackedHit[iframe][phit] |= Chan << 24;
        mRawSummary.FramePackedHit[iframe][phit] |= itdc << 27;
        mRawSummary.FramePackedHit[iframe][phit] |= ichain << 31;
        mRawSummary.nFramePackedHits[iframe]++;

        if (iframe < mRawSummary.FirstFilledFrame)
          mRawSummary.FirstFilledFrame = iframe;
        if (iframe > mRawSummary.LastFilledFrame)
          mRawSummary.LastFilledFrame = iframe;
      }

      mRawSummary.nTDCUnpackedHits[ichain][itdc] = 0;
    }
  }
}

bool Compressor::checkerCheck()
{
  bool status = false;
  mRawSummary.nDiagnosticWords = 0;
  mRawSummary.DiagnosticWord[0] = 0x00000001;
  //  mRawSummary.CheckStatus = false;
  mCounter++;

  auto start = std::chrono::high_resolution_clock::now();

#ifdef CHECKER_VERBOSE
  if (mCheckerVerbose) {
    std::cout << colorBlue
              << "--- CHECK EVENT"
              << colorReset
              << std::endl;
  }
#endif

  /** increment check counter **/
  //    mCheckerCounter++;

  /** check DRM Global Header **/
  if (mRawSummary.DRMGlobalHeader == 0x0) {
    status = true;
    mRawSummary.DiagnosticWord[0] |= DIAGNOSTIC_DRM_HEADER;
    mRawSummary.nDiagnosticWords++;
#ifdef CHECKER_VERBOSE
    if (mCheckerVerbose) {
      printf(" Missing DRM Global Header \n");
    }
#endif
    return status;
  }

  /** check DRM Global Trailer **/
  if (mRawSummary.DRMGlobalTrailer == 0x0) {
    status = true;
    mRawSummary.DiagnosticWord[0] |= DIAGNOSTIC_DRM_TRAILER;
    mRawSummary.nDiagnosticWords++;
#ifdef CHECKER_VERBOSE
    if (mCheckerVerbose) {
      printf(" Missing DRM Global Trailer \n");
    }
#endif
    return status;
  }

  /** increment DRM header counter **/
  mDRMCounters.Headers++;

  /** get DRM relevant data **/
  uint32_t ParticipatingSlotID = GET_DRMSTATUSHEADER1_PARTICIPATINGSLOTID(mRawSummary.DRMStatusHeader1);
  uint32_t SlotEnableMask = GET_DRMSTATUSHEADER2_SLOTENABLEMASK(mRawSummary.DRMStatusHeader2);
  uint32_t L0BCID = GET_DRMSTATUSHEADER3_L0BCID(mRawSummary.DRMStatusHeader3);
  uint32_t LocalEventCounter = GET_DRMGLOBALTRAILER_LOCALEVENTCOUNTER(mRawSummary.DRMGlobalTrailer);

  if (ParticipatingSlotID != SlotEnableMask) {
#ifdef CHECKER_VERBOSE
    if (mCheckerVerbose) {
      printf(" Warning: enable/participating mask differ: %03x/%03x \n", SlotEnableMask, ParticipatingSlotID);
    }
#endif
    mRawSummary.DiagnosticWord[0] |= DIAGNOSTIC_DRM_ENABLEMASK;
  }

  /** check DRM CBit **/
  if (GET_DRMSTATUSHEADER1_CBIT(mRawSummary.DRMStatusHeader1)) {
    status = true;
    mDRMCounters.CBit++;
    mRawSummary.DiagnosticWord[0] |= DIAGNOSTIC_DRM_CBIT;
#ifdef CHECKER_VERBOSE
    if (mCheckerVerbose) {
      printf(" DRM CBit is on \n");
    }
#endif
  }

  /** check DRM FaultID **/
  if (GET_DRMSTATUSHEADER2_FAULTID(mRawSummary.DRMStatusHeader2)) {
    status = true;
    mDRMCounters.Fault++;
    mRawSummary.DiagnosticWord[0] |= DIAGNOSTIC_DRM_FAULTID;
#ifdef CHECKER_VERBOSE
    if (mCheckerVerbose) {
      printf(" DRM FaultID: %x \n", GET_DRMSTATUSHEADER2_FAULTID(mRawSummary.DRMStatusHeader2));
    }
#endif
  }

  /** check DRM RTOBit **/
  if (GET_DRMSTATUSHEADER2_RTOBIT(mRawSummary.DRMStatusHeader2)) {
    status = true;
    mDRMCounters.RTOBit++;
    mRawSummary.DiagnosticWord[0] |= DIAGNOSTIC_DRM_RTOBIT;
#ifdef CHECKER_VERBOSE
    if (mCheckerVerbose) {
      printf(" DRM RTOBit is on \n");
    }
#endif
  }

  /** loop over TRMs **/
  for (int itrm = 0; itrm < 10; ++itrm) {
    uint32_t SlotID = itrm + 3;
    uint32_t trmFaultBit = 1 << (1 + itrm * 3);

    /** check current diagnostic word **/
    auto iword = mRawSummary.nDiagnosticWords;
    if (mRawSummary.DiagnosticWord[iword] & 0xFFFFFFF0) {
      mRawSummary.nDiagnosticWords++;
      iword++;
    }

    /** set current slot id **/
    mRawSummary.DiagnosticWord[iword] = SlotID;

    /** check participating TRM **/
    if (!(ParticipatingSlotID & 1 << (itrm + 1))) {
      if (mRawSummary.TRMGlobalHeader[itrm] != 0x0) {
        status = true;
        mRawSummary.DiagnosticWord[iword] |= DIAGNOSTIC_TRM_UNEXPECTED;
#ifdef CHECKER_VERBOSE
        if (mCheckerVerbose) {
          printf(" Non-participating header found (SlotID=%d) \n", SlotID);
        }
#endif
      }
      continue;
    }

    /** check TRM Global Header **/
    if (mRawSummary.TRMGlobalHeader[itrm] == 0x0) {
      status = true;
      mRawSummary.DiagnosticWord[iword] |= DIAGNOSTIC_TRM_HEADER;
#ifdef CHECKER_VERBOSE
      if (mCheckerVerbose) {
        printf(" Missing TRM Header (SlotID=%d) \n", SlotID);
      }
#endif
      continue;
    }

    /** check TRM Global Trailer **/
    if (mRawSummary.TRMGlobalTrailer[itrm] == 0x0) {
      status = true;
      mRawSummary.DiagnosticWord[iword] |= DIAGNOSTIC_TRM_TRAILER;
#ifdef CHECKER_VERBOSE
      if (mCheckerVerbose) {
        printf(" Missing TRM Trailer (SlotID=%d) \n", SlotID);
      }
#endif
      continue;
    }

    /** increment TRM header counter **/
    mTRMCounters[itrm].Headers++;

    /** check TRM empty flag **/
    if (!mRawSummary.HasHits[itrm])
      mTRMCounters[itrm].Empty++;

    /** check TRM EventCounter **/
    uint32_t EventCounter = GET_TRMGLOBALHEADER_EVENTNUMBER(mRawSummary.TRMGlobalHeader[itrm]);
    if (EventCounter != LocalEventCounter % 1024) {
      status = true;
      mTRMCounters[itrm].EventCounterMismatch++;
      mRawSummary.DiagnosticWord[iword] |= DIAGNOSTIC_TRM_EVENTCOUNTER;
#ifdef CHECKER_VERBOSE
      if (mCheckerVerbose) {
        printf(" TRM EventCounter / DRM LocalEventCounter mismatch: %d / %d (SlotID=%d) \n", EventCounter, LocalEventCounter, SlotID);
      }
#endif
      continue;
    }

    /** check TRM EBit **/
    if (GET_TRMGLOBALHEADER_EBIT(mRawSummary.TRMGlobalHeader[itrm])) {
      status = true;
      mTRMCounters[itrm].EBit++;
      mRawSummary.DiagnosticWord[iword] |= DIAGNOSTIC_TRM_EBIT;
#ifdef CHECKER_VERBOSE
      if (mCheckerVerbose) {
        printf(" TRM EBit is on (SlotID=%d) \n", SlotID);
      }
#endif
    }

    /** loop over TRM chains **/
    for (int ichain = 0; ichain < 2; ichain++) {
      uint32_t chainFaultBit = trmFaultBit << (ichain + 1);

      /** check TRM Chain Header **/
      if (mRawSummary.TRMChainHeader[itrm][ichain] == 0x0) {
        status = true;
        mRawSummary.DiagnosticWord[iword] |= DIAGNOSTIC_TRMCHAIN_HEADER(ichain);
#ifdef CHECKER_VERBOSE
        if (mCheckerVerbose) {
          printf(" Missing TRM Chain Header (SlotID=%d, chain=%d) \n", SlotID, ichain);
        }
#endif
        continue;
      }

      /** check TRM Chain Trailer **/
      if (mRawSummary.TRMChainTrailer[itrm][ichain] == 0x0) {
        status = true;
        mRawSummary.DiagnosticWord[iword] |= DIAGNOSTIC_TRMCHAIN_TRAILER(ichain);
#ifdef CHECKER_VERBOSE
        if (mCheckerVerbose) {
          printf(" Missing TRM Chain Trailer (SlotID=%d, chain=%d) \n", SlotID, ichain);
        }
#endif
        continue;
      }

      /** increment TRM Chain header counter **/
      mTRMChainCounters[itrm][ichain].Headers++;

      /** check TDC errors **/
      if (mRawSummary.HasErrors[itrm][ichain]) {
        status = true;
        mTRMChainCounters[itrm][ichain].TDCerror++;
        mRawSummary.DiagnosticWord[iword] |= DIAGNOSTIC_TRMCHAIN_TDCERRORS(ichain);
#ifdef CHECKER_VERBOSE
        if (mCheckerVerbose) {
          printf(" TDC error detected (SlotID=%d, chain=%d) \n", SlotID, ichain);
        }
#endif
      }

      /** check TRM Chain EventCounter **/
      auto EventCounter = GET_TRMCHAINTRAILER_EVENTCOUNTER(mRawSummary.TRMChainTrailer[itrm][ichain]);
      if (EventCounter != LocalEventCounter) {
        status = true;
        mTRMChainCounters[itrm][ichain].EventCounterMismatch++;
        mRawSummary.DiagnosticWord[iword] |= DIAGNOSTIC_TRMCHAIN_EVENTCOUNTER(ichain);
#ifdef CHECKER_VERBOSE
        if (mCheckerVerbose) {
          printf(" TRM Chain EventCounter / DRM LocalEventCounter mismatch: %d / %d (SlotID=%d, chain=%d) \n", EventCounter, EventCounter, SlotID, ichain);
        }
#endif
      }

      /** check TRM Chain Status **/
      auto Status = GET_TRMCHAINTRAILER_STATUS(mRawSummary.TRMChainTrailer[itrm][ichain]);
      if (Status != 0) {
        status = true;
        mTRMChainCounters[itrm][ichain].BadStatus++;
        mRawSummary.DiagnosticWord[iword] |= DIAGNOSTIC_TRMCHAIN_STATUS(ichain);
#ifdef CHECKER_VERBOSE
        if (mCheckerVerbose) {
          printf(" TRM Chain bad Status: %d (SlotID=%d, chain=%d) \n", Status, SlotID, ichain);
        }
#endif
      }

      /** check TRM Chain BunchID **/
      uint32_t BunchID = GET_TRMCHAINHEADER_BUNCHID(mRawSummary.TRMChainHeader[itrm][ichain]);
      if (BunchID != L0BCID) {
        status = true;
        mTRMChainCounters[itrm][ichain].BunchIDMismatch++;
        mRawSummary.DiagnosticWord[iword] |= DIAGNOSTIC_TRMCHAIN_BUNCHID(ichain);
#ifdef CHECKER_VERBOSE
        if (mCheckerVerbose) {
          printf(" TRM Chain BunchID / DRM L0BCID mismatch: %d / %d (SlotID=%d, chain=%d) \n", BunchID, L0BCID, SlotID, ichain);
        }
#endif
      }

    } /** end of loop over TRM chains **/
  }   /** end of loop over TRMs **/

  /** check current diagnostic word **/
  auto iword = mRawSummary.nDiagnosticWords;
  if (mRawSummary.DiagnosticWord[iword] & 0xFFFFFFF0)
    mRawSummary.nDiagnosticWords++;

#ifdef CHECKER_VERBOSE
  if (mCheckerVerbose) {
    std::cout << colorBlue
              << "--- END CHECK EVENT: " << mRawSummary.nDiagnosticWords << " diagnostic words"
              << colorReset
              << std::endl;
  }
#endif

  auto finish = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> elapsed = finish - start;

  mIntegratedTime += elapsed.count();

  return status;
}

void Compressor::checkSummary()
{
  char chname[2] = {'a', 'b'};

  std::cout << colorBlue
            << "--- SUMMARY COUNTERS: " << mCounter << " events "
            << colorReset
            << std::endl;
  if (mCounter == 0)
    return;
  printf("\n");
  printf("    DRM ");
  float drmheaders = 100. * (float)mDRMCounters.Headers / (float)mCounter;
  printf("  \033%sheaders: %5.1f %%\033[0m ", drmheaders < 100. ? "[1;31m" : "[0m", drmheaders);
  if (mDRMCounters.Headers == 0)
    return;
  float cbit = 100. * (float)mDRMCounters.CBit / float(mDRMCounters.Headers);
  printf("     \033%sCbit: %5.1f %%\033[0m ", cbit > 0. ? "[1;31m" : "[0m", cbit);
  float fault = 100. * (float)mDRMCounters.Fault / float(mDRMCounters.Headers);
  printf("    \033%sfault: %5.1f %%\033[0m ", fault > 0. ? "[1;31m" : "[0m", cbit);
  float rtobit = 100. * (float)mDRMCounters.RTOBit / float(mDRMCounters.Headers);
  printf("   \033%sRTObit: %5.1f %%\033[0m ", rtobit > 0. ? "[1;31m" : "[0m", cbit);
  printf("\n");
  //      std::cout << "-----------------------------------------------------------" << std::endl;
  //      printf("    LTM | headers: %5.1f %% \n", 0.);
  for (int itrm = 0; itrm < 10; ++itrm) {
    printf("\n");
    printf(" %2d TRM ", itrm + 3);
    float trmheaders = 100. * (float)mTRMCounters[itrm].Headers / float(mDRMCounters.Headers);
    printf("  \033%sheaders: %5.1f %%\033[0m ", trmheaders < 100. ? "[1;31m" : "[0m", trmheaders);
    if (mTRMCounters[itrm].Headers == 0.) {
      printf("\n");
      continue;
    }
    float empty = 100. * (float)mTRMCounters[itrm].Empty / (float)mTRMCounters[itrm].Headers;
    printf("    \033%sempty: %5.1f %%\033[0m ", empty > 0. ? "[1;31m" : "[0m", empty);
    float evCount = 100. * (float)mTRMCounters[itrm].EventCounterMismatch / (float)mTRMCounters[itrm].Headers;
    printf("  \033%sevCount: %5.1f %%\033[0m ", evCount > 0. ? "[1;31m" : "[0m", evCount);
    float ebit = 100. * (float)mTRMCounters[itrm].EBit / (float)mTRMCounters[itrm].Headers;
    printf("     \033%sEbit: %5.1f %%\033[0m ", ebit > 0. ? "[1;31m" : "[0m", ebit);
    printf(" \n");
    for (int ichain = 0; ichain < 2; ++ichain) {
      printf("      %c ", chname[ichain]);
      float chainheaders = 100. * (float)mTRMChainCounters[itrm][ichain].Headers / (float)mTRMCounters[itrm].Headers;
      printf("  \033%sheaders: %5.1f %%\033[0m ", chainheaders < 100. ? "[1;31m" : "[0m", chainheaders);
      if (mTRMChainCounters[itrm][ichain].Headers == 0) {
        printf("\n");
        continue;
      }
      float status = 100. * mTRMChainCounters[itrm][ichain].BadStatus / (float)mTRMChainCounters[itrm][ichain].Headers;
      printf("   \033%sstatus: %5.1f %%\033[0m ", status > 0. ? "[1;31m" : "[0m", status);
      float bcid = 100. * mTRMChainCounters[itrm][ichain].BunchIDMismatch / (float)mTRMChainCounters[itrm][ichain].Headers;
      printf("     \033%sbcID: %5.1f %%\033[0m ", bcid > 0. ? "[1;31m" : "[0m", bcid);
      float tdcerr = 100. * mTRMChainCounters[itrm][ichain].TDCerror / (float)mTRMChainCounters[itrm][ichain].Headers;
      printf("   \033%sTDCerr: %5.1f %%\033[0m ", tdcerr > 0. ? "[1;31m" : "[0m", tdcerr);
      printf("\n");
    }
  }
  printf("\n");
}

} // namespace tof
} // namespace o2
