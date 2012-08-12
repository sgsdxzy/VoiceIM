All : googlespeech-oss googlespeech-openal

googlespeech-openal : googlespeech-openal.o upload.o record-openal.o
	cc -o $@ $+ -lcurl -lpthread -lopenal -Wall

googlespeech-oss : googlespeech-oss.o upload.o record-oss.o
	cc -o $@ $+ -lcurl -lpthread -Wall

googlespeech-oss.o : googlespeech-oss.c
	cc -c $+

googlespeech-openal.o : googlespeech-openal.c
	cc -c $+

record-oss.o : record-oss.c
	cc -c $+

record-openal.o : record-openal.c
	cc -c $+

upload.o : upload.c
	cc -c $+

.PHONY : clean
clean :
	-rm googlespeech-oss.o googlespeech-openal.o upload.o record-oss.o record-openal.o
