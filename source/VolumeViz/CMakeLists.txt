include(CMakeDependentOption)

project(VolumeVizExamples)

# Show this option only in OIV_DEV_BUILD
# set OIV_VOLUMEVIZ_CONFIGURATION to FALSE to avoid running VolumeViz ARTs in all configurations
cmake_dependent_option(OIV_VOLUMEVIZ_CONFIGURATION "Execute VolumeViz ARTs in all configurations (raycasting, slicing, slabbing...)" TRUE "OIV_DEV_BUILD" TRUE)

set(OIV_MODULE VolumeViz)

if (NOT OIV_LOCAL_PROJECT AND OIV_VOLUMEVIZ_CONFIGURATION)
  ADD_TEST_CONF("CONF_RAYCASTING_OFF")
  ADD_TEST_CONF("CONF_SLABBING_ON")
endif()

add_oiv_link_libraries( "${LDM_LIBRARIES}" )
add_oiv_link_libraries( "${VolumeViz_LIBRARIES}" )
add_oiv_link_libraries( "${OpenInventor_LIBRARIES}" )
add_oiv_link_libraries( "${OpenInventorGui_LIBRARIES}" )
add_oiv_link_libraries( "${OpenInventorBase_LIBRARIES}" )
add_oiv_link_libraries( "${ScaleViz_LIBRARIES}" )

if(APPLE)
  add_oiv_link_libraries( "${OpenInventorQt_LIBRARIES}" )
  add_oiv_link_libraries( "${QT_LIBRARIES}" )
  add_oiv_link_libraries( "${DialogViz_LIBRARIES}" )
else()
  add_oiv_link_libraries( "${OpenInventorXt_LIBRARIES}" )
  add_oiv_link_libraries( "${OpenInventorWin_LIBRARIES}" )
  add_oiv_link_libraries( "${DialogViz_LIBRARIES}" )
endif()

if(NOT WIN32 AND NOT APPLE)
 add_oiv_link_libraries( Xm )
 add_oiv_link_libraries( Xt )
endif()

if(APPLE)
  set(subdirectories
   contrib
   Converters
   examples
   VolRend
   LDMWriter
   Machining
   tools
  )
else(APPLE)
  set(subdirectories
   contrib
   Compute
   Converters
   examples
   VolRend
   LDMWriter
   Machining
   tools
  )
endif(APPLE)


foreach(subdirectory ${subdirectories})
 add_subdirectory(${subdirectory})
endforeach(subdirectory)
