# List libraries (if any) for this package
libnames := amisvc

# List source files for each library
libsrcs_amisvc := $(wildcard *.cc)

# List special include directories (if any) needed by lib_a as
# <project>/<incdir>. Note that the top level release directory is
# already in the search path.
libincs_amisvc := pdsdata/include

# List system include directories (if any) needed by lib_a as <incdir>.
# libsinc_lib_a := /usr/include

tgtnames := tcptest

tgtsrcs_tcptest := tcptest.cc
tgtlibs_tcptest := ami/amisvc pdsdata/xtcdata
tgtslib_tcptest := $(USRLIBDIR)/rt
