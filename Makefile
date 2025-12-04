# _ EXEC _______________________________________________________________________
USER_TARGET = tests
KERN_TARGET = fifo


# _ DIRECTORIES ________________________________________________________________
INC_DIR  = includes
SRCS_DIR = srcs
BIN_DIR  = bin
TEST_DIR = tests

# _ COMPILER ___________________________________________________________________
ccflags-y += -I$(PWD)/$(INC_DIR)

# _ FONT _______________________________________________________________________
RED      = \e[31m
GREEN    = \e[32m
YELLOW   = \e[33m
BLUE     = \e[34m
MAGENTA  = \e[35m
CYAN     = \e[36m
WHITE    = \e[37m

RST      = \e[0m
BOLD     = \e[1m
CLEAR    = \e[2J
CUR_HOME = \e[H


ifneq ($(KERNELRELEASE),)
    obj-m := $(KERN_TARGET).o
	$(KERN_TARGET)-objs := main.o $(SRCS_DIR)/buffer.o $(SRCS_DIR)/fops.o $(SRCS_DIR)/class.o 
else
   KERNELDIR ?= /lib/modules/$(shell uname -r)/build
   PWD := $(shell pwd)

default:
	@echo "$(YELLOW)--KERNEL SPACE COMPILATION: $(RST)$(BOLD)$(OBJS)$(RST)"
	@echo "$(MAGENTA)~COMPILING $(RST)$(BOLD)$(KERN_TARGET)$(RST)$(MAGENTA)...$(RST)"

	@$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
	@echo "$(BOLD)$(GREEN)~ DONE ~$(RST)"

clean:
	@echo "$(BOLD)$(RED)~ CLEANING DIRECTORY... ~$(RST)"
	@$(MAKE) -C $(KERNELDIR) M=$(PWD) clean
	@rm -rf $(BIN_DIR)/$(USER_TARGET)
	@echo "$(BOLD)$(GREEN)~ DONE ~$(RST)"

insert: default
	@echo "\n$(BLUE)~INSERTING $(RST)$(BOLD)$(KERN_TARGET).ko$(RST)$(BLUE) INTO KERNEL $(RST)"
	@sudo insmod $(KERN_TARGET).ko

remove: clean
	@echo "\n$(BLUE)~REMOVING $(RST)$(BOLD)$(KERN_TARGET).ko$(RST)$(BLUE) FROM KERNEL $(RST)"
	@sudo rmmod $(KERN_TARGET)

update: remove insert

test: default
	@echo "$(YELLOW)--USER SPACE COMPILATION: $(RST)$(BOLD)$(OBJS)$(RST)"
	@echo "$(MAGENTA)~COMPILING $(RST)$(BOLD)$(USER_TARGET)$(RST)$(MAGENTA) TO $(RST)$(BOLD)$(BIN_DIR)/$(USER_TARGET)$(RST)"
	@$(CC) $(TEST_DIR)/$(USER_TARGET).c -o $(BIN_DIR)/$(USER_TARGET) -I$(INC_DIR)

.PHONY: clean default insert remove update
endif
