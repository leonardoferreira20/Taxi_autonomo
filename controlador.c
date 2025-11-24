#include "Settings.h"

int running = 1;
int tempo = TEMPO_INICIAL;


void thread_id(Authentication *a){
    printf("[%s Thread %d]",a->username,a->pid);
}

void * gestor_tempo(void * arg){
    while(running){
        ++tempo;
        sleep(1);
    }
    return NULL;
}


void * chat_clinte(void * arg){
    Authentication *a = (Authentication *) arg;
    Mensagem m;
    int servico=1;
    
    int td = open(a->fifo_name,O_RDWR);
    if (td == -1) {
        perror("[sistema] Erro na abertura do named pipe para leitura");
        exit(EXIT_FAILURE);
    }
    printf("criei uma thread!\n");
    strcpy(m.msg,"estou aqui dentro");
    write(td,&m,sizeof(m));
    while(servico=1){
        thread_id(a);
        printf("A receber ordens de serviço para cliente :\n");
        break;
    }

    return NULL;
}




int verficaClienteRegistado (char clientesAtivos [MAXCLI][20],char *user, int nClientes){
    for(int i = 0; i < nClientes; i++){
        if(strcmp(clientesAtivos[i], user)==0){
            return 1;
        }
    }
    return 0;
}

int main(int argc, char * argv[]){
    int cl;
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
    pthread_t thread_tempo;
    if(pthread_create(&thread_tempo,NULL,gestor_tempo,NULL) != 0){
        perror("[sistem]Erro a criar a thread");
    }

    while (running) {
        Authentication a;
        Mensagem m;
        m.login = 0;
        strcpy(m.msg,"");

        printf("[controlador: %ds] A receber Usernames: \n",tempo);
        int nbytes = read(fd, &a, sizeof(a));
        
        if (access(a.fifo_name, F_OK) != 0) {
        printf("[sistema] O Cliente não se encontra em execucao\n");
        }

        if (nbytes == -1) {
            perror("[sistema] Ocorreu um erro na leitura do Username!\n");
        }else{

            cl = open(a.fifo_name, O_RDWR);
            if (cl == -1) {
            perror("[sistema] Erro na abertura do named pipe para leitura do cliente");
            close (cl);
            }

            if(nClientes <MAXCLI && verficaClienteRegistado(clientesAtivos, a.username, nClientes) == 0){
                printf("[controlador] Cliente aceite! E este é o seu Username: %s\n", a.username);
                strcpy(clientesAtivos[nClientes],a.username);
                nClientes++;

            strcpy(m.msg,"[controlador] Login verificado! Seja bem vindo!\n");
            m.login = 1;
            write(cl,&m,sizeof(m));
            
            //Uma variavel nao irá bastar
            //Porque nos vamos poder ter 20 threads a funcionar ao mesmo tempo
            //Muito provavelmente iremos precisar de um array de threads
            //Porque no fim vaos ter de fazer join das threads
            //Ou seja esperar que todas as threads terminem
            pthread_t thread; 
            
            if(pthread_create(&thread,NULL,chat_clinte,&a) != 0){
                perror("[sistem]Erro a criar a thread");
            }


            }else {
            printf("[controlador] Cliente nao aceite\n");
            strcpy(login, "falhou");
                
            write(cl,login,strlen(login)+1);
            close(cl);
            }
        }
    }


    close(fd);
    unlink(SERVERFIFO);
    return 0;
}