
typedef struct jogador jogador;
typedef struct jogador{
	int player_fd;
	int cor[3];
	int score;
	jogador* next;
	int ativo; //1 jogador ativo, 0jogador saiu
	int vencedor;
} jogador;


jogador* IniciaLista(void);
jogador* AdicionaElemento(jogador* lista, int player_fd, int cor[3],pthread_rwlock_t *mux_lista);
void MostraElementosLista(jogador* lista, pthread_rwlock_t *mux_lista);
int* cria_cor(jogador*lista,pthread_rwlock_t *mux_lista);
void broadcast(jogador* lista, int x, int y, int r, int g, int b, int flag, char*str,pthread_rwlock_t *mux_lista);
void retira_lista(jogador* lista, int* cor,pthread_rwlock_t *mux_lista);
jogador* apaga_lista(jogador*lista,pthread_rwlock_t *mux_lista);
void close_sockets(jogador *lista,pthread_rwlock_t *mux_lista);
void atualiza_lista_end_game (jogador*lista,pthread_rwlock_t *mux_lista);
void ve_vencedor(jogador* lista,pthread_rwlock_t *mux_lista);
