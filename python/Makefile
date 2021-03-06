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
#  Special rules for creating python C++ extension libraries
#
pyso  := $(libdir)/pyami.so
pysoabi := $(patsubst %.so,%.abi3.so,$(pyso))
pyso2 := $(addsuffix .2,$(pyso))
pyso3 := $(addsuffix .3,$(pyso))

userall: $(pyso) $(pysoabi) $(pyso2) $(pyso3)

userclean:
	$(quiet)$(RM) $(pyso) $(pysoabi) $(pyso2) $(pyso3)

# Libraries
$(libdir)/py%.abi3.$(LIBEXTNS):
	@echo "[LD] Build python library $*"
	$(quiet)ln -s py$*.$(LIBEXTNS).3 $@

$(libdir)/py%.$(LIBEXTNS):
	@echo "[LD] Build python library $*"
	$(quiet)ln -s py$*.$(LIBEXTNS).2 $@

$(libdir)/py%.$(LIBEXTNS).2:
	@echo "[LD] Build python library $*"
	$(quiet)ln -s libpy$*.$(LIBEXTNS) $@

$(libdir)/py%.$(LIBEXTNS).3:
	@echo "[LD] Build python library $*"
	$(quiet)ln -s libpy3$*.$(LIBEXTNS) $@

endif
