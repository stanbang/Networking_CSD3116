# DDS Asteroids - build file (GNU make). Recipes use '>' instead of TAB.
.RECIPEPREFIX = >

CC    ?= gcc
# Sanitizer build: make SAN=asan  (ASan+UBSan)  or  make SAN=tsan
SAN   ?=
ifneq ($(SAN),)
  ifneq ($(SAN),asan)
    ifneq ($(SAN),tsan)
      $(error Unsupported SAN value '$(SAN)'; expected asan or tsan)
    endif
  endif
endif
OUT   := build/$(if $(SAN),$(SAN),release)
GEN   := build/gen

CSTD   := -std=gnu11
WARN   := -Wall -Wextra -Wformat=2 -Wformat-security
OPT    := -O2 -g
# Week 8 built-in protections: stack canary, fortify, PIE (+ full RELRO at link time)
HARDEN := -fstack-protector-strong -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fPIE
LDHARD := -pie -Wl,-z,relro,-z,now,-z,noexecstack

ifeq ($(SAN),asan)
  SANFLAGS := -fsanitize=address,undefined -fno-omit-frame-pointer
else ifeq ($(SAN),tsan)
  SANFLAGS := -fsanitize=thread -fno-omit-frame-pointer
endif

INC     := -Isrc -I$(GEN) -isystem third_party
CFLAGS  := $(CSTD) $(OPT) $(WARN) $(HARDEN) $(SANFLAGS) $(INC)
LDFLAGS := $(LDHARD) $(SANFLAGS)

DDS_CF := $(shell pkg-config --cflags CycloneDDS)
DDS_LD := $(shell pkg-config --libs CycloneDDS)
GL_CF  := $(shell pkg-config --cflags glfw3)
GL_LD  := $(shell pkg-config --libs glfw3) -lGL -lm
SSL_CF := $(shell pkg-config --cflags openssl)
SSL_LD := $(shell pkg-config --libs openssl)

COMMON_SRC := $(wildcard src/common/*.c)
SERVER_SRC := $(wildcard src/server/*.c)
CLIENT_SRC := $(wildcard src/client/*.c)

.PHONY: all baseline clean check-hardening test
all: $(OUT)/server $(OUT)/client

# Generate DDS C bindings from the IDL (messages.c / messages.h are NOT committed)
$(GEN)/messages.c $(GEN)/messages.h &: idl/messages.idl
> mkdir -p $(GEN)
> cd $(GEN) && idlc -l c ../../idl/messages.idl

HEADERS := $(wildcard src/common/*.h src/server/*.h src/client/*.h)
$(OUT)/server: $(SERVER_SRC) $(COMMON_SRC) $(GEN)/messages.c $(GEN)/messages.h $(HEADERS) Makefile
> @test -n "$(SERVER_SRC)" || (echo "No sources in src/server yet"; exit 1)
> mkdir -p $(OUT)
> $(CC) $(CFLAGS) $(DDS_CF) $(SSL_CF) $(filter %.c,$^) -o $@ $(LDFLAGS) $(DDS_LD) $(SSL_LD) -pthread -lm

$(OUT)/client: $(CLIENT_SRC) $(COMMON_SRC) $(GEN)/messages.c $(GEN)/messages.h $(HEADERS) Makefile
> @test -n "$(CLIENT_SRC)" || (echo "No sources in src/client yet"; exit 1)
> mkdir -p $(OUT)
> $(CC) $(CFLAGS) $(DDS_CF) $(GL_CF) $(SSL_CF) $(filter %.c,$^) -o $@ $(LDFLAGS) $(DDS_LD) $(GL_LD) $(SSL_LD) -pthread

# The lecturer-provided programs, kept for before/after comparison
baseline: $(OUT)/baseline-server $(OUT)/baseline-client
BASEGEN := build/baseline-gen
# Preserve lecturer wire types independently of the evolving v2 IDL.
$(BASEGEN)/messages.c $(BASEGEN)/messages.h &: idl/baseline.idl
> mkdir -p $(BASEGEN)
> cp $< $(BASEGEN)/messages.idl
> cd $(BASEGEN) && idlc -l c messages.idl
$(OUT)/baseline-server: baseline/server.c $(BASEGEN)/messages.c $(BASEGEN)/messages.h
> mkdir -p $(OUT)
> $(CC) -I$(BASEGEN) $(CFLAGS) $(DDS_CF) $(filter %.c,$^) -o $@ $(LDFLAGS) $(DDS_LD) -pthread
$(OUT)/baseline-client: baseline/client.c $(BASEGEN)/messages.c $(BASEGEN)/messages.h
> mkdir -p $(OUT)
> $(CC) -I$(BASEGEN) $(CFLAGS) $(DDS_CF) $(GL_CF) $(filter %.c,$^) -o $@ $(LDFLAGS) $(DDS_LD) $(GL_LD) -pthread

$(OUT)/test-protocol: tests/test_protocol.c $(COMMON_SRC) $(GEN)/messages.c $(GEN)/messages.h $(HEADERS) Makefile
> mkdir -p $(OUT)
> $(CC) $(CFLAGS) $(DDS_CF) $(SSL_CF) $(filter %.c,$^) -o $@ $(LDFLAGS) $(DDS_LD) $(SSL_LD) -pthread -lm
$(OUT)/test-sim: tests/test_sim.c src/common/sim.c src/common/sim.h src/common/config.h Makefile
> mkdir -p $(OUT)
> $(CC) $(CFLAGS) tests/test_sim.c src/common/sim.c -o $@ $(LDFLAGS) -lm
test: $(OUT)/test-protocol $(OUT)/test-sim
> ./$(OUT)/test-protocol
> ./$(OUT)/test-sim

# Verify PIE / NX / RELRO / stack canary are really present in the binaries
check-hardening: all
> bash tools/check_hardening.sh $(OUT)/server $(OUT)/client

clean:
> rm -rf build
