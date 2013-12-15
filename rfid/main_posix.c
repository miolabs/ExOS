#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>

#include <pthread.h>

//#define kRfidDeviceRoute        "/dev/cu.usbmodem621"
#define kRfidDevice        "/dev/usbkb0"
#define kSendRFIDValueWebService    "http://adingo.es/api/exos.php?valor="
#define kSendTempValueWebService    "http://adingo.es/api/temp.php?valor="

#define kSendRFIDValueWS    "/api/exos.php?valor="
#define kSendTempValueWS    "/api/temp.php?valor="

//Network related includes:
#include <sys/socket.h>
#include <netinet/in.h>


#include "temperature.h"

void SendValue(int value, int type)
{
    struct sockaddr_in server;

    int sd = socket(AF_INET, SOCK_STREAM, 0);
    
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr("87.98.231.17");
    server.sin_port = htons(80);
    
    int ret = connect(sd, (const struct sockaddr *)&server, sizeof(server));
    if (ret == -1)
    {
        close(sd);
        return;
    }
    
    char message[200];
    char *ws = type == 0 ? kSendRFIDValueWS : kSendTempValueWS;
    sprintf(message, "GET %s%i HTTP/1.1\nHost: adingo.es\n\n", ws, value);
    size_t len = strlen(message);
    send(sd, message, len, 0);
    char msg[1000];
    recv(sd, msg, 1000, 0);
    close(sd);
}

void cleanString(char cardID[], char buffer[], int l)
{
    int i=0,pos=0;
    for(i=0; i<l; i++)
    {
        if(buffer[i] != '\n' && buffer[i] != '\r' && buffer[i] != '-' && buffer[i] != '>')
        {
            cardID[pos] = buffer[i];
            pos++;
        }
    }
    cardID[pos] = '\0';
}

void *read_rfid(void *x_void_ptr)
{
    int *x_ptr = (int *)x_void_ptr;
    char nombre[] = kRfidDevice;
    char baseurl[] = kSendRFIDValueWebService;
    char buffer[512];
    char cardID[512];
    char url[1024];
    int l;
  
    int fd = open(nombre, O_RDWR);
    
    if (fd < 0)
    {
        //fprintf(stderr, "Can't open input file !!\n");
        close(fd);
        return NULL;
    }
    
    while(1)
    {
        l = (int)read(  fd,  buffer,  512);
        //printf("%s",buffer);
        *x_ptr = *x_ptr + 1;
        
        cleanString(cardID, buffer, l);
        
        SendValue(10, 0);
    }
    
    /* the function must return something - NULL will do */
    return NULL;
}

int threadTestRFID()
{
    int x = 0, ant=0;

    static float _temp;
    
    pthread_t thread_rfid_reader;
    
    if(pthread_create(&thread_rfid_reader, NULL, read_rfid, &x)) {
        //fprintf(stderr, "Error creating thread\n");
        return 1;
    }
    
    //main thread
    while(1)
    {
        if(x != ant)
        {
            ant=x;
            printf("Main Thread: %d\n", ant);

        }
      
      _temp = temp_read();
      SendValue(_temp, 1);      
      sleep(1);
    }
    
    return 0;
}

int posix_main(void)
{
    threadTestRFID();
    
    return 0;
}
