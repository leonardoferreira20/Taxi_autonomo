#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#define FIFO_CONTROLADOR "/tmp/controlador_fifo"

int main(){
  int fd;
  char buffer[100];
  
  // Cria o FIFO público (se ainda não existir)
  if (mkfifo(FIFO_CONTROLADOR, 0666) == -1) {
    perror("mkfifo");
    // não sair se já existir
  }

  printf("[CONTROLADOR] A aguardar clientes em %s...\n", FIFO_CONTROLADOR);

  // Abre FIFO para leitura (bloqueante)
  fd = open(FIFO_CONTROLADOR, O_RDONLY);

  if (fd == -1) {
    perror("open");
    exit(1);
  }

  while (1) {
    ssize_t n = read(fd, buffer, sizeof(buffer) - 1);
    if (n > 0) {
      buffer[n] = '\0';
      printf("[CONTROLADOR] Recebido: %s\n", buffer);

      char cmd[16], user[64], fifo_cli[128];
      if (sscanf(buffer, "%s %s %s", cmd, user, fifo_cli) == 3) {
        if (strcmp(cmd, "LOGIN") == 0) {
          int fd_cli = open(fifo_cli, O_WRONLY);
          if (fd_cli != -1) {
            write(fd_cli, "OK\n", 3);
            close(fd_cli);
            printf("[CONTROLADOR] Login aceite para %s\n", user);
          } else {
            perror("Erro a responder ao cliente");
          }
        }
      }

    } else {
      // Se o cliente fechar o FIFO, reabre para não bloquear o próximo
      close(fd);
      fd = open(FIFO_CONTROLADOR, O_RDONLY);
    }
  }

  close(fd);
  unlink(FIFO_CONTROLADOR);

  return 0;
}