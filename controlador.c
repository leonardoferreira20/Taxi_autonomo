#include "Settings.h"
int running = 1;
int tempo = TEMPO_INICIAL;
Utilizador utilizadores[MAXCLI];
Servico_Marcado marcados[MAX_SERVICES];
Viagem viagens[30];
int nViagens;
int nServicos = 0;
int nClientes = 0;
int idServico = 0;
int kms = 0;
int fd_servidor = -1;

pthread_mutex_t mutex_servicos = PTHREAD_MUTEX_INITIALIZER;

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

    //FALTA TERMINAR SERVICOS ATIVOS

    printf("[CONTROLADOR] Utilizador removido: %s\n", user);
}

int encontraServicoId(int id, int indiceCliente){
    for(int i = 0; i < utilizadores[indiceCliente].servicos_ativos; i++){
        if(utilizadores[indiceCliente].servicos[i].id == id) return i;
    }
    return -1;
}

void listarUtilizadores() {
    printf("\n------------------------------------------------------------\n");
    printf("\n                    Lista de utilizadores                   \n");
    printf("\n------------------------------------------------------------\n");

    if ( nClientes == 0){
        printf("\n Nao existe nenhum utilizador registado.\n");
    }
    else {
        printf("\n    ID  | Username           | PID   | Estado       | S.A   \n");
        printf("\n------------------------------------------------------------\n");

        for ( int i = 0; i < nClientes; ++i ){
            char *estado;

            if (utilizadores[i].em_viagem) {
                estado = "EM VIAGEM";
            } else if (utilizadores[i].servicos_ativos > 0) {
                estado = "À ESPERA";
            } else {
                estado = "LIVRE";
            }

            printf("\n    %d  | %s           | %d   | %s       | %d   \n", i + 1, utilizadores[i].username, utilizadores[i].pid, estado, utilizadores[i].servicos_ativos);

        }
    }
    
    printf("\n------------------------------------------------------------\n");
    printf("\n Total: %d/%d utilizadores\n", nClientes, MAXCLI);
    printf("\n------------------------------------------------------------\n");
}

void listarServicos() {
    printf("\n-----------------------------------------------------------------------------\n");
    printf("\n                             Serviços Agendados                              \n");
    printf("\n-----------------------------------------------------------------------------\n");
    
    int total = 0;
    int encontrou = 0;
    
    for (int i = 0; i < nClientes; i++) {
        total += utilizadores[i].servicos_ativos;
    }
    
    if (total == 0) {
        printf("\n Nao existe nenhum serviço agendado.\n");
    } else {
        printf("\n    ID  | Username           | Hora   | Local       | Distancia | Estado     \n");
        printf("\n-----------------------------------------------------------------------------\n");
        
        for (int i = 0; i < nClientes; i++) {
            for (int j = 0; j < utilizadores[i].servicos_ativos; j++) {
                // Determinar estado (por agora só AGENDADO, depois adicionar EM_EXECUCAO)        
                printf("\n    %d  │ %s           │ %d   │ %s       │ %d │ %s      \n", utilizadores[i].servicos[j].id, utilizadores[i].servicos[j].username, utilizadores[i].servicos[j].hora, utilizadores[i].servicos[j].local, utilizadores[i].servicos[j].distancia, utilizadores[i].servicos[j].estado);
                encontrou = 1;
            }
        }
    }
    
    printf("-----------------------------------------------------------------------------\n");
    printf("   Total: %d serviços agendados                                             \n", total);
    printf("-----------------------------------------------------------------------------\n");
}

void eliminaServico(int indice_Serv, int indiceCliente){
    if (indice_Serv < 0 ||
        indice_Serv >= utilizadores[indiceCliente].servicos_ativos) {
        return;
    }

    int idRemovido = utilizadores[indiceCliente].servicos[indice_Serv].id;

    // Shift à esquerda a partir do removido
    for (int i = indice_Serv;
         i < utilizadores[indiceCliente].servicos_ativos - 1;
         i++) {
        utilizadores[indiceCliente].servicos[i] =
            utilizadores[indiceCliente].servicos[i+1];
    }

    utilizadores[indiceCliente].servicos_ativos--;
    nServicos--;

    printf("[CONTROLADOR] Servico removido: %d\n", idRemovido);
}

void cancelarServicoAdmin(int id){
    if (id == 0) {
        // CANCELAR TODOS OS SERVIÇOS
        printf("[CONTROLADOR] A cancelar TODOS os serviços...\n");
        
        int total_cancelados = 0;
        for (int i = 0; i < nClientes; i++) {
            int servicos_cliente = utilizadores[i].servicos_ativos;
            while (utilizadores[i].servicos_ativos > 0) {
                eliminaServico(0, i);
                total_cancelados++;
            }
        }
        
        printf("[CONTROLADOR] ✓ %d serviços cancelados!\n\n", total_cancelados);
        return;
    }
    
    // CANCELAR SERVIÇO ESPECIFICO
    for (int i = 0; i < nClientes; i++) {
        int indice = encontraServicoId(id, i);
        if (indice != -1) {
            printf("[CONTROLADOR] Serviço #%d encontrado (cliente: %s)\n", 
                   id, utilizadores[i].username);
            
            // TODO: Se estiver EM_EXECUCAO, enviar SIGUSR1 ao veículo
            // if (servico em execução) {
            //     kill(pid_veiculo, SIGUSR1);
            // }
            
            eliminaServico(indice, i);
            printf("[CONTROLADOR] ✓ Serviço #%d cancelado!\n\n", id);
            return;
        }
    }
    
    printf("[CONTROLADOR] ✗ Serviço #%d não encontrado!\n\n", id);
}


void * gestor_comandos_controlador(void * arg){
    while(running){
        char comando [MAXCMD];

        printf("Admin >");
        fflush(stdout);
        if (fgets(comando, sizeof(comando), stdin) == NULL) {
            break;
        }

        comando[strcspn(comando, "\n")] = '\0';
        if (strcmp(comando, "hora") == 0) {
            printf("\n--------------------------------\n");
            printf("\n          Tempo Simulado        \n");
            printf("\n--------------------------------\n");
            printf("\n  Tempo atual: %d segundos      \n ", tempo);
            printf("\n--------------------------------\n");
        }
        else if (strcmp(comando, "km") == 0) {
            printf("\n--------------------------------\n");
            printf("\n     Quilometros percorridos    \n");
            printf("\n--------------------------------\n");
            printf("\n  Tempo atual: %d km.           \n ", kms);
            printf("\n--------------------------------\n");
        }
        else if (strcmp(comando, "utiliz") == 0) {
            listarUtilizadores();
        }
        else if (strcmp(comando, "listar") == 0) {
            listarServicos();
        }
        else if (strncmp(comando, "cancelar", 8) == 0) {
            int id;

            if (sscanf(comando, "cancelar %d", &id) != 1) {
                printf("[CONTROLADOR] ✗ Uso: cancelar <id>\n");
                printf("[CONTROLADOR]   Ou:  cancelar 0  (cancela todos)\n\n");
            } else {
                cancelarServicoAdmin(id);
            }

        }
        else if (strcmp(comando, "frota") == 0) {
            printf("[CONTROLADOR] (frota ainda nao implementada - falta veiculos)\n");
        }
        else if (strcmp(comando, "terminar") == 0) {
            printf("[CONTROLADOR] Comando terminar recebido. A encerrar o sistema...\nCriar mensagem para acordar o controlador\n");
            Mensagem acorda;
            int fd_acorda = open(SERVERFIFO, O_WRONLY);
            if (fd_acorda == -1) {
                perror("[CONTROLADOR] Erro a abrir SERVERFIFO para acordar o main");
            }else {
                strcpy(acorda.msg,"Acorda burro!\n");
                acorda.tipo = MSG_ADMINSHUTDOWN;
                if (write(fd_acorda, &acorda, sizeof(acorda)) == -1) {
                    perror("[CONTROLADOR] Erro a escrever mensagem fake");
                }
                close(fd_acorda);
            }
            running = 0;
        }
        else if (comando[0] != '\0') {
            printf("[CONTROLADOR] Comando invalido: %s\n", comando);
            printf("[CONTROLADOR] Comandos: utiliz, listar, frota, km, hora, cancelar <id>, terminar\n");
        }
    }
    //raise(SIGINT); // força um sinal
    pthread_exit(NULL);
}

void * gestor_tempo(void * arg){
    //IR BUSCAR A VARIAVEL DE AMBIENTE
    const char *valueEnv = getenv(VARAMB);

    //VERIFICAR O VALOR OU SE NAO EXISTE
    if(valueEnv!= NULL){
        int valueAmb = atoi(valueEnv);
        printf("VARAMB = %s\n", valueAmb);
    }else {
        printf("Variavel de ambient nao encontrada!\n");
    }
    
    while(running){
        
        ++tempo;
        
        pthread_mutex_lock(&mutex_servicos);

        for (int i = 0; i < nClientes; i++) {
            for (int j = 0; j < utilizadores[i].servicos_ativos; j++) {

                if(utilizadores[i].servicos[j].hora == tempo && strcmp(utilizadores[i].servicos[j].estado,"Agendado")==0){
                    int fd_taxi[2];
                    //Aqui tentamos criar o pipe anonimo e associar ao nosso fd_taxi
                    //na posicao 0 fica a extremidade de leitura que nao vai ser usada
                    //na posicao 1 fica a extremidade de escrita que vai ser usada
                    //para o veiculo mandar mensagens ao controlador
                    if(pipe(fd_taxi)==-1){
                        perror("Erro na abertura de pipe anonimo");
                    }
                    pid_t pid = fork();
                    if(pid < 0){
                        perror("Failed fork!");
                    }
                    //Este é o codigo que o processo filho
                    //que acabou de ser criado vai executar
                    if(pid == 0){
                        close(fd_taxi[0]); //Fechar o lado de leitura
                        dup2(fd_taxi[1],STDOUT_FILENO); //Estamos a substituir o STDOUT pela extremidade do pipe-anonimo
                        close(fd_taxi[1]);
                        //execl("./veiculo","veiculo",/*local*/,/*distancia*/,NULL);//Aqui transformamos o filho num veiculo e nao estamos a passar argumemntos na linha de comando
                    }else{//isto sera paa o controlador receber as mensagens dos veiculos
                        close(fd_taxi[1]);
                        //O pai vai ler do veiculo na posicao 0 do fd_taxi
                        //serao feitos reads na posicao 0 do fd taxi
                    }}


            }
        }

        pthread_mutex_unlock(&mutex_servicos);

        sleep(TEMPOINSTANTE);
    }
    return NULL;
}

int main(int argc, char * argv[]){
    int TotalServicos = 0;
    int totalTentativasLigacao = 0;
    int totalUtilizadoresLigados = 0;
    int totalUtilizadoresRejeitados = 0;

    //DEFENIR VARIAVEL DE AMBIENTE EM STRING
    if (setenv(VARAMB, "30", 1)!=0){
        perror("setenv failed");
        return 1;
    }

    struct sigaction sa;
    sa.sa_handler = handler_sigint;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);

    // VERIFICA
    if (argc != 1){
        perror("[CONTROLADOR] Numero invalido de parametros!\n");
        exit(-1);
    }

    printf("\n----------------------------------------------------------------------------\n");
    printf("\n                        SISTEMA DE TAXIS AUTONOMOS                          \n");
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
        perror("[CONTROLADOR] Erro a criar a thread de tempo!");
    }

    fd_servidor = open(SERVERFIFO, O_RDWR);
    if (fd_servidor == -1) {
        perror("[CONTROLADOR] Erro na abertura do named pipe para leitura");
        unlink(SERVERFIFO);
        exit(EXIT_FAILURE);
    }

    printf("[CONTROLADOR] Sistema iniciado!\n");
    printf("[CONTROLADOR] Comandos: utiliz, listar, frota, km, hora, terminar\n\n");

    pthread_t thread_linha_comando;
    if(pthread_create(&thread_linha_comando, NULL, gestor_comandos_controlador, NULL) != 0){
        perror("[CONTROLADOR] Erro a criar a thread de comandos do controlador!");
    }

    while (running) {
        Mensagem pedido;
        Mensagem resp;
        memset(&pedido, 0, sizeof(pedido));
        memset(&resp, 0, sizeof(resp));

        int nbytes = read(fd_servidor, &pedido, sizeof(pedido));
        if (nbytes <= 0) {
            if (nbytes == -1 && errno != EINTR) {
                perror("[CONTROLADOR] Erro na leitura do pedido!");
            } else {
                printf("[CONTROLADOR] Leitura terminada (EOF ou interrompida).\n");
            }
            break;
        }

        if(strcmp(pedido.msg,"Acorda burro!\n")==0 && pedido.tipo == MSG_ADMINSHUTDOWN){
            continue;
        };

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
                strcpy(resp.msg, "[CONTROLADOR] Utilizador não autenticado.");
                resp.tipo = MSG_NAOAUTENTICADO;
            } else {
                resp.chave = utilizadores[indiceCliente].chave;
                switch (pedido.tipo) {
                    case MSG_AGENDAR:
                        printf("\n----> Funcao Agendar\n");

                        pthread_mutex_lock(&mutex_servicos);

                        // Falta veiculo e disticao de servico marcado para seguir ou previsto
                        if(nServicos<MAX_SERVICES && utilizadores[indiceCliente].servicos_ativos <MAX_SERVICES){
                            resp.tipo = MSG_ACEITA;
                            int iServico = utilizadores[indiceCliente].servicos_ativos;
                            strcpy(utilizadores[indiceCliente].servicos[iServico].username,pedido.username);
                            strcpy(utilizadores[indiceCliente].servicos[iServico].local,pedido.local);
                            utilizadores[indiceCliente].servicos[iServico].hora = pedido.hora;
                            utilizadores[indiceCliente].servicos[iServico].distancia = pedido.distancia;
                            strcpy(utilizadores[indiceCliente].servicos[iServico].estado,"Agendado");
                            utilizadores[indiceCliente].servicos[iServico].id = ++idServico;
                            utilizadores[indiceCliente].servicos_ativos ++;
                            sprintf(resp.msg, "[CONTROLADOR] Pedido de agendamento de %s recebido: id:%d horas:%dh local:%s distancia: %d",
                                utilizadores[indiceCliente].servicos[iServico].username,
                                utilizadores[indiceCliente].servicos[iServico].id, 
                                utilizadores[indiceCliente].servicos[iServico].hora, 
                                utilizadores[indiceCliente].servicos[iServico].local, 
                                utilizadores[indiceCliente].servicos[iServico].distancia);
                            nServicos++;
                        }else{
                            sprintf(resp.msg, "[CONTROLADOR] Pedido de agendamento de %s rejeitado!",pedido.username);
                            resp.tipo = MSG_RECUSA;
                        }
                        printf("%s\n",resp.msg);

                        pthread_mutex_unlock(&mutex_servicos);
                        
                        break;
                    case MSG_CONSULTAR:
                        resp.tipo = MSG_CONSULTAR;

                        pthread_mutex_lock(&mutex_servicos);

                        if(utilizadores[indiceCliente].servicos_ativos > 0){
                            resp.tipo = MSG_ACEITA;
                            for (int i = 0; i < utilizadores[indiceCliente].servicos_ativos; ++i){
                                sprintf (resp.msg,"%s\n\tServico>> id: %d hora: %d local: %s distancia: %d",
                                    resp.msg,utilizadores[indiceCliente].servicos[i].id,
                                    utilizadores[indiceCliente].servicos[i].hora,
                                    utilizadores[indiceCliente].servicos[i].local,
                                    utilizadores[indiceCliente].servicos[i].distancia);
                            }
                        printf("%s\n",resp.msg);
                        }else{
                            resp.tipo = MSG_RECUSA;
                            sprintf(resp.msg, "[CONTROLADOR] Utilizador %s nao tem nenhum Servico ativo neste momento!",pedido.username);
                            printf("%s\n",resp.msg);
                        }

                        pthread_mutex_unlock(&mutex_servicos);

                        break;
                    case MSG_CANCELAR:
                        int indice = encontraServicoId(pedido.servico_id, indiceCliente);

                        if (pedido.servico_id == 0) {
                            while (utilizadores[indiceCliente].servicos_ativos > 0) {
                                eliminaServico(0, indiceCliente);
                            }
                            strcpy(resp.msg, "[CONTROLADOR] Todos os servicos do utilizador apagados!");
                            resp.tipo = MSG_ACEITA;
                        } else {
                            if (indice != -1) {
                                eliminaServico(indice, indiceCliente);
                                resp.tipo = MSG_ACEITA;
                                sprintf(resp.msg,
                                        "[CONTROLADOR] Servico com id: %d cancelado",
                                        pedido.servico_id);
                            } else {
                                resp.tipo = MSG_RECUSA;
                                sprintf(resp.msg,
                                        "[CONTROLADOR] Nao foi possivel efetuar o cancelamento do servico com id: %d",
                                        pedido.servico_id);
                            }
                        }
                        printf("%s\n", resp.msg);
                        break;
                    case MSG_ENTRAR:
                        break;
                    case MSG_SAIR:
                        break;
                    case MSG_TERMINAR:
                        resp.tipo = MSG_TERMINAR;

                        pthread_mutex_lock(&mutex_servicos);

                        if (utilizadores[indiceCliente].pagou == 1 &&
                            utilizadores[indiceCliente].em_viagem == 0 &&
                            utilizadores[indiceCliente].servicos_ativos == 0) {
                            
                            eliminaUtilizador(pedido.username,indiceCliente);
                            strcpy(resp.msg, "[CONTROLADOR] Espero que tenha gostado dos nossos servicos! Volte sempre!");
                            
                            resp.tipo = MSG_ACEITA;
                        } else {
                            strcpy(resp.msg, "[CONTROLADOR] Não pode terminar agora (serviço ativo ou pagamento em falta).");
                            resp.tipo = MSG_RECUSA;
                        }

                        pthread_mutex_unlock(&mutex_servicos);
                        
                        break;
                    default:
                        printf("\n ----> Comando inválido!\n");
                        strcpy(resp.msg, "[CONTROLADOR] Comando inválido.");
                        break;
                }
            }
        }
        
        // --- "ÚNICO WRITE" (este write serve para mensagem): enviar resposta ao FIFO do cliente ---
        //           pode existir mais writes mas sao no switch com continue (ex consultar)
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
    
    pthread_cancel(thread_tempo);
    pthread_join(thread_tempo, NULL);
    pthread_cancel(thread_linha_comando); //Cancela a thread para sair do fgets se é bonito ou nao nao sei mas funciona!!!
    pthread_join(thread_linha_comando, NULL);
    
    //Encerrar os clientes no array de clientes - enviar mensagem a cada um deles
    union sigval sigValue;
    for (int i = 0; i < nClientes; i++){
        sigqueue(utilizadores[i].pid, SIGUSR1, sigValue);
        /*
        int fd_cliente = open(utilizadores[i].fifo_name, O_WRONLY);
        if (fd_cliente == -1) {
            perror("[CONTROLADOR] Erro na abertura do named pipe do cliente para escrita");
            // não conseguimos responder, mas não rebentamos o servidor
        } else {
            if (write(fd_cliente, &resp, sizeof(resp)) == -1) {
                perror("[CONTROLADOR] Erro ao enviar resposta ao cliente");
            }
            close(fd_cliente);
        }
        write(utilizadores[i].fifo_name);
        */
    }


    close(fd_servidor);
    unlink(SERVERFIFO);

    return 0;
}
