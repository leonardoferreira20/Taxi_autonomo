#include "Settings.h"

int main(int argc, char * argv[]){
  int kms = 0;
  int destino = *argv[3];

  if (argc!=2){
    perror("[sistema] Numero invalido de parametros!\n");
    exit(-1);
  }

  while(kms != destino){
    ++kms;
    

    sleep (TEMPOINSTANTE);
  }


  return 0;
}