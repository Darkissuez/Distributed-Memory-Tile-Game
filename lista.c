#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include "connection.h"
#include "lista.h"

jogador* IniciaLista(void){
    //lista em que o primeiro elemento nao tem informacao util
    //desta forma o topo da lista é sempre contante e nunca tem de ser atualizado
    jogador* lista;
    lista = (jogador*)calloc(1, sizeof(jogador));
    lista->next = NULL;
    return lista;
}

jogador* AdicionaElemento(jogador* lista, int fd, int color[3], pthread_rwlock_t *mux_lista){
    //adiciona sempre ao fim da lista (a ordem dos jogadores nao é relevante)
    jogador* local;
    jogador* ap_anterior;
    int i;

    //criacao do novo elemento
    local = (jogador*)calloc(1, sizeof(jogador));
    if (local == NULL){
        printf("erro na alocacao de memoria\n");
        return lista;
    }

    local->player_fd = fd;
    for (i=0; i<3; i++)
        local->cor[i] = color[i];
    local->score = 0;
    local->next = NULL; //insere sempre no fim
    local->ativo = 1;
    local->vencedor = 0;

    //insercao do novo elemento na lista
    pthread_rwlock_wrlock(mux_lista);
    for (ap_anterior = lista; ap_anterior->next != NULL; ap_anterior = ap_anterior->next);
    ap_anterior->next = local;
    pthread_rwlock_unlock(mux_lista);
    free(color);
    return local;
}


void MostraElementosLista(jogador* lista, pthread_rwlock_t *mux_lista){
    int x = 0;

    if ((lista == NULL)||(lista->next == NULL)){
        printf("Lista Vazia\n");
        return;
    }

    pthread_rwlock_rdlock(mux_lista);
    for (lista = lista->next; lista!= NULL; lista= lista->next) {
            x = x+1;
            printf("Elemento:%d,  cor R=%d G=%d B=%d,   fd=%d,   score=%d, estado:%d\n",x,
                      lista->cor[0], lista->cor[1], lista->cor[2],
                      lista->player_fd, lista->score, lista->ativo);

    }
    pthread_rwlock_unlock(mux_lista);
    printf("O jogo tem %d jogadores\n", x);
}


int vizinhanca (int r1, int g1, int b1, int r2, int g2, int b2){
    int sum;

    sum = abs(r1 - r2) + abs(g1 - g2) + abs(b1 - b2);
    if (sum > 20)
        return 1;
    else
        return 0;
}


//cores proibidas (200,200,200) - cinzento; (255,255,255) branco; (0,0,0)-preto
//(255,0,0)-vermelho
int* cria_cor(jogador*lista,pthread_rwlock_t *mux_lista){
    int*cor;
    cor = (int*)calloc(3, sizeof(int));
    int ok = 0;
    time_t t;
    srand((unsigned)time(&t));
    int r,g,b;
    while(!ok){
        r = rand()%256;
        g = rand()%256;
        b = rand()%256;

        if(vizinhanca(r,g,b,200,200,200) == 0 || vizinhanca(r,g,b,255,255,255) == 0 ||
                vizinhanca(r,g,b,0,0,0) == 0 || vizinhanca(r,g,b,255,0,0) == 0);
                  //a cor gerada está na vizinhanca de uma cor proibida
        else{
            ok = 1;
            pthread_rwlock_rdlock(mux_lista);
            for (lista = lista->next; lista!= NULL; lista= lista->next){
                  if(vizinhanca(r,g,b,lista->cor[0],lista->cor[1],lista->cor[2]) == 0){
                        ok = 0;
                        break;
                  }
            }
            pthread_rwlock_unlock(mux_lista);
          }
      }

      cor[0]=r;
      cor[1]=g;
      cor[2]=b;

    return cor;
}


//esta funcao apenas desativa o jogador (coloca o bit ativo a 0)
void retira_lista(jogador* lista, int *cor, pthread_rwlock_t *mux_lista){
  if ((lista==NULL)||(lista->next == NULL))
      return;

  pthread_rwlock_rdlock(mux_lista);
  for (lista = lista->next; lista!=NULL; lista = lista->next){
      if ((lista->cor[0] == cor[0])&& (lista->cor[1] == cor[1]) && (lista->cor[2] == cor[2])){
          lista->ativo = 0;
          close(lista->player_fd);
          break;
        }
  }
  pthread_rwlock_unlock(mux_lista);

}


//apaga a lista e faz free da memoria
jogador* apaga_lista(jogador*lista,pthread_rwlock_t *mux_lista){
  jogador *aux;
  pthread_rwlock_wrlock(mux_lista);
  while (lista != NULL){
    aux = lista;
    lista = lista->next;
    free(aux);
  }
  pthread_rwlock_unlock(mux_lista);
  return NULL;
}


void atualiza_lista_end_game (jogador*lista,pthread_rwlock_t *mux_lista){

  if ((lista==NULL)||(lista->next == NULL))
      return;
    jogador* aux = lista;
    jogador* eliminar;
    jogador* lista_local = lista;
    int change = 0;
    pthread_rwlock_wrlock(mux_lista);
    while(1){
      change = 0;
      //percorre a lista e procura um elemento inativo e se encontrar tira da lista
      for (lista = lista->next; lista!=NULL; lista = lista->next){
          if (lista->ativo == 0){
              eliminar = aux->next;
              (aux->next) = eliminar->next;
              free(eliminar);
              change = 1;
              break;
            }
            aux = aux->next;

          }
          if (change == 0)
              break;
          lista = lista_local;
          aux = lista;
        }
   pthread_rwlock_unlock(mux_lista);
}


//no caso do servidor se fechar, apaga as linhas de comunicacao ativas
void close_sockets(jogador* lista, pthread_rwlock_t *mux_lista){
    if ((lista == NULL)|| (lista->next == NULL))
        return; //lista vazia

    pthread_rwlock_rdlock(mux_lista);
    lista = lista->next;
    while (lista!= NULL){
        if (lista->ativo == 1){
            close(lista->player_fd);
          }
        lista = lista->next;
    }
    pthread_rwlock_unlock(mux_lista);

}


void broadcast(jogador* lista, int x, int y, int r, int g, int b, int flag, char*str,pthread_rwlock_t *mux_lista){
  void* aux_comm = malloc(sizeof(layer_com));
  memset(aux_comm, '0', sizeof(layer_com));
  layer_com buffer;

  buffer = preenche_buffer(x,y,r,g,b,flag,str);
  memcpy(aux_comm, &buffer, sizeof(buffer));

  pthread_rwlock_rdlock(mux_lista);
  for (lista = lista->next; lista!= NULL; lista= lista->next)
    if (lista->ativo==1)
        write(lista->player_fd, aux_comm, sizeof(layer_com));
  pthread_rwlock_unlock(mux_lista);
  free(aux_comm);

}




void ve_vencedor (jogador*lista,pthread_rwlock_t *mux_lista){
  jogador* auxiliar = lista;
  int score_max = 0;

  pthread_rwlock_rdlock(mux_lista);
  for (auxiliar = auxiliar->next; auxiliar!= NULL; auxiliar= auxiliar->next)
      if (auxiliar->score > score_max)
          score_max = auxiliar->score;

  for (lista = lista->next; lista!= NULL; lista= lista->next){
      if(lista->score==score_max){
          lista->vencedor = 1;
          printf("vencedor = %d\n", lista->player_fd);
        }
      }
  pthread_rwlock_unlock(mux_lista);

}
