// simula uma corrida Madison em um velódromo
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <stdbool.h>
#include <pthread.h>
#include <string.h>

#define QTD_PISTAS 10
#define MIN_VOLTAS 10
#define MAX_VOLTAS 1250

#define MIN_LEN_VELODROMO 100
#define MAX_LEN_VELODROMO 2500
#define MAX_LEN_PISTA 250 // MAX_LEN_VELODROMO/QTD_PISTAS

#define MIN_EQUIPES 5
#define MAX_EQUIPES 1249 // ⌊MAX_LEN_VELODROMO/2⌋−1

#define QUANTUM 60 // intervalo de tempo em ms que define um passo da simulação

typedef struct {
    int tempo_por_volta; // velocidade
    int tempo_restante_para_andar; 
    int voltas_realizadas; // voltas realizadas enquanto ativo, usado para implementar revezamento
    char estado[12]; // ativo, descansando
    char nome[12]; // número da equipe + a|b
    int equipe;
} struct_ciclista;

typedef struct {
    int pontos;
    int volta_atual;
} struct_equipe;
    
typedef struct {
    int voltas;
    int qtd_equipes;
    int comprimento_velodromo;
    int volta_atual; // volta atual da equipe em primeiro lugar
    char *velodromo[QTD_PISTAS][MAX_LEN_PISTA]; // 10 pistas e até 250m de comprimento de pista, totalizando até 2500m
    struct_equipe *equipes[MAX_EQUIPES];
    struct_ciclista *ciclistas[MAX_EQUIPES*2];
} struct_corrida;

void printa_ciclistas(struct_ciclista *ciclistas[MAX_EQUIPES*2], int qtd_equipes) {
    for(int i = 0; i < qtd_equipes*2; i++) {
        struct_ciclista *c = ciclistas[i];
        printf("ciclista: %s, equipe: %d, estado: %s\n", 
                c->nome, c->equipe, c->estado);
    }
}

void inicia_corrida(struct_corrida* corrida, bool debug) {
    // sorteia ciclistas ativos e descansando para cada equipe
    for(int i = 0; i < corrida->qtd_equipes*2; i = i+2) {
        int idx = (rand() % 2 == 0) ? i : i+1;
        sprintf(corrida->ciclistas[idx]->estado, "ativo");
    }

    printa_ciclistas(corrida->ciclistas, corrida->qtd_equipes);

    // posiciona corredores
    // primeira volta todos andam 1m a cada 120ms
    // um ciclista de cada equipe larga em fila com ordenação aleatória
    // largam antes da linha de chegada, com 5 ciclistas no máximo lado a lado nas pistas internas
    // temos várias filas com 5 e uma fila final com até 5 ciclistas
    // os ciclistas que não largaram ficam na pista mais externa a 15km/h (1m a cada 240ms)
}

void arbitro(struct_corrida* corrida, bool debug) {
    if(corrida->volta_atual == 0) {
        inicia_corrida(corrida, debug);
    }

    // versão ingênua, um semáforo para controlar acesso à matriz velódromo
    // versão eficiente, um semáforo para cada posição ou coluna da matriz

    // quando uma equipe completa a prova, as suas duas threads ciclistas devem ser destruídas

    // caso duas ciclistas completem uma volta ao mesmo tempo, as duas equipes recebem ponto
    // empates ao final da prova devem ser resolvidos aleatoriamente

    // entidade central
    // imprime posições na tela
    // controla relógio global
    // atualizar colocações
    // enquanto (houver ciclistas):
    // faça ciclistas andarem 1 passo de forma concorrente;
    // destrua as threads de ciclistas que precisam ser destruídas;
    // avance o relógio em 60ms;
    // imprima as informações na tela;
    // ao final da prova, faz desempate e destrói as threads
    // imprime relatórios no final de cada volta e da prova
    //   as posições devem avançar da direita para a esquerda
    //   ao final da volta, imprime a posição de cada equipe
    //   ao final da corrida, imprime o ranqueamento das equipes
    //      posição da equipe, instante de tempo em que finalizaram a corrida, qtd de voltas vencidas

    //é necessário dar_free nos dados de cada ciclista que foram alocados em gera_ciclistas
    //free(ciclistas_array[i]);
}

void* ciclista(void* arg) {
    // struct_ciclista* dados = (struct_ciclista*) arg;
    // controla sua própria velocidade
    //   caso a volta anterior tenha sido feita a 30Km/h, o sorteio é feito com 80%
    //   de chance de escolher 60Km/h e 20% de chance de escolher 30Km/h. Caso a volta anterior tenha sido
    //   feita a 60Km/h, o sorteio é feito com 40% de chance de escolher 60Km/h e 60% de chance de escolher 30Km/h
    // controla quando deve avançar
    // se está a 60km/h e há um na frente a 30km/h, anda a 30km/h se não consegue ultrapassar
    // para ultrapassar deve haver espaço à frente em uma pista mais externa, movimentação entre pistas é instantânea
    // revezamento acontece a partir de quando a cicilista ativa completa 5 voltas
    //   a nova ciclista entra na prova com a mesma velocidade da ativa
    //   a ciclista que entrou em recuperação vai para a pista mais externa e pedala a 15km/h
    // atualiza sua posição no velódromo (seção crítica), remove identificador na posição antiga
    return NULL;
}

// main
// flag debug
//   a cada 60ms imprime na stderr o velódromo com a posição de cada ciclista
//   não imprime o relatório ao final de cada volta, apenas ao final da corrida
//   fprintf(stderr, );

void printa_velodromo(char *velodromo[10][MAX_LEN_PISTA], int comprimento_velodromo, bool debug) {
    FILE *saida = debug ? stderr : stdout;

    int linhas = 10;
    int colunas = comprimento_velodromo/10;

    for(int i = linhas-1; i >= 0; i--) {
        for(int j = 0; j < colunas; j++) {
            char *display_string = velodromo[i][j] == NULL ? ". " : velodromo[i][j];
            fprintf(saida, "%s ", display_string);
        }
        fprintf(saida, "\n");
    }
}

bool check_debug_flag(char *debug_arg, int argc) {
    if (argc != 6) {
        return false;
    }

    char *debug = debug_arg;
    printf("%s\n", debug);
    bool check_dash = (strncmp(debug, "--", 2) == 0);
    
    debug += 2;
    bool check_debug = (strcmp(debug, "debug") == 0);
    
    if(!check_dash || !check_debug) {
        printf("para rodar em modo debug, o último argumento deve ter formato --debug\n");
        exit(1);
    }

    return true;
}

void gera_ciclistas(int qtd_equipes, struct_ciclista *ciclistas[MAX_EQUIPES*2]) {
    for(int i=0; i < 2*qtd_equipes; i++) {
        struct_ciclista *ciclista_i = malloc(sizeof(struct_ciclista));
        ciclistas[i] = ciclista_i;
    }

    for(int i = 0; i < qtd_equipes*2; i = i+2) {
        ciclistas[i]->equipe = i/2;
        sprintf(ciclistas[i]->nome, "%da", i/2);
        sprintf(ciclistas[i]->estado, "descansando");
        ciclistas[i]->tempo_por_volta = 120;
        ciclistas[i]->tempo_restante_para_andar = 120;
        ciclistas[i]->voltas_realizadas = 0;

        ciclistas[i+1]->equipe = i/2;
        sprintf(ciclistas[i+1]->nome, "%db", i/2);
        sprintf(ciclistas[i+1]->estado, "descansando");
        ciclistas[i+1]->tempo_por_volta = 120;
        ciclistas[i+1]->tempo_restante_para_andar = 120;
        ciclistas[i+1]->voltas_realizadas = 0;
    }
}

int main(int argc, char **argv) {
    if (argc < 5 || argc > 6) {
        fprintf(stderr, "Uso: %s <n> <d> <k> <i|e> --debug\n", argv[0]);
        return 1;
    }

    bool debug = check_debug_flag(argv[5], argc);
    
    // lê n, d, k, <i|e>, 'i' para ineficiente 'e' para eficiente
    // cada corrida possui n voltas (10 ≤ n ≤ 1250)
    // o velódromo tem d metros (100 ≤ d ≤ 2500)
    // k equipes começam a prova (5 ≤ k ≤ ⌊d/2⌋−1)
    int n = atoi(argv[1]);
    int d = atoi(argv[2]);
    int k = atoi(argv[3]);
    char *exec_mode = argv[4];
    printf("%d, %d, %d, %s\n", n, d, k, exec_mode);
    srand(time(NULL));

    struct_corrida* corrida = malloc(sizeof(struct_corrida));
    corrida->voltas = n;
    corrida->qtd_equipes = k;
    corrida->comprimento_velodromo = d;
    corrida->volta_atual = 0;
    
    printa_velodromo(corrida->velodromo, corrida->comprimento_velodromo, debug);

    gera_ciclistas(corrida->qtd_equipes, corrida->ciclistas);

    // cria 2*k threads ciclista iguais
    pthread_t ciclistas[2*k];
    for(int i=0; i < 2*k; i++) {
        pthread_create(&ciclistas[i], NULL, ciclista, corrida->ciclistas[i]);
    }

    arbitro(corrida, debug); // passa a corrida para que o árbitro a gerencie

    return 0;
}
