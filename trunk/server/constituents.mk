# List libraries (if any) for this package
libnames := server

# List source files for each library
libsrcs_server := $(filter-out servertest.cc serverapp.cc,$(wildcard *.cc))
#libsrcs_server := Server.cc ServerManager.cc 

# List special include directories (if any) needed by lib_a as
# <project>/<incdir>. Note that the top level release directory is
# already in the search path.
libincs_server := pdsdata/include

# List system include directories (if any) needed by lib_a as <incdir>.
# libsinc_lib_a := /usr/include

#tgtnames := servertest

tgtsrcs_servertest := servertest.cc
tgtlibs_servertest := pdsdata/xtcdata pdsdata/camdata pdsdata/acqdata
tgtlibs_servertest += ami/amisvc ami/amidata ami/server
tgtlibs_servertest += qt/QtCore
tgtslib_servertest := $(USRLIBDIR)/rt
