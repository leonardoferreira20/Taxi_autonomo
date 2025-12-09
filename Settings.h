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
#include <pthread.h>

// CONSTANTES
#define MAXCMD 256
#define MAXCLI 30
#define MAX_VEHICLES 10
#define MAX_SERVICES 3
#define TEMPODEASSINCRONAR 0
#define TEMPOINSTANTE 5
//#define TEMPODEASSINCRONAR 100 * 1000

#define VARAMB "NVEICULO"

#define MAX_USERNAME 50
#define MAX_LOCAL 100
#define MAX_DESTINO 100
#define MAX_MSG 512*4

#define SERVERFIFO "fifo_server"
#define CLIENTE_FIFO_PREFIX "cli_"
#define VEICULOFIFO "veiculo"
#define TEMPO_INICIAL 1

typedef enum {
	MSG_LIMPA = 0,
	MSG_LOGIN = 1,
	MSG_AGENDAR = 2,
	MSG_CONSULTAR = 3,
	MSG_CANCELAR = 4,
	MSG_RESPOSTA = 5,
	MSG_TERMINAR = 6,
	MSG_NOTIFICACAO = 7,
	MSG_ACEITA = 8, //
	MSG_RECUSA = 9, //
	MSG_NAOAUTENTICADO = 10,
	MSG_VEICULO = 11,
	MSG_ADMINSHUTDOWN = 12
} TipoMensagem;

typedef struct {
	char msg[MAX_MSG];
	int em_viagem; //1 em viagem e 0 acaba
	float kms;
} Telemetria;

// AUTENTICAÇÃO
typedef struct {
	char username[MAX_USERNAME];
	pid_t pid;
	char fifo_name[MAX_MSG];
} Authentication;

// MENSAGEM
typedef struct {
	TipoMensagem tipo;
	Telemetria telm;
	char username[MAX_USERNAME]; // Quem envia
	char msg[MAX_MSG];           // Mensagem/feedback textual
	char fifo_name[MAX_MSG];
	int chave;
	int pid;
	int tempo_viagem;
	char veredito[10];
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
	char estado[15];
	char local[MAX_LOCAL];
} Servico_Marcado;

typedef struct{
	int nServicos;
	Servico_Marcado servicosT[MAX_SERVICES];
} Servico_Taxi;

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
    int ativo;                // 0 = livre, 1 = em uso

	int nTaxi;
    int indiceCliente;
    int indiceServico;

    pid_t pid_veiculo;        // PID do processo veiculo (depois do fork)

    int fd_leitura;      // fd do lado de leitura do pipe anonimo

	char username[MAX_USERNAME];
    char fifo_cliente[MAX_MSG];
    char local[MAX_LOCAL];
    int distancia;
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