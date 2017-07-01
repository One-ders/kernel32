
#DRIVERS_DIR := os stddrv stmusb drivers arch
#APP_DIR := cec_a1_ctrl blinky timer_test

#SUBDIRS:=$(DRIVERS_DIR) $(APP_DIR)

#.PHONY: all

#all: $(SUBDIRS)

.PHONY: clean

pavo:
	$(MAKE) -C arch/boards/pavo all

discovery:
	$(MAKE) -C arch/boards/discovery all

pavo-clean:
	$(MAKE) -C arch/boards/pavo clean

discovery-clean:
	$(MAKE) -C arch/boards/discovery clean



clean:	$(SUBDIRS)


.PHONY: $(SUBDIRS)

$(SUBDIRS):
	@$(MAKE) -C $@ $(MAKECMDGOALS)
