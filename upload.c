#include <stdio.h>
#include <stdlib.h>
#include <curl/curl.h>
#include "commontypes.h"

void* upload(void* arg)
{
    WAVPROP* wavp = arg;

    curl_global_init(CURL_GLOBAL_ALL);
    CURL* handle = curl_easy_init();

    curl_easy_setopt(handle, CURLOPT_URL, "http://www.google.com/speech-api/v1/recognize?xjerr=1&client=chromium&lang=zh-CN&maxresults=8");

    struct curl_slist *headers=NULL;
    headers = curl_slist_append(headers, "Content-Type: audio/L16; rate=8000");
    curl_easy_setopt(handle, CURLOPT_HTTPHEADER, headers);

    curl_easy_setopt(handle, CURLOPT_USERAGENT, "Mozilla/5.0");

    curl_easy_setopt(handle, CURLOPT_POSTFIELDS, wavp->ptr);
    curl_easy_setopt(handle, CURLOPT_POSTFIELDSIZE, wavp->len);

    curl_easy_perform(handle); /* post away! */

    curl_slist_free_all(headers); /* free the header list */

    curl_global_cleanup();
    free(wavp->ptr);
    free(wavp);

    return NULL;
}
