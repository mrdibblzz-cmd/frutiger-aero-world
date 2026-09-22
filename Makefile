TARGET = FrutigerAeroWorld
OBJS = src/main.o

CFLAGS = -O2 -G0 -Wall -ffast-math
LIBS = -lpspgum -lpspgu -lpspctrl -lpspdisplay -lm

EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = Frutiger Aero World
PSP_EBOOT_ICON = data/ICON0.PNG

PSPSDK = $(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak
