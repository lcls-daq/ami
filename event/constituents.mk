# List libraries (if any) for this package
libnames := event calib

# List source files for each library
#libsrcs_extra := PhasicsHandler.cc EpixHandler.cc
libsrcs_extra := PhasicsHandler.cc
libsrcs_calib := Calib.cc CalibFile.cc FrameCalib.cc FccdCalib.cc CspadCalib.cc PnccdCalib.cc GainSwitchCalib.cc

ifeq ($(build_extra),$(true))
libsrcs_event := $(filter-out $(libsrcs_calib), $(wildcard *.cc))
else
libsrcs_event := $(filter-out $(libsrcs_calib) $(libsrcs_extra), $(wildcard *.cc))
endif

# List special include directories (if any) needed by lib_a as
# <project>/<incdir>. Note that the top level release directory is
# already in the search path.
libincs_calib := ndarray/include boost/include  pdsdata/include
libincs_event := ndarray/include boost/include  pdsdata/include psalg/include

# List system include directories (if any) needed by lib_a as <incdir>.
# libsinc_lib_a := /usr/include
