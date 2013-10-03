# List targets (if any) for this package
libnames := amil3t

libsrcs_amil3t := $(wildcard *.cc)

liblibs_amil3t := ami/amisvc ami/amidata
liblibs_amil3t += pdsdata/xtcdata pdsdata/psddl_pdsdata
liblibs_amil3t += $(qtlibdir)
liblibs_amil3t += $(qtslibdir)
liblibs_amil3t += pdsalg/pdsalg

libincs_amil3t := ndarray/include boost/include pdsdata/include 
