#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <linux/sockios.h>
#include <linux/errqueue.h>

#define BUFFER_SIZE 3 * 1024 * 1024
#define on_error(...) { fprintf(stderr, __VA_ARGS__); fflush(stderr); exit(1); }

int main (int argc, char *argv[]) {
  if (argc < 2) on_error("Usage: %s [port]\n", argv[0]);

  int port = atoi(argv[1]);

  int server_fd, client_fd, err;
  struct sockaddr_in server, client;
  char buf[BUFFER_SIZE];

  server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) on_error("Could not create socket\n");

  server.sin_family = AF_INET;
  server.sin_port = htons(port);
  server.sin_addr.s_addr = htonl(INADDR_ANY);

  int opt_val = 1;
  setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt_val, sizeof opt_val);

  err = bind(server_fd, (struct sockaddr *) &server, sizeof(server));
  if (err < 0) on_error("Could not bind socket\n");

  err = listen(server_fd, 128);
  if (err < 0) on_error("Could not listen on socket\n");

  printf("Server is listening on %d\n", port);

  int dest_base = 0x0f800000;
  int dest_base2 = 0x10000000;
  int dest_base3 = 0x10800000;
  int dest_base4 = 0x11000000;
  int dh = open("/dev/mem", O_RDWR);
  int size = 4 * 1024 * 1024 * 4;
  uint64_t * data = (uint64_t *) mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_NORESERVE, dh, dest_base);
  uint64_t * data2 = (uint64_t *) mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_NORESERVE, dh, dest_base2);
  uint64_t * data3 = (uint64_t *) mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_NORESERVE, dh, dest_base3);
  uint64_t * data4 = (uint64_t *) mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_NORESERVE, dh, dest_base4);

  int buffer_info_base = 0x19000000;
  int dm = open("/dev/mem", O_RDWR | O_SYNC);
  volatile uint32_t * buffer_info = (uint32_t *) mmap(NULL, sizeof(uint64_t) * 4096, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_SHARED, dm, buffer_info_base);

  while (1) {
    socklen_t client_len = sizeof(client);
    client_fd = accept(server_fd, (struct sockaddr *) &client, &client_len);

    if (client_fd < 0) on_error("Could not establish new connection\n");

    unsigned buf_size = 250 * 1024;
    setsockopt(client_fd, SOL_SOCKET, SO_SNDBUF, &buf_size, sizeof(buf_size));

    unsigned one = 1;
    if (setsockopt(client_fd, SOL_SOCKET, SO_ZEROCOPY, &one, sizeof(one))) {
        error(1, errno, "setsockopt zerocopy");
    }
    

    while (1) {
      // err = send(client_fd, data, size, 0 /* MSG_ZEROCOPY */);
      do {
      } while (0xFC00000 == buffer_info[0]);
      err = send(client_fd, data, 2304 * 1296, 0 /* MSG_ZEROCOPY */);
 
      do {
      } while (0x10400000 == buffer_info[1]);
      err = send(client_fd, data2, 2304 * 1296, 0 /* MSG_ZEROCOPY */);

      do {
      } while (0x10C00000 == buffer_info[0]);
      err = send(client_fd, data3, 2304 * 1296, 0 /* MSG_ZEROCOPY */);
 
      do {
      } while (0x11400000 == buffer_info[1]);
      err = send(client_fd, data4, 2304 * 1296, 0 /* MSG_ZEROCOPY */);


      if (err < 0) {
        fprintf(stderr, "Client write failed: %d\n", errno);
        break;
        // error(1, errno, "Client write failed");
      }
    }
  }

  return 0;
}
