libnames := pyami

libsrcs_pyami := pyami.cc
libsrcs_pyami += Discovery.cc
libsrcs_pyami += Client.cc
libincs_pyami := python/include/python2.5 pdsdata/include ndarray/include boost/include 
libincs_pyami += ndarray/include boost/include 
liblibs_pyami := pdsdata/xtcdata pdsdata/psddl_pdsdata pdsdata/compressdata
liblibs_pyami += pdsalg/pdsalg
liblibs_pyami += gsl/gsl gsl/gslcblas
liblibs_pyami += ami/amisvc ami/amidata ami/server ami/client
liblibs_pyami += qt/QtCore
libslib_pyami := $(USRLIBDIR)/rt gomp
