# Trinity Tools Build
# Builds q3asm and q3lcc (q3cpp, q3rcc) from submodules

# Platform detection
ifeq ($(OS),Windows_NT)
  PLATFORM=mingw32
  ifeq ($(PROCESSOR_ARCHITEW6432),AMD64)
    ARCH=x86_64
  else ifeq ($(PROCESSOR_ARCHITECTURE),AMD64)
    ARCH=x86_64
  else
    ARCH=x86
  endif
else
  UNAME_S := $(shell uname -s)
  UNAME_M := $(shell uname -m)
  ifeq ($(UNAME_S),Linux)
    PLATFORM=linux
  else ifeq ($(UNAME_S),Darwin)
    PLATFORM=darwin
  else ifeq ($(UNAME_S),SunOS)
    PLATFORM=sunos
  else
    PLATFORM=unix
  endif
  ifeq ($(UNAME_M),x86_64)
    ARCH=x86_64
  else ifeq ($(UNAME_M),aarch64)
    ARCH=arm64
  else ifeq ($(UNAME_M),armv7l)
    ARCH=arm
  else
    ARCH=$(UNAME_M)
  endif
endif

export PLATFORM
export ARCH

ifeq ($(PLATFORM),mingw32)
  E=.exe
else
  E=
endif

BINDIR=bin

all: $(BINDIR) q3asm q3lcc

$(BINDIR):
	mkdir -p $(BINDIR)

q3asm:
	$(MAKE) -C q3asm
	cp q3asm/q3asm$(E) $(BINDIR)/

q3lcc:
	$(MAKE) -C q3lcc
	cp q3lcc/build-$(PLATFORM)-$(ARCH)/q3lcc$(E) $(BINDIR)/
	cp q3lcc/build-$(PLATFORM)-$(ARCH)/q3cpp$(E) $(BINDIR)/
	cp q3lcc/build-$(PLATFORM)-$(ARCH)/q3rcc$(E) $(BINDIR)/

clean:
	$(MAKE) -C q3asm clean
	$(MAKE) -C q3lcc clean
	rm -rf $(BINDIR)

.PHONY: all q3asm q3lcc clean
