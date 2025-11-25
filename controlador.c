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
        perror("[CONTROLADOR] Erro na abertura do named pipe para leitura");
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

void handler_sigint(int sig) {
    printf("\n[CONTROLADOR] Sinal recebido. A terminar...\n");
    running = 0;
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
    char clientesAtivos[MAXCLI][20];
    int nClientes = 0;
    char login[20];

    // VERIFICA
    if (argc != 1){
        perror("[CONTROLADOR] Numero invalido de parametros!\n");
        exit(-1);
    }

    printf("\n--------------------------------------------------------\n");
    printf("\n              SISTEMA DE TÁXIS AUTÓNOMOS                \n");
    printf("\n--------------------------------------------------------\n\n");

    if (access(SERVERFIFO, F_OK) == 0) {
        printf("[CONTROLADOR] O servidor ja se encontra em execucao\n");
        exit(0);
    }

    signal(SIGINT, handler_sigint);

    if (mkfifo(SERVERFIFO, 0666) == -1) {
        if (errno != EEXIST) {
        perror("[CONTROLADOR] Erro na criacao do named pipe");
        exit(EXIT_FAILURE);
        }
    }

    int fd_servidor = open(SERVERFIFO, O_RDWR);
    if (fd_servidor == -1) {
        perror("[CONTROLADOR] Erro na abertura do named pipe para leitura");
        unlink(SERVERFIFO);
        exit(EXIT_FAILURE);
    }

    printf("[CONTROLADOR] Sistema iniciado!\n");
    printf("[CONTROLADOR] Comandos: utiliz, listar, frota, km, hora, terminar\n\n");
    
    // LANÇAR THREAD DE GESTÃO DO TEMPO
    pthread_t thread_tempo;
    if(pthread_create(&thread_tempo, NULL, gestor_tempo, NULL) != 0){
        perror("[CONTROLADOR] Erro a criar a thread");
    }

    while (running) {
        Authentication auth;
        Mensagem resp;
        int nbytes = read(fd_servidor, &auth, sizeof(auth));

        if (nbytes != -1) {
            int fd_cliente = open(auth.fifo_name, O_RDWR);
            if (fd_cliente == -1) {
                perror("[CONTROLADOR] Erro na abertura do named pipe para leitura do cliente");
                close (fd_cliente);
            }

            memset(&resp, 0, sizeof(resp));
            resp.tipo = MSG_RESPOSTA;

            if(nClientes < MAXCLI && verficaClienteRegistado(clientesAtivos, auth.username, nClientes) == 0){
                printf("[CONTROLADOR] Cliente aceite! E este é o seu Username: %s\n", auth.username);
                strcpy(clientesAtivos[nClientes], auth.username);
                nClientes++;

                resp.login = LOGIN_ACEITE;
                strcpy(resp.msg, "Login aceite!");
                
                //Uma variavel nao irá bastar
                //Porque nos vamos poder ter 20 threads a funcionar ao mesmo tempo
                //Muito provavelmente iremos precisar de um array de threads
                //Porque no fim vaos ter de fazer join das threads
                //Ou seja esperar que todas as threads terminem
                pthread_t thread; 
                if(pthread_create(&thread, NULL, chat_clinte, &auth) != 0){
                    perror("[CONTROLADOR] Erro a criar a thread");
                }
            }
            else if (nClientes >= MAXCLI ) {
                printf("[CONTROLADOR] Cliente nao aceite\n");
                resp.login = LOGIN_REJEITADO;
                strcpy(resp.msg, "Limite de utilizadores atingido");
            }
            else {
                printf("[CONTROLADOR] Cliente nao aceite\n");
                resp.login = LOGIN_REJEITADO;
                strcpy(resp.msg, "Username já em uso");
            }

            write(fd_cliente, &resp, sizeof(resp));
            close(fd_cliente);
        }

        usleep(50000); // 50ms
    }

    printf("[CONTROLADOR] A encerrar...\n");
    
    pthread_join(thread_tempo, NULL);
    
    close(fd_servidor);
    unlink(SERVERFIFO);
    
    printf("[CONTROLADOR] Encerrado.\n");
    return 0;
}