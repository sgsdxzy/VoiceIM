All : record upload

record : record.c
	cc -o record $+ -lasound

upload : upload.c
	cc -o upload $+ -lcurl

.PHONY : clean
clean :
	rm record upload
