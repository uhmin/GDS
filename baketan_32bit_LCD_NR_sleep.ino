#include <avr/sleep.h>
#include <avr/wdt.h>
#include <SPI.h>
#include <TFT.h>
#define CS    10
#define DC     9
#define RESET  8
#define BYTES  4
#define BUZZ  A0  // 圧電スピーカーをつなげる端子
#define ANT   A5  // 浮動電圧を測る端子
#define VERYGOOD 0 // とても良い反応 （0以上の整数で設定）
#define GOOD     1 // 良い反応
#define BAD     10 // 悪い反応
#define VERYBAD 12 // とても悪い反応（16以下の整数で設定）
#define SIZE 5 // 画面サイズに合わせた倍率
#define VIEWSIZE 80
#define OFFSETY 25
#define HEIGHT 80+OFFSETY
#define WIDTH 160
TFT TFTscreen = TFT(CS, DC, RESET);

unsigned long Xn = 'b';
union {
  unsigned long int sum=0;
  unsigned char div[BYTES];
} data;

void setup() {
  unsigned int i;
  unsigned char ypos[4];
  
  Xn = analogRead(BUZZ) ^ analogRead(ANT); // M系列を電圧値で初期化
  
  // バイナリデータ保存用変数の初期化
  for(i=0; i<BYTES*8; i++){
    data.sum <<=1;
    data.sum |=i % 2;
  }

  // LCD初期化
  TFTscreen.begin();
  TFTscreen.background(128,128,128);
  ypos[1]=VERYBAD*SIZE;
  ypos[2]=BAD*SIZE;
  ypos[3]=GOOD*SIZE;
  ypos[3]=VERYGOOD*SIZE;
  TFTscreen.stroke(255,255,0);
  TFTscreen.line(0, OFFSETY+ypos[1], WIDTH, OFFSETY+ypos[1]);
  TFTscreen.line(0, OFFSETY+ypos[2], WIDTH, OFFSETY+ypos[2]);
  TFTscreen.line(0, OFFSETY+ypos[3], WIDTH, OFFSETY+ypos[3]);

  Serial.begin( 9600 ); //シリアル通信開始

  // スリープモードの設定
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  wdt_enable_test(WDTO_15MS);
}

void loop() {
  int xPos;
  char count;
  for(xPos=WIDTH; xPos>0; xPos--){
    count=baketan();
    // 悪い反応の時にブザーを鳴らす
    if(count > BAD) buzz(400, 300);
    draw2LCD(xPos, count); // LCDにグラフ表示
  }
}

char baketan() {
  unsigned char i;
  unsigned int sensorValue=0;
  char popcount;
  char staticCount=0;

  do{
    randomDelay();// ランダムな時間待つ
    // ブザーと浮動電圧の電圧を読み取ってXORで足し合わせる
    sensorValue = analogRead(BUZZ) ^ analogRead(ANT);
    staticCount++;
  }while(sensorValue==0 || sensorValue==1023); // sensorValue が 0 か 1023 の場合静電気の影響を受けていると判断して再度電圧を取得する
  sensorValue = __builtin_popcount(sensorValue);
  
  data.sum <<=1; data.div[0] |=sensorValue & 1; // 偶奇性をメモリに登録
  for(i=0, popcount=0; i<BYTES; i++){
    // 1が立っている数を数えて期待値との差をとる
    popcount+=abs(__builtin_popcount(data.div[i])-BYTES);
  }
  // シリアルモニタに送信
  sendSerial(popcount, __builtin_popcount(data.div[0])-4);
  if(staticCount>1){
    popcount*=-1;
  }
  return popcount;
}

void randomDelay(){
  unsigned char i;
  unsigned char wait;
  for(i=0, wait=0; i<6; i++){
    wait*=2;
    wait+=mseq();
  }
  sleep_enable();
  //delay(wait*3);
  Serial.flush(); 
  ADCSRA &= ~(1 << ADEN); // ADC off
  for(i=0; i<wait; i++) {asm("sleep");};
  sleep_disable();
  ADCSRA |= (1 << ADEN); // ADC on
}

void sendSerial(char data1, char data2){// シリアルモニタに送信
  Serial.print(data1*1);
  Serial.print(", ");
  Serial.print(data2*1);
  Serial.print("\n");
}

void buzz(int freq, int jikan){
    pinMode(BUZZ, OUTPUT);
    tone(BUZZ,freq, jikan) ;
    delay (jikan+50);
    digitalWrite(BUZZ, LOW);
    delay(50);
    pinMode(BUZZ, INPUT);
    delay(50);
}


void draw2LCD(int xPos, char popcount){   // LCDにグラフ表示
  int ypos[4];
  static int avg=0;
  ypos[1]=BAD*SIZE;
  ypos[2]=VERYBAD*SIZE;
  ypos[3]=GOOD*SIZE;

  TFTscreen.stroke(255,255,255);
  TFTscreen.fill(64,64,255);
  TFTscreen.rect(xPos-10, 0, 10, HEIGHT);
  
  TFTscreen.stroke(255,255,0);
  TFTscreen.line(xPos, OFFSETY+ypos[1]-1, xPos, OFFSETY+ypos[1]+1);
  TFTscreen.line(xPos, OFFSETY+ypos[2]-1, xPos, OFFSETY+ypos[2]+1);
  TFTscreen.line(xPos, OFFSETY+ypos[3]-1, xPos, OFFSETY+ypos[3]+1);
  // 検出時のマーカー描画
  if(popcount < 0){ // 静電気の影響を受けた表示
    TFTscreen.stroke(0,0,0);
    TFTscreen.line(xPos, HEIGHT, xPos, HEIGHT-8);
    popcount*=-1;
  }
  ypos[0]=popcount*SIZE;
  if(popcount > VERYBAD){
    TFTscreen.stroke(0,255,255);
    TFTscreen.line(xPos, HEIGHT, xPos, HEIGHT-18);
  }else if(popcount >BAD){
    TFTscreen.stroke(0,0,255);
    TFTscreen.line(xPos, HEIGHT, xPos, HEIGHT-9);
  }else if(popcount == GOOD){
    TFTscreen.stroke(255,0, 255);
    TFTscreen.line(xPos, HEIGHT, xPos, HEIGHT-9);
  }else if(popcount == VERYGOOD){
    TFTscreen.stroke(255,0,0);
    TFTscreen.line(xPos, HEIGHT, xPos, HEIGHT-18);
  }
  
  avg=avg * 7/8 + ypos[0];
  TFTscreen.stroke(255,64,255);
  TFTscreen.line(xPos, OFFSETY+avg/8-2, xPos, OFFSETY+avg/8+2);
  TFTscreen.stroke(0,0,0);
  TFTscreen.line(xPos, OFFSETY+ypos[0]-1, xPos, OFFSETY+ypos[0]+1);
}

// M系列の変換
char mseq(){
  // シフトレジスタ
  //static unsigned long Xn = 'b';
  unsigned ret;

  ret = ((Xn & 16384) >> 14) ^ (Xn & 1);
  Xn = (Xn << 1) + ret;

  return ret;
}

//引数にはWDTO_500MS WDTO_1S WDTO_2S WDTO_4Sなどと設定
void wdt_enable_test(byte b){
  //ウォッチドッグ・タイマー分周比設定ビットの４ビット目がWDTCSRレジスタで２ビットずれているための処理
  b = ((0b00001000 & b) << 2) | (0b00000111 & b);
  //WDCE ウォッチドッグ変更許可ビットを立てる
  b |= 0b00010000;
  //WDRF ウォッチドッグ・システム・リセット・フラグにゼロを書き込んでリセットの準備をする
  MCUSR &= 0b11110111;
  //WDE ウォッチドッグ・システム・リセット許可のビットを立ててリセット　WDCE ウォッチドッグ変更許可により4サイクル以内は設定を変更可能
  WDTCSR |= 0b00011000;
  //事前に作成しておいた設定を流し込む
  WDTCSR = b;
  //WDIE ウォッチドッグ・割込み許可ビットを立てる
  WDTCSR |= 0b01000000;
}

//ウォッチドッグの割り込み処理ルーチン(空だけど必要)
ISR(WDT_vect) {
}
