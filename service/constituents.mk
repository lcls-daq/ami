# List libraries (if any) for this package
libnames := service

# List source files for each library
libsrcs_service := $(wildcard *.cc)

# List special include directories (if any) needed by lib_a as
# <project>/<incdir>. Note that the top level release directory is
# already in the search path.
libincs_service := pdsdata/include

# List system include directories (if any) needed by lib_a as <incdir>.
# libsinc_lib_a := /usr/include

tgtnames := tcptest

tgtsrcs_tcptest := tcptest.cc
tgtlibs_tcptest := ami/service pdsdata/xtcdata
tgtslib_tcptest := $(USRLIBDIR)/rt
