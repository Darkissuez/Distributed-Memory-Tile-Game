#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include "board_library.h"
#include "connection.h"

int dim_board;
board_place * board;


int linear_conv(int i, int j){
  return j*dim_board+i;
}
  char * get_board_place_str(int i, int j){
  return board[linear_conv(i, j)].v;
}

board_place * get_board_place_card(int i, int j){
return &(board[linear_conv(i, j)]);
}


board_place* init_board(int dim, int*n_corrects){
  int count  = 0;
  int i, j;
  char * str_place;
  char c1, c2;
  dim_board= dim;
  *n_corrects = 0;
  board = malloc(sizeof(board_place)* dim *dim);

  for( i=0; i < (dim_board*dim_board); i++){
    board[i].v[0] = '\0';
    board[i].cor[0] = -1;
    board[i].estado = -2;
  }

  for (c1 = 'a' ; c1 < ('a'+dim_board); c1++){
    for (c2 = 'a' ; c2 < ('a'+dim_board); c2++){
      do{
        i = random()% dim_board;
        j = random()% dim_board;
        str_place = get_board_place_str(i, j);
      }while(str_place[0] != '\0');
      str_place[0] = c1;
      str_place[1] = c2;
      str_place[2] = '\0';
      //printf("%d %d -%s-\n", i, j, str_place);
      do{
        i = random()% dim_board;
        j = random()% dim_board;
        str_place = get_board_place_str(i, j);
        //printf("%d %d -%s-\n", i, j, str_place);
      }while(str_place[0] != '\0');
      str_place[0] = c1;
      str_place[1] = c2;
      str_place[2] = '\0';
      count += 2;

      //Imprimir o tabuleiro completo
      if (count == dim_board*dim_board){
            for (i=0; i< dim_board; i++){
                for (j=0; j<dim_board; j++){
                  str_place = get_board_place_str(i,j);
                  printf("%d %d -%s-\n", i, j, str_place);
                }
              }
            return board;
      }

    }
  }
}



play_response board_play(int x, int y, int*play1, int*cor, pthread_mutex_t* mux_score, pthread_mutex_t*vetor_mux, int*n_corrects,sem_t* sem){
  play_response resp;
  int i;
  printf("%d %d\n", x,y);

  resp.code =10;
  if (x>= dim_board || y>= dim_board){
      printf("jogada não válida\n");
      return resp; //ignorado pelo servidor
  }
  board_place* card = get_board_place_card(x,y);

  pthread_mutex_t mutex_card = vetor_mux[y*dim_board+x];

    if(play1[0]== -1){
        printf("FIRST pick\n");
        //verificar se a primeira pick é uma carta ativa de outro jogador
        pthread_mutex_lock(&mutex_card);
        if (card->estado != -2){
            resp.code = 10;
            pthread_mutex_unlock(&mutex_card);
            printf("FILLED (carta nao pode ser selecionada)\n");
          }

        else{ //caso a pick nao seja uma carta ativa de outro jogador
            card->estado = 1;
            pthread_mutex_unlock(&mutex_card);
            resp.code =1;
            play1[0]=x;
            play1[1]=y;
            resp.play1[0]= play1[0];
            resp.play1[1]= play1[1];
            strcpy(resp.str_play1, get_board_place_str(x, y));
          }


      }else{
        printf("SECOND pick\n");
        board_place* card1 = get_board_place_card(play1[0], play1[1]);//1st play card

        //carregar na mesma carta
        if ((play1[0]==x) && (play1[1]==y)){
          resp.play1[0]= x;
          resp.play1[1]= y;
          resp.code =-1;
          play1[0]=-1;
          card->estado = -2;
          printf("FILLED (carregou na mesma carta)\n");
        } else{//nao ter carregado na mesma carta

          resp.play1[0]= play1[0];
          resp.play1[1]= play1[1];
          strcpy(resp.str_play1, card1->v);
          resp.play2[0]= x;
          resp.play2[1]= y;
          strcpy(resp.str_play2, card->v);

          pthread_mutex_lock(&mutex_card);
          if(card->estado != -2){ //caso a carta escolhida nao seja valida
                resp.code = -1; //virar a primeira carta para baixo
                card1->estado = -2;
                pthread_mutex_unlock(&mutex_card);
              }

          else if (strcmp(card->v, card1->v) == 0){
            printf("CORRECT!!!\n");
            card1->estado = 0;
            card->estado = 0;
            pthread_mutex_unlock(&mutex_card);

            pthread_mutex_lock(mux_score);
            *n_corrects +=2;


            if (*n_corrects == dim_board* dim_board)
                resp.code =3;
            else{
              resp.code =2;
              /*if (*n_corrects == (dim_board*dim_board) - 2) //ultimo case 2 (o proximo sera case 3)
                  sem_wait(sem);//decremento semaforo*/
                }

            pthread_mutex_unlock(mux_score); //para impedir que hajam duas threads
                                            //a retornar code = 3 ao mesmo tempo
                                            //o que provocaria o reinicio do jogo duas vezes
          }else{ //as duas cartas nao coincidem
            card1->estado = 2;
            card->estado = 2;
            pthread_mutex_unlock(&mutex_card);
            printf("INCORRECT\n");
            resp.code = -2;
          }
          play1[0]= -1;//de volta à primeira pick
        }
      }
  return resp;
}
