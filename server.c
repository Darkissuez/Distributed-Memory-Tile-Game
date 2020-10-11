/*
Inês Bernardino 83689
Rodrigo Oliveira 83728
*/


#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <poll.h>
#include <semaphore.h>
#include "connection.h"
#include "board_library.h"
#include "UI_library.h"
#include "lista.h"



//pthread_mutex_t mux_lista;
pthread_rwlock_t mux_lista;
pthread_mutex_t * mux_board;
pthread_mutex_t mux_score;
pthread_mutex_t mux_n_jogadores;
pthread_mutex_t mux_end;
pthread_cond_t  cond_end;
sem_t sem;

int dim_board;
int ignorar;
int sock_fd;
int players_fd;
int numero_jogadores;
jogador* lista;
board_place * board;
int n_corrects;



static void handle_disc_client (int signo){
 		printf("o servidor tentou escrever num socket ja fechado pelo cliente\n");
}


static void ctrl_c_callback_handler_server(int signum){
  int i;
  printf("Caught signal Ctr-C.\n");
  printf("A encerrar o programa\nlista atual de jogadores");
  MostraElementosLista(lista,&mux_lista);
  close_sockets(lista, &mux_lista); //fecha os sockets
	lista = apaga_lista(lista,&mux_lista);
  printf("lista final\n");
  MostraElementosLista(lista,&mux_lista);
  close(sock_fd);
  pthread_rwlock_destroy(&mux_lista);
  pthread_mutex_destroy(&mux_n_jogadores);
  pthread_mutex_destroy(&mux_score);
  for (i=0; i< (dim_board*dim_board); i++){
      pthread_mutex_destroy(&(mux_board[i]));
  }
  sem_destroy(&sem);
  free(mux_board);
  free(board);
	exit(0);
}


layer_com preenche_buffer(int x, int y, int r, int g, int b, short flag,
												char letras[3]){
	layer_com buffer;
	buffer.x = x;
	buffer.y = y;
	buffer.r = r;
	buffer.g = g;
	buffer.b = b;
	buffer.flag = flag;
	strcpy(buffer.string, letras);

	return buffer;
}




void board_client_meio(int fd) {
  int i,j;
  board_place * card;
  void* aux_comm = malloc(sizeof(layer_com));
  memset(aux_comm, '0', sizeof(layer_com));
  layer_com buffer;

  for (i=0; i< dim_board; i++){
      for (j=0; j<dim_board; j++){
        card = get_board_place_card(i,j);
        if (card->cor[0] != -1){
            buffer = preenche_buffer(i, j, card->cor[0],
                  card->cor[1], card->cor[2], 0, "\0");
            memcpy(aux_comm, &buffer, sizeof(buffer));
            write(fd, aux_comm, sizeof(buffer));
						if (card->estado == 0)
								buffer = preenche_buffer(i, j, 0, 0, 0, 1, card->v);
						else if(card->estado == 1)
								buffer = preenche_buffer(i, j, 200, 200, 200, 1, card->v);
						else if(card->estado == 2)
								buffer = preenche_buffer(i, j, 255, 0, 0, 1, card->v);
            memcpy(aux_comm, &buffer, sizeof(buffer));
            write(fd, aux_comm, sizeof(buffer));
						printf("cliente entrou a meio, alteracoes %d %d estado %d\n", i,j,card->estado);
        }
      }
      }

  free(aux_comm);
  return;
}

//quando um jogador se disconecta, as suas cartas ativas no board (estado = 1 ou 2) sao
//reiniciadas (estado = -2). pares feitos (estado=0) permanecem
void out_client (jogador* this_player){
	int i, j,k;

	for (i=0; i< dim_board; i++){
			for (j=0; j<dim_board; j++){
				board_place* card = get_board_place_card(i,j);
				if ((card->cor[0]==this_player->cor[0]) && (card->cor[1]==this_player->cor[1])
									&&(card->cor[2]==this_player->cor[2])){
					if ((card->estado == 1) || (card->estado == 2)){
						for (k=0; k<3; k++)
								card->cor[k]=-1;
						card->estado = -2;
						printf("apaguei a carta %d %d, estado %d\n", i,j,card->estado);
						broadcast(lista, i,j,255,255,255,0,"",&mux_lista);
						clear_card(i,j);
					}
				}
			}
		}
}


void atualiza_board(int estado, int x, int y, jogador* this_player, char*str){
	int i;
	board_place* card = get_board_place_card(x,y);

  if (estado == -2){
		for (i=0; i<3; i++)
				card->cor[i]=-1;
		card->estado = estado;
	}

	else{
		for (i=0; i<3; i++)
				card->cor[i]=this_player->cor[i];
}
}


void* fim_do_jogo (void * arg){
  while(1){
  pthread_mutex_lock(& mux_end );
  pthread_cond_wait(&cond_end, &mux_end);
  ignorar = 1;
  printf("Thread de fim de jogo");
  MostraElementosLista(lista,&mux_lista);
  if ((lista==NULL)||(lista->next == NULL))
      return NULL;
  jogador * aux;
  aux = lista;
  void* aux_comm = malloc(sizeof(layer_com));
  memset(aux_comm, '0', sizeof(layer_com));
  layer_com buffer;

  printf("vê vencedor\n");
  ve_vencedor(lista, &mux_lista);

//aproveitar a estrutura existente de comunicacao para enviar o score e a flag de vencedor
  pthread_rwlock_rdlock(&mux_lista);
  for (aux = aux->next; aux!= NULL; aux= aux->next){
      if (aux->ativo == 1){
          printf("fd do player ativo %d\n", aux->player_fd);
          layer_com buffer = preenche_buffer(aux->score,aux->vencedor,0,0,0,2,"");
          aux->score =0;
          aux->vencedor=0;
          memcpy(aux_comm, &buffer, sizeof(buffer));
          write(aux->player_fd, aux_comm, sizeof(layer_com));
    }
}
  pthread_rwlock_unlock(&mux_lista);
  atualiza_lista_end_game(lista, &mux_lista);
  MostraElementosLista(lista,&mux_lista);
  free(board);
  board = init_board(dim_board,&n_corrects);
  sleep(10);

  reinicia_board_branco(300, 300, dim_board);
  ignorar = 0;
  pthread_mutex_unlock(& mux_end);

  free(aux_comm);
}
}



void *thread_func (void * arg){
	int play1[2];
	int i;
	int ret, x, y;
	jogador* this_player = (jogador*)arg;
	board_place * card;
	play1[0]=-1;
	int fim;
  void* coord_aux = malloc(sizeof(coordinates));
  memset(coord_aux, '0', sizeof(coordinates));
  coordinates coord;

	//set the poll
	struct pollfd fd;
	fd.fd = this_player->player_fd;
	fd.events = POLLIN;

  write (this_player->player_fd, &dim_board, sizeof(dim_board)); //enviar para o client a dim
	board_client_meio(this_player->player_fd); //atualiza o board para o client que entra a meio

	while(1){
		fim = read(this_player->player_fd, coord_aux, sizeof(coordinates));
		if (fim == 0){
          //o jogador fechou, retirar da lista e apagar as jogadas ativas
        pthread_mutex_lock(&mux_n_jogadores);
        numero_jogadores--;
        pthread_mutex_unlock(&mux_n_jogadores);
				out_client(this_player); //atualiza a board
				retira_lista(lista, this_player->cor,&mux_lista);
				MostraElementosLista(lista,&mux_lista);
        free(coord_aux);
				break;
		}
    if ((ignorar == 1)||(numero_jogadores<2)){
      printf("a ignorar\n");
    }

    else if(fim == sizeof(coordinates)){
        memcpy(&coord, coord_aux, sizeof(coordinates));
        MostraElementosLista(lista,&mux_lista);
		    printf("%d %d\n\n", coord.x, coord.y);

			play_response resp = board_play(coord.x, coord.y, play1, this_player->cor, &mux_score,
              mux_board,&n_corrects, &sem);
			switch (resp.code) {

				case 1:	//1st play
          atualiza_board(1, resp.play1[0], resp.play1[1], this_player,resp.str_play1);
					broadcast(lista, resp.play1[0], resp.play1[1], this_player->cor[0],
						 		this_player->cor[1], this_player->cor[2], 0, "\0",&mux_lista);

					paint_card(resp.play1[0], resp.play1[1] , this_player->cor[0],
								this_player->cor[1], this_player->cor[2]);
					broadcast(lista,resp.play1[0], resp.play1[1], 200, 200, 200,
						 				1, resp.str_play1,&mux_lista);
					write_card(resp.play1[0], resp.play1[1], resp.str_play1, 200, 200, 200);

					printf("setting timer\n");
					ret = poll(&fd, 1, 5000);
					switch (ret) {
							case -1:
									// Error
									break;
							case 0:
								// Timeout  pintar de branco
                atualiza_board(-2, resp.play1[0], resp.play1[1], this_player,resp.str_play1);
								play1[0] = -1;
								printf("end of timer\n");
								broadcast(lista,resp.play1[0],resp.play1[1],255,255,255,0,"\0",&mux_lista);
								clear_card(resp.play1[0], resp.play1[1]);
								break;
						}
				break;

				case -1: //mesma carta, virar para baixo
          atualiza_board(-2, resp.play1[0], resp.play1[1], this_player,resp.str_play1);
					broadcast(lista,resp.play1[0], resp.play1[1], 255, 255, 255, 0, "\0",&mux_lista);
					clear_card(resp.play1[0], resp.play1[1]);
				break;

				case 3:	//END - fim de jogo
          this_player->score ++;
          atualiza_board(0, resp.play1[0], resp.play1[1], this_player,resp.str_play1);
          atualiza_board(0, resp.play2[0], resp.play2[1], this_player,resp.str_play2);
					broadcast(lista,resp.play1[0], resp.play1[1], this_player->cor[0],
									this_player->cor[1], this_player->cor[2], 0, "\0",&mux_lista);
					paint_card(resp.play1[0], resp.play1[1], this_player->cor[0],
									this_player->cor[1], this_player->cor[2]);
					broadcast(lista, resp.play1[0], resp.play1[1], 0, 0, 0,	1, resp.str_play1,&mux_lista);
					write_card(resp.play1[0], resp.play1[1], resp.str_play1, 0, 0, 0);
					broadcast(lista, resp.play2[0], resp.play2[1], this_player->cor[0],
									this_player->cor[1], this_player->cor[2], 0, "\0",&mux_lista);
					paint_card(resp.play2[0], resp.play2[1], this_player->cor[0],
									this_player->cor[1], this_player->cor[2]);
					broadcast(lista, resp.play2[0], resp.play2[1], 0, 0, 0,	1, resp.str_play2,&mux_lista);
					write_card(resp.play2[0], resp.play2[1], resp.str_play2, 0, 0, 0);
          int z;
          for (z = 0; z < (dim_board*dim_board/2)-1; z++)
              sem_wait(& sem);
          pthread_mutex_lock(& mux_end);
          pthread_cond_signal(&cond_end);
          pthread_mutex_unlock(& mux_end);
          break;

				case 2: //segunda jogada certa
          atualiza_board(0, resp.play1[0], resp.play1[1], this_player,resp.str_play1);
          atualiza_board(0, resp.play2[0], resp.play2[1], this_player,resp.str_play2);
          this_player->score ++; //atualiza o score do jogador
					broadcast(lista, resp.play1[0], resp.play1[1], this_player->cor[0],
									this_player->cor[1], this_player->cor[2], 0, "\0",&mux_lista);
					paint_card(resp.play1[0], resp.play1[1], this_player->cor[0],
									this_player->cor[1], this_player->cor[2]);
					broadcast(lista, resp.play1[0], resp.play1[1], 0, 0, 0,	1, resp.str_play1,&mux_lista);
					write_card(resp.play1[0], resp.play1[1], resp.str_play1, 0, 0, 0);
					broadcast(lista, resp.play2[0], resp.play2[1], this_player->cor[0],
									this_player->cor[1], this_player->cor[2], 0, "\0",&mux_lista);
					paint_card(resp.play2[0], resp.play2[1], this_player->cor[0],
									this_player->cor[1], this_player->cor[2]);
					broadcast(lista, resp.play2[0], resp.play2[1], 0, 0, 0,	1, resp.str_play2,&mux_lista);
					write_card(resp.play2[0], resp.play2[1], resp.str_play2, 0, 0, 0);
          sem_post(& sem);
				break;

				case -2: //segunda jogada errada
          atualiza_board(2, resp.play1[0], resp.play1[1], this_player,resp.str_play1);
          atualiza_board(2, resp.play2[0], resp.play2[1], this_player,resp.str_play2);
					broadcast(lista, resp.play1[0], resp.play1[1], this_player->cor[0],
									this_player->cor[1], this_player->cor[2], 0, "\0",&mux_lista);
					paint_card(resp.play1[0], resp.play1[1], this_player->cor[0],
									this_player->cor[1], this_player->cor[2]);
					broadcast(lista, resp.play1[0], resp.play1[1], 255, 0, 0,	1, resp.str_play1,&mux_lista);
					write_card(resp.play1[0], resp.play1[1], resp.str_play1, 255, 0, 0);
					broadcast(lista, resp.play2[0], resp.play2[1], this_player->cor[0],
									this_player->cor[1], this_player->cor[2], 0, "\0",&mux_lista);
					paint_card(resp.play2[0], resp.play2[1], this_player->cor[0],
									this_player->cor[1], this_player->cor[2]);
					broadcast(lista, resp.play2[0], resp.play2[1], 255, 0, 0,	1, resp.str_play2,&mux_lista);
					write_card(resp.play2[0], resp.play2[1], resp.str_play2, 255, 0, 0);

					//sleep(2), enquanto está no sleep, descartar as jogadas todas que entrem
					time_t begin = time(NULL);
					while((time(NULL) - begin) < 2){
							ret = poll(&fd, 1, 2000-(time(NULL)-begin)*1000);
							switch (ret) {
									case -1:
									// Error
										break;
									case 0:
										break;

									default: //quando nao é 0 nem -1
                      read(this_player->player_fd, coord_aux, sizeof(coordinates));
						}
					}
          atualiza_board(-2, resp.play1[0], resp.play1[1], this_player,resp.str_play1);
          atualiza_board(-2, resp.play2[0], resp.play2[1], this_player,resp.str_play2);
					broadcast(lista, resp.play1[0], resp.play1[1], 255, 255, 255, 0, "\0",&mux_lista);
					paint_card(resp.play1[0], resp.play1[1] , 255, 255, 255);
					broadcast(lista, resp.play2[0], resp.play2[1], 255, 255, 255, 0, "\0",&mux_lista);
					paint_card(resp.play2[0], resp.play2[1] , 255, 255, 255);
				break;
			}
		}
	}
}




int main(int argc, char * argv[]){
	signal(SIGPIPE, handle_disc_client);
  signal(SIGINT, ctrl_c_callback_handler_server);
  pthread_rwlock_init(&mux_lista, NULL);
  pthread_mutex_init(&mux_score, NULL);
  pthread_mutex_init(&mux_n_jogadores, NULL);
  pthread_mutex_init(&mux_end, NULL);
	pthread_cond_init(& cond_end,NULL);
  sem_init(&sem, 0, 0);
	struct sockaddr_in local_addr;
  pthread_t thread_id;
	int *cor;
  int i, ret;
	jogador* local;

  if (argc <2){
		printf("second argument should be board dimension \n");
		exit(-1);
	}

  ret = sscanf(argv[1], "%d", &dim_board);
  if (ret != 1){
      printf("Dimensao nao foi lida corretamente");
      exit(-1);
  }
  else if ((dim_board%2 != 0)||(dim_board>26)) {
      printf("A dimensao especificada está fora dos limites.");
      printf("Especificar uma dimensao entre 2 e 26 (par)\n");
      printf("A abortar o tabuleiro");
      exit(-2);
  }

  ignorar = 0;
  numero_jogadores = 0;

	//inicializao do socket
	sock_fd= socket(AF_INET, SOCK_STREAM, 0);
	if (sock_fd == -1){
	perror("socket: ");
	exit(-1);
	}
	local_addr.sin_family = AF_INET;
	local_addr.sin_port= htons(PORT);
	local_addr.sin_addr.s_addr= INADDR_ANY;
	ret= bind(sock_fd, (struct sockaddr *)&local_addr, sizeof(local_addr));

	if(ret == -1) {
		perror("bind");
		exit(-1);
	}

	printf(" socket created and binded \n");
	listen(sock_fd, 5);

	//inicializao da biblioteca grafica
	if( SDL_Init( SDL_INIT_VIDEO ) < 0 ) {
		printf( "SDL could not initialize! SDL_Error: %s\n", SDL_GetError() );
		exit(-1);
	}

	if(TTF_Init()==-1) {
		printf("TTF_Init: %s\n", TTF_GetError());
		exit(2);
	}



	create_board_window(300, 300, dim_board);
	board = init_board(dim_board,&n_corrects);

  //inicializacao dos mutexes
  mux_board = malloc(sizeof(pthread_mutex_t)* dim_board *dim_board);
  for (i=0; i< (dim_board*dim_board); i++){
      pthread_mutex_init(&(mux_board[i]), NULL);
  }

  pthread_create(&thread_id, NULL, fim_do_jogo,NULL); //end of game thread handler

	lista = (jogador*) IniciaLista();
	while(1){
		printf("Waiting for players\n");
		players_fd= accept(sock_fd, NULL, NULL);
		printf("client connected\n");

    pthread_mutex_lock(&mux_n_jogadores);
    numero_jogadores++;
    pthread_mutex_unlock(&mux_n_jogadores);

		cor = cria_cor(lista,&mux_lista);
		printf("%d %d %d\n", cor[0], cor[1], cor[2]);
		local = AdicionaElemento(lista, players_fd, cor,&mux_lista);
		MostraElementosLista(lista,&mux_lista);
		pthread_create(&thread_id, NULL, thread_func,local);
	}
}
