# DDS Asteroids - build file (GNU make). Recipes use '>' instead of TAB.
.RECIPEPREFIX = >

CC    ?= gcc
# Sanitizer build: make SAN=asan  (ASan+UBSan)  or  make SAN=tsan
SAN   ?=
OUT   := build/$(if $(SAN),$(SAN),release)
GEN   := build/gen

CSTD   := -std=gnu11
WARN   := -Wall -Wextra -Wformat=2 -Wformat-security
OPT    := -O2 -g
# Week 8 built-in protections: stack canary, fortify, PIE (+ full RELRO at link time)
HARDEN := -fstack-protector-strong -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fPIE
LDHARD := -pie -Wl,-z,relro,-z,now

ifeq ($(SAN),asan)
  SANFLAGS := -fsanitize=address,undefined -fno-omit-frame-pointer
else ifeq ($(SAN),tsan)
  SANFLAGS := -fsanitize=thread
endif

INC     := -Isrc -I$(GEN) -Ithird_party
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

.PHONY: all baseline clean check-hardening
all: $(OUT)/server $(OUT)/client

# Generate DDS C bindings from the IDL (messages.c / messages.h are NOT committed)
$(GEN)/messages.c $(GEN)/messages.h: idl/messages.idl
> mkdir -p $(GEN)
> cd $(GEN) && idlc -l c ../../idl/messages.idl

$(OUT)/server: $(SERVER_SRC) $(COMMON_SRC) $(GEN)/messages.c
> @test -n "$(SERVER_SRC)" || (echo "No sources in src/server yet"; exit 1)
> mkdir -p $(OUT)
> $(CC) $(CFLAGS) $(DDS_CF) $(SSL_CF) $^ -o $@ $(LDFLAGS) $(DDS_LD) $(SSL_LD) -pthread -lm

$(OUT)/client: $(CLIENT_SRC) $(COMMON_SRC) $(GEN)/messages.c
> @test -n "$(CLIENT_SRC)" || (echo "No sources in src/client yet"; exit 1)
> mkdir -p $(OUT)
> $(CC) $(CFLAGS) $(DDS_CF) $(GL_CF) $(SSL_CF) $^ -o $@ $(LDFLAGS) $(DDS_LD) $(GL_LD) $(SSL_LD) -pthread

# The lecturer-provided programs, kept for before/after comparison
baseline: $(OUT)/baseline-server $(OUT)/baseline-client
$(OUT)/baseline-server: baseline/server.c $(GEN)/messages.c
> mkdir -p $(OUT)
> $(CC) $(CFLAGS) $(DDS_CF) $^ -o $@ $(LDFLAGS) $(DDS_LD) -pthread
$(OUT)/baseline-client: baseline/client.c $(GEN)/messages.c
> mkdir -p $(OUT)
> $(CC) $(CFLAGS) $(DDS_CF) $(GL_CF) $^ -o $@ $(LDFLAGS) $(DDS_LD) $(GL_LD) -pthread

# Verify PIE / NX / RELRO / stack canary are really present in the binaries
check-hardening: all
> @for b in server client; do echo "== $$b"; \
>   readelf -h $(OUT)/$$b | grep 'Type:'; \
>   readelf -lW $(OUT)/$$b | grep -E 'GNU_STACK|GNU_RELRO'; \
>   echo -n "stack_chk refs: "; readelf -sW $(OUT)/$$b | grep -c __stack_chk_fail || true; done

clean:
> rm -rf build
