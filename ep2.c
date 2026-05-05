// simula uma corrida Madison em um velódromo
#define _GNU_SOURCE
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

pthread_mutex_t mutex_velodromo; // usado para implementação ineficiente
pthread_cond_t sinal_destruicao;
pthread_barrier_t barreira_passo;

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
    bool terminou;
    bool revezando;
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

typedef struct {
    struct_ciclista *ciclista;
    struct_ciclista *colega;
    struct_equipe *equipe;
    int voltas;
    int comprimento_velodromo;
    char *(*velodromo)[MAX_LEN_VELODROMO];
} struct_ciclista_arg;

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

void printa_velodromo(char *velodromo[10][MAX_LEN_VELODROMO], int comprimento_velodromo, bool error_out) {
    FILE *saida = error_out ? stderr : stdout;

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
    //printa_velodromo(corrida->velodromo, corrida->comprimento_velodromo, debug);
}

bool acabou_corrida(struct_equipe *equipes[], int qtd_equipes) {
    for(int i = 0; i < qtd_equipes*2; i++) {
        if(equipes[i]->terminou == false) return false;
    }
    return true;
}

bool pode_ultrapassar(int pista,
                      int coluna,
                      int comprimento_pista,
                      char *velodromo[10][MAX_LEN_VELODROMO]) {
    for(int p = pista; p < QTD_PISTAS-1; p++) {
        if(velodromo[p][(coluna + 1) % comprimento_pista] == NULL) {
            return true;
        }
    }
    return false;
}

void avanca_posicao(struct_ciclista *ciclista,
                    char *velodromo[10][MAX_LEN_VELODROMO],
                    int comprimento_pista) {
    int coluna = ciclista->coluna;
    int pista = ciclista->pista;
    int coluna_antiga = coluna, pista_antiga = pista;

    // se pode andar para a frente
    if(velodromo[pista][(coluna + 1) % comprimento_pista] == NULL) {
        coluna = (coluna + 1) % comprimento_pista;
    }
    
    // se não, se pode ultrapassar
    else if(pode_ultrapassar(pista, coluna, comprimento_pista, velodromo)) {
        for(int p = pista; p < QTD_PISTAS-1; p++) {
            if(velodromo[p][(coluna + 1) % comprimento_pista] == NULL) {
                pista = p;
                coluna = (coluna + 1) % comprimento_pista;
                break;
            }
        }
    }
    
    else {
        // se não, espera alguém sair da frente (como pedido nas especificações)
        return;
    }

    velodromo[pista_antiga][coluna_antiga] = NULL;
    velodromo[pista][coluna] = ciclista->nome;
    ciclista->coluna = coluna;
    ciclista->pista = pista;
    ciclista->tempo_restante_para_andar = ciclista->tempo_por_volta;
}

void reposiciona_pista(struct_ciclista *ciclista,
                      char *velodromo[10][MAX_LEN_VELODROMO]) {
    // se reposiciona de pista, ciclistas ativos devem ir às pistas internas, outros às pistas externas
    int coluna = ciclista->coluna;
    int pista = ciclista->pista;
    int pista_antiga = pista;
    char *estado = ciclista->estado;

    if(strcmp(estado, "ativo") == 0) {
        for(int p = pista; p >= 0; p--) {
            if(velodromo[p][coluna] == NULL) {
                pista = p;
            }
        }
    }
    else {
        for(int p = pista; p < QTD_PISTAS; p++) {
            if(velodromo[p][coluna] == NULL) {
                pista = p;
            }
        }
    }

    velodromo[pista_antiga][coluna] = NULL;
    velodromo[pista][coluna] = ciclista->nome;
    ciclista->pista = pista;
}

void atualiza_velocidade(struct_ciclista *ciclista, int velocidade_colega, bool revezando) {
    if(revezando) {
        ciclista->tempo_por_volta = strcmp(ciclista->estado, "ativo") == 0 ? ciclista->tempo_por_volta : velocidade_colega;
        return;
    }

    if(strcmp(ciclista->estado, "descansando") == 0) {
        ciclista->tempo_por_volta = 240;
    }

    if(ciclista->tempo_por_volta == 120) {
        ciclista->tempo_por_volta = (rand() % 100 < 80) ? 60 : 120;
    }
    else if (ciclista->tempo_por_volta == 60) {
        ciclista->tempo_por_volta = (rand() % 100 < 40) ? 60 : 120;
    }
    ciclista->tempo_restante_para_andar = ciclista->tempo_por_volta;
}

void* ciclista(void* arg) {
    struct_ciclista_arg* dados = (struct_ciclista_arg*) arg;
    while(dados->equipe->terminou == false) {
        pthread_mutex_lock (&mutex_velodromo);
        
        int coluna_antiga = dados->ciclista->coluna;
        // ou avança posição ou se reposiciona na pista (sem contar ultrapassagens)
        if(dados->ciclista->tempo_restante_para_andar == 0) {
            avanca_posicao(dados->ciclista, dados->velodromo, dados->comprimento_velodromo);
        }
        else {
            reposiciona_pista(dados->ciclista, dados->velodromo);
        }
        pthread_mutex_unlock (&mutex_velodromo);

        int coluna_nova = dados->ciclista->coluna;

        if(coluna_nova == coluna_antiga) {
            dados->ciclista->tempo_restante_para_andar = dados->ciclista->tempo_restante_para_andar - 60;
        }
        
        // se completou uma volta como ciclista ativo
        if(coluna_nova < coluna_antiga && strcmp(dados->ciclista->estado, "ativo") == 0) {
            dados->equipe->volta_atual++;
            dados->ciclista->voltas_realizadas++;
            atualiza_velocidade(dados->ciclista, dados->colega->tempo_por_volta, dados->equipe->revezando);

            if(dados->ciclista->voltas_realizadas >= 5) {
                dados->equipe->revezando = true;
            }
        }

        // se está revezando, atualiza a própria velocidade
        if(dados->equipe->revezando) {
            atualiza_velocidade(dados->ciclista,
                                dados->colega->tempo_por_volta,
                                dados->equipe->revezando);
        }

        // se está revezando, verifica se pode trocar estado com colega de equipe
        // o ciclista que está revezando 'informa' o colega que agora ele está ativo
        if(dados->equipe->revezando && strcmp(dados->ciclista->estado, "descansando")==0) {
            char *nome = dados->ciclista->nome;
            for(int i = 0; i < QTD_PISTAS; i++) {
                char *pos = dados->velodromo[i][dados->ciclista->coluna];
                if(pos != NULL && strncmp(pos, nome, strlen(nome)-1) == 0){
                    sprintf(dados->ciclista->estado, "ativo");
                    sprintf(dados->colega->estado, "descansando");
                    dados->equipe->revezando = false;
                }
            }
        }
        
        pthread_barrier_wait(&barreira_passo); 
        pthread_barrier_wait(&barreira_passo);
    }

    pthread_cond_wait(&sinal_destruicao, &mutex_velodromo);
    return NULL;
}

void arbitro(struct_corrida* corrida, bool debug) {
    posiciona_inicio_corrida(corrida, debug);

    pthread_mutex_init(&mutex_velodromo, NULL);
    pthread_barrier_init(&barreira_passo, NULL, corrida->qtd_equipes*2+1);
    pthread_cond_init(&sinal_destruicao, NULL);
    pthread_t t_ciclista[corrida->qtd_equipes*2];

    
    for(int i = 0; i < corrida->qtd_equipes*2; i++) {
        struct_ciclista_arg *arg_i = malloc(sizeof(struct_ciclista_arg));
        arg_i->ciclista = corrida->ciclistas[i];
        arg_i->equipe = corrida->equipes[i/2];
        arg_i->voltas = corrida->voltas;
        arg_i->comprimento_velodromo = corrida->comprimento_velodromo;
        arg_i->velodromo = corrida->velodromo;
        arg_i->colega = (i % 2 == 0) ? corrida->ciclistas[i+1] : corrida->ciclistas[i-1];
        pthread_create(&t_ciclista[i], NULL, ciclista, arg_i);
    }

    sleep(1); // dorme para garantir que todas as threads estão esperando sinal do árbitro
    while(acabou_corrida(corrida->equipes, corrida->qtd_equipes) == false) {
        pthread_barrier_wait(&barreira_passo);

        if(debug) {
            printa_velodromo(corrida->velodromo, corrida->comprimento_velodromo, true);
        }

        pthread_barrier_wait(&barreira_passo);
        sleep(QUANTUM/50);
    }

    for(int i = 0; i < corrida->qtd_equipes * 2; i++) {
        pthread_join(t_ciclista[i], NULL);
    }
    
    pthread_mutex_destroy(&mutex_velodromo);
    pthread_barrier_destroy(&barreira_passo);
    pthread_cond_destroy(&sinal_destruicao);
    
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

    // é necessário dar_free nos dados ao final da corrida, ciclistas, ciclistas_args, equipes e corrida

    //   a cada 60ms imprime na stderr o velódromo com a posição de cada ciclista
    //   não imprime o relatório ao final de cada volta, apenas ao final da corrida
    //   fprintf(stderr, );

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
        equipe_i->terminou = false;
        equipe_i->revezando = false;
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
