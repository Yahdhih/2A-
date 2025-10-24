FREEIMAGE_ROOT?=/home/dureaud/softs/FreeImage
UTILS=../../utils

CC=mpicxx
CFLAGS=
OFLAGS=-O3  -use_fast_math
IFLAGS=-I$(UTILS) -I$(FREEIMAGE_ROOT)/include
LFLAGS=$(FREEIMAGE_ROOT)/lib/libfreeimage.a -lm

.PHONY: all clean

all: img_diffcol.exe img_splitv.exe img_findcol.exe img_findcol2.exe

img_diffcol.exe: Image_diffcol.c $(UTILS)/kernel.h $(UTILS)/image.h $(UTILS)/basics.h $(UTILS)/image.c
	$(CC) $(CFLAGS) $(OFLAGS) $(IFLAGS) $(UTILS)/image.c Image_diffcol.c -o $@ $(LFLAGS)

img_splitv.exe: Image_splitv.c $(UTILS)/kernel.h $(UTILS)/image.h $(UTILS)/basics.h $(UTILS)/image.c
	$(CC) $(CFLAGS) $(OFLAGS) $(IFLAGS) $(UTILS)/image.c Image_splitv.c -o $@ $(LFLAGS)

img_findcol.exe: Image_findcol.c $(UTILS)/kernel.h $(UTILS)/image.h $(UTILS)/basics.h $(UTILS)/image.c
	$(CC) $(CFLAGS) $(OFLAGS) $(IFLAGS) $(UTILS)/image.c Image_findcol.c -o $@ $(LFLAGS)

img_findcol2.exe: Image_findcol2.c $(UTILS)/kernel.h $(UTILS)/image.h $(UTILS)/basics.h $(UTILS)/image.c
	$(CC) $(CFLAGS) $(OFLAGS) $(IFLAGS) $(UTILS)/image.c Image_findcol2.c -o $@ $(LFLAGS)

clean:
	rm -Rf *~ *.o *.exe new_img.png slurm-*
