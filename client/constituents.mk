# List targets (if any) for this package
tgtnames := clienttest

# List source files for each target
tgtsrcs_clienttest := clienttest.cc

# List system libraries (if any) needed by exe_a as <dir>/<lib>. 
# Note that <lib> is the name of the library, not of the file: i.e.
# <lib> for 'libc.so' is 'c'. Low level first.
tgtslib_clienttest := /usr/lib/rt

# List project libraries (if any) needed by exe_a as <project>/<lib>.
# Note that <lib> is the name of the library, not of the file: i.e.
# <lib> for 'libc.so' is 'c'. Low level first.
tgtlibs_clienttest := pdsdata/xtcdata 
tgtlibs_clienttest += ami/service ami/data ami/client
tgtlibs_clienttest += qt/QtCore

# List special include directories (if any) needed by exe_a as
# <project>/<incdir>. Note that the top level release directory is
# already in the search path.
# tgtincs_exe_a := prj_x/include prj_x/include/Linux

# List system include directories (if any) needed by exe_a as <incdir>.
# tgtsinc_exe_a := /usr/include

# List libraries (if any) for this package
libnames := client

# List source files for each library
libsrcs_client := VClientSocket.cc VClientManager.cc

# List special include directories (if any) needed by lib_a as
# <project>/<incdir>. Note that the top level release directory is
# already in the search path.
# libincs_lib_a := prj_x/include/Linux

# List system include directories (if any) needed by lib_a as <incdir>.
# libsinc_lib_a := /usr/include
