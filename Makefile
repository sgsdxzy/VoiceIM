All : googlespeech

googlespeech : googlespeech.o upload.o record-oss.o
	cc -o $@ $+ -lcurl -lpthread -Wall

googlespeech.o : googlespeech.c
	cc -c $+

record-oss.o : record-oss.c
	cc -c $+

upload.o : upload.c
	cc -c $+

.PHONY : clean
clean :
	rm googlespeech.o upload.o
