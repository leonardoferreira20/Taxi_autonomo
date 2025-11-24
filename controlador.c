#include "Settings.h"

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

int running = 1;

int verficaClienteRegistado (char clientesAtivos [MAXCLI][20],char *user, int nClientes){
    for(int i = 0; i < nClientes; i++){
        if(strcmp(clientesAtivos[i], user)==0){
            return 1;
        }
    }
    return 0;
}

int main(int argc, char * argv[]){
    Mensagem m;
    char clientesAtivos[MAXCLI][20];
    int nClientes = 0;
    char login[20];

    //Verifica
    if (argc!=1){
        perror("[sistema] Numero invalido de parametros!\n");
        exit(-1);
    }

    if (access(SERVERFIFO, F_OK) == 0) {
        printf("[sistema] O servidor ja se encontra em execucao\n");
        exit(0);
    }

    if (mkfifo(SERVERFIFO, 0666) == -1) {
        if (errno != EEXIST) {
        perror("[sistema] Erro na criacao do named pipe");
        exit(EXIT_FAILURE);
        }
    }

    int fd = open(SERVERFIFO, O_RDWR);
    if (fd == -1) {
        perror("[sistema] Erro na abertura do named pipe para leitura");
        unlink(SERVERFIFO);
        exit(EXIT_FAILURE);
    }

    printf("[controlador] O controlador está pronto a receber clientes!\n");

    char user[20];

    while (running) {
        printf("[controlador] A receber Usernames: \n");
        int nbytes = read(fd, &user, sizeof(user));
        
        if (nbytes == -1) {
            perror("[sistema] Ocorreu um erro na leitura do Username!\n");
        }else{
            if(nClientes <MAXCLI && verficaClienteRegistado(clientesAtivos, user, nClientes) == 0){
                printf("[controlador] Cliente aceite! E este é o seu Username: %s\n", user);
                strcpy(clientesAtivos[nClientes],user);
                nClientes++;






            }else {
                printf("[controlador] Cliente nao aceite\n");
                strcpy(login, "falhou");
            }
        }
    }


    close(fd);
    unlink(SERVERFIFO);
    return 0;
}