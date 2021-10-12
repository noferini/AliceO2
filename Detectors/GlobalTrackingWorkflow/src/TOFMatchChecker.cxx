// Copyright 2019-2020 CERN and copyright holders of ALICE O2.
// See https://alice-o2.web.cern.ch/copyright for details of the copyright holders.
// All rights not expressly granted are reserved.
//
// This software is distributed under the terms of the GNU General Public
// License v3 (GPL Version 3), copied verbatim in the file "COPYING".
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// @file   TOFMatchChecker.cxx

#include <vector>
#include <string>
#include "TStopwatch.h"
#include "Framework/ConfigParamRegistry.h"
#include "DetectorsBase/GeometryManager.h"
#include "DetectorsBase/Propagator.h"
#include "DetectorsCommonDataFormats/NameConf.h"
#include "DataFormatsParameters/GRPObject.h"
#include "CommonDataFormat/InteractionRecord.h"
#include "DataFormatsGlobalTracking/RecoContainer.h"
#include "DataFormatsGlobalTracking/RecoContainerCreateTracksVariadic.h"
#include "Framework/Task.h"
#include "Framework/DataProcessorSpec.h"

// from Tracks
#include "ReconstructionDataFormats/GlobalTrackID.h"
#include "ReconstructionDataFormats/GlobalTrackAccessor.h"
#include "ReconstructionDataFormats/GlobalTrackID.h"
#include "GlobalTracking/MatchTPCITS.h"
#include "DataFormatsTPC/TrackTPC.h"
#include "DataFormatsTRD/TrackTRD.h"

// from TOF
#include "DataFormatsTOF/Cluster.h"
//#include "GlobalTracking/MatchTOF.h"
#include "GlobalTrackingWorkflow/TOFMatchChecker.h"

using namespace o2::framework;
// using MCLabelsTr = gsl::span<const o2::MCCompLabel>;
// using GID = o2::dataformats::GlobalTrackID;
// using DetID = o2::detectors::DetID;

using evIdx = o2::dataformats::EvIndex<int, int>;
using MatchOutputType = std::vector<o2::dataformats::MatchInfoTOF>;
using GID = o2::dataformats::GlobalTrackID;

namespace o2
{
namespace globaltracking
{

class TOFMatchChecker : public Task
{
 public:
  TOFMatchChecker(std::shared_ptr<DataRequest> dr, bool useMC) : mDataRequest(dr), mUseMC(useMC) {}
  ~TOFMatchChecker() override = default;
  void init(InitContext& ic) final;
  void run(ProcessingContext& pc) final;
  void endOfStream(framework::EndOfStreamContext& ec) final;
  void checkMatching(GID gid);

 private:
  bool mIsTPC;
  bool mIsTPCTRD;
  bool mIsITSTPCTRD;
  bool mIsITSTPC;
  gsl::span<const o2::tof::Cluster> mTOFClustersArrayInp;     ///< input TOF clusters

  RecoContainer mRecoData;
  std::shared_ptr<DataRequest> mDataRequest;
  bool mUseMC = true;
  TStopwatch mTimer;
};

void TOFMatchChecker::checkMatching(GID gid){
  if(gid.getSource() == GID::TPCTRD){
    return;
  }
  if(gid.getSource() == GID::TPC){
    return;
  }
  if(gid.getSource() == GID::ITSTPCTRD){
    return;
  }
  if(gid.getSource() == GID::ITSTPC){
    return;
  }
  if(!mIsTPCTRD && gid.getSource() == GID::TPCTRDTOF){
    return;
  }
  if(!mIsTPC && gid.getSource() == GID::TPCTOF){
    return;
  }
  if(!mIsITSTPCTRD && gid.getSource() == GID::ITSTPCTRDTOF){
    return;
  }
  if(!mIsITSTPC && gid.getSource() == GID::ITSTPCTOF){
    return;
  }

  const o2::dataformats::MatchInfoTOF& match =  mRecoData.getTOFMatch(gid);

  int tofcl = match.getIdxTOFCl();
  int trIndex = match.getTrackIndex();
  float chi2 = match.getChi2();
  float ttof = mTOFClustersArrayInp[tofcl].getTime();
  float x = mTOFClustersArrayInp[tofcl].getX();
  float y = mTOFClustersArrayInp[tofcl].getY();
  float z = mTOFClustersArrayInp[tofcl].getZ();
  int sector = mTOFClustersArrayInp[tofcl].getSector();
  LOG(INFO) << "cl=" << tofcl << " - trk=" << trIndex << " - chi2 =" << chi2 << " - time=" << ttof << " - coordinates (to be rotated, sector=" << sector<< ") = (" << x << "," << y << "," << z << ")";
}

void TOFMatchChecker::init(InitContext& ic)
{
  mTimer.Stop();
  mTimer.Reset();
  //-------- init geometry and field --------//
  o2::base::GeometryManager::loadGeometry();
  o2::base::Propagator::initFieldFromGRP();
  std::unique_ptr<o2::parameters::GRPObject> grp{o2::parameters::GRPObject::loadFrom()};
}

void TOFMatchChecker::run(ProcessingContext& pc)
{
  mTimer.Start(false);

  mRecoData.collectData(pc, *mDataRequest.get());

  mIsTPC = (mRecoData.isTrackSourceLoaded(o2::dataformats::GlobalTrackID::Source::TPCTOF) && mRecoData.isMatchSourceLoaded(o2::dataformats::GlobalTrackID::Source::TPCTOF));
  mIsITSTPC = (mRecoData.isTrackSourceLoaded(o2::dataformats::GlobalTrackID::Source::ITSTPCTOF) && mRecoData.isMatchSourceLoaded(o2::dataformats::GlobalTrackID::Source::ITSTPCTOF));
  mIsITSTPCTRD = (mRecoData.isTrackSourceLoaded(o2::dataformats::GlobalTrackID::Source::ITSTPCTRDTOF) && mRecoData.isMatchSourceLoaded(o2::dataformats::GlobalTrackID::Source::ITSTPCTRDTOF));
  mIsTPCTRD = (mRecoData.isTrackSourceLoaded(o2::dataformats::GlobalTrackID::Source::TPCTRDTOF) && mRecoData.isMatchSourceLoaded(o2::dataformats::GlobalTrackID::Source::TPCTRDTOF));

  mTOFClustersArrayInp = mRecoData.getTOFClusters();

  LOG(INFO) << "isTrackSourceLoaded: TPC -> " << mIsTPC;
  LOG(INFO) << "isTrackSourceLoaded: ITSTPC -> " << mIsITSTPC;
  LOG(INFO) << "isTrackSourceLoaded: TPCTRD -> " << mIsTPCTRD;
  LOG(INFO) << "isTrackSourceLoaded: ITSTPCTRD -> " << mIsITSTPCTRD;
  LOG(INFO) << "TOF cluster size = " << mTOFClustersArrayInp.size();

  if(!mTOFClustersArrayInp.size()){
    return;
  }

  auto creator = [this](auto& trk, GID gid, float time0, float terr) {
    this->checkMatching(gid);
    return true;
  };
  mRecoData.createTracksVariadic(creator);

  mTimer.Stop();
}

void TOFMatchChecker::endOfStream(EndOfStreamContext& ec)
{
  LOGF(INFO, "TOF matching total timing: Cpu: %.3e Real: %.3e s in %d slots",
       mTimer.CpuTime(), mTimer.RealTime(), mTimer.Counter() - 1);
}

DataProcessorSpec getTOFMatchCheckerSpec(GID::mask_t src, bool useMC)
{
  auto dataRequest = std::make_shared<DataRequest>();

  // request TOF clusters
  dataRequest->requestTracks(src, useMC);
  dataRequest->requestClusters(GID::getSourceMask(GID::TOF), useMC);
  dataRequest->requestTOFMatches(useMC);

  return DataProcessorSpec{
    "tof-matcher",
    dataRequest->inputs,
    {},
    AlgorithmSpec{adaptFromTask<TOFMatchChecker>(dataRequest, useMC)},
    Options{}};
}

} // namespace globaltracking
} // namespace o2
