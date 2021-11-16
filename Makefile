#Modify this to point to the PJSIP location.
PJBASE=/home/vlbrazhnikov/Local_Rep/eltex_answer_machine/pjproject-2.11_x86

include $(PJBASE)/build.mak

CC      = $(PJ_CC)
LDFLAGS = $(PJ_LDFLAGS)
LDLIBS  = $(PJ_LDLIBS)
CFLAGS  = $(PJ_CFLAGS)
CPPFLAGS = -g3
#CPPFLAGS= ${CFLAGS}


# If your application is in a file named myapp.cpp or myapp.c
# this is the line you will need to build the binary.
all: sbc  

myapp: sbc.c
	$(CC) -o $@ $< $(CPPFLAGS) $(LDFLAGS) $(LDLIBS)

clean:
	rm -f sbc.o mini-sbc 
