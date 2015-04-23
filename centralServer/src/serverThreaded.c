#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <fcntl.h>
//#include <sys/filio.h>
#include <sys/ioctl.h>

#define SOCKETAPP  "socketApp"
#define SOCKETSNMP "socketSnmp"

void *serverThread_App(void *arg);
void *serverThread_Snmp(void *arg);
int setNonBlocking(int fd);


int setNonBlocking(int fd)
{
  int flags;

  /* If O_NONBLOCK is defined, use the fcntl function */
#if defined(O_NONBLOCK)
  if(-1 == (flags = fcntl(fd, F_GETFL, 0))){
    flags = 0;
  }
  return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
#else

  /* otherwise use the old method */
  flags = 1;
  printf("using ioctl\n");
  return ioctl(fd, FIONBIO, &flags);
#endif
}


void *serverThread_Snmp(void *arg)
{

  int s, s2, t, len;
  struct sockaddr_un local, remote;
  char str[100];

  int flags;

  if(( s = socket(AF_UNIX, SOCK_STREAM, 0)) == -1){
    perror("socket");
    exit(1);
  }

  local.sun_family = AF_UNIX;
  strcpy(local.sun_path, SOCKETSNMP);
  unlink(local.sun_path);
  len = strlen(local.sun_path) + sizeof(local.sun_family);
  if( bind(s, (struct sockaddr *)&local, len) == -1){
    perror("bind");
    exit(1);
  }

  if(listen(s, 5) == -1){
    perror("listen");
    exit(1);
  }

  for(;;){
    int done, n;
    printf("Waiting for a connection...\n");
    t = sizeof(remote);
    if( (s2 = accept(s, (struct sockaddr *)&remote, &t)) == -1){
      perror("accept");
      exit(1);
    }

    setNonBlocking(s2);
    printf("connected.\n");
    done = 0;

    do{
      n = recv(s2, str, 100, 0);
      printf("n:%d\n",n);
      perror("recv");
      if( n == -1){
        if (errno == EAGAIN || errno == EWOULDBLOCK){
          printf("would have blocked\n");
          // try again later
        }
      }
      sleep(1);

      //if(!done)
      //  if(send(s2, str, n, 0) < 0){
      //    perror("send");
      //    done = 1;
      //  }

    }while(!done);

    close(s2);

  }

  // Cast the parameter into what is needed.
  int *incoming = (int *)arg;
  // Do whatever you want
  char *buffer = (char *)malloc(64 * sizeof(char));
  snprintf(buffer, 64, "this is thread reporting, got %d from main", *incoming + 2);

  // Thread terminates with this function
  return buffer;
}



void *serverThread_App(void *arg)
{

  int s, s2, t, len;
  struct sockaddr_un local, remote;
  char str[100];

  if(( s = socket(AF_UNIX, SOCK_STREAM, 0)) == -1){
    perror("socket");
    exit(1);
  }

  local.sun_family = AF_UNIX;
  strcpy(local.sun_path, SOCKETAPP);
  unlink(local.sun_path);
  len = strlen(local.sun_path) + sizeof(local.sun_family);
  if( bind(s, (struct sockaddr *)&local, len) == -1){
    perror("bind");
    exit(1);
  }

  if(listen(s, 5) == -1){
    perror("listen");
    exit(1);
  }

  for(;;){
    int done, n;
    printf("Waiting for a connection...\n");
    t = sizeof(remote);
    if( (s2 = accept(s, (struct sockaddr *)&remote, &t)) == -1){
      perror("accept");
      exit(1);
    }

    printf("connected.\n");
    done = 0;

    do{
      n = recv(s2, str, 100, 0);
      if( n <= 0){
        if(n < 0) perror("recv");
        done = 1;
      }

      if(!done)
        if(send(s2, str, n, 0) < 0){
          perror("send");
          done = 1;
        }

    }while(!done);

    close(s2);

  }

  // Cast the parameter into what is needed.
  int *incoming = (int *)arg;
  // Do whatever you want
  char *buffer = (char *)malloc(64 * sizeof(char));
  snprintf(buffer, 64, "this is thread reporting, got %d from main", *incoming);

  // Thread terminates with this function
  return buffer;
}

int main(void)
{
  pthread_t thread_ID[2];
  void *threadRet[2];
  int value;

  value = 42;

  // Create the thread, passing &value for the argument
  pthread_create(&thread_ID[0], NULL, serverThread_App , &value);
  pthread_create(&thread_ID[1], NULL, serverThread_Snmp , &value);

  // Main program continues while thre thread executes
  
  // wait of the thread to terminate
  pthread_join(thread_ID[0], &threadRet[0]);
  pthread_join(thread_ID[1], &threadRet[1]);

  printf("I got \"%s\" back from the thread #0.\n", (char *)threadRet[0]);
  printf("I got \"%s\" back from the thread #1.\n", (char *)threadRet[1]);
  free(threadRet[0]);
  free(threadRet[1]);

  return 0;
}

