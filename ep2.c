// simula uma corrida Madison em um velódromo
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <stdbool.h>
#include <pthread.h>
#include <string.h>

// cada corrida possui n voltas (10 ≤ n ≤ 1250)
// o velódromo tem d metros (100 ≤ d ≤ 2500)
// k equipes começam a prova (5 ≤ k ≤ ⌊d/2⌋−1)

// estado global do velódromo (representado como matriz)
// velódromo é um vetor circular de tamanho d

// ciclitas iniciam a prova ao mesmo tempo no mesmo lado do velódromo
// a cada momento apenas um ciclista de cada equipe está participando da prova
// os ciclistas restantes de cada equipe se recuperam na pista mais externa do velódromo
// o revezamento acontece quando um ciclista ultrapassa outro da mesma equipe, ação chamada de 'estilingue'
// a equipe vencedora é a que terminou mais voltas em primeiro
// a cada momento, apenas 10 ciclistas podem estar lado a lado em cada ponto do velódromo
// a pista mais externa é usada por ciclistas que estão se recuperando
// cada ciclista ocupa 1 metro de comprimento do velódromo

// criar 2*k threads ciclista iguais
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

// começo da corrida
// primeira volta todos andam 1m a cada 120ms
// um ciclista de cada equipe larga em fila com ordenação aleatória
// largam antes da linha de chegada, com 5 ciclistas no máximo lado a lado nas pistas internas
// temos várias filas com 5 e uma fila final com até 5 ciclistas
// os ciclistas que não largaram ficam na pista mais externa a 15km/h (1m a cada 240ms)

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

// versão ingênua, um semáforo para controlar acesso à matriz velódromo
// versão eficiente, um semáforo para cada posição ou coluna da matriz

// quando uma equipe completa a prova, as suas duas threads ciclistas devem ser destruídas
// caso duas ciclistas completem uma volta ao mesmo tempo, as duas equipes recebem ponto
// empates ao final da prova devem ser resolvidos aleatoriamente

// main
// flag debug
//   a cada 60ms imprime na stderr o velódromo com a posição de cada ciclista
//   não imprime o relatório ao final de cada volta, apenas ao final da corrida

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

int main(int argc, char **argv) {
    if (argc < 5 || argc > 6) {
        fprintf(stderr, "Uso: %s <n> <d> <k> <i|e> --debug\n", argv[0]);
        return 1;
    }

    bool debug = check_debug_flag(argv[5], argc);
    
    // lê n, d, k, <i|e>, 'i' para ineficiente 'e' para eficiente
    int n = atoi(argv[1]);
    int d = atoi(argv[2]);
    int k = atoi(argv[3]);
    char *exec_mode = argv[4];
    printf("%d, %d, %d, %s\n", n, d, k, exec_mode);

    if(debug) {
        printf("rodando em modo debug\n");
    }
    return 0;
}
