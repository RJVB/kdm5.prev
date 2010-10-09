# - Try to find the dmtx library
# Once done this will define
#
#  DMTX_FOUND - system has the datamatrix library
#  DMTX_INCLUDE_DIR - the datamatrix library include dir
#  DMTX_LIBRARIES - the libraries used to link datamatrix

# Copyright (C) 2010 Sune Vuorela <sune@debian.org>

# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

if (DMTX_INCLUDE_DIR AND DMTX_LIBRARIES)
  # in cache already
  set(DMTX_FOUND TRUE)
else (DMTX_INCLUDE_DIR AND DMTX_LIBRARIES)

  find_path(DMTX_INCLUDE_DIR dmtx.h)

  find_library(DMTX_LIBRARIES NAMES dmtx)

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(Dmtx DEFAULT_MSG DMTX_LIBRARIES DMTX_INCLUDE_DIR)

  mark_as_advanced(DMTX_INCLUDE_DIR DMTX_LIBRARIES)

endif (DMTX_INCLUDE_DIR AND DMTX_LIBRARIES)
