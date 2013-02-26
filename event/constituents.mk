# List libraries (if any) for this package
libnames := event calib

# List source files for each library
libsrcs_calib := Calib.cc FccdCalib.cc CspadCalib.cc
libsrcs_event := $(filter-out $(libsrcs_calib), $(wildcard *.cc))

# List special include directories (if any) needed by lib_a as
# <project>/<incdir>. Note that the top level release directory is
# already in the search path.
# libincs_lib_a := prj_x/include/Linux

# List system include directories (if any) needed by lib_a as <incdir>.
# libsinc_lib_a := /usr/include
