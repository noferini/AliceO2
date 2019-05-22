set(MODULE_NAME "TOFWorkflow")
set(MODULE_BUCKET_NAME TOF_workflow_bucket)

O2_SETUP(NAME ${MODULE_NAME})

set(SRCS
  src/DigitReaderSpec.cxx
  src/TOFClusterizerSpec.cxx
  src/TOFClusterWriterSpec.cxx
  src/RecoWorkflowSpec.cxx
  )

set(HEADERS
#  include/ReconstructionDataFormats/MatchInfoTOF.h
  include/${MODULE_NAME}/RecoWorkflowSpec.h
  include/${MODULE_NAME}/DigitReaderSpec.h
  include/${MODULE_NAME}/TOFClusterizerSpec.h
  include/${MODULE_NAME}/TOFClusterWriterSpec.h
)

set(LINKDEF src/TOFWorkflowLinkDef.h)
set(LIBRARY_NAME ${MODULE_NAME})
set(BUCKET_NAME ${MODULE_BUCKET_NAME})

O2_GENERATE_LIBRARY()

O2_GENERATE_EXECUTABLE(
  EXE_NAME o2-tof-reco-workflow

  SOURCES
  src/tof-reco-workflow.cxx

  MODULE_LIBRARY_NAME ${LIBRARY_NAME}
  BUCKET_NAME ${BUCKET_NAME}
)

