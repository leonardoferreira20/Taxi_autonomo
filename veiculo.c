#include "Settings.h"

int main(int argc, char * argv[]){
  printf("Hello! Sou o veiculo.\n");

  if (argc!=2){
    perror("[sistema] Numero invalido de parametros!\n");
    exit(-1);
  }

  if (access(SERVERFIFO, F_OK) == 0) {
    printf("[sistema] O servidor ja se encontra em execucao\n");
    exit(0);
  }

  int fd = open(SERVERFIFO, O_RDWR);
    //verificar se abriu o pipe
  if(fd < 0){
    perror("Erro ao abrir pipe do servidor!\n");
    exit(-1);
  }

  if (mkfifo(VEICULOFIFO, 0666) == -1) {
    if (errno != EEXIST) {
    perror("[sistema] Erro na criacao do named pipe");
    exit(EXIT_FAILURE);
    }
  }

  while(1){
    scanf("");
  }
  close(fd);
  //unlink(VEICULOFIFO);
  return 0;
}