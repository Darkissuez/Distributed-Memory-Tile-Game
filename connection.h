#define PORT 3002
#define _GNU_SOURCE

typedef struct layer_com{
		int x;
		int y;
		int r;
		int g;
		int b;
		short flag; //0 para print_card, 1 para write_card, 2 end game
		char string[3];
}layer_com;

typedef struct coordinates{
	int x;
	int y;
}coordinates;

layer_com preenche_buffer(int x, int y, int r, int g, int b, short flag,char letras[3]);
static void handle_disc_client (int signo);
//compilar
//gcc client.c -lpthread  UI_library.c -o client -lSDL2 -lSDL2_ttf
//gcc server.c -lpthread  UI_library.c board_library.c lista.c -o server -lSDL2 -lSDL2_ttf
//gcc simple_bot.c board_library.c -o simple_bot

//regioes criticas:
		//Lista (mutex na insercao e remoçao de elementos)
		//board
		//n_corrects
		//numero_jogadores
