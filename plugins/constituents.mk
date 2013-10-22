# List libraries (if any) for this package
libnames := timetool

# List source files for each library
libsrcs_timetool := TimeToolM.cc

# List special include directories (if any) needed by lib_a as
# <project>/<incdir>. Note that the top level release directory is
# already in the search path.
libincs_timetool := pdsdata/include ndarray/include pdsalg/include

# List system include directories (if any) needed by lib_a as <incdir>.
# libsinc_lib_a := /usr/include

liblibs_timetool := timetool/ttsvc pdsalg/pdsalg
