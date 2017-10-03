/*
// Projeto SO - exercicio 1, version 03
// Sistemas Operativos, DEI/IST/ULisboa 2017-18
*/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "matrix2d.h"
#include "mplib3.h"
#include "leQueue.h"
#include "pthread.h"


/*--------------------------------------------------------------------
 | Function: Escravos 
 * Estrutura arugmentos para escravos
 ---------------------------------------------------------------------*/
typedef struct Arg{
    int ID;
    int numIteracoes;
    int numTrabalhadoras;
    int N;
}EscravoArgs;








/*--------------------------------------------------------------------
| Function: simul
---------------------------------------------------------------------*/

DoubleMatrix2D *simul(DoubleMatrix2D *matrix, DoubleMatrix2D *matrix_aux, int linhas, int colunas, int numIteracoes) {

  DoubleMatrix2D *m, *aux, *tmp;
  int iter, i, j;
  double value;


  if(linhas < 2 || colunas < 2)
    return NULL;

  m = matrix;
  aux = matrix_aux;

  for (iter = 0; iter < numIteracoes; iter++) {
  
    for (i = 1; i < linhas - 1; i++)
      for (j = 1; j < colunas - 1; j++) {
        value = ( dm2dGetEntry(m, i-1, j) + dm2dGetEntry(m, i+1, j) +
		dm2dGetEntry(m, i, j-1) + dm2dGetEntry(m, i, j+1) ) / 4.0;
        dm2dSetEntry(aux, i, j, value);
      }

    tmp = aux;
    aux = m;
    m = tmp;
  }

  return m;
}

void *simulFatia(void* args){
	int numThread, totalThreads;
	int linhas, colunas, numIteracoes;
	int i;
	DoubleMatrix2D* matrixGrande, matrix, matrix_aux;
	
	matrix = dm2dNew(linhas, colunas);
	matrix_aux = dm2dNew(linhas, colunas);
	
	for (i = 0; i < numIteracoes; i++){
	
		if ( numThread != 0)
			enviarMensagem( numThread, numThread -1, (void*) dm2dGetLine(matrix, 1), #tamanho_linha);
		if ( numThread != totalThreads -1)
			enviarMensagem (numThread, numThread +1, (void*) dm2dGetLine(matrix, linhas-2), #tamanho_linha);
	
		if ( numThread != 0)
			receberMensagem( numThread-1, numThread, #linha0 , #tamanho_linha);
		if (numThread != totalThreads-1)
			receberMensagem( numThread+1, numThread, #linha_coluna-1, #tamanho_linha);
			
		
	
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
  

  DoubleMatrix2D *matrix, *matrix_aux, *result;


  fprintf(stderr, "\nArgumentos:\n"
	" N=%d tEsq=%.1f tSup=%.1f tDir=%.1f tInf=%.1f iteracoes=%d\n",
	N, tEsq, tSup, tDir, tInf, iteracoes);

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
  
  int inicializarMPlib(int csz, int trab);

  matrix = dm2dNew(N+2, N+2);
  matrix_aux = dm2dNew(N+2, N+2);

  if (matrix == NULL || matrix_aux == NULL) {
    fprintf(stderr, "\nErro: Nao foi possivel alocar memoria para as matrizes.\n\n");
    return -1;
  }

  int i;

  /* ALTERACOES PEDIDAS  <inicio>  

  int i;

  for(i=0; i<N+2; i++)
    dm2dSetLineTo(matrix, i, i);
  dm2dPrint(matrix);


  for(i=0; i<N+2; i++)
    dm2dSetColumnTo(matrix_aux, i, i);
  dm2dPrint(matrix_aux);


  if(3 <= N+1) {
    double *line;
    line = dm2dGetLine(matrix, 3);
    dm2dSetLine(matrix_aux, 2, line);
    dm2dPrint(matrix_aux);
  }

   ALTERACOES PEDIDAS  <fim>  */


  for(i=0; i<N+2; i++)
    dm2dSetLineTo(matrix, i, 0);

  dm2dSetLineTo (matrix, 0, tSup);
  dm2dSetLineTo (matrix, N+1, tInf);
  dm2dSetColumnTo (matrix, 0, tEsq);
  dm2dSetColumnTo (matrix, N+1, tDir);

  dm2dCopy (matrix_aux, matrix);
  
  pthread_t tid[trab];

  result = simul(matrix, matrix_aux, N+2, N+2, iteracoes);
  if (result == NULL) {
    printf("\nErro na simulacao.\n\n");
    return -1;
  }

  dm2dPrint(result);

  dm2dFree(matrix);
  dm2dFree(matrix_aux);

  return 0;
}
