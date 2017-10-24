/*
// Projeto SO - exercicio 1, version 03
// Sistemas Operativos, DEI/IST/ULisboa 2017-18
*/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "matrix2d.h"


/*--------------------------------------------------------------------
| Function: simul
---------------------------------------------------------------------*/

//funcao auxiliar para ter a funcao da prox temperatura

double formula (DoubleMatrix2D *m, int l, int c){
	
	return  ( dm2dGetEntry(m,l - 1,c) + dm2dGetEntry(m,l + 1,c) +
				dm2dGetEntry(m,l,c - 1) + dm2dGetEntry(m,l,c + 1) ) / 4;

}

DoubleMatrix2D *simul(DoubleMatrix2D *matrix, DoubleMatrix2D *matrix_aux, int linhas, int colunas, int numIteracoes) {
	
	 int n, i, j;		//n iteraçoes : i 
	 DoubleMatrix2D *ponteiro_aux;
	 
	 for (n = 0; n < numIteracoes; n++){
		 
		 for ( i = 1; i < (linhas -1) ; i++ ){
			 
			 for ( j = 1 ; j < (colunas -1); j++){
				 
				 dm2dSetEntry(matrix_aux, i, j, formula(matrix, i, j) );
				 
			 } //j fica logo reset com o for, certo??
			 
		 }	
		 //trocar a ordem das matrizes 
		 ponteiro_aux = matrix;
		 matrix = matrix_aux;
		 matrix_aux = ponteiro_aux;
		 
	 }
	
  return matrix;
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

  if(argc != 7) {
    fprintf(stderr, "\nNumero invalido de argumentos.\n");
    fprintf(stderr, "Uso: heatSim N tEsq tSup tDir tInf iteracoes\n\n");
    return 1;
  }

  /* argv[0] = program name */
  int N = parse_integer_or_exit(argv[1], "N");
  double tEsq = parse_double_or_exit(argv[2], "tEsq");
  double tSup = parse_double_or_exit(argv[3], "tSup");
  double tDir = parse_double_or_exit(argv[4], "tDir");
  double tInf = parse_double_or_exit(argv[5], "tInf");
  int iteracoes = parse_integer_or_exit(argv[6], "iteracoes");

  DoubleMatrix2D *matrix, *matrix_aux, *result;


  fprintf(stderr, "\nArgumentos:\n"
	" N=%d tEsq=%.1f tSup=%.1f tDir=%.1f tInf=%.1f iteracoes=%d\n",
	N, tEsq, tSup, tDir, tInf, iteracoes);


  matrix = dm2dNew(N+2, N+2);
  matrix_aux = dm2dNew(N+2, N+2);


  /* FAZER ALTERACOES AQUI */


  dm2dSetLineTo (matrix, 0, tSup);
  dm2dSetLineTo (matrix, N+1, tInf);
  dm2dSetColumnTo (matrix, 0, tEsq);
  dm2dSetColumnTo (matrix, N+1, tDir);

  dm2dCopy (matrix_aux, matrix);	//ficam as duas iguais

  result = simul(matrix, matrix_aux, N+2, N+2, iteracoes);

  dm2dPrint(result);

  dm2dFree(matrix);
  dm2dFree(matrix_aux);

  return 0;
}
