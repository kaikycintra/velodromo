// simula uma corrida Madison em um velódromo
#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <stdbool.h>
#include <pthread.h>
#include <string.h>
#include <signal.h>

#define QTD_PISTAS 10
#define MIN_VOLTAS 10
#define MAX_VOLTAS 1250

#define MIN_LEN_VELODROMO 100
#define MAX_LEN_VELODROMO 2500

#define MIN_EQUIPES 5
#define MAX_EQUIPES 1249 // ⌊MAX_LEN_VELODROMO/2⌋−1

#define QUANTUM 60 // intervalo de tempo em ms que define um passo da simulação

pthread_mutex_t mutex_velodromo; // usado para implementação ineficiente
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
    bool cruzou_largada;
} struct_ciclista;

typedef struct {
    int pontos;
    int volta_atual;
    float timer; // tempo acumulado em corrida em segundos
    bool terminou;
    bool revezando;
} struct_equipe;
    
typedef struct {
    int voltas;
    int qtd_equipes;
    int comprimento_velodromo; // comprimento de cada pista
    int volta_atual; // volta atual da equipe em último lugar, esclarecido por email
    int volta_avancada; // volta atual do primeiro lugar da corrida
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
        for(int j = colunas - 1; j >= 0; j--) {
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
    for(int i = 0; i < qtd_equipes; i++) {
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
        if(dados->ciclista->tempo_restante_para_andar <= 0) {
            avanca_posicao(dados->ciclista, dados->velodromo, dados->comprimento_velodromo);
        }
        else {
            reposiciona_pista(dados->ciclista, dados->velodromo);
        }

        int coluna_nova = dados->ciclista->coluna;

        if(coluna_nova == coluna_antiga) {
            dados->ciclista->tempo_restante_para_andar = dados->ciclista->tempo_restante_para_andar - 60;
        }
        
        // se completou uma volta como ciclista ativo
        if(coluna_nova < coluna_antiga && strcmp(dados->ciclista->estado, "ativo") == 0) {
            if(!dados->ciclista->cruzou_largada) {
                dados->ciclista->cruzou_largada = true;
                dados->colega->cruzou_largada = true;
            }
            else{
                dados->equipe->volta_atual++;
                dados->ciclista->voltas_realizadas++;
            }
                
            atualiza_velocidade(dados->ciclista, dados->colega->tempo_por_volta, dados->equipe->revezando);

            if(dados->ciclista->voltas_realizadas >= 5) {
                dados->equipe->revezando = true;
            }
            
            if(dados->equipe->volta_atual == dados->voltas) {
                dados->equipe->terminou = true;
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
        
        pthread_mutex_unlock (&mutex_velodromo);      
        pthread_barrier_wait(&barreira_passo);
        pthread_barrier_wait(&barreira_passo); 
    }

    while(true) {
        pthread_barrier_wait(&barreira_passo);
        pthread_barrier_wait(&barreira_passo); 
    }

    return NULL;
}

int count_equipes_finalizaram(struct_equipe *equipes[], int qtd_equipes) {
    int count = 0;
    for(int i = 0; i < qtd_equipes; i++) {
        if(equipes[i]->terminou) {
            count++;
        }
    }

    return count;
}

void atualiza_timers_equipes(struct_equipe *equipes[], int qtd_equipes) {
    for(int i = 0; i < qtd_equipes; i++) {
        if(!equipes[i]->terminou) {
            equipes[i]->timer = equipes[i]->timer + QUANTUM/1000.0;
        }
    }
}

void printa_equipes(struct_equipe *equipes[], int qtd_equipes) {
    for(int i = 0; i < qtd_equipes; i++) {
        printf("equipe: %d, pontos: %d, timer: %f\n",
             i, equipes[i]->pontos, equipes[i]->timer);
    }
}

void get_voltas_equipes(struct_equipe *equipes[], int qtd_equipes, int *voltas) {
    for(int i = 0; i < qtd_equipes; i++) {
        voltas[i] = equipes[i]->volta_atual;
    }
}

int get_volta_atual(int voltas[], int qtd_equipes) {
    // volta é concluída quando o último ciclista a encerra (esclarecido por email)
    // então achamos o mínimo do array de voltas das equipes
    int min = voltas[0];

    for (int i = 1; i < qtd_equipes; i++) {
        if (voltas[i] < min) {
            min = voltas[i];
        }
    }

    return min;
}

int get_volta_avancada(int voltas[], int qtd_equipes) {
    // volta avançada é a volta atual do primeiro lugar
    // usamos esse dado para atribuir pontos aos primeiros que avançam uma volta
    int max = voltas[0];

    for (int i = 1; i < qtd_equipes; i++) {
        if (voltas[i] > max) {
            max = voltas[i];
        }
    }

    return max;
}
void atribui_pontos(struct_equipe *equipes[],
                    int qtd_equipes,
                    int volta_avancada,
                    int voltas[],
                    int novas_voltas[]) {
    // todos que avançaram a volta juntos recebem pontos
    for(int i = 0; i < qtd_equipes; i++) {
        if(novas_voltas[i] > voltas[i]) {
            equipes[i]->pontos++;
        }
    }
}

void ranqueia_equipes(struct_equipe *equipes[], int qtd_equipes, int indices_rank[]) {
    // bubble sort para ranquear
    for (int i = 0; i < qtd_equipes; i++) {
        indices_rank[i] = i;
    }

    for (int i = 0; i < qtd_equipes - 1; i++) {
        for (int j = 0; j < qtd_equipes - i - 1; j++) {
            int idx_atual = indices_rank[j];
            int idx_proximo = indices_rank[j + 1];

            bool trocar = false;

            if (equipes[idx_atual]->pontos < equipes[idx_proximo]->pontos) {
                trocar = true;
            } 

            else if (equipes[idx_atual]->pontos == equipes[idx_proximo]->pontos) {
                if (rand() % 2 == 0) {
                    trocar = true;
                }
            }

            if (trocar) {
                int temp = indices_rank[j];
                indices_rank[j] = indices_rank[j + 1];
                indices_rank[j + 1] = temp;
            }
        }
    }
}

void printa_relatorio_final(struct_equipe *equipes[], int qtd_equipes) {
    // posição da equipe, instante de tempo em que finalizaram a corrida, qtd de voltas vencidas
    int ranking[qtd_equipes];
    ranqueia_equipes(equipes, qtd_equipes, ranking);

    for (int i = 0; i < qtd_equipes; i++) {
        int id_equipe = ranking[i];
        printf("%d Lugar: Equipe %d | Pontos: %d | Tempo: %fs\n", 
                i + 1, id_equipe, equipes[id_equipe]->pontos, equipes[id_equipe]->timer);
    }
}

void remove_equipes_finalizaram(struct_corrida* corrida) {
    for(int i = 0; i < corrida->qtd_equipes; i++) {
            if(corrida->equipes[i]->terminou) {
                struct_ciclista *c1 = corrida->ciclistas[i*2];
                struct_ciclista *c2 = corrida->ciclistas[i*2 + 1];
                corrida->velodromo[c1->pista][c1->coluna] = NULL;
                corrida->velodromo[c2->pista][c2->coluna] = NULL;
            }
        }
}
        
    
void arbitro(struct_corrida* corrida, bool debug) {
    posiciona_inicio_corrida(corrida, debug);

    pthread_mutex_init(&mutex_velodromo, NULL);
    pthread_barrier_init(&barreira_passo, NULL, corrida->qtd_equipes*2+1);
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

    while(!acabou_corrida(corrida->equipes, corrida->qtd_equipes)) {
        int volta_antiga = corrida->volta_atual;
        int volta_avancada_antiga = corrida->volta_avancada;
        int voltas[corrida->qtd_equipes], novas_voltas[corrida->qtd_equipes];
        get_voltas_equipes(corrida->equipes, corrida->qtd_equipes, voltas);

        pthread_barrier_wait(&barreira_passo);
        
        get_voltas_equipes(corrida->equipes, corrida->qtd_equipes, novas_voltas);
        corrida->volta_atual = get_volta_atual(novas_voltas, corrida->qtd_equipes);
        corrida->volta_avancada = get_volta_avancada(novas_voltas, corrida->qtd_equipes);
        
        
        if(corrida->volta_avancada > volta_avancada_antiga) {
            atribui_pontos(corrida->equipes, corrida->qtd_equipes, corrida->volta_avancada, voltas, novas_voltas);
        }
        
        if(corrida->volta_atual > volta_antiga && !debug) {
            printf("fim da volta: %d\n", volta_antiga+1);
            printa_velodromo(corrida->velodromo, corrida->comprimento_velodromo, false);
        }

        if(debug) {
            fprintf(stderr, "------------------------------\n");
            printa_velodromo(corrida->velodromo, corrida->comprimento_velodromo, true);
            //printa_ciclistas(corrida->ciclistas, corrida->qtd_equipes*2);
        }

        atualiza_timers_equipes(corrida->equipes, corrida->qtd_equipes);
        remove_equipes_finalizaram(corrida);

        pthread_barrier_wait(&barreira_passo);
        if(debug) usleep(QUANTUM*1000);
    }

    printa_relatorio_final(corrida->equipes, corrida->qtd_equipes);

    for(int i = 0; i < corrida->qtd_equipes*2; i++) {
        pthread_kill(t_ciclista[i], 9);
    }
    
    pthread_mutex_destroy(&mutex_velodromo);
    pthread_barrier_destroy(&barreira_passo);
    
    // é necessário dar_free nos dados ao final da corrida, ciclistas, ciclistas_args, equipes e corrida
    
    // versão ingênua, um semáforo para controlar acesso à matriz velódromo
    // versão eficiente, um semáforo para cada posição ou coluna da matriz

}

bool check_debug_flag(char *debug_arg, int argc) {
    if (argc != 6) {
        return false;
    }

    char *debug = debug_arg;
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
        ciclistas[i]->cruzou_largada = false;
        
        ciclistas[i+1]->equipe = i/2;
        sprintf(ciclistas[i+1]->nome, "%db", i/2);
        sprintf(ciclistas[i+1]->estado, "descansando");
        ciclistas[i+1]->tempo_por_volta = 240;
        ciclistas[i+1]->tempo_restante_para_andar = 240;
        ciclistas[i+1]->voltas_realizadas = 0;
        ciclistas[i+1]->cruzou_largada = false;
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
    corrida->volta_avancada = 0;
    
    gera_ciclistas(corrida->ciclistas, corrida->qtd_equipes);
    gera_equipes(corrida->equipes, corrida->qtd_equipes);

    arbitro(corrida, debug); // passa a corrida para que o árbitro a gerencie

    return 0;
}
