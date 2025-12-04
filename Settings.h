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
#define MAX_SERVICES 3
#define TEMPODEASSINCRONAR 0
#define TEMPOINSTANTE 5
//#define TEMPODEASSINCRONAR 100 * 1000

#define MAX_USERNAME 50
#define MAX_LOCAL 100
#define MAX_DESTINO 100
#define MAX_MSG 512*4

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
	MSG_LIMPA = 0,
	MSG_LOGIN = 1,
	MSG_AGENDAR = 2,
	MSG_CONSULTAR = 3,
	MSG_CANCELAR = 4,
	MSG_ENTRAR = 5,
	MSG_SAIR = 6,
	MSG_RESPOSTA = 7,
	MSG_TERMINAR = 8,
	MSG_NOTIFICACAO = 9,
	MSG_ACEITA = 10,
	MSG_RECUSA = 11,
	MSG_NAOAUTENTICADO = 12
} TipoMensagem;

// AUTENTICAÇÃO
typedef struct {
	char username[MAX_USERNAME];
	pid_t pid;
	char fifo_name[MAX_MSG];
} Authentication;

// MENSAGEM
typedef struct {
	TipoMensagem tipo;
	char username[MAX_USERNAME]; // Quem envia
	char msg[MAX_MSG];           // Mensagem/feedback textual
	char fifo_name[MAX_MSG];
	int chave;
	int pid;
	int tempo_viagem;
	char resposta[MAX_MSG];
	
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
	int id;
	int hora;
	int distancia;
	char local[MAX_LOCAL];
} Servico_Marcado;

typedef struct {
	char username[MAX_USERNAME];
	char fifo_name[MAX_MSG];
	int chave;
	int pid;
	int distancia;
	int ativo;
	int pagou;
	int em_viagem;              // 1 se está em viagem, 0 caso contrário
	int servicos_ativos;		// Indice do arry de servicos
	Servico_Marcado servicos[MAX_SERVICES];      
} Utilizador;

typedef struct {
	char username[MAX_USERNAME];
	int nServicos;
} Viagem;

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