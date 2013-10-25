# List libraries (if any) for this package
libnames := timetool

# List source files for each library
libsrcs_timetool := TimeToolM.cc

# List special include directories (if any) needed by lib_a as
# <project>/<incdir>. Note that the top level release directory is
# already in the search path.
libincs_timetool := pdsdata/include ndarray/include pdsalg/include

# List system include directories (if any) needed by lib_a as <incdir>.
# libsinc_lib_a := /usr/include

liblibs_timetool := timetool/ttsvc pdsalg/pdsalg

libnames += xppbase
libsrcs_xppbase := XppBase.cc
libincs_xppbase := pdsdata/include ndarray/include

libnames += UserIpm
libsrcs_UserIpm := UserIpm.cc
libincs_UserIpm := pdsdata/include ndarray/include
liblibs_UserIpm := ami/xppbase

libnames += XPPIpm
libsrcs_XPPIpm := XPPIpm.cc
libincs_XPPIpm := pdsdata/include ndarray/include
liblibs_XPPIpm := ami/xppbase

libnames += XPPOther
libsrcs_XPPOther := XPPOther.cc
libincs_XPPOther := pdsdata/include ndarray/include
liblibs_XPPOther := ami/xppbase

libnames += phascav
libsrcs_phascav := PhaseCavEBeam
libincs_phascav := pdsdata/include ndarray/include
liblibs_phascav := ami/xppbase

libnames += XPPIpmSB1
libsrcs_XPPIpmSB1 := XPPIpmSB1.cc
libincs_XPPIpmSB1 := pdsdata/include ndarray/include
liblibs_XPPIpmSB1 := ami/xppbase

libnames += XPPSummary
libsrcs_XPPSummary := XPPSummary.cc
libincs_XPPSummary := pdsdata/include ndarray/include
