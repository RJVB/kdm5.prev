# Find KDE4 and provide all necessary variables and macros to compile software for it.
#
# Please look in FindKDE4Internal.cmake and KDE4Macros.cmake for more information.
# They are installed with the KDE 4 libraries in $KDEDIR/share/apps/cmake/modules/.
#
# Author: Alexander Neundorf <neundorf@kde.org>


# First try to find kde-config
FIND_PROGRAM(KDE4_KDECONFIG_EXECUTABLE NAMES kde-config
   PATHS
   ${CMAKE_INSTALL_PREFIX}/bin
   $ENV{KDEDIR}/bin
   /opt/kde4/bin
   /opt/kde
   )


IF (KDE4_KDECONFIG_EXECUTABLE)

   # then ask kde-config for the kde data dirs
   EXEC_PROGRAM(${KDE4_KDECONFIG_EXECUTABLE} ARGS --path data OUTPUT_VARIABLE _data_DIR )

   # replace the ":" with ";" so that it becomes a valid cmake list
   STRING(REGEX REPLACE ":" ";" _data_DIR "${_data_DIR}")

   # then check the data dirs for FindKDE4Internal.cmake
   FIND_PATH(KDE4_DATA_DIR cmake/modules/FindKDE4Internal.cmake ${_data_DIR})

   # if it has been found...
   IF (KDE4_DATA_DIR)

      SET(CMAKE_MODULE_PATH  ${KDE4_DATA_DIR}/cmake/modules ${CMAKE_MODULE_PATH})

      IF (KDE4_FIND_QUIETLY)
         SET(_quiet QUIET)
      ENDIF (KDE4_FIND_QUIETLY)

      IF (KDE4_FIND_REQUIRED)
         SET(_req REQUIRED)
      ENDIF (KDE4_FIND_REQUIRED)

      # use FindKDE4Internal.cmake to do the rest
      FIND_PACKAGE(KDE4Internal ${_req} ${_quiet})

   ENDIF (KDE4_DATA_DIR)

ENDIF (KDE4_KDECONFIG_EXECUTABLE)


IF (KDE4_FIND_REQUIRED AND NOT KDE4_FOUND)
   MESSAGE(FATAL_ERROR "Could not find KDE4")
ENDIF (KDE4_FIND_REQUIRED AND NOT KDE4_FOUND)

