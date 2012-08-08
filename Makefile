All : googlespeech

googlespeech : googlespeech.o upload.o
	cc -o $@ $+ -lcurl

googlespeech.o : googlespeech.c
	cc -c $+

upload.o : upload.c
	cc -c $+

.PHONY : clean
clean :
	rm googlespeech.o upload.o
