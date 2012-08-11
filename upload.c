#include <stdio.h>
#include <stdlib.h>
#include "commontypes.h"

extern pthread_mutex_t lock;
extern CURL* handle;

void upload_init(struct curl_slist *headers)
{
    curl_global_init(CURL_GLOBAL_ALL);
    handle = curl_easy_init();

    curl_easy_setopt(handle, CURLOPT_URL, "http://www.google.com/speech-api/v1/recognize?xjerr=1&client=chromium&lang=zh-CN&maxresults=8");

    headers = curl_slist_append(headers, "Content-Type: audio/L16; rate=8000");
    curl_easy_setopt(handle, CURLOPT_HTTPHEADER, headers);

    curl_easy_setopt(handle, CURLOPT_USERAGENT, "Mozilla/5.0");
    
    return;
}



void* upload(void* arg)
{
    WAVPROP* wavp = arg;

    int err = pthread_mutex_lock(&lock);
    if (err != 0) 
    {
	fprintf(stderr, "can't lock: %s\n", strerror(err));
	exit(1);
    }

    curl_easy_setopt(handle, CURLOPT_POSTFIELDS, wavp->ptr);
    curl_easy_setopt(handle, CURLOPT_POSTFIELDSIZE, wavp->len);

    curl_easy_perform(handle); /* post away! */

    err = pthread_mutex_unlock(&lock);
    if (err != 0) 
    {
	fprintf(stderr, "can't unlock: %s\n", strerror(err));
	exit(1);
    }

    free(wavp->ptr);
    free(wavp);
    
    return NULL;
}

void upload_clean(struct curl_slist *headers)
{

    curl_slist_free_all(headers); /* free the header list */

    curl_global_cleanup();

}
