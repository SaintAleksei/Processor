proc:
	make -C src/proc
	mv src/proc/proc bin/proc

assm:
	make -C src/assm 
	mv src/assm/assm bin/assm

clean:
	make -C src/assm clean
	make -C src/proc clean

