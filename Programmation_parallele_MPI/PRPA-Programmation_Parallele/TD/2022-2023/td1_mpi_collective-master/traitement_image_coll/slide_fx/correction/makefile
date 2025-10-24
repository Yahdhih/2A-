FREEIMAGE_ROOT?=/home/dureaud/softs/FreeImage
UTILS=../../utils

CC=mpicxx
CFLAGS=
OFLAGS=-O3  -use_fast_math
IFLAGS=-I$(UTILS) -I$(FREEIMAGE_ROOT)/include
LFLAGS=$(FREEIMAGE_ROOT)/lib/libfreeimage.a -lm

.PHONY: all clean

all: img_slidefx.exe 

img_slidefx.exe: Image_slide.c $(UTILS)/kernel.h $(UTILS)/image.h $(UTILS)/basics.h $(UTILS)/image.c
	$(CC) $(CFLAGS) $(OFLAGS) $(IFLAGS) $(UTILS)/image.c Image_slide.c -o $@ $(LFLAGS)

clean:
	rm -Rf *~ *.o *.exe new_img.png slurm-*
