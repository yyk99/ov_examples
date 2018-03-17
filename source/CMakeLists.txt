add_subdirectory(DialogViz)

if(OIV_ENABLE_DIRECTVIZ)
  add_subdirectory(DirectViz)
endif(OIV_ENABLE_DIRECTVIZ)

if(OIV_ENABLE_FXVIZ)
  add_subdirectory(FXViz)
endif(OIV_ENABLE_FXVIZ)

add_subdirectory(HardCopy)

add_subdirectory(Inventor)

if(OIV_ENABLE_IMAGEVIZ)
  add_subdirectory(ImageViz)
endif()

add_subdirectory(MeshViz)

if(OIV_ENABLE_MESHIVIZ)
  add_subdirectory(MeshVizXLM)
endif(OIV_ENABLE_MESHIVIZ)

if(OIV_ENABLE_MULTIPIPE)
  add_subdirectory(MultiPipe)
endif()

if(OIV_ENABLE_REMOTEVIZ)
  add_subdirectory(RemoteViz)
endif()

add_subdirectory(ScaleViz)
add_subdirectory(SolidViz)
add_subdirectory(TerrainViz)

add_subdirectory(Medical)

add_subdirectory(VolumeViz)

if(OIV_ENABLE_QT_DEMOS)
  add_subdirectory(IvTune)
endif()
