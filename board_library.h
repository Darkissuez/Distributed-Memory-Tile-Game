
typedef struct board_place{
  char v[3];
  int cor[3]; //(-1,-1,-1)
  int estado; //0 para par feito(letras a preto), 1 para 1st pick(cinzento),
              //2 para jogada errada (vermelho)  -2 inicializacao
} board_place;

typedef struct play_response{
  int code;
            // 10 - na inicializacao
            // -1 - virar 1ºcarta para baixo
            // 0 - filled / fora de jogo
            // 1 - 1st play
            // 2 2nd - same plays
            // 3 END - fim de jogo
            // -2 2nd - diffrent
  int play1[2];
  int play2[2];
  char str_play1[3], str_play2[3];
} play_response;

char * get_board_place_str(int i, int j);
int linear_conv(int i, int j);
board_place * get_board_place_card(int i, int j);
board_place* init_board(int dim, int*n_corrects);
play_response board_play(int x, int y, int*play1, int*cor,pthread_mutex_t*mux_score,pthread_mutex_t*vetor_mux, int*n_corrects,sem_t* sem;);
