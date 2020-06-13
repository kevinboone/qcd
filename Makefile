NAME    := qcd
VERSION := 0.1a
LIBS    := ${EXTRA_LIBS} 
KLIB    := klib
KLIB_INC := $(KLIB)/include
KLIB_LIB := $(KLIB)
TARGET	:= $(NAME)
SOURCES := $(shell find src/ -type f -name *.c)
OBJECTS := $(patsubst src/%,build/%,$(SOURCES:.c=.o))
DEPS	:= $(OBJECTS:.o=.deps)
DESTDIR := /
PREFIX  := /usr
MANDIR  := $(DESTDIR)/$(PREFIX)/share/man
BINDIR  := $(DESTDIR)/$(PREFIX)/bin
SHARE   := $(DESTDIR)/$(PREFIX)/share/$(TARGET)
CFLAGS  := -O3 -fpie -fpic -Wall -Werror -DNAME=\"$(NAME)\" -DVERSION=\"$(VERSION)\" -DSHARE=\"$(SHARE)\" -DPREFIX=\"$(PREFIX)\" -I $(KLIB_INC) -DSQLITE_THREADSAFE=0 -DSQLITE_OMIT_LOAD_EXTENSION ${EXTRA_CFLAGS} -ffunction-sections -fdata-sections

LDFLAGS := -s -pie -Wl,--gc-sections ${EXTRA_LDFLAGS}

$(TARGET): $(OBJECTS) 
	echo $(SOURCES)
	make -C klib
	$(CC) $(LDFLAGS) -o $(TARGET) $(OBJECTS) $(LIBS) $(KLIB)/klib.a

build/%.o: src/%.c
	@mkdir -p build/
	$(CC) $(CFLAGS) -MD -MF $(@:.o=.deps) -c -o $@ $<

clean:
	$(RM) -r build/ $(TARGET) 
	make -C klib clean

install: $(TARGET)
	mkdir -p $(DESTDIR)/$(PREFIX) $(DESTDIR)/$(BINDIR) $(DESTDIR)/$(MANDIR)
	strip $(TARGET)
	install -m 755 $(TARGET) $(DESTDIR)/${BINDIR}
	install -m 755 qcd_init.sh $(DESTDIR)/${BINDIR}
	mkdir -p $(DESTDIR)/$(MANDIR)/man1
	cp -p man1/* $(DESTDIR)/${MANDIR}/man1/
	@echo   === To activate, add \". /usr/bin/qcd_init.sh\" to .bashrc ===


-include $(DEPS)

.PHONY: clean

