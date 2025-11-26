#include "Settings.h"
int running = 1;
int tempo = TEMPO_INICIAL;
Utilizador utilizadores[MAXCLI];
Servico_Marcado marcados[MAX_SERVICES];
int nServicos = 0;
int nClientes = 0;
int idServico = 0;

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

/*POSSIVEL MELHORIA NA SEGURANCA
void mudaChave(int i){
    utilizadores[i].chave = tempo + utilizadores[i].pid;
}
*/

int usernameLogado(char *user) {
    for (int i = 0; i < nClientes; i++) {
        if (strcmp(utilizadores[i].username, user) == 0) {
            return 1;
        }
    }
    return 0;
}

int verficaClienteRegistado (char *user, int pid, int chave){
    for(int i = 0; i < nClientes; i++){
        if(strcmp(utilizadores[i].username, user)==0){
            if(utilizadores[i].chave == chave){
                return i;
            }
        }
    }
    return -1;
}


void adicionaUtilizador (char *user,  char * fifo_name, int pid){
    strcpy(utilizadores[nClientes].username, user);
    strcpy(utilizadores[nClientes].fifo_name, fifo_name);
    utilizadores[nClientes].pid = pid;
    utilizadores[nClientes].chave = tempo;
    utilizadores[nClientes].distancia = 0;
    utilizadores[nClientes].ativo = 1;
    utilizadores[nClientes].pagou = 1;
    utilizadores[nClientes].em_viagem = 0;
    utilizadores[nClientes].servicos_ativos = 0;

    nClientes++;
}

void eliminaUtilizador(char * user,int indice){
    if (indice == -1) {
        printf("[CONTROLADOR] Falha ao tentar remover utilizador não registado: %s\n", user);
        return;
    }
    for(int i = indice; i < nClientes - 1; i++){
        utilizadores[i] = utilizadores[i+1];
    }
    nClientes--;
    printf("[CONTROLADOR] Utilizador removido: %s\n", user);
}

int encontraServicoId(int id, int indiceCliente){
    for(int i = 0; i < nServicos; i++){
        if(utilizadores[indiceCliente].servicos[i].id == id) return i;
    }
    return -1;
}


void eliminaServico(int indice_Serv, int indiceCliente){
    if(indice_Serv != -1){
        for(int i = indice_Serv; i < nServicos - 1; i++){
            printf("[CONTROLADOR] Servico removido: %d\n", utilizadores[indiceCliente].servicos[i].id);
            utilizadores[indiceCliente].servicos[i] = utilizadores[indiceCliente].servicos[i+1];
        }
        nServicos--;
    }
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
            perror("[CONTROLADOR] Erro na leitura do pedido!");
        } else { // interrupção por sinal (CTRL+C)
            perror("[CONTROLADOR] Encerrar a leitura de mensagens!");
        }
        break;
    }

    // Preparar resposta base
    resp.tipo = MSG_RESPOSTA;
    strcpy(resp.fifo_name, pedido.fifo_name); // ATRIBUIR FIFO À RESPOSTA
    strcpy(resp.username, pedido.username);   // ATRIBUIR USERNAME À RESPOSTA

    if (pedido.tipo == MSG_LOGIN) {

        // --- LÓGICA DE LOGIN ---

        resp.tipo = MSG_RECUSA; // por omissão
        if (nClientes < MAXCLI && usernameLogado(pedido.username) == 0) {
            printf("[CONTROLADOR] Cliente aceite! Username: %s\n", pedido.username);
            adicionaUtilizador(pedido.username, pedido.fifo_name, pedido.pid);
            resp.chave=utilizadores[nClientes-1].chave;

            resp.tipo = MSG_ACEITA;
            strcpy(resp.msg, "Login aceite!");
        } else if (nClientes >= MAXCLI) {
            totalUtilizadoresRejeitados++;
            printf("[CONTROLADOR] Cliente nao aceite (limite atingido)\n");
            strcpy(resp.msg, "Limite de utilizadores atingido");
        } else {
            totalUtilizadoresRejeitados++;
            printf("[CONTROLADOR] Cliente nao aceite (username em uso)\n");
            strcpy(resp.msg, "Username já em uso");
        }

    } else {
        // --- MENSAGENS NORMAIS (não-login) ---
        imprimirTipoDePedido(pedido.tipo);
        int indiceCliente = verficaClienteRegistado(pedido.username, pedido.pid, pedido.chave);
        if (indiceCliente == -1) {
            // cliente não autenticado
            strcpy(resp.msg, "[Controlador] Utilizador não autenticado.");
            resp.tipo = MSG_NAOAUTENTICADO;
        } else {
            resp.chave = utilizadores[indiceCliente].chave;
            switch (pedido.tipo) {
                case MSG_AGENDAR:
                    
                    printf("\n----> Funcao Agendar\n");
                    // Falta veiculo e disticao de servico marcado para seguir ou previsto
                    if(nServicos<MAX_SERVICES){
                        resp.tipo = MSG_ACEITA;
                        strcpy(utilizadores[indiceCliente].servicos[nServicos].username,pedido.username);
                        strcpy(utilizadores[indiceCliente].servicos[nServicos].local,pedido.local);
                        utilizadores[indiceCliente].servicos[nServicos].hora = pedido.hora;
                        utilizadores[indiceCliente].servicos[nServicos].distancia = pedido.distancia;
                        utilizadores[indiceCliente].servicos[nServicos].id = ++idServico;
                        sprintf(resp.msg, "[Controlador] Pedido de agendamento de %s recebido: id:%d horas:%dh local:%s distancia: %d",utilizadores[indiceCliente].servicos[nServicos].username, utilizadores[indiceCliente].servicos[nServicos].id, utilizadores[indiceCliente].servicos[nServicos].hora, utilizadores[indiceCliente].servicos[nServicos].local, utilizadores[indiceCliente].servicos[nServicos].distancia);
                        nServicos++;
                    }else{
                        sprintf(resp.msg, "[Controlador] Pedido de agendamento de %s rejeitado!",pedido.username);
                        resp.tipo = MSG_RECUSA;
                    }
                    printf("%s\n",resp.msg);
                    break;

                case MSG_CONSULTAR:
                    resp.tipo = MSG_CONSULTAR;
                    printf("\n----> Funcao Consultar\n");
                    // TODO: implementar consultar
                    strcpy(resp.msg, "[Controlador] Pedido de consulta recebido.");
                    
                    break;

                case MSG_CANCELAR:
                    resp.tipo = MSG_CANCELAR;
                    printf("\n----> Funcao Cancelar\n");
                    // TODO: implementar cancelar
                    int indice = encontraServicoId(pedido.servico_id, indiceCliente);
                    if(indice != -1){
                        eliminaServico(indice,indiceCliente);
                        resp.tipo = MSG_ACEITA;
                        sprintf(resp.msg, "[Controlador] Servico com id: %d cancelado",pedido.servico_id);
                    }else{
                        resp.tipo = MSG_RECUSA;
                        sprintf(resp.msg, "[Controlador] Nao foi possivel efetuar o cancelamento do servico com id: %d",pedido.servico_id);
                    }
                    printf("%s\n",resp.msg);
                    break;
                case MSG_TERMINAR:
                    resp.tipo = MSG_TERMINAR;
                    if (utilizadores[indiceCliente].pagou == 1 &&
                        utilizadores[indiceCliente].em_viagem == 0 &&
                        utilizadores[indiceCliente].servicos_ativos == 0) {
                        
                        
                        eliminaUtilizador(pedido.username,indiceCliente);
                        strcpy(resp.msg, "[Controlador] Espero que tenha gostado dos nossos servicos! Volte sempre!");
                        
                        resp.tipo = MSG_ACEITA;
                    } else {
                        strcpy(resp.msg, "[Controlador] Não pode terminar agora (serviço ativo ou pagamento em falta).");
                        resp.tipo = MSG_RECUSA;
                    }
                    break;
                default:
                    printf("\n ----> Comando inválido!\n");
                    strcpy(resp.msg, "[Controlador] Comando inválido.");
                    break;
            }
        }
    }
    // --- ÚNICO WRITE: enviar resposta ao FIFO do cliente ---
    int fd_cliente = open(pedido.fifo_name, O_WRONLY);
    if (fd_cliente == -1) {
        perror("[CONTROLADOR] Erro na abertura do named pipe do cliente para escrita");
        // não conseguimos responder, mas não rebentamos o servidor
    } else {
        if (write(fd_cliente, &resp, sizeof(resp)) == -1) {
            perror("[CONTROLADOR] Erro ao enviar resposta ao cliente");
        }
        close(fd_cliente);
    }
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























