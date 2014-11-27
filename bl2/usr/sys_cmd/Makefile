
USR_SCMD=$(OBJ)/usr/sys_cmd

all: $(USR_SCMD) $(USR_SCMD)/sys_cmd.srec

$(USR_SCMD):
	mkdir $@

$(USR_SCMD)/sys_cmd.srec: $(USR_SCMD)/sys_cmd
	$(OBJCOPY) -O srec $< $@
	
$(USR_SCMD)/sys_cmd: $(USR_SCMD)/sys_cmd.o
	$(LD) $(LDFLAGS) -o $@ $<

sys_cmd.o: 

clean:
	rm -rf sys_cmd *.o
