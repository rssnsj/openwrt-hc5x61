EXE:=8021xd

EXTRA_CFLAGS += -O2 -Wall -g $(CFLAGS)

# If you want to support multiple radius server for multiple bssid, add following line
EXTRA_CFLAGS +=  -DMULTIPLE_RADIUS=1

# If you want to debug daemon, add following line
EXTRA_CFLAGS +=  -DDBG=1

OBJS =	rtdot1x.o eloop.o eapol_sm.o radius.o md5.o  \
	config.o ieee802_1x.o  \
	sta_info.o   radius_client.o

all: $(EXE) 

$(EXE): $(OBJS)
	$(CC) $(EXTRA_CFLAGS) -o $@ $(OBJS)

clean:
	-@rm -f *~ *.o $(EXE) *.d

$(OBJS): %.o : %.c
	$(CC) $(EXTRA_CFLAGS) -c $< -o $@

