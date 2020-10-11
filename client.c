#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <pthread.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <unistd.h>
#include "connection.h"
#include "UI_library.h"
#include <signal.h>


int sock_fd;
SDL_Event event;
int dim_board;
void* aux_comm;
void* coord_aux;

static void ctrl_c_callback_handler(int signum){
	close(sock_fd);
	printf("Caught signal Ctr-C. Closing client\n");
	free(aux_comm);
	free(coord_aux);
	exit(0);
}

//thread de leitura dos dados do servidor
void *thread_func (void* arg){
	aux_comm = malloc(sizeof(layer_com));
	layer_com buffer;
	int ret;


	while(1){
		ret = read(sock_fd, aux_comm, sizeof(layer_com));
		if (ret == 0){
				printf("O servidor fechou esta linha de comunicacao (servidor abortou)\n");
				printf("A fechar cliente\n");
				close(sock_fd);
				free (aux_comm);
				free(coord_aux);
				exit(-1);
		}
		else if(ret == sizeof(layer_com)){
				memcpy(&buffer, aux_comm, sizeof(layer_com));
				if (buffer.flag == 0)
					paint_card(buffer.x, buffer.y , buffer.r, buffer.g, buffer.b);

				else if (buffer.flag == 1)
					write_card(buffer.x, buffer.y, buffer.string, buffer.r, buffer.g, buffer.b);

				else if(buffer.flag == 2){ //fim do jogo
					if(buffer.y == 1)
						printf("VENCEDOR (score = %d)\n", buffer.x);
					else
						printf("PERDEDOR (score = %d)\n", buffer.x);
					sleep(10);
					printf("A reiniciar a board\n");
					reinicia_board_branco(300, 300, dim_board);
					}
				}
}
}




int main(int argc, char * argv[]){
	signal(SIGINT, ctrl_c_callback_handler);
	struct sockaddr_in server_addr;
	pthread_t thread_id;
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


  //inicializao da biblioteca grafica
	if( SDL_Init( SDL_INIT_VIDEO ) < 0 ) {
		printf( "SDL could not initialize! SDL_Error: %s\n", SDL_GetError() );
		exit(-1);
	}
	if(TTF_Init()==-1) {
		printf("TTF_Init: %s\n", TTF_GetError());
		exit(2);
	}

	//leitura da dimensao do tabuleiro e sua criacao
	if (read(sock_fd, &dim_board, sizeof(int)) != sizeof(int)){
				printf("board dimension not read successfully\n Exiting process\n");
				exit(-1);
	}

	create_board_window(300, 300, dim_board);

  pthread_create(&thread_id, NULL, thread_func,NULL);


//thread de envio de jogadas ao servidor
  while(1){
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
        case SDL_QUIT: {
					  close(sock_fd);
        		exit(1);
        }

        case SDL_MOUSEBUTTONDOWN:{ //     (coordenadas  -  (coluna, linha)
          int board_x, board_y;
          get_board_card(event.button.x, event.button.y, &board_x, &board_y);
					coord.x = board_x;
					coord.y = board_y;
					memcpy(coord_aux, &coord, sizeof(coordinates));
          printf("click (%d %d) -> (%d %d)\n", event.button.x, event.button.y,
									board_x, board_y);
          write(sock_fd, coord_aux, sizeof(coordinates));
        }
      }
    }
  }
}
