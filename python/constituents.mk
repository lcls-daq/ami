libnames := pyami

libsrcs_pyami := pyami.cc
libsrcs_pyami += Discovery.cc
libsrcs_pyami += Handler.cc Client.cc L3TClient.cc
libincs_pyami := python/include/python2.5 pdsdata/include ndarray/include boost/include 
libincs_pyami += ndarray/include boost/include 
liblibs_pyami := pdsdata/xtcdata pdsdata/psddl_pdsdata pdsdata/compressdata pdsdata/appdata
liblibs_pyami += psalg/psalg
liblibs_pyami += gsl/gsl gsl/gslcblas
liblibs_pyami += ami/amisvc ami/amidata ami/server ami/client ami/calib ami/event ami/app
liblibs_pyami += qt/QtCore
libslib_pyami := $(USRLIBDIR)/rt gomp
