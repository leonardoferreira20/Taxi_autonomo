#include "Settings.h"
int running = 1;
int tempo = TEMPO_INICIAL;
Utilizador utilizadores[MAXCLI];
int nClientes = 0;

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


int verficaClienteRegistado (char *user, int pid){
    for(int i = 0; i < nClientes; i++){
        if(strcmp(utilizadores[i].username, user)==0){
            return i;
        }
    }
    return -1;
}

void adicionaUtilizador (char *user,  char * fifo_name, int pid){
    strcpy(utilizadores[nClientes].username, user);
    strcpy(utilizadores[nClientes].fifo_name, fifo_name);
    utilizadores[nClientes].pid = pid;
    utilizadores[nClientes].distancia = 0;
    utilizadores[nClientes].ativo = 1;
    utilizadores[nClientes].pagou = 1;
    utilizadores[nClientes].em_viagem = 0;
    utilizadores[nClientes].servico_ativo = -1;

    nClientes++;
}

void eliminaUtilizador(char * user, char * fifo_name, int pid){
    int cliente = verficaClienteRegistado(user,pid);
    if (cliente == -1) {
        printf("[CONTROLADOR] Falha ao tentar remover utilizador não registado: %s\n", user);
        return;
    }
    for(int i = cliente; i < nClientes - 1; i++){
        utilizadores[i] = utilizadores[i+1];
    }
    nClientes--;
    printf("[CONTROLADOR] Utilizador removido: %s\n", user);
}


int main(int argc, char * argv[]){
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

    printf("\n----------------------------------------------------------------------------\n");
    printf("\n                        SISTEMA DE TÁXIS AUTÓNOMOS                          \n");
    printf("\n----------------------------------------------------------------------------\n\n");

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
    // LANÇAR THREAD DE GESTÃO DO TEMPO
    pthread_t thread_tempo;
    if(pthread_create(&thread_tempo, NULL, gestor_tempo, NULL) != 0){
        perror("[CONTROLADOR] Erro a criar a thread");
    }

    int fd_servidor = open(SERVERFIFO, O_RDWR);
    if (fd_servidor == -1) {
        perror("[CONTROLADOR] Erro na abertura do named pipe para leitura");
        unlink(SERVERFIFO);
        exit(EXIT_FAILURE);
    }

    printf("[CONTROLADOR] Sistema iniciado!\n");
    printf("[CONTROLADOR] Comandos: utiliz, listar, frota, km, hora, terminar\n\n");

    while (running) {
        Mensagem pedido;
        Mensagem resp;
        memset(&pedido, 0, sizeof(pedido));
        memset(&resp, 0, sizeof(resp));

        int nbytes = read(fd_servidor, &pedido, sizeof(pedido));
        if (nbytes == -1) {
            if (errno != EINTR) {
            perror("[CONTROLADOR] Erro na leitura do pedido!\n");
            }else{ //Este é o erro do sinal!
            perror("[CONTROLADOR] Encerrar a leitura de mensagens!\n");
            }
        break;
        }

        if (nbytes > 0) {
            totalTentativasLigacao++;///****estatistica*****
            resp.tipo = MSG_RESPOSTA;
            strcpy(resp.fifo_name,pedido.fifo_name); //ATRIBUIR FIFO Á RESPOSTA
            strcpy(resp.username,pedido.username); //ATRIBUIR USERNAME Á RESPOSTA
            if (pedido.tipo == MSG_LOGIN){
                int fd_cliente = open(pedido.fifo_name, O_RDWR);
                if (fd_cliente == -1) {
                    perror("[CONTROLADOR] Erro na abertura do named pipe para leitura do cliente");
                    continue;
                }else{
                    resp.tipo = MSG_RESPOSTA;
                    resp.login = LOGIN_REJEITADO; //Valor por omissao 
                    if(nClientes < MAXCLI && verficaClienteRegistado(pedido.username, pedido.pid) == -1){
                        printf("[CONTROLADOR] Cliente aceite! E este é o seu Username: %s\n", pedido.username);
                        adicionaUtilizador(pedido.username,pedido.fifo_name, pedido.pid);
                        totalUtilizadoresLigados++;///****estatistica*****

                        resp.login = LOGIN_ACEITE;
                        strcpy(resp.msg, "Login aceite!"); //ATRIBUIR LOGIN ACEITE NA MENSAGEM
                    }else if (nClientes >= MAXCLI ) {
                        totalUtilizadoresRejeitados++;
                        printf("[CONTROLADOR] Cliente nao aceite\n");
                        strcpy(resp.msg, "Limite de utilizadores atingido");
                    }else {
                        printf("[CONTROLADOR] Cliente nao aceite\n");
                        strcpy(resp.msg, "Username já em uso");
                    }
                }
                write(fd_cliente, &resp, sizeof(resp));
                close(fd_cliente);
            }else{ ////AQUI VÊM PARAR AS MENSAGENS SEM SER DE LOGIN (NECESSARIO CRIAR VERIFICACOES PARA VER SE O UTILIZADOR É O MESMO QUE SE LOGOU "MAN IN THE MIDDLE")
                int fd_cliente = open(pedido.fifo_name, O_RDWR);
                if (fd_cliente == -1) {
                    perror("[CONTROLADOR] aaa Erro na abertura do named pipe para leitura do cliente");
                    continue;
                }
                imprimirTipoDePedido(pedido.tipo);
                int indiceCliente = verficaClienteRegistado(pedido.username, pedido.pid);
                
                switch(pedido.tipo){
                    case MSG_AGENDAR:
                    printf("\n----> Funcao Agendar\n");
                    break;
                    case MSG_CONSULTAR:
                    printf("\n----> Funcao Consultar\n");
                    break;
                    case MSG_CANCELAR:
                    printf("\n----> Funcao Cancelar\n");
                    break;
                    case MSG_TERMINAR:
                    if(utilizadores[indiceCliente].pagou == 1 && utilizadores[indiceCliente].em_viagem == 0 && utilizadores[indiceCliente].servico_ativo == -1){
                        eliminaUtilizador(pedido.username, pedido.fifo_name, pedido.pid);
                        strcpy(resp.msg,"[Controlador] Espero que tenha gostado dos nossos servicos! Volte sempre!");
                        resp.encerra = TERMINO_ACEITE;
                    }else{
                        strcpy(resp.msg,"[Controlador] Não pode terminar agora (serviço ativo ou pagamento em falta).");
                        resp.encerra = TERMINO_REJEITADO;
                    }
                    break;
                    default: 
                    printf("\n ----> Comando inválido!");
                    break;
                }
                write(fd_cliente, &resp, sizeof(resp));
                close(fd_cliente);
            }
        }else 
            break;
        printf("AQUI WHILE:%s\n",pedido.fifo_name);
    }

    printf("[CONTROLADOR] A encerrar...\n");
    
    pthread_join(thread_tempo, NULL);
    
    /* Encerrar os clientes no array de clientes - enviar mensagem a cada um deles
    for(int i = 0; i < nClientes;i++){
        
    }

    */
    close(fd_servidor);
    unlink(SERVERFIFO);

    return 0;
}

























/*
void print1(){
        printf("\n------------------------------------INFO------------------------------------\n\n");
    printf("[SISTEMA] Servico esteve em execucao durante: %d segundos!\n",tempo);
    printf("[SISTEMA] Servico recebeu %d tentativas de ligacao!\n",totalTentativasLigacao);
    printf("[SISTEMA] Dessas tentativas de ligacao:\n");
    printf("             >Servico rejeitou %d utilizadores!\n",totalUtilizadoresRejeitados);
    printf("             >Servico autenticou %d utilizadores!\n",totalUtilizadoresLigados);
    printf("[SISTEMA] Servico efetuou %d viagens!\n",TotalServicos);
    printf("\n----------------------------------ENCERRAR----------------------------------\n");
    printf("[SISTEMA] Encerrado.");
    printf("\n----------------------------------------------------------------------------\n");
    printf("\n\n");

    printf("             _________                          _X_\n");
    printf("            //  ||\\ \\                        // \\\n");
    printf("  \\  _____//___||_\\ \\___       ~ ~ ~ ~ ~~~~  | |\n");
    printf("  -->=)  _           _    \\       ~ ~ ~ ~ ~~~~ | |\n");
    printf("  //  |_/ \\________/ \\___|       ~ ~ ~ ~ ~~~~ | |\n");
    printf("________\\_/________\\_/________________________| |\n");
    printf("________________________________________________| |\n");
}

*/