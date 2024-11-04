#include <assert.h>
#include <netinet/ip.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>


void test_setsockopt(int priority_first)
{
  int priority = 6;
  int iptos = IPTOS_CLASS_CS6;

  int fd = socket(AF_INET, SOCK_STREAM, 0);

  if (priority_first){
    if(setsockopt(fd, SOL_SOCKET, SO_PRIORITY, &priority,
                  sizeof(priority)) < 0){
      printf("Oh no\n");
    }
  }

  if(setsockopt(fd, IPPROTO_IP, IP_TOS, &iptos, sizeof(iptos)) < 0){
    printf("Oh no\n");
  }

  if (!priority_first){
    if(setsockopt(fd, SOL_SOCKET, SO_PRIORITY, &priority,
                  sizeof(priority)) < 0){
      printf("Oh no\n");
    }
  }

 int new_priority;
 int optlen = sizeof(new_priority);
 if (getsockopt(fd, SOL_SOCKET, SO_PRIORITY, &new_priority, &optlen) < 0){
   printf("Oh no\n");
 }
 if (priority_first){
   printf("Priority is %d if setsockopt(SO_PRIORITY) *before* setsockopt(IP_TOS)\n",
     new_priority);
 } else {
   printf("Priority is %d if setsockopt(SO_PRIORITY) *after* setsockopt(IP_TOS)\n",
     new_priority);
 }

 close(fd);
}

int main(void)
{
  // background:  http://lists.openwall.net/netdev/2009/12/21/59

  int priority_first = 0;
  test_setsockopt(priority_first);

  priority_first = 1;
  test_setsockopt(priority_first);

  return 0;
}
