#
# Copyright 2016 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

find_path(OPENEXR_INCLUDE_DIR
    OpenEXR/half.h
HINTS
    "${OPENEXR_LOCATION}"
    "$ENV{OPENEXR_LOCATION}"
PATH_SUFFIXES
    include/
DOC
    "OpenEXR headers path"
)

if(OPENEXR_INCLUDE_DIR)
  set(openexr_config_file "${OPENEXR_INCLUDE_DIR}/OpenEXR/OpenEXRConfig.h")
  if(EXISTS ${openexr_config_file})
      file(STRINGS
           ${openexr_config_file}
           TMP
           REGEX "#define OPENEXR_VERSION_STRING.*$")
      string(REGEX MATCHALL "[0-9.]+" OPENEXR_VERSION ${TMP})

      file(STRINGS
           ${openexr_config_file}
           TMP
           REGEX "#define OPENEXR_VERSION_MAJOR.*$")
      string(REGEX MATCHALL "[0-9]" OPENEXR_MAJOR_VERSION ${TMP})

      file(STRINGS
           ${openexr_config_file}
           TMP
           REGEX "#define OPENEXR_VERSION_MINOR.*$")
      string(REGEX MATCHALL "[0-9]" OPENEXR_MINOR_VERSION ${TMP})
  endif()
endif()

foreach(OPENEXR_LIB
    Half
    Iex
    Imath
    IlmImf
    IlmThread
    IlmImfUtil
    IexMath
    )

    # OpenEXR libraries may be suffixed with the version number, so we search
    # using both versioned and unversioned names.

    find_library(OPENEXR_${OPENEXR_LIB}_LIBRARY_RELEASE
        NAMES
            ${OPENEXR_LIB}-${OPENEXR_MAJOR_VERSION}_${OPENEXR_MINOR_VERSION}
            ${OPENEXR_LIB}
        HINTS
            "${OPENEXR_RELEASE_LOCATION}"
            "$ENV{OPENEXR_RELEASE_LOCATION}"
            "${OPENEXR_LOCATION}"
            "$ENV{OPENEXR_LOCATION}"
        PATH_SUFFIXES
            lib/
        DOC
            "OPENEXR's ${OPENEXR_LIB} release library path"
    )

    # On MacOS and Windows, by default debug libs get a _d suffix
    find_library(OPENEXR_${OPENEXR_LIB}_LIBRARY_DEBUG
        NAMES
            ${OPENEXR_LIB}-${OPENEXR_MAJOR_VERSION}_${OPENEXR_MINOR_VERSION}_d
            ${OPENEXR_LIB}_d
        HINTS
            "${OPENEXR_DEBUG_LOCATION}"
            "$ENV{OPENEXR_DEBUG_LOCATION}"
            "${OPENEXR_LOCATION}"
            "$ENV{OPENEXR_LOCATION}"
        PATH_SUFFIXES
            lib/
        DOC
            "OPENEXR's ${OPENEXR_LIB} debug library path"
    )

    # Figure out whether to use debug or release lib as "the" library

    if(OPENEXR_${OPENEXR_LIB}_LIBRARY_RELEASE AND OPENEXR_${OPENEXR_LIB}_LIBRARY_DEBUG)
        # both were found, decide which to use
        if(DEFINED PXR_USE_DEBUG_BUILD)
            if(PXR_USE_DEBUG_BUILD)
                set(OPENEXR_${OPENEXR_LIB}_LIBRARY_TYPE "DEBUG")
            else()
                set(OPENEXR_${OPENEXR_LIB}_LIBRARY_TYPE "RELEASE")
            endif()
        else()
            string(TOLOWER "${CMAKE_BUILD_TYPE}" CMAKE_BUILD_TYPE_LOWER)
            if(CMAKE_BUILD_TYPE_LOWER MATCHES "^(debug|relwithdebinfo)$")
                set(OPENEXR_${OPENEXR_LIB}_LIBRARY_TYPE "DEBUG")
            else()
                set(OPENEXR_${OPENEXR_LIB}_LIBRARY_TYPE "RELEASE")
            endif()
        endif()
    elseif(OPENEXR_${OPENEXR_LIB}_LIBRARY_RELEASE)
        set(OPENEXR_${OPENEXR_LIB}_LIBRARY_TYPE "RELEASE")
    elseif(OPENEXR_${OPENEXR_LIB}_LIBRARY_DEBUG)
        set(OPENEXR_${OPENEXR_LIB}_LIBRARY_TYPE "DEBUG")
    else()
        set(OPENEXR_${OPENEXR_LIB}_LIBRARY_TYPE "NOTFOUND")
    endif()

    if(OPENEXR_${OPENEXR_LIB}_LIBRARY_TYPE)
        set(OPENEXR_${OPENEXR_LIB}_LIBRARY
            "${OPENEXR_${OPENEXR_LIB}_LIBRARY_${OPENEXR_${OPENEXR_LIB}_LIBRARY_TYPE}}"
            CACHE
            FILEPATH
            "OPENEXR's ${OPENEXR_LIB} library path"
        )
    else()
        set(OPENEXR_${OPENEXR_LIB}_LIBRARY OPENEXR_${OPENEXR_LIB}_LIBRARY-NOTFOUND)
    endif()

    if(OPENEXR_${OPENEXR_LIB}_LIBRARY_RELEASE)
        list(APPEND OPENEXR_LIBRARIES_RELEASE ${OPENEXR_${OPENEXR_LIB}_LIBRARY_RELEASE})
    endif()
    if(OPENEXR_${OPENEXR_LIB}_LIBRARY_DEBUG)
        list(APPEND OPENEXR_LIBRARIES_DEBUG ${OPENEXR_${OPENEXR_LIB}_LIBRARY_DEBUG})
    endif()
    if(OPENEXR_${OPENEXR_LIB}_LIBRARY)
        list(APPEND OPENEXR_LIBRARIES ${OPENEXR_${OPENEXR_LIB}_LIBRARY})
    endif()
endforeach(OPENEXR_LIB)

# So #include <half.h> works
list(APPEND OPENEXR_INCLUDE_DIRS ${OPENEXR_INCLUDE_DIR})
list(APPEND OPENEXR_INCLUDE_DIRS ${OPENEXR_INCLUDE_DIR}/OpenEXR)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(OpenEXR
    REQUIRED_VARS
        OPENEXR_INCLUDE_DIRS
        OPENEXR_LIBRARIES
    VERSION_VAR
        OPENEXR_VERSION
)

