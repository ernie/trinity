PK3 = pak3t.pk3

basedir = ../code

QADIR = $(basedir)/game
CGDIR = $(basedir)/cgame
UIDIR = $(basedir)/ui

Q3ASM = ../tools/bin/q3asm -vq3 -r -m -v
Q3LCC = ../../../tools/bin/q3lcc -DQ3_VM -S -Wf-g -I$(QADIR)
7Z = 7z u -tzip -mx=9 -mpass=8 -mfb=255 --

QA_CFLAGS = -DQAGAME -DMISSIONPACK
CG_CFLAGS = -DCGAME -DMISSIONPACK -I$(CGDIR) -I$(UIDIR)
UI_CFLAGS = -DUI -DMISSIONPACK -I$(UIDIR)

EXTRA_ASSETS = ../assets-missionpack/*

include srcs.mk
