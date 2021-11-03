install:
	make -C src/ 
	mv src/assm bin/assm
	mv src/proc bin/proc

clean:
	make -C src/ clean
