compiler: compiler.c
	cc -g -o compiler compiler.c
	./compiler > output.bin

popa.bin: popa.txt svm/build/sas
	svm/build/sas popa.txt > popa.bin

svm/build/%:
	@$(MAKE) -C svm $(patsubst svm/%,%,$@)

run: svm/build/main popa.bin
	svm/build/main popa.bin 2> log

clean:
	rm popa.bin -f
	rm log -f
