#include "keyboard.h"
#include "screen.h"
#include "timer.h"
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define MIN_X 1
#define MAX_X 79
#define MIN_Y 1
#define MAX_Y 23
#define TAM_RAQUETE 10
#define NUM_TIJOLOS 45
#define FPS 60
#define DELAY_FRAME (1000 / FPS)

typedef struct Objeto {
  double x;
  double y;
  double velX;
  double velY;
} Objeto;

typedef struct Tijolo {
  double x;
  double y;
  int durabilidade;
  struct Tijolo *prox;
} Tijolo;

Objeto raquete, bola;
Tijolo *tijolos = NULL;
int placarAtual = 0, highScore = 0;

void wait_ms(int ms);
void printarTijolos();
void liberarTijolos();
void moverRaquete();
void moverBola();
void printarPlacar();
void salvarHighScore();
void escreverHighScore();
void drawGame();
void apresentarMensagem();
void printarCentralizado(char *text, int y);

void inicializar() {
  screenInit(1);
  keyboardInit();
  escreverHighScore();
  printarTijolos();
  raquete.x = (MAX_X / 2) - (TAM_RAQUETE / 2);
  raquete.y = MAX_Y - 2;
  bola.x = raquete.x + (TAM_RAQUETE / 2);
  bola.y = raquete.y - 1;
  bola.velX = 0.28;
  bola.velY = -0.28;
  placarAtual = 0;
  screenClear();
}

void drawGame() {
  screenClear();
  printarPlacar();
  printf("\033[%d;%dH", (int)raquete.y, (int)raquete.x);
  for (int i = 0; i < TAM_RAQUETE; i++) {
      printf("=");
  }
  printf("\033[%d;%dHo", (int)bola.y, (int)bola.x);

  Tijolo *atual = tijolos;
  while (atual) {
    if (atual->durabilidade > 0) {
      printf("\033[%d;%dH[]", (int)atual->y, (int)atual->x);
    }
    atual = atual->prox;
  }
  screenUpdate();
}

void loopJogo() {
  bool jogoAtivo = true;
  timerInit(DELAY_FRAME);
  while (jogoAtivo) {
    if (timerTimeOver()) {
      moverRaquete();
      moverBola();
      drawGame();
      if (tijolos == NULL || bola.y >= MAX_Y) {
        jogoAtivo = false;
      }
      timerUpdateTimer(DELAY_FRAME);
    }
    if (!jogoAtivo) {
      apresentarMensagem();
    }
  }
}

Tijolo* criarTijolo(double x, double y) {
  Tijolo *novo = malloc(sizeof(Tijolo));
  if (novo) {
    novo->x = x;
    novo->y = y;
    novo->durabilidade = 1;
    novo->prox = NULL;
  }
  return novo;
}

void printarTijolos() {
  int linhas = 3;
  int tijolos_por_linha = (MAX_X - 10) / 5;
  for (int linha = 0; linha < linhas; linha++) {
    for (int i = 0; i < tijolos_por_linha; i++) {
      double x = 5 + i * 5;
      double y = 2 + linha * 2;
      Tijolo *novo = criarTijolo(x, y);
      if (novo) {
        novo->prox = tijolos;
        tijolos = novo;
      }
    }
  }
}

void liberarTijolos() {
  while (tijolos) {
    Tijolo *temp = tijolos;
    tijolos = tijolos->prox;
    free(temp);
  }
}

void finalizar() {
  liberarTijolos();
  keyboardDestroy();
  screenDestroy();
  salvarHighScore();
}

void printarPlacar() {
  printf("\033[%d;%dHPlacar: %d", 0, 0, placarAtual);
}

void escreverHighScore() {
  FILE *file = fopen("highscore.txt", "r");
  if (file) {
    fscanf(file, "%d", &highScore);
    fclose(file);
  } else {
      highScore = 0;
  }
}

void salvarHighScore() {
  FILE *file = fopen("highscore.txt", "w");
  if (file) {
    if (placarAtual > highScore) {
      highScore = placarAtual;
    }
    fprintf(file, "%d", highScore);
    fclose(file);
  }
}

void apresentarMensagem() {
  screenClear();
  if (tijolos == NULL) {
    printarCentralizado("Parabéns, você venceu!", MAX_Y / 2 - 1);
  } else {
    printarCentralizado("Game Over", MAX_Y / 2 - 1);
  }
  char message[50];
  sprintf(message, "Score Atual: %d", placarAtual);
  printarCentralizado(message, MAX_Y / 2);
  sprintf(message, "HighScore: %d", highScore);
  printarCentralizado(message, MAX_Y / 2 + 1);
  printarCentralizado("Pressione S para sair ou C para continuar", MAX_Y / 2 + 3);

  char escolha;
  do {
    scanf(" %c", &escolha);
  } while (escolha != 'S' && escolha != 's' && escolha != 'C' && escolha != 'c');

  if (escolha == 'S' || escolha == 's') {
    finalizar();
    exit(0);
  } else if (escolha == 'C' || escolha == 'c') {
    inicializar();
    loopJogo();
  }
}

void printarCentralizado(char *text, int y) {
  int len = strlen(text);
  int x = (MAX_X - len) / 2;
  printf("\033[%d;%dH%s", y, x, text);
}

void moverBola() {
  printf("\033[%d;%dH ", (int)bola.y, (int)bola.x);
  bola.x += bola.velX;
  bola.y += bola.velY;

  if (bola.x <= MIN_X || bola.x >= MAX_X) {
    bola.velX = -bola.velX;
  }
  if (bola.y <= MIN_Y) {
    bola.velY = -bola.velY;
  }
  if (bola.y == raquete.y - 1 && bola.x >= raquete.x &&
    bola.x <= raquete.x + TAM_RAQUETE) {
    bola.velY = -bola.velY;
  }
  Tijolo *anterior = NULL;
  Tijolo *atual = tijolos;
  while (atual) {
    if (bola.x >= atual->x && bola.x <= atual->x + 4 &&
      bola.y >= atual->y && bola.y <= atual->y + 1) {
      atual->durabilidade--;
      bola.velY = -bola.velY;
      if (atual->durabilidade == 0) {
        if (anterior) {
          anterior->prox = atual->prox;
        } else {
          tijolos = atual->prox;
        }
        free(atual);
        placarAtual += 5;
      }
      break;
    }
    anterior = atual;
    atual = atual->prox;
  }
}

void moverRaquete() {
  if (keyhit()) {
    int ch = readch();
    if (ch == '\033') {
      readch();
      switch (readch()) {
      case 'C':
        if (raquete.x < MAX_X - TAM_RAQUETE)
          raquete.x += 2;
        break;
      case 'D':
        if (raquete.x > MIN_X)
          raquete.x -= 2;
        break;
      }
    }
  }
}

int main(void) {
  inicializar();
  loopJogo();
  finalizar();
  return 0;
}
