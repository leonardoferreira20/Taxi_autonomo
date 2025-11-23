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


void comado_invalido(int invalido){
    printf("-> Comando inválido\n");
    if(invalido == 1){
        printf("-> agendar <hora> <local> <distancia>\n");
    } else if(invalido == 2){
        printf("-> cancelar <id>\n");
    } else if(invalido == 3){
        printf("-> entrar <destino>\n");
    }else if(invalido == 4){
        printf("-> consultar\n");
    } else if(invalido == 5){
        printf("-> sair\n");
    }else if(invalido == 6){
        printf("-> terminar\n");
    };
}

char* separa_palavras(const char * palavra, int *nargs){
    char argumento[64];
    //char* aux = palavra;
/*
    while(*aux == ' '){aux++;};
    while(*aux != ' ' || *aux != '\0'){aux++;};
    if(*aux != '\0'){*aux='\0';};

    nargs++;
*/
}


void leitura_comandos_cliente(const char * utilizador_cmd){
    char cmd[MAXCMD];
    char args[MAXCMD];
    int tam, nargs;
    char * comando;
    char * argumento;

    //copiar o array para uma string interna da funcao
    strncpy(cmd, utilizador_cmd,sizeof(cmd));
    //atribuir \0 no final para ter a certeza
    cmd[sizeof(cmd)-1]='\0';

    //meter um '\0' no lugar de um possivel \n caso a string seja mais pequena que o tamanho e registou o enter 
    tam=strlen(cmd);
    if(tam > 0 && cmd[tam -1]== '\n'){
        cmd[tam-1]='\0';
    }

    //Ponteiro para a primeira posicao do cmd
    char *aux = cmd;

    //Salta espaços em branco
    while(*aux==' '){
        aux++;
    }

    //Salta fora do programa caso não tenha comando
    if(*aux == '\0'){
        printf("Comando invalido!\n");
        return;
    }

    //ponteiro comando aponta para a posicao inicial do comando a verificar
    comando = aux;

    //avança o ponteiro até ao final da paralvra
    while (*aux != '\0' && *aux != ' '){
        aux++;
    }

    //caso seja diferente de /0 mete um /0 (isto é para os comandos que têm argumentos)
    //caso não tenham argumentos nao vai entrar aqui porque ja tem /0 que metemos na linha 21
    //ao entrar neste if quer dizer que tem argumentos por isso vamos ver onde começam para meter o ponteiro args para lá
    if (*aux != '\0'){
        *aux = '\0';
        aux++;
        while(*aux == ' '){
            aux++;
        }
        if(*aux != '\0'){
            argumento = aux;
        }else
            argumento = NULL;
    }


    if(strcasecmp(cmd,"agendar")==0){
        if(nargs!=3) comado_invalido(1);
        //COMANDO AGENDAR
    }else if (strcasecmp(cmd,"cancelar")==0){
        if(nargs!=1) comado_invalido(2);
        //COMANDO CANCELAR
    }else if (strcasecmp(cmd,"entrar")==0){
        if(nargs!=1) comado_invalido(3);
        //COMANDO ENTRAR
    }else if (strcasecmp(cmd,"consultar")==0){
                if(args !=NULL) comado_invalido(4);
        //COMANDO CONSULTAR
    }else if (strcasecmp(cmd,"sair")==0){
        if(args !=NULL) comado_invalido(5);
        //COMANDO SAIR
    }else if (strcasecmp(cmd,"terminar")==0){
        if(args !=NULL) comado_invalido(6);
        //COMANDO TERMINAR
    }else printf("Comando invalido!\n");

}





int main(int argc, char* argv[]){
    setbuf(stdout, NULL);

    char acesso[20]; //mensagem de acesso valido ou invalido
    char cmd[MAXCMD];
    Mensagem m;//mensagem campo estrutura
    int login = 0;

    //Verificacao de numero de argumentos
    if (argc!=2){
        perror("Numero invalido de parametros!\n");
        exit(-1);
    }

    //Defenido nas settings o nome do servidor
    //Acesso ao pipe aberto pelo servidor (já deve estar aberto do lado do servidor)
    int fd = open(SERVERFIFO, O_RDWR);
    //verificar se abriu o pipe
    if(fd < 0){
        perror("Erro ao abrir pipe do servidor!\n");
        exit(-1);
    }
    
    //Escrita para o servidor autenticacao
    //Cliente envia o seu username para o servidor para se autenticar
    //Se não existir outro utilizador com este username e ainda houver espaço para o utilizador (servidor tem numeo maximo de utilizadores) O login é bem sucedido
    //write(fd,argv[1],strlen(argv[1])+1);
    
    strcpy(acesso,argv[1]);
    printf("entra\n");
    write(fd,acesso,strlen(acesso)+1);

    //Criar pipe do cliente com o nome do cliente
    char fifo_cliente[64];
    snprintf(fifo_cliente, sizeof(fifo_cliente), "fifo_%s", argv[1]);

    if(mkfifo(fifo_cliente,0666) == -1){
        perror("Erro a criar pipe do cliente!\n");
        exit(-1);
    };

    //Abrir pipe do cliente para receber mensagens do servidor
    int rd = open(fifo_cliente, O_RDWR);
    if(rd == -1){
        perror("Erro a abrir namedpipe do cliente!\n");
        close(fd);
        exit(-1);
    }
    //read (valido invalido)
    int nbytes; //= read(rd,acesso,sizeof(acesso));

    /*
    //Verificar se o acesso é valido ou invalido (existe algum cliente com o mesmo nome)
    //----------//Caso invalido terminar 
    if(nbytes < 0){
        perror("Erro acesso inválido!\n");
        exit(-1);
    }
    */

    while(running){
        printf("-> Introduza comando: \n");
        printf("-> ");
        scanf("%d", &m.tipo_msg);
        //leitura_comandos_cliente(cmd);
        nbytes = write(fd,&m,sizeof(m));
	    
        if(nbytes == -1){
		    if(errno != EPIPE)
			    perror("erro ao escrever a mensagem no pipe");
		    close(fd);
		    close(rd);
		    unlink(fifo_cliente);
		    exit(EXIT_FAILURE);
	    }

        if(m.tipo_msg == 0) break;
    }



    close(fd);
    close(rd);
    unlink(fifo_cliente);
    return 0;
}