# List packages for this project. Low level first.
ifneq ($(findstring x86_64,$(tgt_arch)),)
packages := service data server client python event app qt
else
packages := service data server client event app qt
endif