#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <curl/curl.h>
#include <pthread.h>

#define BUFSIZE 300000
char buffer[BUFSIZE + 1];

size_t lr = 0;  // libcurl thing

/* Standard error function because I am lazy */
void frick(char *msg) {
    puts(msg);
    exit(1);
}

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
void *scrape(void *sockish) {
  /* LOAD HTML FILE FOR TESTING PURPOSES 
  DELETE FOR FINAL COMMIT */
  FILE *fp = fopen("../snowday.html", "r");
  if (fp != NULL) {
    size_t newLen = fread(buffer, sizeof(char), BUFSIZE, fp);
    if(ferror(fp) != 0) fputs("Error reading file", stderr);
    else buffer[newLen++] = '\0'; // Just to be safe.
    fclose(fp);
  }
  /* This is libcurl stuff; set an easy handle / options, and then scrape */
  /* CURL *curlHandle = curl_easy_init();
  curl_easy_setopt(curlHandle, CURLOPT_URL, "http://www.calverthall.com");
  curl_easy_setopt(curlHandle, CURLOPT_FOLLOWLOCATION, 1);
  curl_easy_setopt(curlHandle, CURLOPT_WRITEFUNCTION, filterit);
  curl_easy_setopt(curlHandle, CURLOPT_WRITEDATA, buffer);
  curl_easy_perform(curlHandle);
  curl_easy_cleanup(curlHandle);
  buffer[lr] = 0; */

  /* Awesome parsing algorithm I made that returns the content
  of the <div class="message"> tag */
  bool state = false;
  int i = 0;
  char *token = malloc(BUFSIZE + 1);
  strcpy(token, strtok(buffer, "<>"));
  while(token != NULL && state == false) {
    if(strcmp(token, "div class=\"message\"") == 0) {
      strcpy(token, strtok(NULL, "<>"));
      puts(token);
      if(send(*(int *)sockish, token, strlen(token), 0) < 0) frick("Snow day message failed to be sent");
      puts("Snow day message sent");
      state = true;
    }
    if(i > 300) {
      puts("There's no snow day idiot");
      strcpy(token, "There's no snow day idiot");
      if(send(*(int *)sockish, token, strlen(token), 0) < 0) frick("Snow day message failed to be sent");
      state = true;
    }
    strcpy(token, strtok(NULL, "<>"));
    i++;
  }

  free(token);
}

int main(int argc, char *argv[]) {

  if(argc != 2) frick("give me one argument and one argument only!"); // Boilerplate arg check

  /* Socket creation and server addressing */
  int sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
  if(sock < 0) frick("Socket creation failed");

  struct sockaddr_in server;
  memset(&server, 0, sizeof server);  // Zero out addressing to avoid ~~bad voodoo~~
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = inet_addr(argv[1]);
  server.sin_port = htons(8081);

  /* Establish a connection to the server */
  if(connect(sock, (struct sockaddr *)&server, sizeof server) < 0) frick("Connection to the server failed or was refused");
  puts("Connection to the server established");

  /* Pthread initialization to allow for future asynchronous operation */
  pthread_t scrape_tid;  // Thread ID
  pthread_attr_t scrape_attr;    // Create attributes
  pthread_attr_init(&scrape_attr);
  pthread_create(&scrape_tid, &scrape_attr, scrape, &sock);    // Start the scraping thread

  puts("haha lol this thread is taking forever");
  pthread_join(scrape_tid, NULL);    // Wait until the thread is done

  close(sock);
  exit(0);
}
