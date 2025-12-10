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
char username_global[MAX_USERNAME];

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
        
    }  else if (strcmp(cmd.comando, "terminar") == 0) {
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
void handler_pipe(int sig, siginfo_t *siginfo, void *ctx) {
    (void)sig;
    controlador_ativo = 0;
    if(sig == SIGUSR1) printf("\n[CONTROLADOR]O Controlador terminou. Termino forcado do Sistema!\n");
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
    }else printf("[CLIENTE] ✓ Pedido de agendamento enviado\n");
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
    return 0;
}

int processar_comando(ComandoParsed *cmd, int fd_controlador, int fd_privado, const char *username, Mensagem *pedido) {
    
    if (strcmp(cmd->comando, "agendar") == 0) {
        return enviar_agendar(fd_controlador, fd_privado, username, pedido, cmd->hora, cmd->local, cmd->distancia);
        
    } else if (strcmp(cmd->comando, "consultar") == 0) {
        return enviar_consultar(fd_controlador, fd_privado, username, pedido);
        
    } else if (strcmp(cmd->comando, "cancelar") == 0) {
        return enviar_cancelar(fd_controlador, fd_privado, username,pedido, cmd->id);
        
    } else if (strcmp(cmd->comando, "terminar") == 0) {
        // Verificar se está em serviço
        /* if (em_servico) {
            printf("[CLIENTE] Não pode terminar enquanto está em serviço!\n");
            return -1;
        } */
        return enviar_terminar(fd_controlador,fd_privado, username, pedido);
    }
    
    return 0;
}

void * leituraControlador(void * arg){
    int* p = (int*) arg;
    int fd_privado = *p;

    Mensagem controlador;
    memset(&controlador,0,sizeof(controlador));

    while(controlador_ativo){
        int nbytes = read(fd_privado, &controlador, sizeof(controlador));
        if (nbytes == -1) {
            if (errno != EINTR) {
                perror("[CLIENTE] Erro na leitura da resposta!\n");
            }else{ //Este é o erro do sinal!
                perror("[CLIENTE] Encerrar a leitura de mensagens!\n");
            }
            return 0;
        }
        
        if(nbytes > 0){
            switch(controlador.tipo) {
                case MSG_VEICULO:
                    // TELEMETRIA DO VEÍCULO
                    printf("\n %s", controlador.msg);
                    if(controlador.telm.em_viagem == 0) {
                        printf("[CLIENTE] ✓ Viagem concluída! Pode agendar novo serviço.\n");
                    }
                    break;
                case MSG_ACEITA:
                    printf("\n %s\n", controlador.msg);
                    break;
                case MSG_RECUSA:
                    printf("\n %s\n", controlador.msg);
                    break;
                case MSG_TERMINAR:
                    printf("\n[CONTROLADOR] %s\n", controlador.msg);
                    controlador_ativo = 0;
                    break;
                case MSG_CONSULTAR:
                    if(strcmp(controlador.veredito, "ACEITA")==0){
                        printf("\n------------------------SERVICOS---------------------------------\n");
                        printf("%s\n",controlador.msg);
                        printf("\n-----------------------------------------------------------------\n");
                    }else {
                        printf("\n%s\n", controlador.msg);
                    }
                    break;
                default:
                    printf("\n[CONTROLADOR] %s\n", controlador.msg);
                    break;
            }
        }
    }

    if(controlador_ativo) {
        printf("\n%s > ", username_global);
        //fflush(stdout);
    }
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
    memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = handler_pipe;
    sa.sa_flags = SA_SIGINFO;
    sigemptyset(&sa.sa_mask);

    sigaction(SIGUSR1, &sa, NULL);
    sigaction(SIGINT,  &sa, NULL);

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
    strcpy(username_global, username);

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
    usleep(TEMPODEASSINCRONAR);
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
    printf("> Bem-vindo, %s\n", username);
    printf("  %s\n", resposta.msg);
    printf("\n");

    chave = resposta.chave;
    mostrar_ajuda();

    pthread_t thread_telemetria;
    if(pthread_create(&thread_telemetria, NULL, leituraControlador, &fd_privado) != 0){
        perror("[CONTROLADOR] Erro a criar a thread de leitura!");
        exit(1);
    }

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
    
    pthread_cancel(thread_telemetria);
    pthread_join(thread_telemetria, NULL);

    close(fd_privado);
    close(fd_controlador);
    unlink(private_fifo);
    printf("[CLIENTE] Desconectado.\n");

    return 0;
}