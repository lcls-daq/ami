# List targets (if any) for this package
libnames := amil3t

libsrcs_amil3t := $(wildcard *.cc)

liblibs_amil3t := ami/amisvc ami/app ami/amidata ami/amievent ami/amicalib
liblibs_amil3t += pdsdata/xtcdata pdsdata/psddl_pdsdata pdsdata/appdata pdsdata/compressdata
liblibs_amil3t += $(qtlibdir)
liblibs_amil3t += psalg/psalg
libslib_amil3t := gomp $(qtslibdir)

libincs_amil3t := ndarray/include boost/include  pdsdata/include 

tgtnames := l3ttest
tgtsrcs_l3ttest := l3ttest.cc
tgtlibs_l3ttest := psalg/psalg pdsdata/xtcdata
tgtlibs_l3ttest += pdsdata/appdata pdsdata/psddl_pdsdata
tgtlibs_l3ttest += pdsdata/compressdata
tgtlibs_l3ttest += ami/amisvc ami/amidata ami/server ami/amicalib ami/amievent ami/client ami/app ami/amil3t
tgtlibs_l3ttest += $(qtlibdir)
tgtlibs_l3ttest += gsl/gsl gsl/gslcblas
tgtincs_l3ttest := $(qtincdir) pdsdata/include
