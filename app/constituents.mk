# List targets (if any) for this package
tgtnames := ami ami_proxy ami_collection test tcptest

# List source files for each target
tgtsrcs_ami := ami.cc AmiApp.cc AmiApp.hh
tgtsrcs_ami_proxy := ami_proxy.cc
tgtsrcs_ami_collection := ami_collection.cc
tgtsrcs_test := test.cc
tgtlibs_test := pdsalg/pdsalg pdsdata/xtcdata
tgtlibs_test += pdsdata/appdata pdsdata/psddl_pdsdata
tgtlibs_test += pdsdata/compressdata
tgtlibs_test += ami/amisvc ami/amidata ami/server ami/calib ami/event ami/client ami/app
tgtlibs_test += $(qtlibdir)
tgtincs_test := $(qtincdir) pdsdata/include

tgtsrcs_tcptest := tcptest.cc

# List system libraries (if any) needed by exe_a as <dir>/<lib>. 
# Note that <lib> is the name of the library, not of the file: i.e.
# <lib> for 'libc.so' is 'c'. Low level first.
tgtslib_ami := $(USRLIBDIR)/rt $(USRLIBDIR)/dl
tgtslib_ami_proxy := $(USRLIBDIR)/rt
tgtslib_ami_collection := $(USRLIBDIR)/rt
tgtslib_test := $(USRLIBDIR)/rt
tgtslib_tcptest := $(USRLIBDIR)/rt

tgtlibs_ami_proxy := ami/amisvc ami/amidata ami/server ami/client
tgtlibs_ami_proxy += pdsdata/xtcdata pdsdata/psddl_pdsdata
tgtlibs_ami_proxy += $(qtlibdir)
tgtslib_ami_proxy += $(qtslibdir)
tgtlibs_ami_proxy += pdsalg/pdsalg

tgtlibs_ami_collection := ami/amisvc ami/amidata ami/server ami/client
tgtlibs_ami_collection += pdsdata/xtcdata pdsdata/psddl_pdsdata
tgtlibs_ami_collection += $(qtlibdir)
tgtslib_ami_collection += $(qtslibdir)
tgtlibs_ami_collection += pdsalg/pdsalg

#
# Need all pdsdata libraries to support dynamic linking of plug-in modules
#
tgtlibs_ami := pdsdata/xtcdata
tgtlibs_ami += pdsdata/appdata pdsdata/psddl_pdsdata
tgtlibs_ami += pdsdata/compressdata
tgtlibs_ami += ami/amisvc ami/amidata ami/server ami/calib ami/event ami/client ami/app
tgtincs_ami := $(qtincdir) pdsdata/include
tgtlibs_ami += $(qtlibdir)
tgtslib_ami += $(qtslibdir)
tgtlibs_ami += pdsalg/pdsalg

tgtlibs_tcptest := ami/amisvc pdsdata/xtcdata

# List system include directories (if any) needed by exe_a as <incdir>.
# tgtsinc_exe_a := /usr/include

# List libraries (if any) for this package
libnames := app

# List source files for each library
#libsrcs_app := $(filter-out Agent.cc ami_agent.cc test.cc ami.cc,$(wildcard *.cc))
libsrcs_app := $(filter-out SummaryAnalysis.cc SyncAnalysis.cc test.cc ami.cc ami_proxy.cc tcptest.cc,$(wildcard *.cc))
# libsrcs_lib_b := src_6.cc

# List special include directories (if any) needed by lib_a as
# <project>/<incdir>. Note that the top level release directory is
# already in the search path.
libincs_app := $(qtincdir) ndarray/include boost/include  pdsdata/include 

# List system include directories (if any) needed by lib_a as <incdir>.
# libsinc_lib_a := /usr/include
