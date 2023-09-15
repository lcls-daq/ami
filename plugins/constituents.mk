# List libraries (if any) for this package
libnames := timetool

# List source files for each library
libsrcs_timetool := TimeToolM.cc

# List special include directories (if any) needed by lib_a as
# <project>/<incdir>. Note that the top level release directory is
# already in the search path.
libincs_timetool := pdsdata/include ndarray/include boost/include psalg/include timetool/include

# List system include directories (if any) needed by lib_a as <incdir>.
# libsinc_lib_a := /usr/include

liblibs_timetool := timetool/ttsvc psalg/psalg

libnames += timetooldb
libsrcs_timetooldb := TimeToolC.cc
libincs_timetooldb := pdsdata/include ndarray/include boost/include psalg/include timetool/include
liblibs_timetooldb := timetool/ttsvc psalg/psalg

libnames += timetooldbd
libsrcs_timetooldbd := TimeToolD.cc
libincs_timetooldbd := pdsdata/include ndarray/include boost/include psalg/include timetool/include
liblibs_timetooldbd := timetool/ttsvc psalg/psalg

libnames += timetooldbe
libsrcs_timetooldbe := TimeToolE.cc
libincs_timetooldbe := pdsdata/include ndarray/include boost/include psalg/include timetool/include
liblibs_timetooldbe := timetool/ttsvc psalg/psalg

libnames += xppbase
libsrcs_xppbase := XppBase.cc
libincs_xppbase := pdsdata/include ndarray/include boost/include

libnames += UserIpm
libsrcs_UserIpm := UserIpm.cc
libincs_UserIpm := pdsdata/include ndarray/include boost/include
liblibs_UserIpm := ami/xppbase

libnames += XPPIpm
libsrcs_XPPIpm := XPPIpm.cc
libincs_XPPIpm := pdsdata/include ndarray/include boost/include
liblibs_XPPIpm := ami/xppbase

libnames += XPPOther
libsrcs_XPPOther := XPPOther.cc
libincs_XPPOther := pdsdata/include ndarray/include boost/include
liblibs_XPPOther := ami/xppbase

libnames += phascav
libsrcs_phascav := PhaseCavEBeam.cc
libincs_phascav := pdsdata/include ndarray/include boost/include
liblibs_phascav := ami/xppbase

libnames += XPPIpmSB1
libsrcs_XPPIpmSB1 := XPPIpmSB1.cc
libincs_XPPIpmSB1 := pdsdata/include ndarray/include boost/include
liblibs_XPPIpmSB1 := ami/xppbase

libnames += XPPSummary
libsrcs_XPPSummary := XPPSummary.cc
libincs_XPPSummary := pdsdata/include ndarray/include boost/include

libnames += PnccdModule
libsrcs_PnccdModule := PnccdModule.cc
libincs_PnccdModule := pdsdata/include ndarray/include boost/include

libnames += AmiEpics
libsrcs_AmiEpics := EpicsCA.cc AmiEpics.cc
#libsrcs_AmiEpics := EpicsCA.cc AmiEpics.cc CspadTHandler.cc CspadMiniTHandler.cc PrincetonTHandler.cc
libincs_AmiEpics := pdsdata/include ndarray/include boost/include epics/include epics/include/os/Linux
liblibs_AmiEpics := epics/ca

libnames += CspadTripper
libsrcs_CspadTripper := EpicsCA.cc CspadTripper.cc
libincs_CspadTripper := pdsdata/include ndarray/include boost/include epics/include epics/include/os/Linux
liblibs_CspadTripper := epics/ca

libnames += JungfrauTripper
libsrcs_JungfrauTripper := EpicsCA.cc JungfrauTripper.cc
libincs_JungfrauTripper := pdsdata/include ndarray/include boost/include epics/include epics/include/os/Linux
liblibs_JungfrauTripper := epics/ca

libnames += IpmSumEScan
libsrcs_IpmSumEScan := IpmSumEScan.cc
libincs_IpmSumEScan := pdsdata/include ndarray/include boost/include
liblibs_IpmSumEScan := ami/xppbase

libnames += XPPIpmLODCM
libsrcs_XPPIpmLODCM := XPPIpmLODCM.cc
libincs_XPPIpmLODCM := pdsdata/include ndarray/include boost/include
liblibs_XPPIpmLODCM := ami/xppbase

libnames += CsPadTemp
libsrcs_CsPadTemp := CsPadTemp.cc
libincs_CsPadTemp := pdsdata/include ndarray/include boost/include

libnames += EpixTemp
libsrcs_EpixTemp := EpixTemp.cc
libincs_EpixTemp := pdsdata/include ndarray/include boost/include

libnames += DetectorProtection
libsrcs_DetectorProtection := EpicsCA.cc DetectorProtection.cc
libincs_DetectorProtection := pdsdata/include ndarray/include boost/include epics/include epics/include/os/Linux
liblibs_DetectorProtection := epics/ca
