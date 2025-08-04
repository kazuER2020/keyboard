// 自作10キー　キーボード
// #include <HID_Keyboard_Pico.h>
// #include <KeyboardLayout.h>
// #include <KeyboardLayout_en_US.h>
// #include <KeyboardLayout_ja_JP.h>
// #include <KeyboardPico.h>
#include <Keyboard.h>

#include <ST7032_asukiaaa.h>
#include <Wire.h>

#define LCD_ADDR   0x3E   // AQM1602Y の I2Cアドレス
/* SWのスキャン状態の遷移 */
#define SCAN_SW 0
#define TURN_ON 1
#define DETECT_SW 2
#define RELOAD_SW 3

/*キーボードのレイヤー分け*/
#define LAYER_1 0
#define LAYER_2 1
#define LAYER_3 2
#define MAX_LAYERS 3

#define SW_DATA_MAX 0xFFFFF

/* SW判定個別検出用 */
#define DET_SW1   0x00001
#define DET_SW2   0x00002
#define DET_SW3   0x00004
#define DET_SW4   0x00008
#define DET_SW5   0x00010
#define DET_SW6   0x00020
#define DET_SW7   0x00040
#define DET_SW8   0x00080
#define DET_SW9   0x00100
#define DET_SW10  0x00200
#define DET_SW11  0x00400
#define DET_SW12  0x00800
#define DET_SW13  0x01000
#define DET_SW14  0x02000
#define DET_SW15  0x04000
#define DET_SW16  0x08000
#define DET_SW17  0x10000
#define DET_SW18  0x20000
#define DET_SW19  0x40000
#define DET_SW20  0x80000

/* ポート定義 */
const int LED_PIN = 16;  // RPi2040上のRGB LED(GP16)
const int ROW0 = 14;
const int ROW1 = 15;
const int ROW2 = 26;
const int ROW3 = 27;
const int ROW4 = 28;

const int COL0 = 0;
const int COL1 = 1;
const int COL2 = 2;
const int COL3 = 5;

const int LED_LEFT = 3;
const int LED_CENTER = 4;
const int LED_RIGHT = 8;

const int LCD_BACKLIGHT_EN = 29;
const int KEYLED_EN1 = 9;
const int KEYLED_EN2 = 10;
const int KEYLED_EN3 = 11;
const int KEYLED_EN4 = 12;
const int KEYLED_EN5 = 13;

const int I2C_SDA_LCD = 6;
const int I2C_SCL_LCD = 7;

const int rows[5] = {
  // ROW0, ROW1, ROW2, ROW3, ROW4
  ROW0, ROW1, ROW2, ROW3, ROW4
  // 14, 15, 16, 26, 27, 28
};
const int cols[4] = {
  COL0, COL1, COL2, COL3
  // 0, 1, 2, 5
};

const int LEDS[3] = {
  // LEFT, CENTER, RIGHT
  LED_LEFT, LED_CENTER, LED_RIGHT
};
unsigned char sw_stat[20] = {0};

uint32_t nowtime, starttime;
int now_sw, old_sw;
int pattern;
int layer, old_layer;
int lcdClearFlag = 0;
int lcdCursorPos = 0;
uint32_t datas;
char strbuf[16]; // sprintfで文字+変数名を表示させる用
unsigned long sw_det;
char keyChar;

char layers_array[3][20] = { // レイヤごとのキー割り当て配列:
  { // layer1:
    KEY_BACKSPACE, '/', KEY_KP_ASTERISK, '-',
    '7', '8', '9', KEY_KP_PLUS,
    '4', '5', '6', '=',
    '1', '2', '3', ' ',
    '0', ' ', '.', '\n'
  },

  { // layer2:
    KEY_BACKSPACE , '/'         , KEY_KP_ASTERISK, '-',
    KEY_HOME      , KEY_UP_ARROW, KEY_PAGE_UP    , KEY_BACKSPACE,
    KEY_LEFT_ARROW, '\n'        , KEY_RIGHT_ARROW, KEY_PAGE_DOWN,
    KEY_LEFT_SHIFT, KEY_DOWN_ARROW, KEY_LEFT_GUI, ' ',
    KEY_LEFT_CTRL , ' '         , KEY_DELETE     , '\n'
  },

  { // layer3:
    KEY_ESC       , KEY_F2      , KEY_F4         , KEY_PRINT_SCREEN,
    KEY_HOME      , KEY_UP_ARROW, KEY_PAGE_UP    , KEY_PRINT_SCREEN,
    KEY_LEFT_ARROW, ' '         , KEY_RIGHT_ARROW, KEY_PRINT_SCREEN,
    KEY_LEFT_SHIFT, KEY_DOWN_ARROW, KEY_LEFT_GUI, ' ',
    KEY_LEFT_CTRL , ' '         , KEY_DELETE     , '\n'
  }
};

// SoftWireを渡してLCDオブジェクト生成
ST7032_asukiaaa lcd(LCD_ADDR);

unsigned long cnt0, cnt_cht;  // チャタリング防止のためタイマを設ける
int layer_ischange;           // 1:レイヤ変更がある場合 0:レイヤ変更が検出されない場合
int i;
int isclick, reload_time;
int isASCII;

void setup() {
  // put your setup code here, to run once:
  pinMode(ROW0, OUTPUT);
  pinMode(ROW1, OUTPUT);
  pinMode(ROW2, OUTPUT);
  pinMode(ROW3, OUTPUT);
  pinMode(ROW4, OUTPUT);

  pinMode(COL0, INPUT);
  pinMode(COL1, INPUT);
  pinMode(COL2, INPUT);
  pinMode(COL3, INPUT);

  pinMode(LED_LEFT, OUTPUT);
  pinMode(LED_CENTER, OUTPUT);
  pinMode(LED_RIGHT, OUTPUT);

  pinMode(LCD_BACKLIGHT_EN, OUTPUT);
  pinMode(KEYLED_EN1, OUTPUT);
  pinMode(KEYLED_EN2, OUTPUT);
  pinMode(KEYLED_EN3, OUTPUT);
  pinMode(KEYLED_EN4, OUTPUT);
  pinMode(KEYLED_EN5, OUTPUT);

  pattern = SCAN_SW;
  layer = LAYER_1;
  lcdClearFlag = 0;
  // Serial.begin(115200);         // ← これを必ず書く
  //  while (!Serial) {             // ← 書き込んだ後、シリアルが開くまで待機
  //    delay(10);
  //  }


  Keyboard.begin();
  for (int i = 0; i < (sizeof(LEDS) / sizeof(LEDS[0])); i++) {
    digitalWrite(LEDS[i], HIGH);
    delay(200);
  }
  delay(200);
  for (int i = 2; i > -1; i--) {
    digitalWrite(LEDS[2 - i], LOW);
    delay(100);
  }

  //  Serial.println("Hello from RP2040!");
  // I2Cピンの設定（Pico用）
  // I2C設定時はWire1を使用すること。(Wireは使用しない)
  // setSCL,setSDAはWire1.beginの前に実行すること。
  Wire1.setSDA(I2C_SDA_LCD);
  Wire1.setSCL(I2C_SCL_LCD);
  Wire1.begin();
  lcd.setWire(&Wire1);
  lcd.begin(16, 2);
  lcd.setContrast(40);

  /* イルミネーション */
  for (int i = KEYLED_EN1; i <= KEYLED_EN5; i++) {
    digitalWrite(i, HIGH);
    delay(100);
  }
  for (int i = KEYLED_EN1; i <= KEYLED_EN5; i++) {
    digitalWrite(i, LOW);
    delay(100);
  }
  delay(300);
  for (int i = KEYLED_EN1; i <= KEYLED_EN5; i++) {
    digitalWrite(i, HIGH);
  }
  /* イルミネーションここまで */

  lcd.setCursor(0, 0);
  lcd.print("10-KEYBOARD");
  digitalWrite(LCD_BACKLIGHT_EN, HIGH); //LCDバックライトON
  lcdCursorPos = 0;
  isASCII = 0;
  starttime = millis();
}

void loop() {
  // put your main code here, to run repeatedly:
  debug_led(layer);
  nowtime = millis();
  now_sw = get_sw();
  //  Serial.println(pattern);

  switch (pattern) {
    // SWの全スキャン
    case SCAN_SW:
      digitalWrite(LED_RIGHT, LOW);
      if (now_sw != old_sw) {
        pattern = TURN_ON;
        cnt0 = nowtime;
      }
      break;

    // SWのどれかが押された時
    case TURN_ON:
      nowtime = millis();
      isclick = 0; // 連続押し状態を初期化
      layer_ischange = get_layer_change();
      if (layer_ischange == 1) {   // レイヤ変更があった場合
        old_layer = layer;
        layer++;                   // レイヤの階層を1進める
        if (layer > (MAX_LAYERS - 1)) { // レイヤ数上限に達した場合は最初に戻す
          layer = LAYER_1;
        }
        sprintf(strbuf, "LAYER: %d>>%d", old_layer, layer);
        //lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(strbuf);

        lcdCursorPos = 0;
        clearLine(1);

        for (i = 0; i < 3; i++) {  // レイヤ切り替えをLED点滅で表示
          digitalWrite(LED_LEFT, HIGH);
          digitalWrite(LED_CENTER, HIGH);
          digitalWrite(LED_RIGHT, HIGH);
          delay(100);
          digitalWrite(LED_LEFT, LOW);
          digitalWrite(LED_CENTER, LOW);
          digitalWrite(LED_RIGHT, LOW);
          delay(100);
        }
        /* イルミネーション */
        for (int i = KEYLED_EN1; i <= KEYLED_EN5; i++) {
          digitalWrite(i, LOW);
          delay(100);
        }
        for (int i = KEYLED_EN1; i <= KEYLED_EN5; i++) {
          digitalWrite(i, HIGH);
          delay(100);
        }
        /* イルミネーションここまで */
        debug_led(layer);  // レイヤ切り替え状態を表示
        lcd.setCursor(0, 0);
        lcd.print("10-KEYBOARD ");
        
        while (now_sw > 0); // 同時押しが離されるまで待つ
        pattern = SCAN_SW;  // 検出状態を最初に戻す

      } else {
        if (nowtime - cnt0 > 40) { // チャタリング防止:40ms
          pattern = DETECT_SW;
          cnt0 = cnt_cht;
        }
      }
      break;

    // ここから下にSWが押された時の挙動を書く
    case DETECT_SW:
      digitalWrite(LED_RIGHT, HIGH);
      sw_det = DET_SW1;

      for (int i = 0; i < 20; i++) {
        if (now_sw & sw_det ) {
          keyChar = layers_array[layer][i];
          lcd.setCursor(0, 0);
          lcd.print("Key Pressed:");
          if (keyChar >= 32 && keyChar <= 126) {
            if (isASCII == 0) { // 以前が非ASCII文字の場合、下段をクリアする
              clearLine(1);
              lcdCursorPos = 0;
            }
            lcd.setCursor(lcdCursorPos, 1);
            lcd.print((char)keyChar);  // 表示可能なASCII文字
            isASCII = 1;
          }
          else {
            sprintf(strbuf, "Code: 0x%02X", keyChar); // 非ASCIIキーコード
            clearLine(1);
            lcd.setCursor(0, 1);
            lcd.print(strbuf);
            isASCII = 0; // 前回非ASCII文字とする
          }

          if (lcdCursorPos >= 16) {
            // 16文字を超えたら2行目クリア＆先頭に戻す
            clearLine(1);
            lcdCursorPos = 0;
          }
          else {
            lcdCursorPos++;
          }

          if (keyChar == '=') {
            Keyboard.press(KEY_LEFT_SHIFT);
            Keyboard.press('-'); // 実際は"="
          }
          else if (keyChar == ' ') {
            // do nothing
          }
          else {
            Keyboard.press(keyChar);
          }
        }
        sw_det <<= 0x0001;
      }
      isclick++;
      Keyboard.releaseAll();  // 押しているキーがある場合は離す
      cnt0 = nowtime;
      pattern = RELOAD_SW;
      break;


    case RELOAD_SW:
      if (isclick == 1) reload_time = 350;
      else reload_time = 50;

      now_sw = get_sw();

      if (nowtime - cnt0 > reload_time ) {
        cnt0 = cnt_cht;
        if (now_sw > 0) {
          pattern = DETECT_SW;
        }
        else {
          pattern = SCAN_SW;
        }
      }
      else {
        if (now_sw == 0) {
          pattern = SCAN_SW;
        }
      }
      break;

    default:
      /* NOT REACHED */
      break;
  }

  old_sw = now_sw;

}

/************************************************************************/
/* SW取得(20個)                                                          */
/* 引数 なし　　　　　　　　                                              */
/* 戻り値 1:ON 0:OFF                                                     */
/************************************************************************/
uint32_t get_sw(void) {
  uint32_t ret;
  int i, j, k;
  int now_row;
  uint32_t portdata = 0x0001;

  k = 0; // 現在のSW読み込み位置
  for (i = 0; i < (sizeof(rows) / sizeof(rows[0])); i++) { // ROW側
    set_ROW(i);
    for (j = 0; j < (sizeof(cols) / sizeof(cols[0])); j++) { //COL側
      sw_stat[k] = digitalRead(cols[j]);
      k++;
    }
  }
  ret = 0;

  for (i = 0; i < (sizeof(sw_stat) / sizeof(sw_stat[0])); i++) {
    ret += sw_stat[i] * portdata;
    portdata <<= 1;
  }
  ret = SW_DATA_MAX - ret;
  return ret;
}

void set_ROW(int row) {
  int i;
  // まず全部消す
  for (i = 0; i < 5; i++) {
    digitalWrite(rows[i], HIGH);
  }
  // ROWxxをGPIOの位置に変換
  digitalWrite(rows[row], LOW);
}

/************************************************************************/
/* SWのレイヤ変更検出                                                    */
/* 引数 なし　　　　　　　　                                              */
/* 戻り値 1:レイヤ変更検出 0:レイヤ変更しない(そのまま)                    */
/************************************************************************/
int get_layer_change(void) {
  int ret;
  int s;
  ret = 0;
  s = get_sw();
  if ((s & DET_SW1) && (s & DET_SW2)) {
    ret = 1;
  }
  return ret;
}

/************************************************************************/
/* debug用LED                                                           */
/* 引数 led_pcb:PCBnew用LED  led_esm:Eschema用LED layer:現在のレイヤ数    */
/* 戻り値  なし　　　　　　　　　　　　　　　　　　　　　　　　　　　　　　    */
/* メモ 2bitのLEDとして現在のレイヤを識別するために使用                    */
/************************************************************************/
void debug_led(int layer) {
  int i;

  // まず全部消す
  for (i = 0; i < sizeof(LEDS) / sizeof(LEDS[0]); i++) {
    digitalWrite(LEDS[i], LOW);
  }

  digitalWrite(LEDS[1], (0x02 & layer) >> 1);
  digitalWrite(LEDS[0], 0x01 & layer);
}

// 指定行をクリアする関数
void clearLine(int line) {
  lcd.setCursor(0, line);
  for (int i = 0; i < 16; i++) { // 16文字のLCDの場合
    lcd.print(" ");
  }
}
