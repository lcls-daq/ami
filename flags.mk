include $(RELEASE_DIR)/make/sw/flags.mk

ifneq ($(findstring x86_64-linux,$(tgt_arch)),)
  DEFINES += -fopenmp
else
  DEFINES += -fopenmp
endif