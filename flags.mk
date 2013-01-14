include $(RELEASE_DIR)/make/sw/flags.mk

#DEFINES += -DDBUG

ifneq ($(findstring x86_64-linux,$(tgt_arch)),)
  DEFINES += -fopenmp
else
  DEFINES += -fopenmp
endif