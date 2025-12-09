#include "Settings.h"
int veiculo_running = 1;

void handler_signal(int sig, siginfo_t *siginfo, void *ctx) {
    (void)sig;
    (void)siginfo;
    (void)ctx;
    veiculo_running = 0;
    if(sig == SIGUSR1) printf("\n[VEICULO] Sinal recebido, a terminar viagem...\n");
}

int main(int argc, char * argv[]) {
    printf("%d",argc);
    if (argc != 5) {
        fprintf(stderr, "Uso: %s <id> <local> <distancia>\n", argv[0]);
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

    // Contactar cliente
    // Reportar início (write para controlador)
    //printf("[VEICULO #%d] Cliente entrou. Iniciando viagem de %d km.\n", id, distancia);
    //fflush(stdout);

    Telemetria tel;
    memset(&tel, 0, sizeof(tel));

    sprintf(tel.msg, "[VEICULO #%d] Iniciando viagem de %d km.\n", id, distancia);
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

    sprintf(tel.msg,"[VEICULO #%d] Viagem concluída! \n ->%d km\n ->%s", id, distancia, local);
    tel.em_viagem = 0;
    write(STDOUT_FILENO, &tel, sizeof(tel));

    return 0;
}








/*
int main(int argc, char * argv[]){
  int kms = 0;
  int destino = *argv[3];

  if (argc!=2){
    perror("[sistema] Numero invalido de parametros!\n");
    exit(-1);
  }

  while(kms != destino){
    ++kms;
    printf("alalala");

    sleep (TEMPOINSTANTE);
  }


  return 0;
}
  */