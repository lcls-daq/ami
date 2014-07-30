# List targets (if any) for this package
tgtnames := ami ami_proxy ami_collection l3ftest tcptest

# List source files for each target
tgtsrcs_ami := ami.cc AmiApp.cc AmiApp.hh
tgtsrcs_ami_proxy := ami_proxy.cc
tgtsrcs_ami_collection := ami_collection.cc

tgtsrcs_l3ftest := l3ftest.cc
tgtlibs_l3ftest := psalg/psalg pdsdata/xtcdata
tgtlibs_l3ftest += pdsdata/appdata pdsdata/psddl_pdsdata
tgtlibs_l3ftest += pdsdata/compressdata
tgtlibs_l3ftest += ami/amisvc ami/amidata ami/server ami/calib ami/event ami/client ami/app
tgtlibs_l3ftest += $(qtlibdir)
tgtlibs_l3ftest += gsl/gsl gsl/gslcblas
tgtincs_l3ftest := $(qtincdir) pdsdata/include

tgtsrcs_tcptest := tcptest.cc

# List system libraries (if any) needed by exe_a as <dir>/<lib>. 
# Note that <lib> is the name of the library, not of the file: i.e.
# <lib> for 'libc.so' is 'c'. Low level first.
tgtslib_ami := $(USRLIBDIR)/rt $(USRLIBDIR)/dl
tgtslib_ami_proxy := $(USRLIBDIR)/rt
tgtslib_ami_collection := $(USRLIBDIR)/rt
tgtslib_test := $(USRLIBDIR)/rt
tgtslib_tcptest := $(USRLIBDIR)/rt

tgtlibs_ami_proxy := ami/amisvc ami/amidata ami/server ami/client ami/calib
tgtlibs_ami_proxy += pdsdata/xtcdata pdsdata/psddl_pdsdata
tgtlibs_ami_proxy += $(qtlibdir)
tgtslib_ami_proxy += $(qtslibdir)
tgtlibs_ami_proxy += psalg/psalg
tgtlibs_ami_proxy += gsl/gsl gsl/gslcblas

tgtlibs_ami_collection := ami/amisvc ami/amidata ami/server ami/client ami/calib
tgtlibs_ami_collection += pdsdata/xtcdata pdsdata/psddl_pdsdata
tgtlibs_ami_collection += $(qtlibdir)
tgtslib_ami_collection += $(qtslibdir)
tgtlibs_ami_collection += psalg/psalg
tgtlibs_ami_collection += gsl/gsl gsl/gslcblas

#
# Need all pdsdata libraries to support dynamic linking of plug-in modules
#
tgtlibs_ami := pdsdata/xtcdata
tgtlibs_ami += pdsdata/appdata pdsdata/psddl_pdsdata
tgtlibs_ami += pdsdata/compressdata
tgtlibs_ami += ami/amisvc ami/amidata ami/server ami/calib ami/event ami/client ami/app
tgtincs_ami := $(qtincdir) pdsdata/include ndarray/include boost/include
tgtlibs_ami += $(qtlibdir)
tgtslib_ami += $(qtslibdir)
tgtlibs_ami += psalg/psalg
tgtlibs_ami += gsl/gsl gsl/gslcblas

tgtlibs_tcptest := ami/amisvc pdsdata/xtcdata

# List system include directories (if any) needed by exe_a as <incdir>.
# tgtsinc_exe_a := /usr/include

# List libraries (if any) for this package
libnames := app

# List source files for each library
#libsrcs_app := $(filter-out Agent.cc ami_agent.cc l3ftest.cc ami.cc,$(wildcard *.cc))
libsrcs_app := $(filter-out SummaryAnalysis.cc SyncAnalysis.cc l3ftest.cc ami.cc ami_proxy.cc tcptest.cc,$(wildcard *.cc))
# libsrcs_lib_b := src_6.cc

# List special include directories (if any) needed by lib_a as
# <project>/<incdir>. Note that the top level release directory is
# already in the search path.
libincs_app := $(qtincdir) ndarray/include boost/include  pdsdata/include 

# List system include directories (if any) needed by lib_a as <incdir>.
# libsinc_lib_a := /usr/include
