# List targets (if any) for this package
libnames := amil3t

libsrcs_amil3t := $(wildcard *.cc)

liblibs_amil3t := ami/amisvc ami/app ami/amidata ami/event ami/calib
liblibs_amil3t += pdsdata/xtcdata pdsdata/psddl_pdsdata pdsdata/appdata pdsdata/compressdata
liblibs_amil3t += $(qtlibdir)
liblibs_amil3t += $(qtslibdir)
liblibs_amil3t += psalg/psalg
libslib_amil3t := gomp

libincs_amil3t := ndarray/include boost/include  pdsdata/include 
