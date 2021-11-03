install:
	make -C src/assm 
	mv src/assm/assm bin/assm
	make -C src/proc
	mv src/proc/proc bin/proc

clean:
	make -C src/assm clean
	make -C src/proc clean

