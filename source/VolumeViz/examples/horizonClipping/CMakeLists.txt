add_oiv_link_libraries( ${VolumeViz_LIBRARIES} )

set(src_SOURCES
  horizonClipping.cxx
)

set(resources_SOURCES
  interface.iv
  scenegraph.iv
)

add_oiv_executable(horizonClipping
  ${src_SOURCES} 
  ${resources_SOURCES}
)

create_vcproj_hierarchy("Resource Files" ./${resources_SOURCES})
