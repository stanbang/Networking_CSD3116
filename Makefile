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
# Explicit build-time override lets isolated dependency checks instrument every
# native security plugin as well as libddsc. Normal Ubuntu builds keep this path.
DDS_PLUGIN_DIR ?= /usr/lib/x86_64-linux-gnu/libddsc0debian
# Week 8 built-in protections: stack canary, fortify, PIE (+ full RELRO at link time)
HARDEN := -fstack-protector-strong -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fPIE
LDHARD := -pie -Wl,-z,relro,-z,now,-z,noexecstack

ifeq ($(SAN),asan)
  SANFLAGS := -fsanitize=address,undefined -fno-omit-frame-pointer
else ifeq ($(SAN),tsan)
  SANFLAGS := -fsanitize=thread -fno-omit-frame-pointer
endif

INC     := -Isrc -I$(GEN) -isystem third_party
CFLAGS  := $(CSTD) $(OPT) $(WARN) $(HARDEN) $(SANFLAGS) $(INC) -DAST_PLUGIN_DIR='"$(DDS_PLUGIN_DIR)/"'
LDFLAGS := $(LDHARD) $(SANFLAGS)

DDS_CF := $(shell pkg-config --cflags CycloneDDS)
DDS_LD := $(shell pkg-config --libs CycloneDDS)
GL_CF  := $(shell pkg-config --cflags glfw3)
GL_LD  := $(shell pkg-config --libs glfw3) -lGL -lm
SSL_CF := $(shell pkg-config --cflags openssl)
SSL_LD := $(shell pkg-config --libs openssl)
SQLITE_CF := $(shell pkg-config --cflags sqlite3)
SQLITE_LD := $(shell pkg-config --libs sqlite3)
WIRE_GEN := $(GEN)/messages.c $(GEN)/messages.h $(GEN)/accounts.c $(GEN)/accounts.h

COMMON_SRC := $(wildcard src/common/*.c)
SERVER_SRC := $(wildcard src/server/*.c)
CLIENT_SRC := $(wildcard src/client/*.c)

.PHONY: all baseline clean check-hardening test
all: $(OUT)/server $(OUT)/client

# Generate DDS C bindings from the IDL (messages.c / messages.h are NOT committed)
$(GEN)/messages.c $(GEN)/messages.h &: idl/messages.idl
> mkdir -p $(GEN)
> cd $(GEN) && idlc -l c ../../idl/messages.idl

$(GEN)/accounts.c $(GEN)/accounts.h &: idl/accounts.idl
> mkdir -p $(GEN)
> cd $(GEN) && idlc -l c ../../idl/accounts.idl

HEADERS := $(wildcard src/common/*.h src/server/*.h src/client/*.h)
$(OUT)/server: $(SERVER_SRC) $(COMMON_SRC) $(WIRE_GEN) $(HEADERS) Makefile
> @test -n "$(SERVER_SRC)" || (echo "No sources in src/server yet"; exit 1)
> mkdir -p $(OUT)
> $(CC) $(CFLAGS) $(DDS_CF) $(SSL_CF) $(SQLITE_CF) $(filter %.c,$^) -o $@ $(LDFLAGS) $(DDS_LD) $(SSL_LD) $(SQLITE_LD) -pthread -lm

$(OUT)/client: $(CLIENT_SRC) $(COMMON_SRC) $(WIRE_GEN) $(HEADERS) Makefile
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

$(OUT)/test-protocol: tests/test_protocol.c $(COMMON_SRC) $(WIRE_GEN) $(HEADERS) Makefile
> mkdir -p $(OUT)
> $(CC) $(CFLAGS) $(DDS_CF) $(SSL_CF) $(filter %.c,$^) -o $@ $(LDFLAGS) $(DDS_LD) $(SSL_LD) -pthread -lm
$(OUT)/test-sim: tests/test_sim.c src/common/sim.c src/common/sim.h src/common/config.h Makefile
> mkdir -p $(OUT)
> $(CC) $(CFLAGS) tests/test_sim.c src/common/sim.c -o $@ $(LDFLAGS) -lm
$(OUT)/test-event-queue: tests/test_event_queue.c $(COMMON_SRC) $(WIRE_GEN) $(HEADERS) Makefile
> mkdir -p $(OUT)
> $(CC) $(CFLAGS) $(DDS_CF) $(SSL_CF) $(filter %.c,$^) -o $@ $(LDFLAGS) $(DDS_LD) $(SSL_LD) -pthread -lm

$(OUT)/account-join-probe: tests/account_join_probe.c $(COMMON_SRC) $(WIRE_GEN) $(HEADERS) Makefile
> mkdir -p $(OUT)
> $(CC) $(CFLAGS) $(DDS_CF) $(SSL_CF) $(filter %.c,$^) -o $@ $(LDFLAGS) $(DDS_LD) $(SSL_LD) -pthread -lm

$(OUT)/test-account-store: tests/test_account_store.c src/server/account_store.c src/server/account_store.h Makefile
> mkdir -p $(OUT)
> $(CC) $(CFLAGS) $(SQLITE_CF) $(SSL_CF) $(filter %.c,$^) -o $@ $(LDFLAGS) -Wl,--wrap=RAND_bytes,--wrap=time $(SQLITE_LD) $(SSL_LD)

test: $(OUT)/test-protocol $(OUT)/test-sim $(OUT)/test-event-queue $(OUT)/test-account-store
> ./$(OUT)/test-protocol
> ./$(OUT)/test-sim
> ./$(OUT)/test-event-queue
> ./$(OUT)/test-account-store

# Verify PIE / NX / RELRO / stack canary are really present in the binaries
check-hardening: all
> bash tools/check_hardening.sh $(OUT)/server $(OUT)/client

clean:
> rm -rf build
