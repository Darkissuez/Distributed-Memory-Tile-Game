#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include "board_library.h"
#include "connection.h"
#include <signal.h>

int sock_fd;
int dim_board;
board_place * board;
void* coord_aux;

static void ctrl_c_callback_handler(int signum){
	close(sock_fd);
	printf("Caught signal Ctr-C. Closing client\n");
	free(coord_aux);
	free(board);
	exit(0);
}

int main (int argc, char * argv[]) {
	struct sockaddr_in server_addr;
	time_t t;
	srand((unsigned)time(&t));
	signal(SIGINT, ctrl_c_callback_handler);
  int x,y;
	if (argc <2){
		printf("second argument should be server address\n");
		exit(-1);
	}
	coord_aux = malloc(sizeof(coordinates));
	memset(coord_aux, '0', sizeof(coordinates));
	coordinates coord;

	sock_fd= socket(AF_INET, SOCK_STREAM, 0);
	if (sock_fd == -1){
		perror("socket: ");
		exit(-1);
	}

  server_addr.sin_family = AF_INET;
	server_addr.sin_port= htons(PORT);
	inet_aton(argv[1], &server_addr.sin_addr);
  if( -1 == connect(sock_fd, (const struct sockaddr *) &server_addr,
																							sizeof(server_addr))){
		printf("Error connecting\n");
		exit(-1);
	}
  printf("client connected\n");


	read(sock_fd, &dim_board, sizeof(int));
  board = malloc(sizeof(board_place)* dim_board *dim_board);

  while(1){
          x = rand()%dim_board;
          y = rand()%dim_board;
          coord.x = x;
          coord.y = y;
          memcpy(coord_aux, &coord, sizeof(coordinates));
          printf("click (%d %d) \n", x,y);
          write(sock_fd, coord_aux, sizeof(coordinates));
          sleep(1);
        }
}
