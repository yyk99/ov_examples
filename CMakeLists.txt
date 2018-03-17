# Define the minimum cmake version required
cmake_minimum_required(VERSION "3.2.1" FATAL_ERROR)

include(CMakeDependentOption)

set(CMAKE_CONFIGURATION_TYPES Debug;Release CACHE STRING "" FORCE)

# Modules examples
Project(AllModulesExamples)

# Detect X11 (then if(X11_FOUND) can be used)
if(UNIX AND NOT APPLE)
  find_package(X11)
endif()

# Hack to get other generators working on windows. Should not be necessary as soon as the copyprepackeddll.cmake file is not used anymore.
if(MSVC_IDE)
  set(OIV_BUILD_TYPE ${CMAKE_CFG_INTDIR})
else()
  set(OIV_BUILD_TYPE ${CMAKE_BUILD_TYPE})
endif()

# Hide (and set to FALSE) OIV_LOCAL_PROJECT option in OIV_DEV_BUILD mode
cmake_dependent_option(OIV_LOCAL_PROJECT "genarate local project ready to distrib" TRUE "NOT OIV_DEV_BUILD" FALSE)

 if (OIV_LOCAL_PROJECT)
	set(CMAKE_SUPPRESS_REGENERATION TRUE)
	set(CMAKE_USE_RELATIVE_PATH TRUE)
 else()
	if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
		message(FATAL_ERROR "CMake generation is not allowed within the
				source directory! Remove the CMakeCache.txt file and try again from another folder, e.g.:

				rm CMakeCache.txt
				mkdir -p build/{debug/release}
				cd build/debug
				cmake  ../..
			")
	endif(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
endif()

set(OIV_ENABLE_MULTIPIPE TRUE)
set(OIV_ENABLE_FXVIZ TRUE)
set(OIV_ENABLE_EBUGS FALSE)

message(STATUS "${CMAKE_CURRENT_SOURCE_DIR} and ${CMAKE_SOURCE_DIR}")

# Detect if we are running Cmake starting from demo directory or from
# main OivSuite directory
if(NOT OIV_DEV_BUILD AND ${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_SOURCE_DIR})

  # Here we have ran CMake directly in the demo directory
  set(OIV_COMMON_CMAKE_DIR ${CMAKE_SOURCE_DIR}/../source/cmake )
  include( ${OIV_COMMON_CMAKE_DIR}/CMakePolicy.cmake NO_POLICY_SCOPE )
  include( ${OIV_COMMON_CMAKE_DIR}/FindGit.cmake )
  include( ${OIV_COMMON_CMAKE_DIR}/FindGLUT.cmake )
  include( ${OIV_COMMON_CMAKE_DIR}/InternalBuildKey.cmake )
  include( ${OIV_COMMON_CMAKE_DIR}/GPUDetection.cmake )

  if(OIV_INTERNAL_BUILD_KEY)
    message(STATUS "SHA1 build key: ${OIV_INTERNAL_BUILD_KEY}")
  endif()

# First try to find OIV and the modules

  include(${OIV_COMMON_CMAKE_DIR}/UsefulMacros.cmake)
  include(${OIV_COMMON_CMAKE_DIR}/FindOpenInventor.cmake)
  include(${OIV_COMMON_CMAKE_DIR}/DefaultCompilerFlags.cmake)
  include(${OIV_COMMON_CMAKE_DIR}/OivSuiteLibraries.cmake)
  include(${OIV_COMMON_CMAKE_DIR}/TargetProperties.cmake)

# Setup the modules according to the available ones.
  option(OIV_ENABLE_WX "Wx widgets are supported" TRUE)

  # ImageViz only Available on Unix 64bit platforms and Windows with MSVC10 minimum
  if(MSVC)
    if(MSVC_VERSION GREATER 1500) # 1500 == MSVC9 greater implies MSVC10
      set(OIV_ENABLE_IMAGEVIZ TRUE)
    else()
      set(OIV_ENABLE_IMAGEVIZ FALSE)
      message(STATUS "Visilog Proxy only supported for Windows and MSVC10 minimum")
    endif()
  else()
    # Linux 64-bit or Apple
    #1. determining platform arch (32-bit vs 64-bit)
    #2. determining gcc minimum version (gcc41)
    if(("${CMAKE_SYSTEM_PROCESSOR}" MATCHES "64") AND (${OIV_CXX_COMPILER_VERSION} GREATER 40))
      set(OIV_ENABLE_IMAGEVIZ TRUE)
    else()
      set(OIV_ENABLE_IMAGEVIZ FALSE)
      message(STATUS "Visilog Proxy only supported for 64-bit platform and gcc-4.1 minimum")
    endif()
  endif()

  # RemoteViz only Available on MSVC and GCC
  if (MSVC OR CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
    set(OIV_ENABLE_REMOTEVIZ TRUE)
  else()
    set(OIV_ENABLE_REMOTEVIZ FALSE)
  endif()

  set(OIV_ENABLE_DIRECTVIZ TRUE)
  set(OIV_ENABLE_MESHIVIZ TRUE)
  set(OIV_ENABLE_HPDF TRUE)
  set(OIV_ENABLE_OPENCL TRUE)

  # Enable _SCL_SECURE_NO_WARNINGS to avoid warning C4996 on Windows
  if(MSVC)
    add_definitions(-D_SCL_SECURE_NO_WARNINGS)
  endif()

  if(CMAKE_COMPILER_IS_GNUCC)
    set(OIV_ENABLE_OPENCL ${OIV_OPENCL_FOUND})
    set(OIV_ENABLE_DIRECTVIZ ${OIV_DIRECTVIZ_FOUND})
    set(OIV_ENABLE_CUDA ${OIV_CUDA_FOUND})

    if( OIV_ENABLE_WX AND OIV_WX_FOUND)
      set(OIV_ENABLE_WX TRUE)
    else()
      set(OIV_ENABLE_WX FALSE)
    endif()

    if(${OIV_CXX_COMPILER_VERSION} MATCHES "^3")
      set(OIV_ENABLE_MESHIVIZ FALSE)
    endif(${OIV_CXX_COMPILER_VERSION} MATCHES "^3")

  endif(CMAKE_COMPILER_IS_GNUCC)


  if(${CMAKE_CXX_PLATFORM_ID} MATCHES "SunOS")

    set(CMAKE_CXX_FLAGS "-xmemalign=8s ${CMAKE_CXX_FLAGS}")
    option(OIV_ENABLE_SOLARIS64 "Enable SOLARIS64 build of Open Inventor (Default:True)" TRUE)

    if(OIV_ENABLE_SOLARIS64)
      set(CMAKE_CXX_FLAGS "-xarch=v9 ${CMAKE_CXX_FLAGS}")
      set(CMAKE_C_FLAGS "-xarch=v9 ${CMAKE_C_FLAGS}")
    endif(OIV_ENABLE_SOLARIS64)

    set(CMAKE_CXX_FLAGS "-mt -xcode=pic32 -xlibmil ${CMAKE_CXX_FLAGS}")
    set(CMAKE_C_FLAGS "-mt -xcode=pic32 -xlibmil ${CMAKE_C_FLAGS}")

    set(OIV_ENABLE_WX FALSE)
    set(OIV_ENABLE_DIRECTVIZ FALSE)
    set(OIV_ENABLE_MESHIVIZ FALSE)
    set(OIV_ENABLE_HPDF FALSE)
    set(OIV_ENABLE_CUDA FALSE)
    set(OIV_ENABLE_OPENCL FALSE)
    set(OIV_ENABLE_REMOTEVIZ FALSE)
  elseif(APPLE)
    set(OIV_ENABLE_WX FALSE)
    set(OIV_ENABLE_DIRECTVIZ FALSE)
    set(OIV_ENABLE_HPDF FALSE)
    set(OIV_ENABLE_CUDA FALSE)
    set(OIV_ENABLE_OPENCL FALSE)
    set(OIV_ENABLE_MULTIPIPE FALSE)
    set(OIV_ENABLE_FXVIZ FALSE)
    set(OIV_ENABLE_REMOTEVIZ FALSE)
  endif()

  # In any case disable CUDA for VS2013 and above (mscv_ver >  1700 ) as our current
  # version of CUDA is not compatible. Disable also CUDA if GPU is
  # Intel or ATI.
  if ( MSVC AND MSVC_VERSION GREATER 1700 )
    MESSAGE (STATUS "Deactivated CUDA SUPPORT because of non compatible Visual Studio")
    set(OIV_ENABLE_CUDA FALSE)
    set(OIV_ENABLE_CUDA_ALGORITHMS FALSE)
  endif()

  if ( "${GPU_NAME}" MATCHES "INTEL" OR "${GPU_NAME}" MATCHES "ATI")
    MESSAGE (STATUS "Deactivated CUDA SUPPORT because of unsupported GPU")
    set(OIV_ENABLE_CUDA FALSE)
    set(OIV_ENABLE_CUDA_ALGORITHMS FALSE)
  endif()

  if(OIV_ENABLE_CUDA)
    find_package(CUDA)

    # We disable the CUDA examples if we don't find the toolkit.
    if (NOT CUDA_NVCC_EXECUTABLE)
      message( STATUS "Cannot find CUDA, disabling the examples. To avoid this issue please install the CUDA toolkit" )
      set( OIV_ENABLE_CUDA FALSE )
    endif()
  endif(OIV_ENABLE_CUDA)

  if(OIV_ENABLE_OPENCL)
    include(${OIV_COMMON_CMAKE_DIR}/FindOpenCL.cmake)

    # We disable the OpenCL examples if we don't find the toolkit.
    if (NOT OPENCL_FOUND)
      message( STATUS "Cannot find OpenCL, disabling the examples. To avoid this issue please install the OpenCL toolkit" )
      set( OIV_ENABLE_OPENCL FALSE )
    endif()
  endif(OIV_ENABLE_OPENCL)

endif()

# Check for Qt
set(QT_REQUIRED TRUE)
set(QT_USE_OPENGL TRUE)
set(QT_USE_XML TRUE)

# ###################################################
# Experimental option to force all demos to build with Qt
option( OIV_DEMOS_QT_ONLY "Use Qt viewers for all demos." FALSE)
if(APPLE)
  set (OIV_DEMOS_QT_ONLY TRUE)
endif()

# ###################################################
# Option to enable/disable all demos using Qt
option( OIV_ENABLE_QT_DEMOS "Enable all demos using Qt." TRUE)

if(WIN32)
  add_definitions(-DQT_DLL)
endif()

if(OIV_DEMOS_QT_ONLY)
  add_definitions(-DSOQT)
  # Find needed Qt Packages
  find_package(Qt5 COMPONENTS Core Gui OpenGL PrintSupport Xml Widgets REQUIRED)

  if (UNIX AND NOT APPLE)
    find_package(Qt5 COMPONENTS X11Extras REQUIRED)
  endif()

  link_libraries (
    ${Qt5Core_LIBRARIES}
    ${Qt5Gui_LIBRARIES}
    ${Qt5OpenGL_LIBRARIES}
    ${Qt5Widgets_LIBRARIES}
    ${Qt5X11Extras_LIBRARIES}
  )

  if(WIN32)
    set(DirectVizGui_LIBRARIES optimized DirectVizQt debug DirectVizQtD)
    set(ScaleVizGui_LIBRARIES optimized ScaleVizQt debug ScaleVizQtD)
  else()
    set(DirectVizGui_LIBRARIES DirectVizQt )
    set(ScaleVizGui_LIBRARIES ScaleVizQt )
  endif()
  set( OpenInventorWin_LIBRARIES ${OpenInventorQt_LIBRARIES} )
  set( OpenInventorXt_LIBRARIES ${OpenInventorQt_LIBRARIES} )
  set( DialogViz_LIBRARIES ${DialogVizQt_LIBRARIES} )
endif()

# ###############################################
# tests

 if (NOT OIV_LOCAL_PROJECT)
  include(${OIV_COMMON_CMAKE_DIR}/OIVTests.cmake)
  # UnitTests that want to explicitly add a RUN_TEST program must use the ADD_OIV_RUNTEST_EXE macro
  option( OIV_ENABLE_RUNTESTS "Enable binaries execution testing" TRUE )
  mark_as_advanced( OIV_ENABLE_RUNTESTS )

  option( OIV_ENABLE_ART_TESTS "Enable ART test" TRUE )

  if (NOT HOSTNAME)
    site_name(HOSTNAME)
  endif()
  # Remove dots from hostname
  string(REPLACE "." ";" HOSTNAME_LIST ${HOSTNAME})
  list(GET HOSTNAME_LIST 0 HOSTNAME)

  if(OIV_DEV_BUILD)
    set(OIV_ART_REFERENCES_SUBMOD "${CMAKE_SOURCE_DIR}/TestReferences/${HOSTNAME}/")
  else()
    set(OIV_ART_REFERENCES_SUBMOD "${CMAKE_SOURCE_DIR}/../../TestReferences/${HOSTNAME}/")
  endif()

  get_filename_component(OIV_ART_REFERENCES_SUBMOD "${OIV_ART_REFERENCES_SUBMOD}" ABSOLUTE)

  # The reference path for the tests data
  SET(OIV_TESTS_REFERENCE_PATH "${OIV_ART_REFERENCES_SUBMOD}/OIVHOME/src")

  # set the output path for the ART output only if it was not passed as parameter
  if(NOT OIV_TESTS_OUTPUT_PATH)
    SET(OIV_TESTS_OUTPUT_PATH "${OIV_ART_REFERENCES_SUBMOD}/generated/demos")
  else()
    # when the output path is passed as a parameter, it is a relative path to the build dir
    # adding the build dir as prefix to make sure the proper path is found
    SET(OIV_TESTS_OUTPUT_PATH "${CMAKE_BINARY_DIR}/${OIV_TESTS_OUTPUT_PATH}")
  endif()

  OivTestsInit()
endif()

# ############################################

if ( OIV_LOCAL_PROJECT AND NOT OIV_DEV_BUILD )
 configure_file(${CMAKE_SOURCE_DIR}/../source/cmake/RelativeProjectWindows.cmake.in  ${CMAKE_BINARY_DIR}/cmake/RelativeProjectWindows.cmake @ONLY)
else()
  ADD_ART_TEST()
endif()

# In not OIV_DEV_BUILD, ${CMAKE_SOURCE_DIR} is OIVHOME/source and we have to copy CTestCustom.cmake to build directory
# In OIV_DEV_BUILD, this copyt is done by CMakeList.txt at the root of OivSuite.
if ( NOT OIV_DEV_BUILD )
  configure_file(${CMAKE_SOURCE_DIR}/../source/cmake/CTestCustom.cmake ${CMAKE_BINARY_DIR})
  configure_file(${CMAKE_SOURCE_DIR}/../source/cmake/GPUDetection.cmake ${CMAKE_BINARY_DIR})
endif()

include_directories(${OIV_INCLUDE_DIR})
link_directories(${OIV_LIB_DIR})

add_subdirectory(source)
