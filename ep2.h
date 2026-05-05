// esse arquivo não expõe uma API completa do ep2.c
// no entanto, ele expõe funcionalidades que poderiam ser expandidas e reaproveitadas por outros simuladores
#ifndef EP2_H
#define EP2_H

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
pthread_mutex_t mutex_colunas[MAX_LEN_VELODROMO];
pthread_barrier_t barreira_passo;
bool exec_eficiente;

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

// funções auxiliares -------------------------------------------

void printa_ciclistas(struct_ciclista *ciclistas[], int arr_len);

void embaralha_ciclistas(struct_ciclista *c_ativos[],
                         int arr_len);

void printa_velodromo(char *velodromo[10][MAX_LEN_VELODROMO],
                      int comprimento_velodromo,
                      bool error_out);

void printa_equipes(struct_equipe *equipes[], int qtd_equipes);

void printa_relatorio_final(struct_equipe *equipes[], int qtd_equipes);

bool check_debug_flag(char *debug_arg, int argc);

void gera_equipes(struct_equipe *equipes[MAX_EQUIPES], int qtd_equipes);

void gera_ciclistas(struct_ciclista *ciclistas[MAX_EQUIPES*2], int qtd_equipes);

// funções do árbitro -------------------------------------------

void get_ciclistas_por_estado(struct_ciclista *ciclistas[MAX_EQUIPES*2],
                          struct_ciclista *c_ativos[MAX_EQUIPES],
                          int qtd_equipes,
                          char* estado);

void posiciona_ciclistas(struct_ciclista *ciclistas[MAX_EQUIPES*2],
                        char *velodromo[10][MAX_LEN_VELODROMO],
                        int qtd_equipes,
                        int comprimento_pista);

void posiciona_inicio_corrida(struct_corrida* corrida, bool debug);

bool acabou_corrida(struct_equipe *equipes[], int qtd_equipes);

int count_equipes_finalizaram(struct_equipe *equipes[], int qtd_equipes);

void get_voltas_equipes(struct_equipe *equipes[], int qtd_equipes, int *voltas);

void atualiza_timers_equipes(struct_equipe *equipes[], int qtd_equipes);

int get_volta_atual(int voltas[], int qtd_equipes);

int get_volta_avancada(int voltas[], int qtd_equipes);

void atribui_pontos(struct_equipe *equipes[],
                    int qtd_equipes,
                    int volta_avancada);

void ranqueia_equipes(struct_equipe *equipes[], int qtd_equipes, int indices_rank[]);

void remove_equipes_finalizaram(struct_corrida* corrida);

void arbitro(struct_corrida* corrida, bool debug);

// funções dos ciclistas ----------------------------------------

bool pode_ultrapassar(int pista,
                      int coluna,
                      int comprimento_pista,
                      char *velodromo[10][MAX_LEN_VELODROMO]);

void avanca_posicao(struct_ciclista *ciclista,
                    char *velodromo[10][MAX_LEN_VELODROMO],
                    int comprimento_pista);

void reposiciona_pista(struct_ciclista *ciclista,
                      char *velodromo[10][MAX_LEN_VELODROMO]);

void atualiza_velocidade(struct_ciclista *ciclista,
                         int velocidade_colega,
                         bool revezando);

void* ciclista(void* arg);

#endif