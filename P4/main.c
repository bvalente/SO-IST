/*
// Projeto SO - exercicio 3
// Sistemas Operativos, DEI/IST/ULisboa 2017-18
*/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <math.h>

//bibliotecas para forks e waits
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include  <signal.h>
#include <string.h>
#include <errno.h>
#include "matrix2d.h"
#include "util.h"

/*--------------------------------------------------------------------
| Type: thread_info
| Description: Estrutura com Informacao para Trabalhadoras
---------------------------------------------------------------------*/

typedef struct {
    int    id;
    int    N;
    int    iter;
    int    trab;
    int    tam_fatia;
    double maxD;
} thread_info;

/*--------------------------------------------------------------------
| Type: doubleBarrierWithMax
| Description: Barreira dupla com variavel de max-reduction
---------------------------------------------------------------------*/

typedef struct {
    int             total_nodes;
    int             pending[2];
    double          maxdelta[2];
    int             iteracoes_concluidas;
    pthread_mutex_t mutex;
    pthread_cond_t  wait[2];
} DualBarrierWithMax;

/*--------------------------------------------------------------------
| Global variables
---------------------------------------------------------------------*/

DoubleMatrix2D     *matrix_copies[2];
DualBarrierWithMax *dual_barrier;
double              maxD;
int                 interrupt;
int                 alarmeFlag;
pid_t               pidFilho;
int                 tempo;
char*               backupFich;
struct sigaction sig;
sigset_t allSig;

/*--------------------------------------------------------------------
 | Function: backupMatrix
 | Description: guarda matriz num novo ficheiro
 ---------------------------------------------------------------------*/

void backupMatrix(char *fichS){
    pidFilho = fork();

    if (pidFilho < 0){
        die(" falha na criação de um processo filho");

    } else if  (pidFilho == 0){

        char *str;
        str =  (char*) malloc( (strlen(fichS) + 2) *sizeof(char) );
        strcpy(str,"~");
        strcat(str,fichS);

        FILE *file = freopen(str, "w+", stdout);

        if (file == NULL)
            die("unable to open the file to write");
        dm2dPrint(matrix_copies[0]);

        if (fclose(file)!=0){
            die("unable to close the file");
        }

        if (rename(str, fichS) == -1){
            die("renaming file");
        }

        free(str);
        exit(0);
    }

}
/*--------------------------------------------------------------------
| Function: dualBarrierInit
| Description: Inicializa uma barreira dupla
---------------------------------------------------------------------*/

DualBarrierWithMax *dualBarrierInit(int ntasks, int periodoS) {
    DualBarrierWithMax *b;
    b = (DualBarrierWithMax*) malloc (sizeof(DualBarrierWithMax));
    if (b == NULL) return NULL;

    b->total_nodes = ntasks;
    b->pending[0]  = ntasks;
    b->pending[1]  = ntasks;
    b->maxdelta[0] = 0;
    b->maxdelta[1] = 0;
    b->iteracoes_concluidas = 0;

    if (pthread_mutex_init(&(b->mutex), NULL) != 0) {
        fprintf(stderr, "\nErro a inicializar mutex\n");
        exit(1);
    }
    if (pthread_cond_init(&(b->wait[0]), NULL) != 0) {
        fprintf(stderr, "\nErro a inicializar variável de condição\n");
        exit(1);
    }
    if (pthread_cond_init(&(b->wait[1]), NULL) != 0) {
        fprintf(stderr, "\nErro a inicializar variável de condição\n");
        exit(1);
    }
    return b;
}

/*--------------------------------------------------------------------
| Function: dualBarrierFree
| Description: Liberta os recursos de uma barreira dupla
---------------------------------------------------------------------*/

void dualBarrierFree(DualBarrierWithMax* b) {
    if (pthread_mutex_destroy(&(b->mutex)) != 0) {
        fprintf(stderr, "\nErro a destruir mutex\n");
        exit(1);
    }
    if (pthread_cond_destroy(&(b->wait[0])) != 0) {
        fprintf(stderr, "\nErro a destruir variável de condição\n");
        exit(1);
    }
    if (pthread_cond_destroy(&(b->wait[1])) != 0) {
        fprintf(stderr, "\nErro a destruir variável de condição\n");
        exit(1);
    }
    free(b);
}

/*--------------------------------------------------------------------
| Function: dualBarrierWait
| Description: Ao chamar esta funcao, a tarefa fica bloqueada ate que
|              o numero 'ntasks' de tarefas necessario tenham chamado
|              esta funcao, especificado ao ininializar a barreira em
|              dualBarrierInit(ntasks). Esta funcao tambem calcula o
|              delta maximo entre todas as threads e devolve o
|              resultado no valor de retorno
---------------------------------------------------------------------*/

double dualBarrierWait (DualBarrierWithMax* b, int current, double localmax) {
    int next = 1 - current;
    if (pthread_mutex_lock(&(b->mutex)) != 0) {
        fprintf(stderr, "\nErro a bloquear mutex\n");
        exit(1);
    }

    // decrementar contador de tarefas restantes
    b->pending[current]--;
    // actualizar valor maxDelta entre todas as threads
    if (b->maxdelta[current]<localmax)
        b->maxdelta[current]=localmax;
    // verificar se sou a ultima tarefa
    if (b->pending[current]==0) {
        // sim -- inicializar proxima barreira e libertar threads
        b->iteracoes_concluidas++;
        b->pending[next]  = b->total_nodes;
        b->maxdelta[next] = 0;

        //verificar backup
        if (interrupt){

            b->maxdelta[current] = -1;  //interrupts every thread via maxD

            pidFilho = wait(NULL);

            if ( pidFilho >= 0 || (pidFilho == -1 && errno ==  ECHILD)){
                //não há filhos OU filho terminou
                printf("O programa foi interrompido\n" );
                backupMatrix( backupFich );
                printf("Salvaguarda da matriz completa\n" );

            }

        }else if (alarmeFlag){

            alarmeFlag = 0;
            pid_t filho = 0;

            if ( pidFilho == -1 ) { //não há filhos
                backupMatrix( backupFich );
            } else if ( pidFilho >= 0){
                filho = waitpid(pidFilho, NULL, WNOHANG);

                if (filho){
                    backupMatrix( backupFich );

                }
            }
        }

        if (pthread_cond_broadcast(&(b->wait[current])) != 0) {
            fprintf(stderr, "\nErro a assinalar todos em variável de condição\n");
            exit(1);
        }
    }
    else {
        // nao -- esperar pelas outras tarefas
        while (b->pending[current]>0) {
            if (pthread_cond_wait(&(b->wait[current]), &(b->mutex)) != 0) {
                fprintf(stderr, "\nErro a esperar em variável de condição\n");
                exit(1);
            }
        }
    }
    double maxdelta = b->maxdelta[current];

    if (pthread_mutex_unlock(&(b->mutex)) != 0) {
        fprintf(stderr, "\nErro a desbloquear mutex\n");
        exit(1);
    }

    return maxdelta;
}

/*--------------------------------------------------------------------
| Function: tarefa_trabalhadora
| Description: Funcao executada por cada tarefa trabalhadora.
|              Recebe como argumento uma estrutura do tipo thread_info
---------------------------------------------------------------------*/

void *tarefa_trabalhadora(void *args) {
    thread_info *tinfo = (thread_info *) args;
    int tam_fatia = tinfo->tam_fatia;
    int my_base = tinfo->id * tam_fatia;
    double global_delta = INFINITY;
    int iter = 0;

    do {
        int atual = iter % 2;
        int prox = 1 - iter % 2;
        double max_delta = 0;
        // Calcular Pontos Internos
        for (int i = my_base; i < my_base + tinfo->tam_fatia; i++) {
            for (int j = 0; j < tinfo->N; j++) {
                double val = (dm2dGetEntry(matrix_copies[atual], i,   j+1) +
                              dm2dGetEntry(matrix_copies[atual], i+2, j+1) +
                              dm2dGetEntry(matrix_copies[atual], i+1, j) +
                              dm2dGetEntry(matrix_copies[atual], i+1, j+2))/4;
                // calcular delta
                double delta = fabs(val - dm2dGetEntry(matrix_copies[atual], i+1, j+1));
                if (delta > max_delta) {
                    max_delta = delta;
                }
                dm2dSetEntry(matrix_copies[prox], i+1, j+1, val);
            }
        }

        // barreira de sincronizacao; calcular delta global

        global_delta = dualBarrierWait(dual_barrier, atual, max_delta);
    } while (++iter < tinfo->iter && global_delta >= tinfo->maxD);

    return 0;
}


/*--------------------------------------------------------------------
 | Function: signalHandler
 | Description: Comando para SIGINT (Ctrl+C)
 ---------------------------------------------------------------------*/

void signalHandler(int sig){

    if (pthread_mutex_lock(&(dual_barrier->mutex)) != 0)
        die("a bloquear mutex.\n");

    if (sig == SIGALRM){
        alarmeFlag = 1;
        alarm(tempo);

    }else{//SIGINT
        interrupt = 1;
    }
    if (pthread_mutex_unlock(&(dual_barrier->mutex)) != 0)
        die("a desbloquear mutex.\n");
}

/*--------------------------------------------------------------------
| Function: main
| Description: Entrada do programa
---------------------------------------------------------------------*/

int main (int argc, char** argv) {
    int N;
    double tEsq, tSup, tDir, tInf;
    int iter, trab;
    int tam_fatia;
    int res;
    int periodoS;
    char *fichS;
    interrupt = 0;
    alarmeFlag = 0;
    pidFilho = -1;

    if (argc != 11) {
        fprintf(stderr, "Utilizacao: ./heatSim N tEsq tSup tDir tInf iter trab maxD fichS periodoS\n\n");
        die("Numero de argumentos invalido");
    }

    // Ler Input
    N    = parse_integer_or_exit(argv[1], "N",    1);
    tEsq = parse_double_or_exit (argv[2], "tEsq", 0);
    tSup = parse_double_or_exit (argv[3], "tSup", 0);
    tDir = parse_double_or_exit (argv[4], "tDir", 0);
    tInf = parse_double_or_exit (argv[5], "tInf", 0);
    iter = parse_integer_or_exit(argv[6], "iter", 1);
    trab = parse_integer_or_exit(argv[7], "trab", 1);
    maxD = parse_double_or_exit (argv[8], "maxD", 0);
    fichS = argv[9];
    periodoS = parse_integer_or_exit(argv[10], "periodoS", 0);


    fprintf(stderr, "\nArgumentos:\n"
    " N=%d tEsq=%.1f tSup=%.1f tDir=%.1f tInf=%.1f iter=%d trab=%d maxD=%.2f fichS=%s periodoS=%d\n",
    N, tEsq, tSup, tDir, tInf, iter, trab, maxD, fichS, periodoS);

    if (N % trab != 0) {
        fprintf(stderr, "\nErro: Argumento %s e %s invalidos.\n"
                "%s deve ser multiplo de %s.", "N", "trab", "N", "trab");
        return -1;
    }

    tempo = periodoS;
    backupFich = fichS; //copia ponteiros
    // Inicializar Barreira
    dual_barrier = dualBarrierInit(trab, periodoS);
    if (dual_barrier == NULL)
        die("Nao foi possivel inicializar barreira");

    // Calcular tamanho de cada fatia
    tam_fatia = N / trab;

    // Criar e Inicializar Matrizes
    matrix_copies[0] = dm2dNew(N+2,N+2);
    matrix_copies[1] = dm2dNew(N+2,N+2);
    if (matrix_copies[0] == NULL || matrix_copies[1] == NULL) {
        die("Erro ao criar matrizes");
    }

    if( access( fichS, F_OK ) != -1 ) {
        //ficheiro exite, ler do ficheiro
        printf("\nFicheiro já existe.\nLer Matriz do ficheiro.\n");
        FILE *file = fopen(fichS,"r");
        if (file == NULL)
            die("unable to open the file to read.");

        matrix_copies[0] = readMatrix2dFromFile(file, N+2, N+2);

        if (fclose(file)!=0){
            die("unable to close the file");
        }

    } else {
        // file doesn't exist
        dm2dSetLineTo (matrix_copies[0], 0, tSup);
        dm2dSetLineTo (matrix_copies[0], N+1, tInf);
        dm2dSetColumnTo (matrix_copies[0], 0, tEsq);
        dm2dSetColumnTo (matrix_copies[0], N+1, tDir);

    }
    dm2dCopy (matrix_copies[1],matrix_copies[0]);


    // Reservar memoria para trabalhadoras
    thread_info *tinfo = (thread_info*) malloc(trab * sizeof(thread_info));
    pthread_t *trabalhadoras = (pthread_t*) malloc(trab * sizeof(pthread_t));

    if (tinfo == NULL || trabalhadoras == NULL) {
        die("Erro ao alocar memoria para trabalhadoras");
    }
    /*
        - declarar variavel sigaction const struct
    e sigset_t
        - sigemptyset
        - sigaddset(x2)
        - pthread_sigmask BLOCK
        - for dos threads
        - pthread_sigmask UNBLOCK
        -sigemptyset sa_mask
        -definir sa_handler
        -sigaction(x2)

    */


    if (sigemptyset(&allSig) == -1)
        die("em sigemptyset");

    if ( sigaddset(&allSig, SIGALRM) == -1 )
        die(" em sigaddset de SIGALRM ");
    if ( sigaddset(&allSig, SIGINT) == -1 )
        die(" em sigaddset de SIGINT ");

    sig.sa_mask = allSig;
    sig.sa_handler = signalHandler;

    if ( pthread_sigmask(SIG_BLOCK, &allSig, NULL) != 0 )
        die(" em  pthread_sigmask Block ");

    // Criar trabalhadoras
    for (int i=0; i < trab; i++) {
        tinfo[i].id = i;
        tinfo[i].N = N;
        tinfo[i].iter = iter;
        tinfo[i].trab = trab;
        tinfo[i].tam_fatia = tam_fatia;
        tinfo[i].maxD = maxD;
        res = pthread_create(&trabalhadoras[i], NULL, tarefa_trabalhadora, &tinfo[i]);
        if (res != 0) {
            die("Erro ao criar uma tarefa trabalhadora");
        }
    }

    if ( pthread_sigmask(SIG_UNBLOCK, &allSig, NULL) != 0 )
        die(" em  pthread_sigmask Unblock ");

    if ( sigemptyset(&allSig) == -1 )
        die(" em sigemptyset ");

    sigaction( SIGINT, &sig, NULL);

    if (periodoS > 0){
        printf("periodoS is %d\n", tempo );
        sigaction(SIGALRM, &sig, NULL);
        alarm(tempo);
        printf("alarme ativo\n" );
    }


    // Esperar que as trabalhadoras terminem
    for (int i=0; i<trab; i++) {
        res = pthread_join(trabalhadoras[i], NULL);
        if (res != 0)
            die("Erro ao esperar por uma tarefa trabalhadora");
    }

    if(pidFilho >= 0)
        wait(NULL);

    if(interrupt == 0){
        dm2dPrint (matrix_copies[dual_barrier->iteracoes_concluidas%2]);

        if( access( fichS, F_OK ) != -1 ) {
            if (remove(fichS))
                die("eliminar ficheiro");
        }

    }
    dm2dFree(matrix_copies[0]);
    dm2dFree(matrix_copies[1]);
    free(tinfo);
    free(trabalhadoras);
    dualBarrierFree(dual_barrier);

    return 0;
}
