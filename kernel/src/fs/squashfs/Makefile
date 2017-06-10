#
# Makefile for the linux squashfs routines.
#

O_TARGET := squashfs.o

obj-y  := inode.o 
ifdef CONFIG_LZMA_FS_INFLATE
#obj-y += $(TOPDIR)/fs/inflate_lzma/7zlzma.o \
	$(TOPDIR)/fs/inflate_lzma/LZMADecoder.o \
	$(TOPDIR)/fs/inflate_lzma/IInOutStreams.o
obj-y += LzmaDecode.o uncompress.o
endif
obj-m := $(O_TARGET)

include $(TOPDIR)/Rules.make
