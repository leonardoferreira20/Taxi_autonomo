#ifndef SETTINGS_H
#define SETTINGS_H

#define SERVERFIFO "fifo_server"
#define VEICULOFIFO "fifo_veiculo"
#define TAM 256
#define MAXCMD 128
#define MAXCLI 1
#define USERNAME_SIZE 20
#define MAXVEICULO 20

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

typedef enum {
	AUTHENTICATION,
	ORDER,
	USERNAME
}request_type;


typedef struct {
    char msg[TAM];
	int login;
	int tipo_msg;
	request_type type;
}Mensagem;


typedef struct authentication {
	char username[USERNAME_SIZE];
	char fifo_name[256];
	int pid;
	//bool viagem;
	//int ativo;
}Authentication;



typedef struct {
    //char username[MAX_USERNAME];
    char fifo_name[256];
    int ativo;
    int em_viagem;
} Utilizador;







/*
typedef struct {
    int tipo_msg;
    pid_t pid;
	request_type type;
	union request_payload{
		authentication auth;
		order pedido;
	} request_payload;
}Mensagem;

typedef enum {
	AUTHENTICATION,
	ORDER
}request_type;

typedef struct order {
	char msg[TAM];
}order;

typedef struct authentication {
	char username[USERNAME_SIZE];
	char password[PASSWORD_SIZE];
}authentication;

*/

#endif