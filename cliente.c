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

int controlador_ativo = 1;


// ALTERAR
ComandoParsed parsear_comando(const char *linha) {
    ComandoParsed cmd;
    memset(&cmd, 0, sizeof(cmd));
    cmd.valido = 0;
    cmd.id = -1; // Valor inválido por defeito
    
    // Criar cópia da linha
    char buffer[MAXCMD];
    strncpy(buffer, linha, MAXCMD - 1);
    buffer[MAXCMD - 1] = '\0';
    
    // Remover \n
    buffer[strcspn(buffer, "\n")] = '\0';
    
    // Remover espaços iniciais
    char *p = buffer;
    while (*p == ' ' || *p == '\t') p++;
    
    // Se vazio, retornar inválido
    if (*p == '\0') {
        return cmd;
    }
    
    // Separar comando (primeira palavra)
    char *saveptr;
    char *token = strtok_r(p, " \t", &saveptr);
    if (token == NULL) {
        return cmd;
    }
    
    // Copiar e converter para minúsculas
    strncpy(cmd.comando, token, sizeof(cmd.comando) - 1);
    for (int i = 0; cmd.comando[i]; i++) {
        cmd.comando[i] = tolower(cmd.comando[i]);
    }
    
    // ========== PARSE ESPECÍFICO POR COMANDO ==========
    
    if (strcmp(cmd.comando, "agendar") == 0) {
        // agendar <hora> <local> <distancia>
        
        token = strtok_r(NULL, " \t", &saveptr);
        if (token == NULL) {
            printf("[CLIENTE] Erro: falta argumento <hora>\n");
            printf("Uso: agendar <hora> <local> <distancia>\n");
            return cmd;
        }
        cmd.hora = atoi(token);
        if (cmd.hora < 0) {
            printf("[CLIENTE] Erro: hora inválida\n");
            return cmd;
        }
        
        token = strtok_r(NULL, " \t", &saveptr);
        if (token == NULL) {
            printf("[CLIENTE] Erro: falta argumento <local>\n");
            printf("Uso: agendar <hora> <local> <distancia>\n");
            return cmd;
        }
        strncpy(cmd.local, token, MAX_LOCAL - 1);
        
        token = strtok_r(NULL, " \t", &saveptr);
        if (token == NULL) {
            printf("[CLIENTE] Erro: falta argumento <distancia>\n");
            printf("Uso: agendar <hora> <local> <distancia>\n");
            return cmd;
        }
        cmd.distancia = atoi(token);
        if (cmd.distancia <= 0) {
            printf("[CLIENTE] Erro: distância inválida\n");
            return cmd;
        }
        
        cmd.valido = 1;
        
    } else if (strcmp(cmd.comando, "consultar") == 0) {
        // consultar (sem argumentos)
        cmd.valido = 1;
        
    } else if (strcmp(cmd.comando, "cancelar") == 0) {
        // cancelar <id>
        
        token = strtok_r(NULL, " \t", &saveptr);
        if (token == NULL) {
            printf("[CLIENTE] Erro: falta argumento <id>\n");
            printf("Uso: cancelar <id>  (0 = cancelar todos)\n");
            return cmd;
        }
        cmd.id = atoi(token);
        
        cmd.valido = 1;
        
    } else if (strcmp(cmd.comando, "entrar") == 0) {
        // entrar <destino>
        
        token = strtok_r(NULL, " \t", &saveptr);
        if (token == NULL) {
            printf("[CLIENTE] Erro: falta argumento <destino>\n");
            printf("Uso: entrar <destino>\n");
            return cmd;
        }
        strncpy(cmd.destino, token, MAX_DESTINO - 1);
        
        cmd.valido = 1;
        
    } else if (strcmp(cmd.comando, "sair") == 0) {
        // sair (sem argumentos)
        cmd.valido = 1;
        
    } else if (strcmp(cmd.comando, "terminar") == 0) {
        // terminar (sem argumentos)
        cmd.valido = 1;
        
    } else {
        // Comando desconhecido
        printf("[CLIENTE] Comando desconhecido: %s\n", cmd.comando);
        return cmd;
    }
    
    return cmd;
}

// HANDLER PARA DETETAR MORTE DO CONTROLADOR (SIGPIPE)
void handler_pipe(int sig) {
    (void)sig;
    controlador_ativo = 0;
    printf("\n[CLIENTE] A interromper cliente ....\n");
}

// MOSTRAR MENU DE COMANDOS
void mostrar_ajuda() {
    printf("\n-------------------------------------------------------\n");
    printf("\n                 COMANDOS DISPONÍVEIS                  \n");
    printf("\n-------------------------------------------------------\n");
    printf("\t> agendar <hora> <local> <distancia>\n");
    printf("\t> consultar\n");
    printf("\t> cancelar <id>     (0 = cancelar todos)\n");
    printf("\t> entrar <destino>\n");
    printf("\t> sair\n");
    printf("\t> terminar");
    printf("\n-------------------------------------------------------\n");
}

// COMANDO AGENDAR
int enviar_agendar(int fd, int fd_privado, const char *username, Mensagem *pedido, int hora, const char *local, int distancia) {
    pedido->tipo = MSG_AGENDAR;
    pedido->hora = hora;
    strncpy(pedido->local, local, MAX_LOCAL - 1);
    pedido->local[MAX_LOCAL - 1] = '\0';
    pedido->distancia = distancia;
    
    if (write(fd, pedido, sizeof(*pedido)) == -1) {
        perror("[CLIENTE] Erro ao enviar comando agendar");
        return -1;
    }
    
    printf("[CLIENTE] ✓ Pedido de agendamento enviado\n");

    Mensagem resposta;
    memset(&resposta, 0, sizeof(resposta));
    usleep(TEMPODEASSINCRONAR);

    int nbytes = read(fd_privado, &resposta, sizeof(resposta));
    if (nbytes == -1) {
        if (errno != EINTR) {
            perror("[CLIENTE] Erro na leitura da resposta!\n");
        }else{ //Este é o erro do sinal!
            perror("[CLIENTE] Encerrar a leitura de mensagens!\n");
        }
        return -1;
    }

    if(nbytes > 0){
        printf("%s\n",resposta.msg);
        if(resposta.tipo == MSG_RECUSA){
            printf("[CLIENTE] Pedido de agendamento recusado\n");
        }
    }

    return 0;
}

// COMANDO CONSULTAR
int enviar_consultar(int fd, int fd_privado, const char *username, Mensagem *pedido) {

    pedido->tipo = MSG_CONSULTAR;
    if (write(fd, pedido, sizeof(*pedido)) == -1) {
        perror("[CLIENTE] Erro ao enviar comando consultar");
        return -1;
    }
    
    printf("[CLIENTE] ✓ Pedido de consulta enviado\n");

    Mensagem resposta;
    memset(&resposta, 0, sizeof(resposta));
    usleep(100 * 10000);
    int nbytes = read(fd_privado, &resposta, sizeof(resposta));
    if (nbytes == -1) {
        if (errno != EINTR) {
            perror("[CLIENTE] Erro na leitura da resposta!\n");
        }else{ //Este é o erro do sinal!
            perror("[CLIENTE] Encerrar a leitura de mensagens!\n");
        }
        return 0;
    }
    if(nbytes > 0){
        if(resposta.tipo == MSG_ACEITA){
            printf("\n------------------------SERVICOS---------------------------------\n");
            printf("%s\n",resposta.msg);
            printf("\n-----------------------------------------------------------------\n");
        }else{
            printf("%s\n",resposta.msg);
            printf("[CLIENTE] Pedido de consulta recusado!\n");
        }
    }
    return 0;
}

// COMANDO CANCELAR
int enviar_cancelar(int fd,int fd_privado, const char *username,Mensagem *pedido, int id) {
    
    pedido->tipo = MSG_CANCELAR;
    pedido->servico_id = id;
    
    if (write(fd, pedido, sizeof(*pedido)) == -1) {
        perror("[CLIENTE] Erro ao enviar comando cancelar");
        return -1;
    }
    
    if (id == 0) {
        printf("[CLIENTE] ✓ Pedido para cancelar todos os serviços enviado\n");
    } else {
        printf("[CLIENTE] ✓ Pedido para cancelar serviço #%d enviado\n", id);
    }

    Mensagem resposta;
    memset(&resposta, 0, sizeof(resposta));
    usleep(100 * 10000);
    int nbytes = read(fd_privado, &resposta, sizeof(resposta));
    if (nbytes == -1) {
        if (errno != EINTR) {
            perror("[CLIENTE] Erro na leitura da resposta!\n");
        }else{ //Este é o erro do sinal!
            perror("[CLIENTE] Encerrar a leitura de mensagens!\n");
        }
        return 0;
    }

    if(nbytes > 0){
        printf("%s\n",resposta.msg);
    }

    return 0;
}

// COMANDO TERMINAR
int enviar_terminar(int fd, int fd_privado, const char *username, Mensagem *pedido) {
    pedido->tipo = MSG_TERMINAR;
    
    if (write(fd, pedido, sizeof(*pedido)) == -1) {
        perror("[CLIENTE] Erro ao enviar comando terminar");
        return -1;
    }
    printf("[CLIENTE] ✓ Pedido de término enviado\n");
    sleep(1);

    Mensagem resposta_termino;
    memset(&resposta_termino, 0, sizeof(resposta_termino));
    usleep(100 * 10000);
    int nbytes = read(fd_privado, &resposta_termino, sizeof(resposta_termino));
    if (nbytes == -1) {
        if (errno != EINTR) {
        perror("[CLIENTE] Erro na leitura da resposta!\n");
        }else{ //Este é o erro do sinal!
            perror("[CLIENTE] Encerrar a leitura de mensagens!\n");
        }
        return 0;
    }
    if(nbytes > 0){
        if(resposta_termino.tipo == MSG_RECUSA){
            printf("%s\n",resposta_termino.msg);
            return 1;
        }
        printf("%d",resposta_termino.tipo);
        printf("%s\n",resposta_termino.msg);
    }
    return 0;
}

int processar_comando(ComandoParsed *cmd, int fd_controlador, int fd_privado, const char *username, Mensagem *pedido) {
    
    if (strcmp(cmd->comando, "agendar") == 0) {
        return enviar_agendar(fd_controlador, fd_privado, username, pedido, cmd->hora, cmd->local, cmd->distancia);
        
    } else if (strcmp(cmd->comando, "consultar") == 0) {
        return enviar_consultar(fd_controlador, fd_privado, username, pedido);
        
    } else if (strcmp(cmd->comando, "cancelar") == 0) {
        return enviar_cancelar(fd_controlador, fd_privado, username,pedido, cmd->id);
        
    } else if (strcmp(cmd->comando, "entrar") == 0) {
        // TODO: Enviar para veículo (não controlador)
        printf("[CLIENTE] TODO: enviar 'entrar' para veículo\n");
        return 0;
        
    } else if (strcmp(cmd->comando, "sair") == 0) {
        // TODO: Enviar para veículo (não controlador)
        printf("[CLIENTE] TODO: enviar 'sair' para veículo\n");
        return 0;
        
    } else if (strcmp(cmd->comando, "terminar") == 0) {
        // Verificar se está em serviço
        /* if (em_servico) {
            printf("[CLIENTE] Não pode terminar enquanto está em serviço!\n");
            printf("[CLIENTE] Aguarde o fim da viagem ou use 'sair'\n");
            return -1;
        } */
        return enviar_terminar(fd_controlador,fd_privado, username, pedido);
    }
    
    return 0;
}

int main(int argc, char* argv[]){
    setbuf(stdout, NULL);

    int chave = 0;
    char * username;
    char private_fifo[MAX_MSG];
    Mensagem pedido;
    Mensagem resposta;
    memset(&pedido, 0, sizeof(pedido));
    memset(&resposta, 0, sizeof(resposta));
    
    struct sigaction sa;
    sa.sa_handler = handler_pipe;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO;
    sigaction(SIGINT, &sa, NULL);

    // VERIFICACAO DE NUMERO DE ARGUMENTOS
    if (argc != 2){
        perror("Numero invalido de parametros!\n");
        exit(1);
    }

    printf("[CLIENTE] Controlador detetado.\n");

    // ATRIBUIR NOME DO UTILIZADOR E VALIDAR SE ULTRAPASSA O MÁXIMO DEFINIDO 
    // Escrita para o servidor autenticacao
    // Cliente envia o seu username para o servidor para se autenticar
    // Se não existir outro utilizador com este username e ainda houver espaço para o utilizador (servidor tem numeo maximo de utilizadores) O login é bem sucedido
    // write(fd,argv[1],strlen(argv[1])+1);

    username = argv[1];

    if (strlen(username) >= MAX_USERNAME) {
        fprintf(stderr, "[CLIENTE] ERRO: Username demasiado longo (max %d)\n", MAX_USERNAME - 1);
        exit(1);
    }
    
    printf("[CLIENTE] Utilizador: %s\n", username);

    if (access(SERVERFIFO, F_OK) != 0) {
        fprintf(stderr, "[CLIENTE] ERRO: Controlador não está ativo!\n");
        fprintf(stderr, "[CLIENTE] Por favor, inicie o controlador primeiro.\n");
        exit(1);
    }
    
    // CRIAR FIFO PRIVADO PARA O CLIENTE
    snprintf(private_fifo, sizeof(private_fifo), "%s%s_%d", CLIENTE_FIFO_PREFIX, username, getpid());
       
    if( mkfifo(private_fifo, 0666) == -1 ){
        perror("[CLIENTE] Erro a criar pipe do cliente!\n");
        exit(-1);
    }

    printf("[CLIENTE] FIFO privado criado: %s\n", private_fifo);

    //signal(SIGPIPE, handler_pipe);

    // ABRIR FIFO DO CONTROLADOR
    // DEFINIDO NAS SETTINGS O NOME DO SERVIDOR
    // ACESSO AO PIPE ABERTO PELO SERVIDOR (JÁ DEVE ESTAR ABERTO DO LADO DO SERVIDOR)
    int fd_controlador = open(SERVERFIFO, O_WRONLY);
    if (fd_controlador == -1) {
        perror("[CLIENTE] Erro ao conectar ao controlador");
        unlink(private_fifo);
        exit(1);
    }
    
    printf("[CLIENTE] Conectado ao controlador.\n");

    // ENVIAR PEDIDO DE LOGIN
    strncpy(pedido.username, username, MAX_USERNAME - 1);
    pedido.username[MAX_USERNAME - 1] = '\0';
    pedido.pid = getpid();
    pedido.chave = chave;
    
    strncpy(pedido.fifo_name, private_fifo, MAX_MSG - 1);
    pedido.fifo_name[MAX_MSG - 1] = '\0';
    pedido.tipo = MSG_LOGIN;

    if (write(fd_controlador, &pedido, sizeof(pedido)) == -1) {
        perror("[CLIENTE] Erro ao enviar login");
        close(fd_controlador);
        unlink(private_fifo);
        exit(1);
    }
    
    printf("[CLIENTE] ✓ Pedido de login enviado. A aguardar validação...\n");

    // ABRIR FIFO PRIVADO PARA LEITURA
    int fd_privado = open(private_fifo, O_RDONLY);
    if (fd_privado == -1) {
        perror("[CLIENTE] Erro ao abrir FIFO privado");
        close(fd_controlador);
        unlink(private_fifo);
        exit(1);
    }

    // AGUARDAR RESPSOTA DO LOGIN
    usleep(100 * 10000);
    ssize_t nBytes = read(fd_privado, &resposta, sizeof(resposta));
    if (nBytes <= 0) {
        fprintf(stderr, "[CLIENTE] Erro ao receber resposta do login\n");
        close(fd_privado);
        close(fd_controlador);
        unlink(private_fifo);
        exit(1);
    }

    if (resposta.tipo == MSG_RECUSA) {
        printf("\n");
        printf("LOGIN REJEITADO\n");
        printf("> ERRO: %s\n", resposta.msg);
        printf("\n");
        close(fd_privado);
        close(fd_controlador);
        unlink(private_fifo);
        exit(1);
    }

    printf("\n");
    printf("LOGIN ACEITE\n");
    printf("> Bem-vindo, %-40s ║\n", username);
    printf("  %s\n", resposta.msg);
    printf("\n");

    chave = resposta.chave;
    mostrar_ajuda();

    char linha[MAXCMD];
    int terminar = 0;
    while (!terminar && controlador_ativo) {
        memset(&pedido, 0, sizeof(pedido));

        strncpy(pedido.username, username, MAX_USERNAME - 1);
        pedido.username[MAX_USERNAME - 1] = '\0';
        pedido.pid = getpid();
        strncpy(pedido.fifo_name, private_fifo, MAX_MSG - 1);
        pedido.fifo_name[MAX_MSG - 1] = '\0';
        pedido.chave = chave;

        printf("\n%s > ", username);
        fflush(stdout);
        
        if (fgets(linha, sizeof(linha), stdin) == NULL) {
            break;
        }
        
        ComandoParsed cmd = parsear_comando(linha);
        
        if (!cmd.valido) {
            continue;
        }
        
        if (strcmp(cmd.comando, "terminar") == 0) {
            if (processar_comando(&cmd, fd_controlador, fd_privado, username, &pedido) == 0) {
                terminar = 1;
            }
        } else {
            processar_comando(&cmd, fd_controlador, fd_privado, username, &pedido);
        }
    }
    
    printf("\n\n[CLIENTE] A terminar...\n");
    close(fd_privado);
    close(fd_controlador);
    unlink(private_fifo);
    printf("[CLIENTE] Desconectado.\n");

    return 0;
}