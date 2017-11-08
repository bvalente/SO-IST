/*
 // Projeto SO - exercicio 1, versao 2
 // Sistemas Operativos, DEI/IST/ULisboa 2017-18
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "matrix2d.h"

#define IND (bar->i   % 2)
#define IND1 ((bar->i+1 )% 2)

/* Estrutura com Informacao para Escravos */
typedef struct Barrier{
    int i;
    int numThreads[2];
    int totalThreads;
    pthread_cond_t varCond;
    pthread_mutex_t key;
}barrier;

typedef struct {
    int id;
    int N;
    int iter;
    int trab;
    int tam_fatia;
    barrier bar;
} thread_info;

// variaveis globais
DoubleMatrix2D *matrix [2];

barrier *bar;
double maxD;

int Termina = 0;


void terminacaoDin(int N){
    double maxDaux = 0;
    int i,j;
    for (i = 1; i < N - 1; i++){
        for (j = 1; j < N - 1;j++){
            if (dm2dGetEntry(matrix[IND],i,j) - dm2dGetEntry(matrix[IND1],i,j) > maxDaux)
                maxDaux = dm2dGetEntry(matrix[IND],i,j) - dm2dGetEntry(matrix[IND1],i,j);
        }
    }
    if (maxDaux <= maxD){
        Termina = 1;
    }
    return;
}

int barrier_init(int totalThreads){
    int status;
    bar->i = 0;
    bar->numThreads[0] = 0;
    bar->numThreads[1] = 0;
    bar->totalThreads = totalThreads;
    //precisa de verificar erros ao iniciar os mutex e cond
    status = pthread_cond_init(&bar->varCond, NULL);
    if (status != 0)
        return 1;
    status = pthread_mutex_init(&bar->key, NULL);
    if (status != 0)
        return 1;

    return 0;
}

void barrier_wait(int N){

    pthread_mutex_lock(&bar->key);
    int i = bar->i;
    if ( ++bar->numThreads[i] == bar->totalThreads ){
        if ( i )
            bar->i = 0;
        else
            bar->i = 1;

        bar->numThreads[bar->i] = 0;      // reset estado partilhado
        //broadcast

        pthread_cond_broadcast(&bar->varCond);
        terminacaoDin(N);

    }
    else{
        while(bar->numThreads[i] != bar->totalThreads){
            pthread_cond_wait(&bar->varCond,&bar->key);
        }
    }
    pthread_mutex_unlock(&bar->key);
}

void barrier_destroy(){
    pthread_mutex_destroy(&bar->key);
    pthread_cond_destroy(&bar->varCond);
}

/*--------------------------------------------------------------------
 | Function: tarefa_escravo
 | Description: Função executada por cada tarefa escravo
 ---------------------------------------------------------------------*/
void *tarefa_escravo(void* args) {
    thread_info *tinfo = (thread_info *) args;
    int iter;
    int i, j;
    /* Ciclo Iterativo */
    for (iter = 0; iter < tinfo->iter; iter++) {
    // Calcular Pontos Internos
        for (i = tinfo->tam_fatia*(tinfo->id-1) ; i < tinfo->id*tinfo->tam_fatia; i++) {
            for (j = 0; j < tinfo->N; j++) {
                double val = (dm2dGetEntry(matrix[IND], i, j+1) +
                              dm2dGetEntry(matrix[IND], i+1, j) +
                              dm2dGetEntry(matrix[IND], i+2, j+1) +
                              dm2dGetEntry(matrix[IND], i+1, j+2))/4;
                dm2dSetEntry(matrix[IND1], i+1, j+1, val);
            }
        }
        barrier_wait(tinfo->N);
        if (Termina)
            return NULL;
    }

    return NULL;
}

/*--------------------------------------------------------------------
 | Function: parse_integer_or_exit
 | Description: Processa a string str, do parâmetro name, como um inteiro
 ---------------------------------------------------------------------*/

int parse_integer_or_exit(char const *str, char const *name) {
    int value;

    if (sscanf(str, "%d", &value) != 1) {
        fprintf(stderr, "\nErro no argumento \"%s\".\n\n", name);
        exit(1);
    }

    return value;
}

/*--------------------------------------------------------------------
 | Function: parse_double_or_exit
 | Description: Processa a string str, do parâmetro name, como um double
 ---------------------------------------------------------------------*/

double parse_double_or_exit(char const *str, char const *name) {
    double value;

    if (sscanf(str, "%lf", &value) != 1) {
        fprintf(stderr, "\nErro no argumento \"%s\".\n\n", name);
        exit(1);
    }

    return value;
}

/*--------------------------------------------------------------------
 | Function: main
 | Description: Entrada do programa
 ---------------------------------------------------------------------*/

int main (int argc, char** argv) {
    int N;
    double tEsq, tSup, tDir, tInf;
    int iter;
    int trab;
    int tam_fatia;
    int res;
    int i;
    thread_info *tinfo;
    pthread_t *escravos;

    if (argc < 9) {
        fprintf(stderr, "\nNúmero de Argumentos Inválido.\n");
        fprintf(stderr, "Utilização: heatSim_p2 N tEsq tSup tDir tInf iter trab maxD\n\n");
        return 1;
    }

    /* Ler Input */
    N = parse_integer_or_exit(argv[1], "n");
    tEsq = parse_double_or_exit(argv[2], "tEsq");
    tSup = parse_double_or_exit(argv[3], "tSup");
    tDir = parse_double_or_exit(argv[4], "tDir");
    tInf = parse_double_or_exit(argv[5], "tInf");
    iter = parse_integer_or_exit(argv[6], "iter");
    trab = parse_integer_or_exit(argv[7], "trab");
    maxD =  parse_double_or_exit(argv[8], "maxD");

    fprintf(stdout, "\nArgumentos:\n"
            " N=%d tEsq=%.1f tSup=%.1f tDir=%.1f tInf=%.1f iter=%d trab=%d maxD=%.1f",
            N, tEsq, tSup, tDir, tInf, iter, trab, maxD);

    /* Verificacoes de Input */
    if (N < 1 || tEsq < 0 || tSup < 0 || tDir < 0 || tInf < 0 || iter < 1) {
        fprintf(stderr, "\nErro: Argumentos invalidos.\n"
                " Lembrar que N >= 1, temperaturas >= 0 e iter >= 1\n\n");
        return 1;
    }

    if (N % trab != 0) {
        fprintf(stderr, "\nErro: Argumento %s e %s invalidos\n"
                "%s deve ser multiplo de %s.", "N", "trab", "N", "trab");
        return 1;
    }

    /* Inicializar barreira*/
    bar = (barrier*) malloc(sizeof(barrier));
    if (bar == NULL) {
        fprintf(stderr, "\nErro ao alocar memória para a barreira.\n");
        return 1;
    }
    res = barrier_init(trab);
    if (res != 0){
        fprintf(stderr, "\nErro ao iniciar a barreira.\n");
        return 1;
    }

    /* Calcular Tamanho de cada Fatia */
    tam_fatia = N/trab;

    /* Criar Matriz Inicial */
    matrix[0] = dm2dNew(N+2, N+2);
    matrix[1] = dm2dNew(N+2, N+2);

    if (matrix[0] == NULL || matrix[1] == NULL) {
        fprintf(stderr, "\nErro ao criar Matrix2d.\n");
        return 1;
    }

    dm2dSetLineTo(matrix[0], 0, tSup);
    dm2dSetLineTo(matrix[0], N+1, tInf);
    dm2dSetColumnTo(matrix[0], 0, tEsq);
    dm2dSetColumnTo(matrix[0], N+1, tDir);

    dm2dCopy(matrix[1], matrix[0]);

    /* Reservar Memoria para Escravos */
    tinfo = (thread_info *)malloc(trab * sizeof(thread_info));
    escravos = (pthread_t *)malloc(trab * sizeof(pthread_t));

    if (tinfo == NULL || escravos == NULL) {
        fprintf(stderr, "\nErro ao alocar memória para escravos.\n");
        return 1;
    }

    /* Criar Escravos */
    for (i = 0; i < trab; i++) {
        tinfo[i].id = i+1;
        tinfo[i].N = N;
        tinfo[i].iter = iter;
        tinfo[i].trab = trab;
        tinfo[i].tam_fatia = tam_fatia;
        res = pthread_create(&escravos[i], NULL, tarefa_escravo, &tinfo[i]);

        if(res != 0) {
            fprintf(stderr, "\nErro ao criar um escravo.\n");
            return 1;
        }
    }


    /* Esperar que os Escravos Terminem */
    for (i = 0; i < trab; i++) {
        res = pthread_join(escravos[i], NULL);

        if (res != 0) {
            fprintf(stderr, "\nErro ao esperar por um escravo.\n");
            return 1;
        }
    }

    /* Imprimir resultado */
    dm2dPrint(matrix[IND]);

    /* Libertar Memoria */
    dm2dFree(matrix[0]);
    dm2dFree(matrix[1]);
    barrier_destroy();
    free(bar);
    free(tinfo);
    free(escravos);

    return 0;
}
