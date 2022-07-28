#define MSEQ_WIDTH 40
#define BYTES  4
#define sensorPin A0  // センサーピンの場所

const unsigned int one=1; // ビット演算のための定数
unsigned int sensorValue;
unsigned long Xn;
union {
  unsigned long int sum=0;
  unsigned char div[BYTES];
} data;

void setup() {
  unsigned int i;
  for(i=0; i<BYTES*8; i++){
    data.sum <<=1;
    data.sum |=i % 2;
  }

  Serial.begin( 9600 ); 
  Xn = analogRead(sensorPin); // M系列乱数のための初期値をアナログ入力から挿入
}

void loop() {
  unsigned char r=mseq();
  unsigned char i;
  unsigned char wait;
  unsigned char parity;
  unsigned char popcount;
  /* M系列ノイズの01列を8ビット整数に変換 */
  for(i=0, wait=0; i<8; i++){
    wait*=2;
    wait+=mseq();
  }
  /* ランダムな時間待つ */
  delay(wait*3);

  // read the value from the sensor:
  sensorValue = analogRead(sensorPin);
  parity=sensorValue & one; // sensorValue % 2 と同義。センサ値の偶奇性を取得
  //parity=1; // デバック用

  // 01情報をdataに蓄積してゆく
  data.sum <<=1;
  data.div[0] |=parity;
  // 1の数を数える
  for(i=0, popcount=0; i<BYTES; i++){
    popcount+=__builtin_popcount(data.div[i]);
  }
  Serial.print(popcount-BYTES*4); // BYTES * 8 個分の乱数の偏り
  Serial.print(", ");
  Serial.print(__builtin_popcount(data.div[0])-4); // 直近 8 個分の乱数の偏り
  Serial.print("\n");
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
