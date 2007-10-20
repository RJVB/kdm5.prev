# 

  find_path(X11_XKBFILE_INCLUDE_PATH X11/extensions/XKBfile.h "${X11_INC_SEARCH_PATH}")
  if (X11_XKBFILE_INCLUDE_PATH)
    check_library_exists(xkbfile XkbInitAtoms "" HAVE_XKBFILE)
    if (HAVE_XKBFILE)
        set(XKBFILE_FOUND TRUE)
    endif (HAVE_XKBFILE)
  endif (X11_XKBFILE_INCLUDE_PATH)
