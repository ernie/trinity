# Trinity Build

DISTDIR = dist

all: tools qvms

tools:
	$(MAKE) -C tools

qvms: $(DISTDIR)
	$(MAKE) -C build DISTDIR=../$(DISTDIR)

$(DISTDIR):
	mkdir -p $(DISTDIR)

clean:
	$(MAKE) -C build clean
	rm -rf $(DISTDIR)

clean-tools:
	$(MAKE) -C tools clean

clean-all: clean clean-tools

.PHONY: all tools qvms clean clean-tools clean-all
