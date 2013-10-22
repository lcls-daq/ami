# List packages for this project. Low level first.
packages := service data server client event app l3t qt plugins
ifneq ($(findstring x86_64,$(tgt_arch)),)
packages += python
endif