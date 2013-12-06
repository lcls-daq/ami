# List libraries (if any) for this package
libnames := amidata

# List source files for each library
unused_srcs  := Assembler.cc DescImageC.cc EntryImageC.cc Integral.cc 
libsrcs_amidata := $(filter-out $(unused_srcs), $(wildcard *.cc))

libincs_amidata := $(qtincdir) ndarray/include boost/include pdsalg/include  pdsdata/include ndarray/include boost/include gsl/include

# List special include directories (if any) needed by lib_a as
# <project>/<incdir>. Note that the top level release directory is
# already in the search path.
# libincs_lib_a := prj_x/include/Linux

# List system include directories (if any) needed by lib_a as <incdir>.
# libsinc_lib_a := /usr/include
