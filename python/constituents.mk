libnames := pyami py3ami

libsrcs_pyami := pyami.cc
libsrcs_pyami += Discovery.cc
libsrcs_pyami += Handler.cc Client.cc L3TClient.cc
libincs_pyami := python/include/python2.5 
libincs_pyami += pdsdata/include ndarray/include boost/include 
libincs_pyami += ndarray/include boost/include 
liblibs_pyami := pdsdata/xtcdata pdsdata/psddl_pdsdata pdsdata/compressdata pdsdata/appdata
liblibs_pyami += psalg/psalg
liblibs_pyami += gsl/gsl gsl/gslcblas
liblibs_pyami += ami/amisvc ami/amidata ami/server ami/client ami/calib ami/event ami/app
liblibs_pyami += qt/QtCore
libslib_pyami := $(USRLIBDIR)/rt gomp

libsrcs_py3ami := py3ami.cc
libsrcs_py3ami += Discovery.cc
libsrcs_py3ami += Handler.cc Client.cc L3TClient.cc
libincs_py3ami := python3/include/python3.6m
libincs_py3ami += pdsdata/include ndarray/include boost/include
libincs_py3ami += ndarray/include boost/include
liblibs_py3ami := pdsdata/xtcdata pdsdata/psddl_pdsdata pdsdata/compressdata pdsdata/appdata
liblibs_py3ami += psalg/psalg
liblibs_py3ami += gsl/gsl gsl/gslcblas
liblibs_py3ami += ami/amisvc ami/amidata ami/server ami/client ami/calib ami/event ami/app
liblibs_py3ami += qt/QtCore
libslib_py3ami := $(USRLIBDIR)/rt gomp
