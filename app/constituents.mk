# List targets (if any) for this package
tgtnames := ami test

# List source files for each target
tgtsrcs_ami := $(filter-out test.cc,$(wildcard *.cc))
tgtsrcs_test := test.cc

# List system libraries (if any) needed by exe_a as <dir>/<lib>. 
# Note that <lib> is the name of the library, not of the file: i.e.
# <lib> for 'libc.so' is 'c'. Low level first.
tgtslib_ami := /usr/lib/rt
tgtslib_test := /usr/lib/rt

# List project libraries (if any) needed by exe_a as <project>/<lib>.
# Note that <lib> is the name of the library, not of the file: i.e.
# <lib> for 'libc.so' is 'c'. Low level first.
tgtlibs_ami := pdsdata/xtcdata pdsdata/acqdata
tgtlibs_ami += pdsdata/camdata pdsdata/opal1kdata
tgtlibs_ami += pdsdata/controldata
tgtlibs_ami += ami/service ami/data ami/server ami/event
tgtlibs_ami += qt/QtCore

# List special include directories (if any) needed by exe_a as
# <project>/<incdir>. Note that the top level release directory is
# already in the search path.
# tgtincs_exe_a := prj_x/include prj_x/include/Linux
tgtincs_ami := qt/include

# List system include directories (if any) needed by exe_a as <incdir>.
# tgtsinc_exe_a := /usr/include

# List libraries (if any) for this package
# libnames := lib_a lib_b

# List source files for each library
# libsrcs_lib_a := src_4.cc src_5.cc
# libsrcs_lib_b := src_6.cc

# List special include directories (if any) needed by lib_a as
# <project>/<incdir>. Note that the top level release directory is
# already in the search path.
# libincs_lib_a := prj_x/include/Linux

# List system include directories (if any) needed by lib_a as <incdir>.
# libsinc_lib_a := /usr/include
