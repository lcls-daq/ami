include $(RELEASE_DIR)/make/sw/flags.mk
include $(RELEASE_DIR)/make/sw/qtflags.mk

#DEFINES += -DDBUG
#DEFINES += -fopenmp
ifneq ($(findstring -dbg,$(tgt_arch)),)
  DEFINES += -DVALGND
endif

false :=
true  := t
#build_extra := $(true)
build_extra := $(false)
