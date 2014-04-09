
DRIVERS_DIR := os stddrv stmusb drivers
APP_DIR := cec_a1_ctrl blinky

SUBDIRS:=$(DRIVERS_DIR) $(APP_DIR)

.PHONY: all

all: $(SUBDIRS)

.PHONY: clean

clean:	$(SUBDIRS)


.PHONY: $(SUBDIRS)

$(SUBDIRS):
	@$(MAKE) -C $@ $(MAKECMDGOALS)
