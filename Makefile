All : googlevoice

googlevoice : googlevoice.o upload.o
	cc -o $@ $+ -lcurl

googlevoice.o : googlevoice.c
	cc -c $+

upload.o : upload.c
	cc -c $+

.PHONY : clean
clean :
	rm googlevoice.o upload.o
