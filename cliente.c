#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#define FIFO_CONTROLADOR "/tmp/controlador_fifo"

int main(int argc, char *argv[]) {
  
  if (argc != 2) {
    fprintf(stderr, "Uso: %s <username>\n", argv[0]);
    exit(1);
  }

  char *username = argv[1];
  char fifo_privado[256];
  snprintf(fifo_privado, sizeof(fifo_privado), "/tmp/cli_%s", username);

  // Cria FIFO privado do cliente
  if (mkfifo(fifo_privado, 0666) == -1) {
    perror("mkfifo");
    // não sair se já existir
  }

  // Abre FIFO público do controlador para escrita
  int fd_controlador = open(FIFO_CONTROLADOR, O_WRONLY);
  if (fd_controlador == -1) {
    perror("Erro a abrir FIFO do controlador");
    unlink(fifo_privado);
    exit(1);
  }

  // Envia pedido de login
  char mensagem[256];
  snprintf(mensagem, sizeof(mensagem), "LOGIN %s %s", username, fifo_privado);
  write(fd_controlador, mensagem, strlen(mensagem));

  printf("[CLIENTE] Pedido de login enviado: %s\n", mensagem);

  close(fd_controlador);

  // Por agora apenas espera resposta e mostra (ainda sem lógica de OK/ERRO)
  int fd_privado = open(fifo_privado, O_RDONLY);
  if (fd_privado == -1) {
    perror("Erro a abrir FIFO privado");
    unlink(fifo_privado);
    exit(1);
  }

  char resposta[128];
  ssize_t n = read(fd_privado, resposta, sizeof(resposta) - 1);
  if (n > 0) {
    resposta[n] = '\0';
    printf("[CLIENTE] Resposta: %s\n", resposta);
  } else {
    printf("[CLIENTE] Nenhuma resposta recebida.\n");
  }

  close(fd_privado);
  unlink(fifo_privado);

  return 0;
}