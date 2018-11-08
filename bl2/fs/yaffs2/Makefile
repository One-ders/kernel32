
OBJS=yaffs_allocator.o yaffs_attribs.o yaffs_bitmap.o yaffs_checkptrw.o \
     yaffs_ecc.o yaffs_endian.o yaffs_error.o yaffsfs.o yaffs_guts.o \
     yaffs_hweight.o yaffs_nameval.o yaffs_nand.o yaffs_packedtags1.o \
     yaffs_packedtags2.o yaffs_summary.o yaffs_tagscompat.o \
     yaffs_tagsmarshall.o yaffs_verify.o yaffs_yaffs1.o yaffs_yaffs2.o \
     yaffs_drv.o

LIB_NAME=fs

SYS_INC=$(TOP)/incl
SYS_LIB=$(OBJ)/lib

CFLAGS+=$(DEP_INC)

fs_all: $(SYS_LIB) $(SYS_LIB)/$(LIB_NAME).o

$(SYS_LIB):
	mkdir $@

$(SYS_LIB)/$(LIB_NAME).o: $(LIB_NAME).o
	cp $^ $@

$(LIB_NAME).o: $(OBJS)
	$(LD) -r -o $@ $^

mm_clean:
	rm -rf *.o

