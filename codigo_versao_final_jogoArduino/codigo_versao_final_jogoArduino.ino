#include <LedControl.h>
#include <stdint.h>

//definicao do joystick
#define X_AXIS A0
#define Y_AXIS A1
#define button_A 2
#define button_B 3
#define button_C 4
#define button_D 5
#define buzzer 7

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

//inicio
bool pause = false;
bool start = false;
bool fim = false;

//sistema de animacao abertura
bool animacaoAberturaAtiva = false;

byte faseAbertura = 0;
byte passoOnda = 0;
byte numeroAbertura = 3;
byte etapaPisca = 0;

unsigned long tempoEtapaAbertura = 0;
const unsigned long intervaloPassoOnda = 55;
const unsigned long pausaAntesContagem = 200;
const unsigned long intervaloPisca = 250;

//sistema de nave
int linhaNave = 0;
unsigned long tempoNave = millis();

//sistema de animacao final
byte resultadoFinal = 0;
byte faseAnimacaoFim = 0;
int nivelFinal = 1;
bool animacaoFimIniciada = false;
unsigned long tempoAnimaFim = 0;

//sistema de tiro
unsigned long tempoTiro = millis();
unsigned long tempoAtualizarTiro = millis();
unsigned long tempoNascerInimigo = millis();
unsigned long tempoMoverInimigo = millis();
unsigned int recorde = 0; 

int tempoMoverInimigoEspera = 400;
int tempoNascerInimigoEspera = 2000;

//sistema de dash
int direcaoNave = 0;
const byte distanciaDash = 3;
const unsigned long intervaloDash = 800;
unsigned long tempoUltimoDash = 0;


void modificarBitMatriz(int display, int row, int column, bool value){

  if (display < 0 || display >= MOD) return;
  if (row < 0 || row > 7) return;
  if (column < 0 || column > 7) return;

  byte resultado = byteDisplay[display][row];

  bitWrite(resultado, column, value);
  byteDisplay[display][row] = resultado;
}

//manda o byteDisplay para a matriz
void atualizarMatriz() {

  for (byte i = 0; i < MOD; i++){

    for (byte linha = 0; linha < 8; linha++){
      lc.setRow(i, linha, byteDisplay[i][linha]);
    }
  }
}

//inverte a ordem dos bits de cada modulo
byte inverterBits(byte valor){

  byte resultado = 0;

  for (byte i = 0; i < 8; i++){

    bitWrite(resultado, 7 - i, bitRead(valor, i));
  }

  return resultado;
}

void limparBufferAbertura(){

  for (byte d = 0; d < MOD; d++){
    for (byte linha = 0; linha < 8; linha++){
      byteDisplay[d][linha] = B00000000;
    }
  }

  atualizarMatriz();
}

void acenderColunaAbertura(byte colunaGlobal){

  byte display = colunaGlobal / 8;
  byte colunaLocal = colunaGlobal % 8;

  for (byte linha = 0; linha < 8; linha++){
    modificarBitMatriz(display, linha, colunaLocal, true);
  }
}

void desenharNumeroAbertura(byte numero){

  const byte numero3[8] = {
    B00111100,
    B01100110,
    B00000110,
    B00011100,
    B00000110,
    B01100110,
    B00111100,
    B00000000
  };

  const byte numero2[8] = {
    B00111100,
    B01100110,
    B00000110,
    B00001100,
    B00110000,
    B01100000,
    B01111110,
    B00000000
  };

  const byte numero1[8] = {
    B00011000,
    B00111000,
    B00011000,
    B00011000,
    B00011000,
    B00011000,
    B01111110,
    B00000000
  };

  const byte * desenho;

  switch (numero){

    case 3:
      desenho = numero3;
      break;

    case 2:
      desenho = numero2;
      break;

    case 1:
      desenho = numero1;
      break;

    default:
      return;
  }

  //repete o numero nos quatro modulos
  for (byte d = 0; d < MOD; d++){

    for (byte linha = 0; linha < 8; linha++){
      byteDisplay[d][linha] = desenho[linha];
    }
  }

  atualizarMatriz();
}

void iniciarAnimacaoAbertura(){

  animacaoAberturaAtiva = true;

  faseAbertura = 0;
  passoOnda = 0;

  numeroAbertura = 3;
  etapaPisca = 0;

  tempoEtapaAbertura = millis();
  
  //limpa a tela PRESS C
  limparBufferAbertura();
}

void atualizarAnimacaoAbertura(){

  unsigned long agora = millis();

  // ====================================
  // FASE 0: ONDA DO CENTRO PARA FORA
  // ====================================

  if (faseAbertura == 0){

    if (agora - tempoEtapaAbertura >= intervaloPassoOnda){

      tempoEtapaAbertura = agora;

      if (passoOnda < 16){

        //colunas que acendem simultaneamente
        byte colunaEsquerda = 15 - passoOnda;
        byte colunaDireita = 16 + passoOnda;

        acenderColunaAbertura(colunaEsquerda);
        acenderColunaAbertura(colunaDireita);

        atualizarMatriz();

        passoOnda++;

        //quando todos os leds estiverem acessos
        if (passoOnda == 16){

          faseAbertura = 1;
          tempoEtapaAbertura = agora;
        }
      }
    }
  }

  // =================================
  // FASE 1: PAUSA COM TODOS ACESOS
  // =================================

  else if (faseAbertura == 1){

    if (agora - tempoEtapaAbertura >= pausaAntesContagem){

      faseAbertura = 2;
      numeroAbertura = 3;
      etapaPisca = 0;
      tempoEtapaAbertura = agora;

      desenharNumeroAbertura(numeroAbertura);
    }
  }

  // ===============================
  // FASE 2: CONTAGEM REGRESSIVA
  // ===============================

  else if (faseAbertura == 2){

    if (agora - tempoEtapaAbertura >= intervaloPisca){

      tempoEtapaAbertura = agora;
      etapaPisca++;

      // apaga o numero 
      if (etapaPisca == 1 || etapaPisca == 3){
        limparBufferAbertura();
      }

      //acende novamente 
      else if (etapaPisca == 2){
        desenharNumeroAbertura(numeroAbertura);
      }

      // terminou as duas piscadas 
      else if (etapaPisca >= 4){

        if (numeroAbertura > 1){

          numeroAbertura--;
          etapaPisca = 0;
          desenharNumeroAbertura(numeroAbertura);
        } else {

          finalizarAnimacaoAbertura();
        }
      }
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

//sistema de progressao
byte barraProgresso = B00000000;
byte contagemAcertos = 0;
byte vidas = 3;
unsigned int pontos = 0;
int nivel = 1;

Tiro tiros[8]; // Máximo de 8 tipos simultâneos, 1 por coluna
Inimigo inimigos[3]; // Máximo de 3 inimigos simultâneos em tela
byte ultimoInimigo = 0;

struct TiroInimigo {
  byte coluna = 255; 
  byte linha = 255;
  byte origem = 255;
};

//sistema de tiro inimigo
TiroInimigo tirosInimigos[3];

unsigned long tempoTiroInimigo = millis();
unsigned long tempoAtualizarTiroInimigo = millis();

unsigned long tempoTiroInimigoEsperar = 3000;
unsigned long tempoMoverTiroInimigoEsperar = 180;
byte maxTirosInimigosAtivos = 1;

void pintarGrobal(byte coluna, byte linha, bool valor){
  modificarBitMatriz(coluna / 8, linha, coluna % 8, valor);
}

void adicionarTiroInimigo(){
  
  if (millis() - tempoTiroInimigo < tempoTiroInimigoEsperar){
    return;
  }

  // procura um espaço livre no display
  byte livre = 255;
  byte ativos = 0;

  for (byte i = 0; i < 3; i++){
    if (tirosInimigos[i].coluna == 255){
      
      if (livre == 255){
        livre = i;
      }

    } else {
      ativos++;
    }
  }

  //verifica o limite de tiros simultaneos
  if (livre == 255 || ativos >= maxTirosInimigosAtivos){
    return;
  }

  //so atiram os inimigos que estao no modulo 0 e 1
  byte candidatos[3];
  byte total = 0;

  for (byte j = 0; j < 3; j++){

    if (inimigos[j].display == 255 || inimigos[j].display >= 2){
      continue;
    }

    // verifica se um inimigo ja possui um tiro ativo
    bool jaAtirou = false;

    for (byte k = 0; k < 3; k++){

      if (tirosInimigos[k].coluna != 255 && tirosInimigos[k].origem == j){

        jaAtirou = true;
        break;
      }
    }

    if (!jaAtirou){
      candidatos[total] = j;
      total++;
    }
  }

  if (total == 0){
    return;
  }

  byte escolhido = candidatos[random(0, total)];

  //definicao posicao do tiro
  byte coluna = inimigos[escolhido].display * 8 + inimigos[escolhido].y + 2;
  byte linha =  inimigos[escolhido].x;

  tempoTiroInimigo = millis();
  tirosInimigos[livre].coluna = coluna;
  tirosInimigos[livre].linha = linha;
  tirosInimigos[livre].origem = escolhido;

  pintarGrobal(coluna, linha, true);

  //Serial.println("Tiro inimigo criado");
}

void atualizarTiroInimigo(){

  if (millis() - tempoAtualizarTiroInimigo < tempoMoverTiroInimigoEsperar){
    return;
  }

  tempoAtualizarTiroInimigo = millis();

  for (byte i = 0; i < 3; i++){
    if (tirosInimigos[i].coluna == 255){
      continue;
    }

    //apaga a posição antiga e anda uma coluna
    pintarGrobal(tirosInimigos[i].coluna, tirosInimigos[i].linha, false);
    tirosInimigos[i].coluna++;

    // saiu da tela - libera espaço
    if (tirosInimigos[i].coluna > 31){

      tirosInimigos[i].coluna = 255;
      tirosInimigos[i].linha = 255;
      tirosInimigos[i].origem = 255;
      continue;
    }

    //bateu na nave
    byte c = tirosInimigos[i].coluna;
    int l = tirosInimigos[i].linha;
    bool bateu = (c == 30 && l == linhaNave + 1  || 
                  c == 31 && l >= linhaNave && l <= linhaNave + 2);

    if (bateu){

      tirosInimigos[i].coluna = 255;
      tirosInimigos[i].linha = 255;
      tirosInimigos[i].origem = 255;

      perderVida();
      continue;
    }

    //desenha na nave
    pintarGrobal(c, l, true);
  }

}

//função para adicionar um tiro
void adicionarTiro(byte display, byte x, byte y){

  if (x >= 8 || display >= MOD){
    return;
  }

  if (tiros[x].display == 255){
    
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

  //velocidade dos inimigos
  tempoMoverInimigoEspera = constrain(400 - faixa * 20, 240, 400);

  //frequencia de nascimento dos inimigos
  tempoNascerInimigoEspera = constrain(2000 - faixa * 250, 800, 2000);

  //velocidade dos tiros inimigos
  tempoMoverTiroInimigoEsperar = constrain(180 - faixa * 15, 70, 180);

  //frequencia de disparos de inimigos
  tempoTiroInimigoEsperar = constrain(3000 - faixa * 200, 1500, 3000);

  //quant max de tiros inimigos
  maxTirosInimigosAtivos = constrain(1 + faixa, 1, 3);
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

  if (vidas == 0 && !fim){

    fim = true;

    //guarda o nivel alcancado
    nivelFinal = nivel;

    //classifica o desempenho
    if (nivelFinal <= 3){

      resultadoFinal = 0;

    } else if (nivelFinal <= 7){

      resultadoFinal = 1;

    } else {

      resultadoFinal = 2;
    }

    //prepara a animacao final
    animacaoFimIniciada = false;
    faseAnimacaoFim = 0;
    tempoAnimaFim = millis();
    barraProgresso = B00000000;
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

void atualizarIndicadorDash(){

  unsigned long tempoDecorrido = millis() - tempoUltimoDash;
  byte ledsAcessos;

  if (tempoDecorrido >= intervaloDash){

    ledsAcessos = 4;
  } else {

    ledsAcessos = (tempoDecorrido * 4UL) / intervaloDash;
  }

  //desenha 4 LEDs
  for (byte i = 0; i < 4; i++){
    modificarBitMatriz(0, 7, 4 + i, i < ledsAcessos);
  }
}

void exibirMatrizes(bool limpar = false){

  static unsigned long ultimaAtualizacao = 0;

  if (!limpar && millis() - ultimaAtualizacao < 20){
    return;
  }

  ultimaAtualizacao = millis();

  // limpa as matrizes quando solicitado
  if (limpar){

    for (byte d = 0; d < MOD; d++){
      for (byte i = 0; i < 8; i++){
        byteDisplay[d][i] = B00000000;
      }
    }
  }

  //exibe a linha da barra de progresso
  for (int c = 0; c < 8; c++){
    modificarBitMatriz(0, c, 0, bitRead(barraProgresso, c));
  }

  // exibi as vidas e o Dash
  if (start && !fim){

    //exibe as vidas
    for (int v = 0; v < 3; v++){
      modificarBitMatriz(0, 7, 1 + v, v < vidas);
    } 

    //exibe o LOAD do Dash
    atualizarIndicadorDash();

  } else {

    //retira o LOAD do Dash da tela
    for (byte c = 4; c < 8; c++){
      modificarBitMatriz(0, 7, c, false);
    }
  }

  //desenha toda a matriz
  for (byte d = 0; d < MOD; d++){

    for (byte i = 0; i < 8; i++){
      lc.setRow(d, i, byteDisplay[d][i]);
    }
  }
}

void finalizarAnimacaoAbertura(){

  unsigned long agora = millis();
  animacaoAberturaAtiva = false;
  faseAbertura = 0;

  start = true;
  pause = false;
  fim = false;

  // Reinicia os temporizadores do jogo
  tempoNave = agora;
  tempoTiro = agora;
  tempoAtualizarTiro = agora;

  tempoNascerInimigo = agora;
  tempoMoverInimigo = agora;

  tempoTiroInimigo = agora;
  tempoAtualizarTiroInimigo = agora;

  // Dash disponivel desde o inicio
  tempoUltimoDash = agora - intervaloDash;

  // Limpa a tela e prepara a nave
  exibirMatrizes(true);

  exibirNave();
  atualizarMatriz();
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
  resultadoFinal = 0;
  nivelFinal = 1;
  faseAnimacaoFim = 0;
  animacaoFimIniciada = false;
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

  for (int i = 0; i < 3; i++){
    tirosInimigos[i].coluna = 255;
    tirosInimigos[i].linha = 255;
    tirosInimigos[i].origem = 255;
  }

  tempoTiroInimigo = millis();
  tempoAtualizarTiroInimigo = millis();

  ultimoInimigo = 0;
  direcaoNave = 0;
  tempoUltimoDash = millis() - intervaloDash;

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

void executarDash(){

  //verifica se o Dash esta disponivel
  if (millis() - tempoUltimoDash < intervaloDash){
    return;
  }

  int direcao = direcaoNave;

  if (direcao == 0){
    direcao = -1;
  }

  // calcula a nova posicao
  int novaLinha = linhaNave + direcao * distanciaDash;

  //limite matriz
  novaLinha = constrain(novaLinha, -1, 6);

  //se nao puder se mover, nao consome a recarga
  if (novaLinha == linhaNave){
    return;
  }

  //apaga a nave antiga
  exibirNave(true);

  // Atualiza a posicao
  linhaNave = novaLinha;

  //desenha a nova nave
  exibirNave();

  tempoUltimoDash = millis();
}

void atirar(){
  if (millis() - tempoTiro > 200){
    tempoTiro = millis();
    adicionarTiro(MOD - 1, linhaNave + 1, 5);
  }
}

void desenharRostoFinal(byte expressao, bool olhosFechados, bool mostrarBoca){

  // base do rosto
  uint16_t rosto[8] = {
    0x03C0,
    0x0FF0,
    0x1818,
    0x2004,
    0x2004,
    0x1008,
    0x0810,
    0x03C0
  };

  //desenha os olhos 
  if (olhosFechados){

    rosto[3] |= 0x0E70;

  } else {

    //olhos abertos
    rosto[3] |= 0x0420;
  }

  //desenha a boca conforme o desempenho
  if (mostrarBoca){

    switch (expressao){

      case 0:
        //rosto triste 
        rosto[5] |= 0x0420;
        rosto[6] |= 0x03C0;
        break;

      case 1:
        //rosto neutro
        rosto[5] |= 0x07E0;
        break;

      case 2:
        //rosto feliz
        rosto[5] |= 0x03C0;
        rosto[6] |= 0x0420;
        break;
    }
  }

  //limpa os quatro modulos
  for (byte d = 0; d < MOD; d++){

    for (byte linha = 0; linha < 8; linha++){
      byteDisplay[d][linha] = B00000000;
    }
  }

  //desenha o rosto nos dois modulos centrais
  for (byte linha = 0; linha < 8; linha++){

    byteDisplay[1][linha] = inverterBits((rosto[linha] >> 8) & 0xFF);
    byteDisplay[2][linha] = inverterBits(rosto[linha] & 0xFF);
  }

  atualizarMatriz();
}

void fimJogo(){

  unsigned long agora = millis();

  // =======================
  // INICIO DA ANIMACAO
  // =======================

  if (!animacaoFimIniciada){

    //apaga todos os modulos
    for (byte d = 0; d < MOD; d++){

      for (byte linha = 0; linha < 8; linha++){
        byteDisplay[d][linha] = B00000000;
      }
    }

    atualizarMatriz();

    animacaoFimIniciada = true;
    faseAnimacaoFim = 0;
    tempoAnimaFim = agora;
    return;
  }

  // ===================================
  // CONTROLE DO TEMPO DE CADA ETAPA
  // ===================================

  unsigned long intervalo;

  switch (faseAnimacaoFim){

    case 0:
      intervalo = 250;
      break;

    case 1:
      intervalo = 300;
      break;

    case 2:
      intervalo = 600;
      break;

    case 3:
      intervalo = 500;
      break;

    case 4:
      intervalo = 300;
      break;

    case 5:
      intervalo = 500;
      break;

    default:
      return;
  }

  if (agora - tempoAnimaFim < intervalo){
    return;
  }

  tempoAnimaFim = agora;

  // =======================
  // ANIMACAO DO ROSTO
  // =======================

  switch (faseAnimacaoFim){

    case 0:
      
      //aparece o contorno e os olhos
      desenharRostoFinal(resultadoFinal, false, false);
      faseAnimacaoFim++;
      break;

    case 1:

      // aparece a boca correspondente ao desempenho
      desenharRostoFinal(resultadoFinal, false, true);
      faseAnimacaoFim++;
      break;

    case 2:
      
      // primeiro piscar: fecha os olhos
      desenharRostoFinal(resultadoFinal, true, true);
      faseAnimacaoFim++;
      break;

    case 3:

      // abre os olhos
      desenharRostoFinal(resultadoFinal, false, true);
      faseAnimacaoFim++;
      break;

    case 4:
      
      // segundo piscar
      desenharRostoFinal(resultadoFinal, true, true);
      faseAnimacaoFim++;
      break;

    case 5:

      //rosto final com os olhos abertos
      desenharRostoFinal(resultadoFinal, false, true);
      faseAnimacaoFim++;
      break;
  }
}

void silenciarBuzzer(){

  noTone(buzzer);
  digitalWrite(buzzer, LOW);
}

void setup() {
  Serial.begin(9600);

  pinMode(button_A, INPUT_PULLUP);
  pinMode(button_B, INPUT_PULLUP);
  pinMode(button_C, INPUT_PULLUP);
  pinMode(button_D, INPUT_PULLUP);
  pinMode(buzzer, OUTPUT);

  digitalWrite(buzzer, LOW);
  noTone(buzzer);

  randomSeed(analogRead(5));

  for(byte i = 0; i < MOD; i++){
    lc.shutdown(i, false);
    lc.setIntensity(i, 2);
    lc.clearDisplay(i);
  }
}

void loop() {

  // guarda o estado anterior dos botoes
  static bool anteriorA = HIGH;
  static bool anteriorB = HIGH;
  static bool anteriorC = HIGH;
  static bool anteriorD = HIGH;

  // Le os estados atuais
  bool estadoA = digitalRead(button_A);
  bool estadoB = digitalRead(button_B);
  bool estadoC = digitalRead(button_C);
  bool estadoD = digitalRead(button_D);

  // Detecta um novo pressionamento
  bool apertouA = (anteriorA == HIGH && estadoA == LOW);
  bool apertouB = (anteriorB == HIGH && estadoB == LOW);
  bool apertouC = (anteriorC == HIGH && estadoC == LOW);
  bool apertouD = (anteriorD == HIGH && estadoD == LOW);

  // Atualiza os estados anteriores
  anteriorA = estadoA;
  anteriorB = estadoB;
  anteriorC = estadoC;
  anteriorD = estadoD;

  // SILENCIO FORA DO JOGO
  if (!start || animacaoAberturaAtiva || pause || fim){
    silenciarBuzzer();
  }

  // ===========================
  // TELA INICIAL E ANIMACAO
  // ===========================

  if (!start){

    // se a animacao ainda nao comecou
    if (!animacaoAberturaAtiva){

      if (apertouC){

        iniciarAnimacaoAbertura();
      } else {

        exibirMatrizes();
      }
    }

    // se a animacao ja comecou 
    else {

      atualizarAnimacaoAbertura();
    }

    return;
  }

  // ====================
  // VOLTAR AO MENU
  // ====================

  if (apertouA && (pause || fim)){

    voltarTelaInicial();
    return;
  }

  // ====================
  // PAUSAR E RETOMAR 
  // ====================

  if (apertouD && !fim){

    pause = !pause;
  }

  // ======================
  // FIM DE JOGO
  // ======================

  if (fim){
    
    fimJogo();
    exibirMatrizes();
    return;
  }

  // =====================
  // JOGO PAUSADO
  // =====================

  if (pause){

    exibirMatrizes();
    return;
  }

  // =========================
  // MOVIMENTAÇÃO DA NAVE
  // =========================

  int xValue = analogRead(X_AXIS);

  if (xValue < 200){

    direcaoNave = -1;
    esquerda();

  } else if (xValue > 800){

    direcaoNave = 1;
    direita();
  }

  // =====================
  // BOTAO B: ATIRAR
  // =====================

  if (apertouB){

    atirar();
  }

  // ===================
  // BOTAO C: DASH
  // ===================

  if (apertouC){

    executarDash();
  }

  // ========================
  // ATUALIZACAO DO JOGO
  // ========================

  adicionarInimigo();
  atualizarInimigos();

  atualizarTiros();

  adicionarTiroInimigo();
  atualizarTiroInimigo();

  verificarAcerto();

  exibirNave();

  exibirMatrizes();
}

