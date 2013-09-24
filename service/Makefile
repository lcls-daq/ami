# Package level makefile
# ----------------------
%.mk:;

# Checks
# ------
# Check release location variables
ifeq ($(RELEASE_DIR),)
export RELEASE_DIR := $(PWD)/../..
endif

include $(RELEASE_DIR)/make/share/setup.mk
include ../flags.mk

ifndef PREMAKE_DONE
include $(RELEASE_DIR)/make/share/premake.mk
else
include constituents.mk
include $(RELEASE_DIR)/make/sw/package.mk

#
#  Always stamp the build time and tag
#
.PHONY: $(objdir)/BuildStamp.o

#BUILDTIME := \"`date | sed -e 's/\ /_/g'`\"
BUILDTIME := `date`
BUILDTAG  := `cvs log Makefile | grep V..-..-.. | cut -c 2-10 | head -1`

$(objdir)/BuildStamp.o: BuildStamp.cc
	@echo "[TX] Compiling $< $@"
	@echo "[TX] BUILDTIME=$(BUILDTIME)"
	@echo "[TX] BUILDTAG =$(BUILDTAG)"
	$(quiet)$(CXX) $(incdirs_$<) $(CPPFLAGS) $(DEFINES) $(CXXFLAGS) -DAMI_BUILD_TIME='$(BUILDTIME)' -DAMI_BUILD_TAG='$(BUILDTAG)' -c $< -o $@

endif
