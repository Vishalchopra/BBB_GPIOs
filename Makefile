INSTALLDIR=$(shell pwd)/modules
obj-m := lkm.o
lkm-objs := button.o 


all:
	make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) modules
	@rm -rf $(INSTALLDIR)
	@mkdir $(INSTALLDIR)
	@mv *.ko *.mod.c *.o .*.cmd $(INSTALLDIR)
clean:
	make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) clean
	@rm -rf $(INSTALLDIR)
	@rm -rf *.symvers *.order
