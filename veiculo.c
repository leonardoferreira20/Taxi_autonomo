#include "Settings.h"

int veiculo_running = 1;

void handler_signal(int sig, siginfo_t *siginfo, void *ctx) {
    (void)sig;
    (void)siginfo;
    (void)ctx;
    veiculo_running = 0;
    /*
    if(sig == SIGUSR1) printf("\n[VEICULO] Sinal recebido, a terminar viagem...\n");
    printf("\n[VEICULO] A interromper veiculo ....\n");
    */
}

int main(int argc, char * argv[]) {
    if (argc != 5) {
        fprintf(stderr, "Uso: %s <id> <local> <distancia> <username>\n", argv[0]);
        exit(1);
    }

    int id = atoi(argv[1]);
    char * local = argv[2];
    int distancia = atoi(argv[3]);
    char * username = argv[4];

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = handler_signal;
    sa.sa_flags = SA_SIGINFO;
    sigemptyset(&sa.sa_mask);

    sigaction(SIGUSR1, &sa, NULL);
    sigaction(SIGINT,  &sa, NULL);

    Telemetria tel;
    memset(&tel, 0, sizeof(tel));

    //sprintf(tel.msg, "[VEICULO #%d] Iniciando viagem de %d km.\n", id, distancia);
    sprintf(tel.msg, "[VEICULO #%d] Chegou ao local de recolha.\n Cliente %s entrou no veiculo.\n Iniciando viagem de %d km.\n", id, username, distancia);

    tel.em_viagem = 1;
    tel.kms = 0;
    write(STDOUT_FILENO, &tel, sizeof(tel));

    int count = 0;
    int percent = 10;   // próximo marco de percentagem (10%, 20%, ...)

    while (veiculo_running && count < distancia) {
        ++count;
        
        // calcular em float a que km corresponde este marco
        float marco_km = distancia * (percent / 100.0);

        if ((float)count >= marco_km && percent <= 100) {
            sprintf(tel.msg, "[VEICULO #%d] %s: %d%% concluida da viagem para o local %s.\n", id, username, percent, local);
            tel.kms = marco_km;
            write(STDOUT_FILENO, &tel, sizeof(tel));

            percent += 10;  // próximo marco
        }
        sleep(TEMPOINSTANTE);
    }

    // Reportar fim
    if (veiculo_running) {
        // Viagem concluída normalmente
        sprintf(tel.msg, "[VEICULO #%d] Viagem concluida! %s chegou ao destino.\n Distancia percorrida: %d km\n Local: %s\n", id, username, distancia, local);
        tel.kms = distancia;
    } else {
        // Viagem cancelada - apenas do controlador
        sprintf(tel.msg, "[VEICULO #%d] Viagem cancelada aos %.0f km.\n Cliente: %s\n Destino: %s\n", id, tel.kms, username, local);
    }
    
    tel.em_viagem = 0;
    write(STDOUT_FILENO, &tel, sizeof(tel));

    return 0;
}