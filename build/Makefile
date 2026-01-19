# Build both pk3s
DISTDIR ?= .

all: baseq3 missionpack

baseq3:
	$(MAKE) -f Makefile.build CONFIG=baseq3 DISTDIR=$(DISTDIR)

missionpack:
	$(MAKE) -f Makefile.build CONFIG=missionpack DISTDIR=$(DISTDIR)

clean:
	$(RM) -rf vm-baseq3 vm-missionpack staging-baseq3 staging-missionpack

.PHONY: all baseq3 missionpack clean
