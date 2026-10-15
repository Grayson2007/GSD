include config.mk
.PHONY: loader kernel clean


kernel:
	make -f src/kernel/makefile

clean:
	rm -rf build/kernel/$(ARCH)/o/*.o
loader:
	make -f src/loader/impl/$(ARCH)/makefile
copy-gsdb:
	./copy-toiso.sh
boot:
	./boot-img/bootx64.sh
menuconfig:
	