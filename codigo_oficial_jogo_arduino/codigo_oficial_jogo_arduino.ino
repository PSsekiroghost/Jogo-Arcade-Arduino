#include <LedControl.h>
#include <EEPROM.h>

//definicao do joystick
#define X_AXIS A0
#define Y_AXIS A1
#define button_A 2
#define button_B 3
#define button_C 4
#define button_D 5
#define buzzer 7

//registro da EEPROM
#define ENDERECO_RECORDE 0 

//definicao da matriz
#define DIN 12
#define CLK 10
#define CS 9
#define MOD 4 //numero de modulos

// LedControl(DIN, CLK, CS, quant de modulos)
LedControl lc = LedControl(DIN, CLK, CS, MOD);

//cada byte = uma linha visual, o bit mais a esquerda = LED mais a esquerda
//inicia pressionando button_C

int byteDisplay[][8] = {
  {
    B00000000,
    B00111100,
    B00100000,
    B00100000,
    B00100000,
    B00100000,
    B00111100,
    B00000000
  },
  {
    B00000000,
    B11011100,
    B00010000,
    B00010000,
    B11011100,
    B01000100,
    B11011100,
    B00000000
  },
  {
    B00000000,
    B10011101,
    B01010001,
    B01010001,
    B10011101,
    B01010000,
    B01011101,
    B00000000
  },
  {
    B00000000,
    B01110011,
    B01001010,
    B01001010,
    B01110011,
    B01000010,
    B01000010,
    B00000000
  }
};

bool pause = false;
bool start = false;
bool fim = false;
int linhaNave = 0;
byte animaFimFrame = 0;
byte animaFimDisplay = 3;
unsigned long tempoAnimaFim = millis();
unsigned long tempoNave = millis();
unsigned long tempoTiro = millis();
unsigned long tempoAtualizarTiro = millis();
unsigned long tempoNascerInimigo = millis();
unsigned long tempoMoverInimigo = millis();
unsigned int recorde = 0; 

int tempoMoverInimigoEspera = 200;
int tempoNascerInimigoEspera = 1500;

void modificarBitMatriz(int display, int row, int column, bool value){

  if (row < 0 || row > 7) return;

  int resultado = byteDisplay[display][row];
  resultado = bitWrite(resultado, column, value);
  byteDisplay[display][row] = resultado;
}

//manda o byteDisplay para a matriz
void atualizarMatriz() {
  for (byte i = 0; i < MOD; i++) {
    for (byte linha = 0; linha < 8; linha++) {
      lc.setRow(i, linha, byteDisplay[i][linha]);
    }
  }
}

void exibirNave(bool limpar = false){
  if (linhaNave < -1){
    linhaNave = -1;
  }

  if (linhaNave > 6){
    linhaNave = 6;
  }

  //nao desenha na linha do display anterior
  modificarBitMatriz(MOD - 1, linhaNave, 7, linhaNave != -1 && !limpar);

  modificarBitMatriz(MOD - 1, linhaNave + 1, 7, !limpar);
  modificarBitMatriz(MOD - 1, linhaNave + 2, 7, !limpar);
  modificarBitMatriz(MOD - 1, linhaNave + 1, 6, !limpar);
}

struct Tiro {
  byte display = 255;
  byte x = 255;
  byte y = 255;
};

struct Inimigo {
  byte display = 255;
  byte x = 255;
  byte y = 255;
  byte bits[2] = {B10, B01};
};

byte barraProgresso = B00000000;
byte contagemAcertos = 0;
byte vidas = 3;
unsigned int pontos = 0;
int nivel = 1;

Tiro tiros[8]; // Máximo de 8 tipos simultâneos, 1 por coluna
Inimigo inimigos[3]; // Máximo de 3 inimigos simultâneos em tela
byte ultimoInimigo = 0;

//função para adicionar um tiro
void adicionarTiro(byte display, byte x, byte y){

  // Só adiciona se não tem na linha
  if (tiros[display].display == 255 && tiros[x].x != x){
    tiros[x].x = x;
    tiros[x].y = y;
    tiros[x].display = display;
  }
}

//função atualizar tiros
void atualizarTiros(){
  if (millis() - tempoAtualizarTiro > 30){
    tempoAtualizarTiro = millis();

    for (int i = 0; i < 8; i++){

      if (tiros[i].display == 255){
        modificarBitMatriz(0, tiros[i].x, 0, false);
        continue;
      }

      if ((tiros[i].display == 3 && tiros[i].y != 5 || tiros[i].display < 3)){
        modificarBitMatriz(tiros[i].display, tiros[i].x, tiros[i].y + 1, false);
      }

      if (tiros[i].y == 7){
        modificarBitMatriz(tiros[i].display + 1, tiros[i].x, 0, false);
      }

      modificarBitMatriz(tiros[i].display, tiros[i].x, tiros[i].y, true);

      if (tiros[i].y == 0 && tiros[i].display == 0){
        modificarBitMatriz(tiros[i].display, tiros[i].x, tiros[i].y, false);
      }

      if (tiros[i].y == 0){
        tiros[i].y = 7;
        tiros[i].display--;
      } else{
        tiros[i].y--;
      }

      if (tiros[i].display == 255){
        tiros[i].x = 255;
        tiros[i].y = 255;
      }

    }
  }
}

void verificarProgresso(){

  contagemAcertos++;
  pontos++;

  barraProgresso |= 1 << (contagemAcertos) - 1;

  if (barraProgresso == B11111111){
    
    barraProgresso = B00000000;
    contagemAcertos = 0;
    nivel++;

    atualizarDificuldade();
    
    tone(buzzer, 1500, 300);
  }
}

void atualizarDificuldade(){

  int faixa = (nivel - 1) / 3;

  tempoMoverInimigoEspera = constrain(200 - faixa * 25, 80, 200);
  tempoNascerInimigoEspera = constrain(1500 - faixa * 250, 800, 2000);
}

void verificarAcerto(){

  for (int i = 0; i < 8; i++){
    for (int j = 0; j < 3; j++){

      byte displayTiro = tiros[i].display;
      byte displayInimigo = inimigos[j].display;

      if (displayTiro == 255 || displayInimigo == 255){
        continue;
      }

      if (
        ((tiros[i].x == inimigos[j].x) || (tiros[i].x == inimigos[j].x - 1)) &&
        ((tiros[i].y == inimigos[j].y - 1) || (tiros[i].y == inimigos[j].y - 2)) &&
        (displayTiro == displayInimigo)) {

          //limpa
          modificarBitMatriz(tiros[i].display, tiros[i].x, tiros[i].y - 1, false);
          modificarBitMatriz(tiros[i].display, tiros[i].x, tiros[i].y - 2, false);
          modificarBitMatriz(tiros[i].display, tiros[i].x, tiros[i].y, false);
          modificarBitMatriz(tiros[i].display, tiros[i].x, tiros[i].y + 1, false);

          if (tiros[i].x < inimigos[j].x){
            modificarBitMatriz(tiros[i].display, tiros[i].x + 1, tiros[i].y - 1, false);
            modificarBitMatriz(tiros[i].display, tiros[i].x + 1, tiros[i].y - 2, false);
            modificarBitMatriz(tiros[i].display, tiros[i].x + 1, tiros[i].y, false);
            modificarBitMatriz(tiros[i].display, tiros[i].x + 1, tiros[i].y + 1, false);
          }

          if (tiros[i].x == inimigos[j].x){
            modificarBitMatriz(tiros[i].display, tiros[i].x - 1, tiros[i].y - 1, false);
            modificarBitMatriz(tiros[i].display, tiros[i].x - 1, tiros[i].y - 2, false);
            modificarBitMatriz(tiros[i].display, tiros[i].x - 1, tiros[i].y, false);
            modificarBitMatriz(tiros[i].display, tiros[i].x - 1, tiros[i].y + 1, false);
          }

          // fim limpa
          tiros[i].display = 255;
          tiros[i].x = 255;
          tiros[i].y = 255;
          inimigos[j].display = 255;
          inimigos[j].x = 255;
          inimigos[j].y = 255;

          modificarBitMatriz(tiros[i].display, tiros[i].x, tiros[i].y, false);
          tone(buzzer, 1200, 100);

          // TODO Finalizar a barra de progresso
          verificarProgresso();
        }
    }
  }

}

void perderVida(){

  if (vidas > 0){
    vidas--;
  }

  tone(buzzer, 500, 200);

  if (vidas == 0){
    fim = true;
  }
}

void adicionarInimigo(bool semEspera = false, bool atualizarPosicao = true){

  byte posicao = (byte)random(1, 7);

  if (inimigos[ultimoInimigo].display != 255){
    return;
  }

  if (millis() - tempoNascerInimigo > tempoNascerInimigoEspera || semEspera){
    tempoNascerInimigo = millis();
    inimigos[ultimoInimigo].display = 0;
    inimigos[ultimoInimigo].x = posicao;
    inimigos[ultimoInimigo].y = ultimoInimigo;

    if (atualizarPosicao){
      ultimoInimigo = ++ultimoInimigo % 3;
    }
  }
}

void atualizarInimigos(){

  if (millis() - tempoMoverInimigo > tempoMoverInimigoEspera){
    tempoMoverInimigo = millis();

    for (int i = 0; i < 3; i++){
      if (inimigos[i].display == 255){
        continue;
      }

      if (inimigos[i].y == 0 && inimigos[i].display > 0){
        modificarBitMatriz(inimigos[i].display - 1, inimigos[i].x, 6, false);
        modificarBitMatriz(inimigos[i].display - 1, inimigos[i].x - 1, 6, false);
      } else if (inimigos[i].y == 1 && inimigos[i].display > 0){
        modificarBitMatriz(inimigos[i].display - 1, inimigos[i].x, 7, false);
        modificarBitMatriz(inimigos[i].display - 1, inimigos[i].x - 1, 7, false);
      } else {
        modificarBitMatriz(inimigos[i].display, inimigos[i].x, inimigos[i].y - 2, false);
        modificarBitMatriz(inimigos[i].display, inimigos[i].x - 1, inimigos[i].y - 2, false);
      }

      int mode = inimigos[i].y % 2 == 0;
      modificarBitMatriz(inimigos[i].display, inimigos[i].x, inimigos[i].y, bitRead(inimigos[i].bits[mode], 0));
      modificarBitMatriz(inimigos[i].display, inimigos[i].x - 1, inimigos[i].y, bitRead(inimigos[i].bits[mode], 1));
      modificarBitMatriz(inimigos[i].display, inimigos[i].x, inimigos[i].y - 1, bitRead(inimigos[i].bits[!mode], 0));
      modificarBitMatriz(inimigos[i].display, inimigos[i].x - 1, inimigos[i].y - 1, bitRead(inimigos[i].bits[!mode], 1));

      if (inimigos[i].y == 7){
        inimigos[i].y = 0;
        inimigos[i].display++;
      } else {
        inimigos[i].y++;
      }

      if (inimigos[i].display == 3 && inimigos[i].y > 6){

        modificarBitMatriz(inimigos[i].display, inimigos[i].x, inimigos[i].y, 0);
        modificarBitMatriz(inimigos[i].display, inimigos[i].x - 1, inimigos[i].y, 0);
        modificarBitMatriz(inimigos[i].display, inimigos[i].x, inimigos[i].y - 1, 0);
        modificarBitMatriz(inimigos[i].display, inimigos[i].x - 1, inimigos[i].y - 1, 0);

        modificarBitMatriz(inimigos[i].display, inimigos[i].x, inimigos[i].y, 0);
        modificarBitMatriz(inimigos[i].display, inimigos[i].x - 1, inimigos[i].y, 0);
        modificarBitMatriz(inimigos[i].display, inimigos[i].x, inimigos[i].y - 2, 0);
        modificarBitMatriz(inimigos[i].display, inimigos[i].x - 1, inimigos[i].y - 2, 0);

        inimigos[i].display = 255;
        inimigos[i].x = 255;
        inimigos[i].y = 255;

        perderVida(); 
      }

    }
  }
}

void exibirMatrizes(bool limpar = false){

  //exibe a linha da barra de progresso
  for (int c = 0; c < 8; c++){
    modificarBitMatriz(0, c, 0, bitRead(barraProgresso, c));
  }

  // exibi as vidas

  if (start){

    for (int v = 0; v < 3; v++){
      modificarBitMatriz(0, 7, 1 + v, v < vidas);
    } 
  }

  //desenha toda a matriz
  for (byte d = 0; d < MOD; d++){
    for (byte i = 0; i < 8; i++){
      if (limpar){
        byteDisplay[d][i] = B00000000;
      }

      lc.setRow(d, i, byteDisplay[d][i]);
    }
  }
}

void voltarTelaInicial() {
  // Para o jogo e volta ao menu
  start = false;
  pause = false;
  fim = false;

  // Reinicia a nave
  linhaNave = 0;

  //reinicia vida e nivel
  vidas = 3;
  pontos = 0;
  nivel = 1;

  atualizarDificuldade();

  // Reinicia a barra de progresso
  barraProgresso = B00000000;
  contagemAcertos = 0;

  // Reinicia os temporizadores
  tempoNave = millis();
  tempoTiro = millis();
  tempoAtualizarTiro = millis();
  tempoNascerInimigo = millis();
  tempoMoverInimigo = millis();

  // Reinicia a animacao final
  animaFimFrame = 0;
  animaFimDisplay = 3;
  tempoAnimaFim = millis();

  // Limpa todos os tiros
  for (int i = 0; i < 8; i++) {
    tiros[i].display = 255;
    tiros[i].x = 255;
    tiros[i].y = 255;
  }

  // Limpa todos os inimigos
  for (int i = 0; i < 3; i++) {
    inimigos[i].display = 255;
    inimigos[i].x = 255;
    inimigos[i].y = 255;
  }

  ultimoInimigo = 0;

  // Restaura a tela inicial: PRESS C
  byteDisplay[0][0] = B00000000;
  byteDisplay[0][1] = B00111100;
  byteDisplay[0][2] = B00100000;
  byteDisplay[0][3] = B00100000;
  byteDisplay[0][4] = B00100000;
  byteDisplay[0][5] = B00100000;
  byteDisplay[0][6] = B00111100;
  byteDisplay[0][7] = B00000000;

  byteDisplay[1][0] = B00000000;
  byteDisplay[1][1] = B11011100;
  byteDisplay[1][2] = B00010000;
  byteDisplay[1][3] = B00010000;
  byteDisplay[1][4] = B11011100;
  byteDisplay[1][5] = B01000100;
  byteDisplay[1][6] = B11011100;
  byteDisplay[1][7] = B00000000;

  byteDisplay[2][0] = B00000000;
  byteDisplay[2][1] = B10011101;
  byteDisplay[2][2] = B01010001;
  byteDisplay[2][3] = B01010001;
  byteDisplay[2][4] = B10011101;
  byteDisplay[2][5] = B01010000;
  byteDisplay[2][6] = B01011101;
  byteDisplay[2][7] = B00000000;

  byteDisplay[3][0] = B00000000;
  byteDisplay[3][1] = B01110011;
  byteDisplay[3][2] = B01001010;
  byteDisplay[3][3] = B01001010;
  byteDisplay[3][4] = B01110011;
  byteDisplay[3][5] = B01000010;
  byteDisplay[3][6] = B01000010;
  byteDisplay[3][7] = B00000000;

  // Atualiza os quatro displays
  exibirMatrizes();
}

void moverNaveCima(){
  exibirNave(true);
  linhaNave--;
  exibirNave();
}

void moverNaveBaixo(){
  exibirNave(true);
  linhaNave++;
  exibirNave();
}

void esquerda(){
  if (millis() - tempoNave > 200){
    tempoNave = millis();
    moverNaveCima();
  }
}

void direita(){
  if (millis() - tempoNave > 200){
    tempoNave = millis();
    moverNaveBaixo();
  }
}

void atirar(){
  if (millis() - tempoTiro > 200){
    tempoTiro = millis();
    adicionarTiro(MOD - 1, linhaNave + 1, 5);
  }
}

void fimJogo(){

  byteDisplay[0][0] = B00000000;
  barraProgresso = 0;
  int tempoMover = 200;

  if (millis() - tempoAnimaFim > tempoMover && animaFimFrame == 0){

    exibirMatrizes(true);
    byteDisplay[animaFimDisplay][2] = B10000000; 
    byteDisplay[animaFimDisplay][3] = B10000000;
    byteDisplay[animaFimDisplay][4] = B10000000;
    byteDisplay[animaFimDisplay][3] = B11000000;
    exibirMatrizes();

    //delay(tempoMover);
    tempoAnimaFim = millis();
    animaFimFrame++;
  }

  if (millis() - tempoAnimaFim > tempoMover && animaFimFrame == 1){
    byteDisplay[animaFimDisplay][2] = B10000000; 
    byteDisplay[animaFimDisplay][3] = B00000000; 
    byteDisplay[animaFimDisplay][4] = B00000000; 
    byteDisplay[animaFimDisplay][3] = B00000000; 

    byteDisplay[animaFimDisplay][2] = B01110000; 
    byteDisplay[animaFimDisplay][3] = B00110000; 
    byteDisplay[animaFimDisplay][4] = B00010000;
    exibirMatrizes();

    //delay(tempoMover)
    tempoAnimaFim = millis();
    animaFimFrame++; 
  }

  if (millis() - tempoAnimaFim > tempoMover && animaFimFrame == 2){
    tempoMover = millis();
    byteDisplay[animaFimDisplay][2] = B00000000; 
    byteDisplay[animaFimDisplay][3] = B00000000; 
    byteDisplay[animaFimDisplay][4] = B00000000; 

    byteDisplay[animaFimDisplay][0] = B00100000; 
    byteDisplay[animaFimDisplay][1] = B01110000; 
    exibirMatrizes();

    //delay(tempoMover)
    tempoAnimaFim = millis();
    animaFimFrame++;
  }

  if (millis() - tempoAnimaFim > tempoMover && animaFimFrame == 3){
    byteDisplay[animaFimDisplay][1] = B00000000; 
    byteDisplay[animaFimDisplay][0] = B01110000;
    exibirMatrizes();

    //delay(tempoMover)
    tempoAnimaFim = millis();
    animaFimFrame++; 
  }

  if (millis() - tempoAnimaFim > tempoMover && animaFimFrame == 4){
    byteDisplay[animaFimDisplay][0] = B00000000;
    exibirMatrizes();

    //delay(tempoMover * 3)
    tempoAnimaFim = millis();
    animaFimFrame++;
    animaFimDisplay--;

    if (animaFimDisplay == 255){
      animaFimDisplay = 3;
    };
  }

  if (millis() - tempoAnimaFim > tempoMover && animaFimFrame == 5){
    byteDisplay[animaFimDisplay][3] = B10000000; 
    exibirMatrizes();

    //delay(tempoMover)
    tempoAnimaFim = millis();
    animaFimFrame = 0;
  }
  
}

void setup() {
  Serial.begin(9600);

  pinMode(button_A, INPUT_PULLUP);
  pinMode(button_B, INPUT);
  pinMode(button_C, INPUT);
  pinMode(button_D, INPUT);
  pinMode(buzzer, OUTPUT);

  randomSeed(analogRead(5));

  for(byte i = 0; i < MOD; i++){
    lc.shutdown(i, false);
    lc.setIntensity(i, 2);
    lc.clearDisplay(i);
  }
}

void loop() {
  if (start) {

    int xValue = analogRead(X_AXIS);
    int YValue = analogRead(Y_AXIS);

    if (xValue < 200){
      esquerda();
    }

    if (xValue > 800){
      direita();
    }

    if (digitalRead(button_B) == 0){
      atirar();
    }

    if (digitalRead(button_D) == 0){
      pause = !pause;
      while(digitalRead(button_D) == 0);
    }

    if (!pause && !fim) {
      adicionarInimigo();
      atualizarInimigos();
      atualizarTiros();
      verificarAcerto();
      exibirNave();
    }

    if (fim){
      fimJogo();
    }

    // Botao A: voltar para a tela inicial
    if (digitalRead(button_A) == LOW) {
      voltarTelaInicial();

      while (digitalRead(button_A) == LOW) {
        // Aguarda soltar o botao
      }

      return;
    }

  } else {

    if (digitalRead(button_C) == 0){
      start = true;
      exibirMatrizes(true);
      while(digitalRead(button_C) == 0);
    }
  }

  exibirMatrizes();
}
