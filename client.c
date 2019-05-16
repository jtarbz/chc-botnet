#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <curl/curl.h>

#define BUFSIZE 150000
char buffer[BUFSIZE];
char test[] = "<div class=\"message\">\nBecause of weather, Calvert Hall will close today 11:45 a.m.&nbsp; The Howard County us will depart at 12:00.&nbsp; All athletic and activities are cancelled.&nbsp; Detention will not be held.\n</div>";

size_t lr = 0;

/* A libcurl callback function that I pilfered from 
a generous donor online */
size_t filterit(void *ptr, size_t size, size_t nmemb, void *stream) {
  if ( (lr + size*nmemb) > BUFSIZE ) return BUFSIZE;
  memcpy(stream+lr, ptr, size*nmemb);
  lr += size*nmemb;
  return size*nmemb;
}

/* Scrape the CHC website and save content to a buffer;
This should be implemented asynchronously for compatability
with client socket functions */
void scrape() {
  // This is libcurl stuff; set an easy handle / options, and then scrape
  CURL *curlHandle = curl_easy_init();
  curl_easy_setopt(curlHandle, CURLOPT_URL, "http://www.calverthall.com");
  curl_easy_setopt(curlHandle, CURLOPT_FOLLOWLOCATION, 1);
  curl_easy_setopt(curlHandle, CURLOPT_WRITEFUNCTION, filterit);
  curl_easy_setopt(curlHandle, CURLOPT_WRITEDATA, buffer);
  curl_easy_perform(curlHandle);
  curl_easy_cleanup(curlHandle);
  
  // Awesome parsing algorithm I made that returns the content of the <message> tag
  bool state = false;
  char *token = strtok(test, "<>");
  while(token != NULL && state == false) {
    if(strcmp(token, "div class=\"message\"") == 0) {
      token = strtok(NULL, "<>");
      puts(token);
      state = true;
    }
    token = strtok(NULL, "<>");
  }
}

int main() {
  scrape();

  return 0;
}
