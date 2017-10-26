/*
// Projeto SO - exercicio 1, versao 2
// Sistemas Operativos, DEI/IST/ULisboa 2017-18
*/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "matrix2d.h"
#include "mplib4.h"


/* Estrutura com Informacao para Escravos */
typedef struct { 
    int id;
    int N;
    int iter;
    int trab;
    int tam_fatia;
} thread_info;

/*--------------------------------------------------------------------
  | Function: tarefa_escravo
  | Description: Função executada por cada tarefa escravo
  ---------------------------------------------------------------------*/

void *tarefa_escravo(void* args) {
  thread_info *tinfo = (thread_info *) args;
  DoubleMatrix2D *fatias[2];
  int atual, prox;
  int iter;
  int i, j;

  /* Criar Matrizes com Fatias e Linhas Adjacentes */
  fatias[0] = dm2dNew(tinfo->tam_fatia+2,tinfo->N+2);
  fatias[1] = dm2dNew(tinfo->tam_fatia+2,tinfo->N+2);
  
  if (fatias[0] == NULL || fatias[1] == NULL) {
    fprintf(stderr, "\nErro ao criar Matrix2d num escravo.\n"); 
    pthread_exit((void *) 1);
  }

  /* Receber Fatia e Linhas Adjacentes da Tarefa Mestre */
  for (i = 0; i < tinfo->tam_fatia + 2; i++)
    receberMensagem(0, tinfo->id, dm2dGetLine(fatias[0], i), (tinfo->N+2) * sizeof(double));
  
  dm2dCopy(fatias[1], fatias[0]);

  /* Ciclo Iterativo */
  for (iter = 0; iter < tinfo->iter; iter++) {
    
    atual = iter % 2;
    prox = 1 - iter % 2;

    // Calcular Pontos Internos
    for (i = 0; i < tinfo->tam_fatia; i++) {
      for (j = 0; j < tinfo->N; j++) {
        double val = (dm2dGetEntry(fatias[atual], i, j+1) +
                      dm2dGetEntry(fatias[atual], i+2, j+1) +
                      dm2dGetEntry(fatias[atual], i+1, j) +
                      dm2dGetEntry(fatias[atual], i+1, j+2))/4;
        dm2dSetEntry(fatias[prox], i+1, j+1, val);
      }
    }
    
    // Enviar e Receber Novos Valores das Linhas Adjacentes. As Tarefas Par Enviam antes de Receberem.
    if (tinfo->id % 2 == 0) {
      if (tinfo->id > 1)
        enviarMensagem(tinfo->id, tinfo->id - 1, dm2dGetLine(fatias[prox], 1), (tinfo->N+2) * sizeof(double));
      if (tinfo->id < tinfo->trab)
        enviarMensagem(tinfo->id, tinfo->id + 1, dm2dGetLine(fatias[prox], tinfo->tam_fatia), (tinfo->N+2) * sizeof(double));

      if (tinfo->id > 1)
        receberMensagem(tinfo->id - 1, tinfo->id, dm2dGetLine(fatias[prox], 0), (tinfo->N+2) * sizeof(double));
      if (tinfo->id < tinfo->trab)
        receberMensagem(tinfo->id + 1, tinfo->id, dm2dGetLine(fatias[prox], tinfo->tam_fatia + 1), (tinfo->N+2) * sizeof(double));
    }
    else {
      if (tinfo->id > 1)
        receberMensagem(tinfo->id - 1, tinfo->id, dm2dGetLine(fatias[prox], 0), (tinfo->N+2) * sizeof(double));
      if (tinfo->id < tinfo->trab)
        receberMensagem(tinfo->id + 1, tinfo->id, dm2dGetLine(fatias[prox], tinfo->tam_fatia + 1), (tinfo->N+2) * sizeof(double));
      
      if (tinfo->id > 1)
        enviarMensagem(tinfo->id, tinfo->id - 1, dm2dGetLine(fatias[prox], 1), (tinfo->N+2) * sizeof(double));
      if (tinfo->id < tinfo->trab)
        enviarMensagem(tinfo->id, tinfo->id + 1, dm2dGetLine(fatias[prox], tinfo->tam_fatia), (tinfo->N+2) * sizeof(double));
    }
  }
  
  /* Enviar Fatia Final a Tarefa Mestre */
  for (i = 0; i < tinfo->tam_fatia; i++)
    enviarMensagem(tinfo->id, 0, dm2dGetLine(fatias[prox], i + 1), (tinfo->N+2) * sizeof(double));

  /* Libertar Memoria Alocada */
  dm2dFree(fatias[0]);
  dm2dFree(fatias[1]);

  return ((void *) 0);
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
  int csz;
  int tam_fatia;
  int res;
  int i, j;
  DoubleMatrix2D  *matrix;
  thread_info *tinfo;
  pthread_t *escravos;


 if (argc < 9) {
    fprintf(stderr, "\nNúmero de Argumentos Inválido.\n");
    fprintf(stderr, "Utilização: heatSim_p2 N tEsq tSup tDir tInf iter trab csz\n\n");
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
  csz =  parse_integer_or_exit(argv[8], "csz");

  fprintf(stdout, "\nArgumentos:\n"
          " N=%d tEsq=%.1f tSup=%.1f tDir=%.1f tInf=%.1f iter=%d trab=%d csz=%d",
          N, tEsq, tSup, tDir, tInf, iter, trab, csz);

  
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
  
  /* Inicializar Biblioteca de Passagem de Mensagens */
  inicializarMPlib(csz, trab + 1);

  /* Calcular Tamanho de cada Fatia */
  tam_fatia = N/trab;

  /* Criar Matriz Inicial */
  matrix = dm2dNew(N+2, N+2);
  
  if (matrix == NULL) {
    fprintf(stderr, "\nErro ao criar Matrix2d.\n");    
    return 1;
  }
  
  dm2dSetLineTo(matrix, 0, tSup);
  dm2dSetLineTo(matrix, N+1, tInf);
  dm2dSetColumnTo(matrix, 0, tEsq);
  dm2dSetColumnTo(matrix, N+1, tDir);
  
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
  
  /* Enviar Fatias e Linhas Adjacentes, Linha a Linha, a cada Escravo */
  for (i = 0; i < trab; i++)
    for (j = 0; j < (tam_fatia + 2); j++)
      enviarMensagem(0, i+1, dm2dGetLine(matrix, i*tam_fatia + j), (N+2)*sizeof(double));
  
  /* Receber Fatias Finais de cada Escravo e Guardar na Matriz */
  for (i = 0; i < trab; i++)
    for (j = 0; j < tam_fatia; j++)
      receberMensagem(i+1, 0, dm2dGetLine(matrix, i*tam_fatia + j + 1), (N+2)*sizeof(double));
  
  /* Esperar que os Escravos Terminem */
  for (i = 0; i < trab; i++) {
    res = pthread_join(escravos[i], NULL);
    
    if (res != 0) {
      fprintf(stderr, "\nErro ao esperar por um escravo.\n");    
      return 1;
    }  
  }
  
  /* Imprimir resultado */
  dm2dPrint(matrix);

  /* Libertar Memoria */
  dm2dFree(matrix);
  free(tinfo);
  free(escravos);
  libertarMPlib();
 
  return 0;
}
