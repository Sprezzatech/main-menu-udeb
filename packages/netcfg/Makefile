CC		= gcc
TARGETS		?= netcfg-static netcfg

LDOPTS		= -ldebconfclient -ldebian-installer
CFLAGS		= -W -Wall -DNDEBUG 
COMMON_OBJS	= netcfg-common.o wireless.o

WIRELESS	= 1
ifneq ($(DEB_HOST_ARCH_OS),linux)
WIRELESS	= 0
endif
ifeq ($(DEB_HOST_ARCH),s390)
WIRELESS	= 0
endif

ifneq ($(WIRELESS),0)
LDOPTS		+= -liw
CFLAGS		+= -DWIRELESS
endif

ifneq (,$(findstring noopt,$(DEB_BUILD_OPTIONS)))
CFLAGS += -O0 -g3
else
CFLAGS += -Os -fomit-frame-pointer
endif

all: $(TARGETS)

netcfg-static: netcfg-static.o static.o
netcfg: netcfg.o dhcp.o static.o ethtool-lite.o

ethtool-lite: ethtool-lite-test.o
	$(CC) -o $@ $<
	
ethtool-lite-test.o: ethtool-lite.c
	$(CC) -c $(CFLAGS) -DTEST $(DEFS) $(INCS) -o $@ $<

$(TARGETS): $(COMMON_OBJS)
	$(CC) -o $@ $^ $(LDOPTS)

%.o: %.c
	$(CC) -c $(CFLAGS) $(DEFS) $(INCS) -o $@ $<

clean:
	rm -f $(TARGETS) ethtool-lite *.o

.PHONY: all clean

# vim:ts=8:noet
