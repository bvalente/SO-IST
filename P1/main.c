
/*
// Projeto SO - exercicio 1, version 03
// Sistemas Operativos, DEI/IST/ULisboa 2017-18

// Realizado por Bernardo Valente e Francisco Machado
*/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "matrix2d.h"
#include "mplib3.h"
#include "leQueue.h"  //desnecessario?
#include "pthread.h"

typedef struct Args{
    int ID;
    int numIteracoes;
    int numThreads;
    int N;
}EscravoArgs;

/*--------------------------------------------------------------------
| Function: simulFatia
---------------------------------------------------------------------*/

void *simulFatia(void* argumentos){
  EscravoArgs* args = (EscravoArgs*) argumentos;
  int ID = args->ID;
  int numIteracoes = args->numIteracoes;
  int numThreads = args->numThreads;
  int N = args->N;
  int linhas = N / numThreads;
//  int linhas = args->N / args->numThreads;
  int colunas = N;
  int i, l, k;
	DoubleMatrix2D *matrix, *matrix_aux, *ponteiro_aux;

	matrix = dm2dNew(linhas+2, colunas+2);
	matrix_aux = dm2dNew(linhas+2, colunas+2);

  for ( i = 0; i < N + 2; i++){
    receberMensagem(0,ID,(void*) dm2dGetLine(matrix, i) ,sizeof(double) * (N+2) );
    printf("%d %d\n",ID ,i );
  }


  dm2dCopy(matrix_aux,matrix);

	int value;
	for (i = 0; i < numIteracoes; i++){

        //Calculos dos valores
        for (l = 1; l < linhas - 1; l++){
            for (k = 1; k < colunas - 1; k++) {
                value = ( dm2dGetEntry(matrix, l-1, k) + dm2dGetEntry(matrix, l+1, k) + dm2dGetEntry(matrix, l, k-1) + dm2dGetEntry(matrix, l, k+1) ) / 4.0;
                dm2dSetEntry(matrix_aux, l, k, value);
            }
        }
    //retroca das matrizes
    ponteiro_aux = matrix;
    matrix = matrix_aux;
    matrix_aux = ponteiro_aux;

    //enviamos as mensagens
		if ( ID != 1)
			enviarMensagem( ID, ID -1, (void*) dm2dGetLine(matrix, 1), sizeof(double)*(colunas+2));
		if ( ID != numThreads)
			enviarMensagem (ID, ID +1, (void*) dm2dGetLine(matrix, linhas), sizeof(double)*(colunas+2));

		if ( ID != 1)
			receberMensagem( ID -1, ID, &matrix[0] , sizeof(double)*(colunas+2));
		if (ID != numThreads)
			receberMensagem( ID +1, ID, &matrix[linhas+1], sizeof(double)*(colunas+2));
  }
  //enviar mensagem para a main
  receberMensagem( ID, 0, dm2dGetLine(matrix, 0), sizeof(double)*(colunas+2)*(linhas+2) );

  //free
  free(matrix);
  free(matrix_aux);

  return NULL;
}

/*--------------------------------------------------------------------
| Function: parse_integer_or_exit
---------------------------------------------------------------------*/

int parse_integer_or_exit(char const *str, char const *name)
{
  int value;

  if(sscanf(str, "%d", &value) != 1) {
    fprintf(stderr, "\nErro no argumento \"%s\".\n\n", name);
    exit(1);
  }
  return value;
}

/*--------------------------------------------------------------------
| Function: parse_double_or_exit
---------------------------------------------------------------------*/

double parse_double_or_exit(char const *str, char const *name)
{
  double value;

  if(sscanf(str, "%lf", &value) != 1) {
    fprintf(stderr, "\nErro no argumento \"%s\".\n\n", name);
    exit(1);
  }
  return value;
}


/*--------------------------------------------------------------------
| Function: main
---------------------------------------------------------------------*/

int main (int argc, char** argv) {

  if(argc != 9) {
    fprintf(stderr, "\nNumero invalido de argumentos.\n");
    fprintf(stderr, "Uso: heatSim N tEsq tSup tDir tInf iteracoes trab csz\n\n");
    return 1;
  }

  /* argv[0] = program name */
  int N = parse_integer_or_exit(argv[1], "N");
  double tEsq = parse_double_or_exit(argv[2], "tEsq");
  double tSup = parse_double_or_exit(argv[3], "tSup");
  double tDir = parse_double_or_exit(argv[4], "tDir");
  double tInf = parse_double_or_exit(argv[5], "tInf");
  int iteracoes = parse_integer_or_exit(argv[6], "iteracoes");
  int trab = parse_integer_or_exit(argv[7], "trab");
  int csz = parse_integer_or_exit(argv[8], "csz");


  DoubleMatrix2D *matrix;


  fprintf(stderr, "\nArgumentos:\n"
	" N=%d tEsq=%.1f tSup=%.1f tDir=%.1f tInf=%.1f iteracoes=%d trabalhos=%d csz=%d\n",
	N, tEsq, tSup, tDir, tInf, iteracoes, trab, csz);

  if(N < 1 || tEsq < 0 || tSup < 0 || tDir < 0 || tInf < 0 || iteracoes < 1 || trab < 1 || csz < 1) {
    fprintf(stderr, "\nErro: Argumentos invalidos.\n"
	" Lembrar que N >= 1, temperaturas >= 0, iteracoes >= 1, trab >= 1 e csz >= 1\n\n");
    return 1;
  }

  //verificar se trab e valido
  if ( N % trab != 0){
	  fprintf(stderr, "\nErro: N nao e divisivel por trab.\n\n");
	  return 1;
  }

  inicializarMPlib(csz,  trab);

  matrix = dm2dNew(N+2, N+2);

  if (matrix == NULL) {
    fprintf(stderr, "\nErro: Nao foi possivel alocar memoria para a matriz.\n\n");
    return -1;
  }

  int ID, i;

  for(i=0; i<N+2; i++)
    dm2dSetLineTo(matrix, i, 0);

  dm2dSetLineTo (matrix, 0, tSup);
  dm2dSetLineTo (matrix, N+1, tInf);
  dm2dSetColumnTo (matrix, 0, tEsq);
  dm2dSetColumnTo (matrix, N+1, tDir);

  pthread_t *tid = malloc(sizeof(pthread_t)*trab);
  EscravoArgs *argumentos = malloc(sizeof(EscravoArgs)*trab);

  for(i=1; i <= trab; i++){
    argumentos[i-1].ID = i;
    argumentos[i-1].numIteracoes = iteracoes;
    argumentos[i-1].numThreads = trab;
    argumentos[i-1].N = N;

    pthread_create(&tid[i-1], NULL, simulFatia, &argumentos[i-1]);
  }

  int tamFatia = N/trab , j;

  //enviar mensagens para as threads
  j = 0;
  for  ( ID = 1; ID <= trab; ID++ ){
    for ( i = j; i < j + tamFatia + 2; i++){
      enviarMensagem(0,ID,(void*) dm2dGetLine(matrix, i),sizeof(double)* (N+2));
    }
    printf("%d\n",j );
    j += tamFatia;
  }
  printf("hello\n" );
  //correr todas as simulacoes/threads
  //sincronizar as threads?? pthread_join

  //receber as mensagens finais de todas as threads e construir matriz
  j = 0;
  for ( ID = 1; ID <= trab ; ID++ ){

    receberMensagem(ID, 0, (void*) &matrix[j], sizeof(double)* ( N+2 )*( tamFatia + 2));
    j += tamFatia;
  }

  dm2dPrint(matrix);
  dm2dFree(matrix);
  libertarMPlib();

  return 0;
}
