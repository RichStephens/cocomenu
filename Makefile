NAME     = cocomenu

# Where the menu is linked. Every launch goes through Disk Basic's LOADM and a
# stub in the cassette buffer, so the menu never has to survive what it starts -
# its address only has to clear BASIC's own workspace, and a launched program is
# free to use everything above $0E00.
ORG      = 2800
LIMIT    = 8000

# A DECB granule is 2304 bytes, and crossing a boundary silently doubles what
# the file costs on disk. Fail the build instead of finding out later. Build
# with GRANULES=2 if a bigger menu is genuinely wanted.
GRANULE    = 2304
GRANULES   = 1
SIZE_LIMIT = $(shell expr $(GRANULE) \* $(GRANULES))

CMOC     = cmoc
DECB     = decb
WRITECOCOFILE = writecocofile

MENU_SRCS = src/dirent.c src/dsk.c src/loader.c src/main.c
HDRS      = $(wildcard src/*.h)

CFLAGS   = --coco -Isrc -DMENU_NAME=\"$(shell echo $(NAME) | tr a-z A-Z)\"

BIN = $(NAME).bin
DSK = $(NAME).dsk

.DEFAULT_GOAL := disk

.PHONY: disk testdisk clean size

$(BIN): $(MENU_SRCS) $(HDRS)
	$(CMOC) $(CFLAGS) --org=$(ORG) --limit=$(LIMIT) -DMENU_ORG=0x$(ORG) -o $@ $(MENU_SRCS)
	@sz=`stat -c%s $@`; \
	if [ $$sz -gt $(SIZE_LIMIT) ]; then \
		echo ""; \
		echo "*** $@ is $$sz bytes, over the $(GRANULES) granule limit of $(SIZE_LIMIT)."; \
		echo "*** On disk it would take $$(( ($$sz + $(GRANULE) - 1) / $(GRANULE) )) granules instead of $(GRANULES)."; \
		echo "*** Shrink it, or build with GRANULES=2 if that is what you want."; \
		echo ""; \
		rm -f $@; \
		exit 1; \
	fi; \
	echo "$@: $$sz bytes, $$(( $(SIZE_LIMIT) - $$sz )) spare in $(GRANULES) granule(s)"

AUTOEXEC.BAS:
	@printf '10 LOADM"%s":EXEC\n' $(shell echo $(NAME) | tr a-z A-Z) > $@

# Demo programs, only put on the disk by `make testdisk`.
TEST_ORG   = 0E00
TEST_SRCS  = $(wildcard tests/*.c)
TEST_BINS  = $(TEST_SRCS:.c=.bin)
TEST_BAS   = $(wildcard tests/*.bas)
SCROLL_N   = 20
SCROLL_DIR = build/scroll

tests/toobig.bin: tests/toobig.c
	$(CMOC) --coco --org=7000 -o $@ $<

tests/%.bin: tests/%.c
	$(CMOC) --coco --org=$(TEST_ORG) -o $@ $<

# `make disk`     - the menu and its AUTOEXEC, nothing else. This is the thing
#                   you drop on a real disk.
# `make testdisk` - the same plus the demo programs and enough generated filler
#                   to make the list scroll.
# Both write $(DSK); the image is removed first so its contents are never a
# mix of the two.
disk:
	@rm -f $(DSK)
	@$(MAKE) --no-print-directory $(DSK)

testdisk:
	@rm -f $(DSK)
	@$(MAKE) --no-print-directory $(DSK) TESTS=1

$(DSK): $(BIN) AUTOEXEC.BAS $(if $(TESTS),$(TEST_BINS) $(TEST_BAS) tests/scroll.c.in)
	$(DECB) dskini $@
	$(WRITECOCOFILE) -b $@ $(BIN)
	$(WRITECOCOFILE) -n $@ AUTOEXEC.BAS
ifdef TESTS
	@mkdir -p $(SCROLL_DIR)
	@for n in $$(seq 1 $(SCROLL_N)); do \
		nn=`printf "%02d" $$n`; \
		sed "s/@NAME@/PROG$$nn/g" tests/scroll.c.in > $(SCROLL_DIR)/prog$$nn.c; \
		$(CMOC) --coco --org=$(TEST_ORG) -o $(SCROLL_DIR)/prog$$nn.bin $(SCROLL_DIR)/prog$$nn.c || exit 1; \
	done
	@for f in $(TEST_BINS); do $(WRITECOCOFILE) -b $@ $$f; done
	@for f in $(TEST_BAS); do $(WRITECOCOFILE) -n $@ $$f; done
	@for f in $(SCROLL_DIR)/*.bin; do $(WRITECOCOFILE) -b $@ $$f; done
endif
	@$(DECB) dir $@

# Size report. Builds with --intermediate in a temp dir so the .s/.lst/.o it
# generates never land in the repo.
size: $(BIN)
	@t=`mktemp -d`; \
	$(CMOC) $(CFLAGS) --org=$(ORG) --limit=FFFF --intermediate -DMENU_ORG=0x$(ORG) \
		-o $$t/probe.bin $(MENU_SRCS) >/dev/null 2>&1; \
	pe=`grep '^Symbol: .02program_end' $$t/probe.map | sed 's/.*= //'`; \
	sz=`stat -c%s $(BIN)`; \
	echo "on disk : $$sz bytes, $$(( $(SIZE_LIMIT) - $$sz )) spare in $(GRANULES) granule(s)"; \
	echo "in RAM  : \$$$(ORG)-\$$$$pe"; \
	rm -rf $$t *.s *.lst *.o *.map *.link 2>/dev/null; true

clean:
	rm -f $(BIN) $(DSK) $(TEST_BINS) AUTOEXEC.BAS
	rm -rf $(SCROLL_DIR)
	rm -f src/*.o src/*.asm *.s *.lst *.o *.map *.link
