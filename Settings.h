#ifndef SETTINGS_H
#define SETTINGS_H

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <sys/wait.h>

// CONSTANTES
#define MAXCMD 256
#define MAXCLI 30
#define MAX_VEHICLES 10
#define MAX_SERVICES 200

#define MAX_USERNAME 50
#define MAX_LOCAL 100
#define MAX_DESTINO 100
#define MAX_MSG 512

#define SERVERFIFO "fifo_server"
#define CLIENTE_FIFO_PREFIX "cli_"
#define VEICULOFIFO "veiculo"
#define TEMPO_INICIAL 1

/* typedef enum {
	AUTHENTICATION,
	ORDER,
	USERNAME
}request_type;

typedef struct {
	char msg[TAM];
	int login;
	int tipo_msg;
	request_type type;
} Mensagem;

typedef struct authentication {
	char username[USERNAME_SIZE];
	char fifo_name[256];
	int pid;
	//bool viagem;
	//int ativo;
} Authentication; */

typedef enum {
	MSG_LOGIN = 0,
	MSG_AGENDAR = 1,
	MSG_CONSULTAR = 2,
	MSG_CANCELAR = 3,
	MSG_RESPOSTA = 4,
	MSG_TERMINAR = 5,
	MSG_NOTIFICACAO = 6
} TipoMensagem;

typedef enum {
	LOGIN_REJEITADO = 0,
	LOGIN_ACEITE = 1
} EstadoLogin;

// AUTENTICAÇÃO
typedef struct {
	char username[MAX_USERNAME];
	pid_t pid;
	char fifo_name[MAX_MSG];
} Authentication;

// MENSAGEM
typedef struct {
	TipoMensagem tipo;
	EstadoLogin login;           // Só para respostas de login
	char username[MAX_USERNAME]; // Quem envia
	char msg[MAX_MSG];           // Mensagem/feedback textual
	char fifo_name[MAX_MSG];
	int pid;
	int tempo_viagem;

	// Campos específicos (usar conforme o tipo)
	int hora;
	char local[MAX_LOCAL];
	int distancia;
	int servico_id;
	char destino[MAX_DESTINO];
} Mensagem;

// UTILIZADOR - CONTROLADOR
typedef struct {
	char username[MAX_USERNAME];
	char fifo_name[MAX_MSG];
	int pid;
	int distancia;
	int ativo;
	int pagou;
	//int em_viagem;              // 1 se está em viagem, 0 caso contrário
	//int servico_ativo;          // ID do serviço em execução (-1 se nenhum)
} Utilizador;

// COMANDOS - CLIENTE
typedef struct {
	char comando[32];           // "agendar", "consultar", etc.
	int hora;
	char local[MAX_LOCAL];
	int distancia;
	int id;
	char destino[MAX_DESTINO];
	int valido;                 // 1=válido, 0=inválido
} ComandoParsed;



typedef struct {
	Authentication auth;
	int thread_id;
} ThreadClienteArgs;


#endif