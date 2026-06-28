#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TAM_MAX_LINHA 50
#define TAM

typedef enum {
	FIRST_FIT,
	BEST_FIT,
	WORST_FIT,
	BUDDY
} Estrategia;

void finalizar_com_erro(const char *mensagem) {
	fprintf(stderr, "Erro: %s\n", mensagem);
	exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
	if (argc != 3) {
		finalizar_com_erro("Quantidade incorreta de argumentos. Uso: ./simulador <estratégia> <arquivo_entrada>");
	}


	// if estrategia nao existe, finalizaEstrategia alloc_alg;
	Estrategia alloc_alg;
	if (strcmp(argv[1], "FIRST_FIT") == 0){
		alloc_alg = FIRST_FIT;
	} else if (strcmp(argv[1], "BEST_FIT") == 0) {
		alloc_alg = BEST_FIT;
	} else if (strcmp(argv[1], "WORST_FIT") == 0) {
		alloc_alg = WORST_FIT;
	} else if (strcmp(argv[1], "BUDDY") == 0) {
		alloc_alg = BUDDY; 
	} else {
		finalizar_com_erro("Estratégia de alocação inválida. Escolha entre FIRST_FIT - BEST_FIT - WORST_FIT - BUDDY");
	}

	FILE *arquivo_entrada = fopen(argv[2], "r");

	if (!arquivo_entrada) {
		finalizar_com_erro("Arquivo de entrada não encontrado.");
	}


	char nome_log[TAM_MAX_LINHA] = "log_";
	// + argv[2] (só o ultimo) + "_" + argv[1]
	strcat(nome_log, ".txt");

	FILE *log = fopen(nome_log, "w");
	fprintf(log, "oi");


	char buffer[TAM_MAX_LINHA];
	fgets(buffer, TAM_MAX_LINHA, arquivo_entrada);

	int qnt_processos;
	if (sscanf(buffer, "%d", &qnt_processos)!= 1) {
		finalizar_com_erro("eita");
	}

	fgets(buffer, TAM_MAX_LINHA, arquivo_entrada);
	// salve o id dos processos

	char comando[10];
	while (fgets(buffer, TAM_MAX_LINHA, arquivo_entrada)) {
		sscanf(buffer, "%s", comando);
		if (strcmp(comando, "aloca") == 0) {
			// aloca ...
		} else if (strcmp(comando, "libera") == 0) {
			// libera
		} else if (strcmp(comando,"acessa") == 0) {
			// acessa ...
		}
	}

	printf("%d\n", qnt_processos);


	fclose(log);
	fclose(arquivo_entrada);
	return 0;
}
