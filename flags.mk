include $(RELEASE_DIR)/make/sw/flags.mk
include $(RELEASE_DIR)/make/sw/qtflags.mk

#DEFINES += -DDBUG
DEFINES += -fopenmp

false :=
true  := t
#build_extra := $(true)
build_extra := $(false)
