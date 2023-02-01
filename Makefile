
snd-soc-alas-mixer-module1-objs := alsa-mixer-module1.o
 obj-m  += snd-soc-alas-mixer-module1.o

KBUILD_DIR=/lib/modules/`uname -r`/build
INSTALL_DIR=/lib/modules/`uname -r`/

default:
	${MAKE} -C ${KBUILD_DIR} M=${PWD}

clean:
	make -C $(KBUILD_DIR) M=$(PWD) clean

install:
	ln -s `pwd`/${snd-soc-alas-mixer-module1}.ko /lib/modules/5.15.0-58-generic/kernel/sound/soc/generic/
	echo run depmod -a

unstall:
	rm  /lib/modules/5.15.0-58-generic/kernel/sound/soc/generic/${snd-soc-alas-mixer-module1}.ko

probe: # remove the module if already loaded and then load it
	@if [ ! -z "`lsmod | grep snd-soc-alas-mixer-module1`" ]; then rmmod snd-soc-alas-mixer-module1; fi;
	modprobe --first-time ${snd-soc-alas-mixer-module1}

unprobe: # remove the module if already loaded and then load it
	rmmod ${snd-soc-alas-mixer-module1}
