popa.bin: popa.txt svm/build/sas
	svm/build/sas popa.txt > popa.bin

svm/build/%:
	@$(MAKE) -C svm $(patsubst svm/%,%,$@)

run: svm/build/main popa.bin
	echo Hello World > input
	svm/build/main popa.bin < input
	rm input

clean:
	rm popa.bin -f
	rm log -f

compiler: compiler.c
	cc -g -o compiler compiler.c
	./compiler > output.bin

