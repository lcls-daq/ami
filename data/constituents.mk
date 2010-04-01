ifneq ($(findstring x86_64-linux,$(tgt_arch)),)
qtincdir  := qt/include64
else
qtincdir  := qt/include
endif

# List libraries (if any) for this package
libnames := data

# List source files for each library
libsrcs_data := $(wildcard *.cc)

libincs_data := $(qtincdir)

# List special include directories (if any) needed by lib_a as
# <project>/<incdir>. Note that the top level release directory is
# already in the search path.
# libincs_lib_a := prj_x/include/Linux

# List system include directories (if any) needed by lib_a as <incdir>.
# libsinc_lib_a := /usr/include
