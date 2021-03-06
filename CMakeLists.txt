cmake_minimum_required(VERSION 2.8)
project(raptor)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)


enable_language(CXX)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_BUILD_TYPE Release)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
#SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -flto -funroll-loops")

include(FeatureSummary)

option(CRAYXE "Running on Cray XE" OFF)
option(BGQ "Running on BGQ" OFF)
option(WITH_PTSCOTCH "Enable PTScotch Partitioning" OFF)
option(WITH_HYPRE "Add Hypre" OFF)
option(WITH_MFEM "Add MFEM" OFF)
option(WITH_AMPI "Using AMPI" OFF)
option(WITH_MPI "Using MPI" ON)

add_feature_info(hypre WITH_HYPRE "Hypre preconditioner")
add_feature_info(mfem WITH_MFEM "MFEM matrix gallery")
add_feature_info(ampi WITH_AMPI "Compile with AMPI")
add_feature_info(crayxe CRAYXE "Compile on CrayXE")
add_feature_info(bgq BGQ "Compile on BGQ")
add_feature_info(ptscotch WITH_PTSCOTCH "Enable PTScotch Partitioning")


include(options)
include(testing)
include(source)
SET(MPIRUN "")

if (WITH_MPI)
    add_definitions ( -DUSING_MPI )
    find_package(MPI REQUIRED)
    include_directories(${MPI_INCLUDE_PATH})
    SET(MPIRUN mpirun)
endif (WITH_MPI)

include_directories("external")
set(raptor_INCDIR ${CMAKE_CURRENT_SOURCE_DIR}/raptor)
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

SET(HYPRE_DIR "" CACHE STRING "Directory of HYPRE")
SET(METIS_DIR "" CACHE STRING "Directory of metis")
SET(MFEM_DIR "" CACHE STRING "Directory of mfem")
SET(PTSCOTCH_DIR "" CACHE STRING "Directory of mfem")

if (CRAYXE)
    set(EXTERNAL_LIBS "")
    set(MPIRUN aprun)
    set(CMAKE_AR  "gcc-ar-4.7")
    set(CMAKE_CXX_ARCHIVE_CREATE "<CMAKE_AR> qcs <TARGET> <LINK_FLAGS> <OBJECTS>")
    set(CMAKE_CXX_ARCHIVE_FINISH   true)

elseif (BGQ)
    find_library(LAPACK_LIB NAMES liblapack lapack HINTS
        "/soft/libraries/alcf/current/xl/LAPACK/lib")
    find_library(BLAS_LIB NAMES libblas blas HINTS
        "/soft/libraries/alcf/current/xl/BLAS/lib")
    find_library(ESSL_LIB NAMES libesslbg esslbg HINTS
        "/soft/libraries/essl/current/essl/5.1/lib64")
    find_library(XLF_LIB NAMES libxlf90_r.a xlf90_r HINTS
        "/soft/compilers/ibmcmp-may2016/xlf/bg/14.1/bglib64")
    find_library(XLOPT_LIB NAMES libxlopt.a xlopt HINTS
        "/soft/compilers/ibmcmp-may2016/xlf/bg/14.1/bglib64")
    find_library(XLFMATH_LIB NAMES libxlfmath.a xlfmath HINTS
        "/soft/compilers/ibmcmp-may2016/xlf/bg/14.1/bglib64")
    find_library(XL_LIB NAMES libxl.a xl HINTS
        "/soft/compilers/ibmcmp-may2016/xlf/bg/14.1/bglib64")
    find_library(XLSMP_LIB NAMES libxlomp_ser.a xlomp_ser HINTS
        "/soft/compilers/ibmcmp-may2016/xlsmp/bg/3.1/bglib64")

    set(EXTERNAL_LIBS ${LAPACK_LIB} ${BLAS_LIB} ${ESSL_LIB} ${XLF_LIB}
        ${XLOPT_LIB} ${XLFMATH_LIB} ${XL_LIB} ${XLSMP_LIB})
else()
	find_library(LAPACK_LIB NAMES liblapack.so.3 lapack HINTS "/usr/lib/x86_64-linux-gnu/")
	find_library(BLAS_LIB NAMES libblas.so.3 blas HINTS "/usr/lib/x86_64-linux-gnu/")
	set(EXTERNAL_LIBS ${LAPACK_LIB} ${BLAS_LIB})
endif()


if (WITH_PTSCOTCH)
    add_definitions ( -DUSING_PTSCOTCH )
    find_package(Ptscotch)
    if (PTSCOTCH_FOUND)
        include_directories(${PTSCOTCH_INCLUDE_DIRS})
    else()
        message(FATAL_ERROR "Cannot find PTSCOTCH.  Try setting PTSCOTCH_DIR" )
        set(WITH_PTSCOTCH ON)
    endif(PTSCOTCH_FOUND)
endif(WITH_PTSCOTCH)

if(WITH_HYPRE)
    add_definitions ( -DUSING_HYPRE )
    find_package(Hypre)
    if(HYPRE_FOUND)
        include_directories(${HYPRE_INCLUDE_DIRS})
	else()
	    message(FATAL_ERROR "Cannot find HYPRE. Try setting HYPRE_DIR" )
		set(WITH_HYPRE OFF)
    endif(HYPRE_FOUND)
endif(WITH_HYPRE)

if(WITH_MFEM)
    add_definitions ( -DUSING_MFEM )
    add_definitions ( -DUSING_HYPRE )
	set(WITH_HYPRE ON)
	if(NOT HYPRE_FOUND)
		find_package(Hypre)
		if(HYPRE_FOUND)
			include_directories(${HYPRE_INCLUDE_DIRS})
		else()
			message(FATAL_ERROR "Cannot find HYPRE (required for MFEM). Try setting HYPRE_DIR.")
		endif(HYPRE_FOUND)
	endif(NOT HYPRE_FOUND)

    add_definitions ( -DUSING_METIS )
	find_package(Metis)
	if(METIS_FOUND)
		include_directories(${METIS_INCLUDE_DIRS})
	else()
		message(FATAL_ERROR "Cannot find metis (required for MFEM). Try setting METIS_DIR.")
	endif(METIS_FOUND)

	find_package(Mfem)
	if (MFEM_FOUND)
		include_directories(${MFEM_INCLUDE_DIRS})
	else()
		message(FATAL_ERROR "Cannot find mfem. Try setting MFEM_DIR.")
	endif(MFEM_FOUND)
    add_definitions( -DMFEM_MESH_DIR=\"${MFEM_MESH_PATH}\" )
endif(WITH_MFEM)

if (WITH_AMPI)
	add_definitions(-DUSE_AMPI)
endif(WITH_AMPI)

#/////////////////////////// star information of google test ///////////////////////////////
set(GOOGLETEST_ROOT external/googletest CACHE STRING "Google Test source root")
#MESSAGE( STATUS "GOOGLETEST_ROOT: "    ${GOOGLETEST_ROOT} )

include_directories(
    ${PROJECT_SOURCE_DIR}/${GOOGLETEST_ROOT}
    ${PROJECT_SOURCE_DIR}/${GOOGLETEST_ROOT}/include
    )

#MESSAGE( STATUS "PROJECT_SOURCE_DIR: "    ${PROJECT_SOURCE_DIR} )

set(GOOGLETEST_SOURCES
    ${PROJECT_SOURCE_DIR}/${GOOGLETEST_ROOT}/src/gtest-all.cc
    ${PROJECT_SOURCE_DIR}/${GOOGLETEST_ROOT}/src/gtest_main.cc
    )


foreach(_source ${GOOGLETEST_SOURCES})
    set_source_files_properties(${_source} PROPERTIES GENERATED 1)
    #MESSAGE( STATUS "_source: "                 ${_source} )
endforeach()


add_library(googletest STATIC ${GOOGLETEST_SOURCES})
#///////////////////////////// end information of google test //////////////////////////////

add_subdirectory(raptor)

if (BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()
