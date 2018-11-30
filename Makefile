#########################
SOURCES := cmdline.c BVTserialInterfacer.c serial_jjm.c convenient_wrapper_functions.c 
PROG := BVTserialInterfacer
CFLAGS := -Wall -Wextra -std=gnu99
LDFLAGS := -lserialport

#For install
ifeq ($(PREFIX),)
    PREFIX := /usr/local
endif
########################

# -MMD generates dependencies while compiling
CFLAGS += -MMD
CC := gcc

#for release 
release: $(PROG) 
release: CFLAGS += -O3


OBJFILES := $(SOURCES:.c=.o)
DEPFILES := $(SOURCES:.c=.d)

$(PROG) : $(OBJFILES)
	$(LINK.o) $(LDFLAGS) -o $@ $^

#For debug 
debug: CFLAGS += -DDEBUG -g  -O2
debug: cmdline #Assuming that the debug request means that the person is a developer, 
#and therefore wants to re-generate the command line options from the GNU gengetopts script.
debug: $(PROG)

cmdline:
	gengetopt < $(srcdir)genOptions.ggo 

clean :
	rm -f $(PROG) $(OBJFILES) $(DEPFILES)

install : 
	install -d $(DESTDIR)$(PREFIX)/bin
	install $(PROG) $(DESTDIR)$(PREFIX)/bin/
-include $(DEPFILES)
