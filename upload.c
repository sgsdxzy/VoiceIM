#include <stdio.h>
#include <stdlib.h>
#include <curl/curl.h>

int main()
{
    FILE* rec = fopen("/tmp/rec.wav", "rb");
    fseek(rec, 0, SEEK_SET);
    long begin = ftell(rec);
    fseek(rec,0,SEEK_END);
    long end = ftell(rec);
    size_t total_len = end - begin;
    fseek(rec,0,SEEK_SET);
    void* ptr = malloc(total_len);
    fread(ptr, 1, total_len, rec);

    curl_global_init(CURL_GLOBAL_ALL);
    CURL* handle = curl_easy_init();

    curl_easy_setopt(handle, CURLOPT_URL, "http://www.google.com/speech-api/v1/recognize?xjerr=1&client=chromium&lang=zh-CN&maxresults=5");

    struct curl_slist *headers=NULL;  
    headers = curl_slist_append(headers, "Content-Type: audio/L16; rate=8000");
    curl_easy_setopt(handle, CURLOPT_HTTPHEADER, headers);

    curl_easy_setopt(handle, CURLOPT_USERAGENT, "Mozilla/5.0");

    curl_easy_setopt(handle, CURLOPT_POSTFIELDS, ptr);
    curl_easy_setopt(handle, CURLOPT_POSTFIELDSIZE, total_len);

    curl_easy_perform(handle); /* post away! */

    curl_slist_free_all(headers); /* free the header list */

    curl_global_cleanup();

    return 0;
}



