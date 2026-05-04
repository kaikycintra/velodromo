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

#define MIN_EQUIPES 5
#define MAX_EQUIPES 1249 // ⌊MAX_LEN_VELODROMO/2⌋−1

#define QUANTUM 60 // intervalo de tempo em ms que define um passo da simulação

typedef struct {
    int pista;
    int coluna;
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
    int comprimento_velodromo; // comprimento de cada pista
    int volta_atual; // volta atual da equipe em primeiro lugar
    char *velodromo[QTD_PISTAS][MAX_LEN_VELODROMO];
    struct_equipe *equipes[MAX_EQUIPES];
    struct_ciclista *ciclistas[MAX_EQUIPES*2];
} struct_corrida;

void printa_ciclistas(struct_ciclista *ciclistas[], int arr_len) {
    for(int i = 0; i < arr_len; i++) {
        struct_ciclista *c = ciclistas[i];
        printf("ciclista: %s, equipe: %d, estado: %s\n", c->nome, c->equipe, c->estado);
    }
}

void printa_equipes(struct_equipe *equipes[MAX_EQUIPES],
                    struct_ciclista *ciclistas[MAX_EQUIPES*2],
                    int qtd_equipes) {
    for(int i = 0; i < qtd_equipes; i++) {
        struct_equipe *e = equipes[i];
        struct_ciclista *c1 = ciclistas[2*i];
        struct_ciclista *c2 = ciclistas[2*i+1];
        printf("equipe: %d, volta atual: %d, pontos: %d\n", i, e->volta_atual, e->pontos);
        printf("ciclista: %s, equipe: %d, estado: %s\n", c1->nome, c1->equipe, c1->estado);
        printf("ciclista: %s, equipe: %d, estado: %s\n", c2->nome, c2->equipe, c2->estado);
    }
}

void get_ciclistas_por_estado(struct_ciclista *ciclistas[MAX_EQUIPES*2],
                          struct_ciclista *c_ativos[MAX_EQUIPES],
                          int qtd_equipes,
                          char* estado) {
    int idx = 0;
    
    for(int i = 0; i < qtd_equipes*2; i++) {
        if(strcmp(ciclistas[i]->estado, estado) == 0) {
            c_ativos[idx] = ciclistas[i];
            idx++;
        }
    }
}

void embaralha_ciclistas(struct_ciclista *c_ativos[],
                         int arr_len) {
    int idx_antigo, idx_novo;
    struct_ciclista *c_temp, *c_antigo;

    idx_antigo = 0;
    for(int i = 0; i < arr_len; i++) {
        idx_novo = rand() % arr_len;
        c_antigo = c_ativos[idx_antigo];
        c_temp = c_ativos[idx_novo];
        c_ativos[idx_novo] = c_antigo;
        c_ativos[idx_antigo] = c_temp;
        idx_antigo = idx_novo;
    }
}

void printa_velodromo(char *velodromo[10][MAX_LEN_VELODROMO], int comprimento_velodromo, bool debug) {
    FILE *saida = debug ? stderr : stdout;

    int linhas = 10;
    int colunas = comprimento_velodromo;

    for(int i = linhas-1; i >= 0; i--) {
        for(int j = 0; j < colunas; j++) {
            char *display_string = velodromo[i][j] == NULL ? ". " : velodromo[i][j];
            fprintf(saida, "%s ", display_string);
        }
        fprintf(saida, "\n");
    }
}

void posiciona_ciclistas(struct_ciclista *ciclistas[MAX_EQUIPES*2],
                        char *velodromo[10][MAX_LEN_VELODROMO],
                        int qtd_equipes,
                        int comprimento_pista) {
    //embaralha ciclistas e os posiciona em filas de 5 nas pistas mais internas
    struct_ciclista* c_ativos[MAX_EQUIPES];
    struct_ciclista* c_descansando[MAX_EQUIPES];

    get_ciclistas_por_estado(ciclistas, c_ativos, qtd_equipes, "ativo");
    get_ciclistas_por_estado(ciclistas, c_descansando, qtd_equipes, "descansando");

    embaralha_ciclistas(c_ativos, qtd_equipes);
    embaralha_ciclistas(c_descansando, qtd_equipes);

    // posiciona ciclistas ativos
    int len_ultima_fila = qtd_equipes % 5;
    int qtd_filas = qtd_equipes / 5 + 1; // divisão inteira
    if(len_ultima_fila == 0) qtd_filas--;
    int idx = 0;

    for(int coluna = 0; coluna < qtd_filas; coluna++) {
        if(coluna == qtd_filas - 1 && len_ultima_fila != 0) {
            for(int pista = 0; pista < len_ultima_fila; pista++) {
                c_ativos[idx]->coluna = comprimento_pista-(1+coluna);
                c_ativos[idx]->pista = pista;
                velodromo[pista][comprimento_pista-(1+coluna)] = c_ativos[idx++]->nome;
            }
        }
        else{
            for(int pista = 0; pista<5; pista++) {
                c_ativos[idx]->coluna = comprimento_pista-(1+coluna);
                c_ativos[idx]->pista = pista;
                velodromo[pista][comprimento_pista-(1+coluna)] = c_ativos[idx++]->nome;
            }
        }
    }

    // posiciona ciclistas descansando
    idx = 0;
    int pista_externa = QTD_PISTAS-1;
    for(int coluna = 0; coluna < qtd_equipes; coluna++) {
        c_descansando[idx]->coluna = comprimento_pista-(1+2*coluna);
        c_descansando[idx]->pista = pista_externa;
        velodromo[pista_externa][comprimento_pista-(1+2*coluna)] = c_descansando[idx]->nome;
        idx++;
    }

}

void posiciona_inicio_corrida(struct_corrida* corrida, bool debug) {
    posiciona_ciclistas(corrida->ciclistas,
                        corrida->velodromo,
                        corrida->qtd_equipes,
                        corrida->comprimento_velodromo);
    printa_velodromo(corrida->velodromo, corrida->comprimento_velodromo, debug);
}

void arbitro(struct_corrida* corrida, bool debug) {
    if(corrida->volta_atual == 0) {
        posiciona_inicio_corrida(corrida, debug);
    }
    printa_ciclistas(corrida->ciclistas, corrida->qtd_equipes*2);
    
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

    //   a cada 60ms imprime na stderr o velódromo com a posição de cada ciclista
    //   não imprime o relatório ao final de cada volta, apenas ao final da corrida
    //   fprintf(stderr, );
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

void gera_equipes(struct_equipe *equipes[MAX_EQUIPES], int qtd_equipes) {
    for(int i=0; i < qtd_equipes; i++) {
        struct_equipe *equipe_i = malloc(sizeof(struct_equipe));
        equipe_i->pontos = 0;
        equipe_i->volta_atual = 0;
        equipes[i] = equipe_i;
    }
}

void gera_ciclistas(struct_ciclista *ciclistas[MAX_EQUIPES*2], int qtd_equipes) {
    for(int i=0; i < 2*qtd_equipes; i++) {
        struct_ciclista *ciclista_i = malloc(sizeof(struct_ciclista));
        ciclistas[i] = ciclista_i;
    }

    for(int i = 0; i < qtd_equipes*2; i = i+2) {
        ciclistas[i]->equipe = i/2;
        sprintf(ciclistas[i]->nome, "%da", i/2);
        sprintf(ciclistas[i]->estado, "descansando");
        ciclistas[i]->tempo_por_volta = 240;
        ciclistas[i]->tempo_restante_para_andar = 240;
        ciclistas[i]->voltas_realizadas = 0;

        ciclistas[i+1]->equipe = i/2;
        sprintf(ciclistas[i+1]->nome, "%db", i/2);
        sprintf(ciclistas[i+1]->estado, "descansando");
        ciclistas[i+1]->tempo_por_volta = 240;
        ciclistas[i+1]->tempo_restante_para_andar = 240;
        ciclistas[i+1]->voltas_realizadas = 0;
    }

    // sorteia ciclistas ativos para cada equipe
    for(int i = 0; i < qtd_equipes*2; i = i+2) {
        int idx = (rand() % 2 == 0) ? i : i+1;
        sprintf(ciclistas[idx]->estado, "ativo");
        ciclistas[idx]->tempo_por_volta = 120;
        ciclistas[idx]->tempo_restante_para_andar = 120;
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
    srand(time(NULL));

    struct_corrida* corrida = malloc(sizeof(struct_corrida));
    corrida->voltas = n;
    corrida->qtd_equipes = k;
    corrida->comprimento_velodromo = d;
    corrida->volta_atual = 0;
    
    gera_ciclistas(corrida->ciclistas, corrida->qtd_equipes);
    gera_equipes(corrida->equipes, corrida->qtd_equipes);

    arbitro(corrida, debug); // passa a corrida para que o árbitro a gerencie

    return 0;
}
