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
/* THREAD ANTIGA DO CHAT CLIENTE - NAO APAGAR PODE SER UTIL
void * chat_clinte(void * arg){
    Authentication *a = (Authentication *) arg;
    Mensagem m;
    int servico=1;
    printf("%s",a->fifo_name);
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
*/

void handler_sigint(int sig) {
    printf("\n[CONTROLADOR] Sinal recebido. A terminar...\n");
    running = 0;
}

void imprimirTipoDePedido(TipoMensagem tipo){
    switch(tipo){
        case MSG_AGENDAR:
        printf("[CONTROLADOR] Pedido do tipo Agendar\n");
        break;
        case MSG_CONSULTAR:
        printf("[CONTROLADOR] Pedido do tipo Consultar\n");
        break;
        case MSG_CANCELAR:
        printf("[CONTROLADOR] Pedido do tipo Cancelar\n");
        break;
        case MSG_TERMINAR:
        printf("[CONTROLADOR] Pedido do tipo Terminar\n");
        break;
        default: 
        printf("[CONTROLADOR] Comando Inválido\n");
        break;
    }
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
    int erro=0; //Experiencia
    int TotalServicos = 0;
    int totalTentativasLigacao = 0;
    int totalUtilizadoresLigados = 0;
    int totalUtilizadoresRejeitados = 0;


    struct sigaction sa;
    sa.sa_handler = handler_sigint;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO;
    sigaction(SIGINT, &sa, NULL);



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

    //signal(SIGINT, handler_sigint);

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
        Mensagem pedido;
        Mensagem resp;

        int nbytes = read(fd_servidor, &pedido, sizeof(pedido));
        if (nbytes == -1) {
            if (errno != EINTR) {
            perror("[CONTROLADOR] Erro na leitura do pedido!\n");
            }else{
            perror("[CONTROLADOR] Encerrar a leitura de mensagens!\n");
            }
        break;
        }

        if (nbytes > 0) {
            totalTentativasLigacao++;///****estatistica*****
            resp.tipo = MSG_RESPOSTA;
            strcpy(resp.fifo_name,pedido.fifo_name); //ATRIBUIR FIFO Á RESPOSTA
            strcpy(resp.username,pedido.fifo_name); //ATRIBUIR USERNAME Á RESPOSTA
            if (pedido.tipo == MSG_LOGIN){
                int fd_cliente = open(pedido.fifo_name, O_RDWR);
                if (fd_cliente == -1) {
                    perror("[CONTROLADOR] Erro na abertura do named pipe para leitura do cliente");
                    close (fd_cliente);
                    continue;
                }else{
                    memset(&resp, 0, sizeof(resp));
                    resp.tipo = MSG_RESPOSTA;
                    resp.login = LOGIN_REJEITADO; //Valor por omissao! nao sei se vale a pena    
                    if(nClientes < MAXCLI && verficaClienteRegistado(clientesAtivos, pedido.username, nClientes) == 0){
                        printf("[CONTROLADOR] Cliente aceite! E este é o seu Username: %s\n", pedido.username);
                        strcpy(clientesAtivos[nClientes], pedido.username);
                        nClientes++;
                        totalUtilizadoresLigados++;///****estatistica*****

                        resp.login = LOGIN_ACEITE;
                        strcpy(resp.msg, "Login aceite!"); //ATRIBUIR LOGIN ACEITE NA MENSAGEM
                    }
                    else if (nClientes >= MAXCLI ) {
                        totalUtilizadoresRejeitados++;
                        printf("[CONTROLADOR] Cliente nao aceite\n");
                        resp.login = LOGIN_REJEITADO;
                        strcpy(resp.msg, "Limite de utilizadores atingido");
                    }
                    else {
                        printf("[CONTROLADOR] Cliente nao aceite\n");
                        resp.login = LOGIN_REJEITADO;
                        strcpy(resp.msg, "Username já em uso");
                    }
                }
            write(fd_cliente, &resp, sizeof(resp));
            close(fd_cliente);
            }else{ ////AQUI VÊM PARAR AS MENSAGENS SEM SER DE LOGIN (NECESSARIO CRIAR VERIFICACOES PARA VER SE O UTILIZADOR É O MESMO QUE SE LOGOU "MAN IN THE MIDDLE")
                imprimirTipoDePedido(pedido.tipo);
                switch(pedido.tipo){
                    case MSG_AGENDAR:
                    printf("Funcao Agendar\n");
                    break;
                    case MSG_CONSULTAR:
                    printf("Funcao Consultar\n");
                    break;
                    case MSG_CANCELAR:
                    printf("Funcao Cancelar\n");
                    break;
                    case MSG_TERMINAR:
                    printf("Funcao Terminar\n");
                    break;
                    default: 
                    printf("Comando inválido!");
                    break;
                }
            }
        }else 
            break;
    }

    printf("[CONTROLADOR] A encerrar...\n");
    
    pthread_join(thread_tempo, NULL);
    
    /* Encerrar os clientes no array de clientes - enviar mensagem a cada um deles
    for(int i = 0; i < nClientes;i++){

    }

    */
    close(fd_servidor);
    unlink(SERVERFIFO);
    printf("[SISTEMA] Servico esteve em execucao durante: %d segundos!\n",tempo);
    printf("[SISTEMA] Servico recebeu %d tentativas de ligacao!\n",totalTentativasLigacao);
    printf("[SISTEMA] Dessas tentativas de ligacao:\n");
    printf("[SISTEMA] Servico rejeitou %d utilizadores!\n",totalUtilizadoresRejeitados);
    printf("[SISTEMA] Servico autenticou %d utilizadores!\n",totalUtilizadoresLigados);
    printf("[SISTEMA] Servico efetuou %d viagens!\n",TotalServicos);
    printf("[SISTEMA] Encerrado.\n");
    return 0;
}