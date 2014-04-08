# List packages for this project. Low level first.
packages :=
ifneq ($(findstring x86_64,$(tgt_arch)),)
packages += service data server client event app l3t qt plugins python
endif