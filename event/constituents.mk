# List libraries (if any) for this package
libnames := amievent amicalib

# List source files for each library
#libsrcs_extra := PhasicsHandler.cc EpixHandler.cc
libsrcs_extra := PhasicsHandler.cc
libsrcs_amicalib := Calib.cc CalibFile.cc FrameCalib.cc FccdCalib.cc CspadCalib.cc PnccdCalib.cc GainSwitchCalib.cc

ifeq ($(build_extra),$(true))
libsrcs_amievent := $(filter-out $(libsrcs_amicalib), $(wildcard *.cc))
else
libsrcs_amievent := $(filter-out $(libsrcs_amicalib) $(libsrcs_extra), $(wildcard *.cc))
endif

# List special include directories (if any) needed by lib_a as
# <project>/<incdir>. Note that the top level release directory is
# already in the search path.
libincs_amicalib := ndarray/include boost/include  pdsdata/include
libincs_amievent := ndarray/include boost/include  pdsdata/include psalg/include

# List system include directories (if any) needed by lib_a as <incdir>.
# libsinc_lib_a := /usr/include
