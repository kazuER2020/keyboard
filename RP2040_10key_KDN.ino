// 自作10キー　キーボード
// #include <HID_Keyboard_Pico.h>
// #include <KeyboardLayout.h>
// #include <KeyboardLayout_en_US.h>
// #include <KeyboardLayout_ja_JP.h>
// #include <KeyboardPico.h>
#include <Keyboard.h>

#include <ST7032_asukiaaa.h>
#include <Wire.h>

#define LCD_ADDR 0x3E  // AQM1602Y の I2Cアドレス
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
#define DET_SW1 0x00001
#define DET_SW2 0x00002
#define DET_SW3 0x00004
#define DET_SW4 0x00008
#define DET_SW5 0x00010
#define DET_SW6 0x00020
#define DET_SW7 0x00040
#define DET_SW8 0x00080
#define DET_SW9 0x00100
#define DET_SW10 0x00200
#define DET_SW11 0x00400
#define DET_SW12 0x00800
#define DET_SW13 0x01000
#define DET_SW14 0x02000
#define DET_SW15 0x04000
#define DET_SW16 0x08000
#define DET_SW17 0x10000
#define DET_SW18 0x20000
#define DET_SW19 0x40000
#define DET_SW20 0x80000

#define TENKEY 0
#define CALC 1

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
unsigned char sw_stat[20] = { 0 };

uint32_t nowtime, starttime;
int now_sw, old_sw;
int pattern;
int layer, old_layer;
int lcdClearFlag = 0;
int lcdCursorPos = 0;
uint32_t datas;
char strbuf[16];  // sprintfで文字+変数名を表示させる用
unsigned long sw_det;
char keyChar;
char inputBuffer[17] = "";
uint8_t inputLen = 0;

char layers_array[3][20] = {  // レイヤごとのキー割り当て配列:
  {                           // layer1:
    KEY_BACKSPACE, '/', KEY_KP_ASTERISK, '-',
    '7', '8', '9', KEY_KP_PLUS,
    '4', '5', '6', '=',
    '1', '2', '3', ' ',
    '0', ' ', '.', '\n' },

  { // layer2:
    KEY_BACKSPACE, '/', KEY_KP_ASTERISK, '-',
    KEY_HOME, KEY_UP_ARROW, KEY_PAGE_UP, KEY_BACKSPACE,
    KEY_LEFT_ARROW, '\n', KEY_RIGHT_ARROW, KEY_PAGE_DOWN,
    KEY_LEFT_SHIFT, KEY_DOWN_ARROW, KEY_LEFT_GUI, ' ',
    KEY_LEFT_CTRL, ' ', KEY_DELETE, '\n' },

  { // layer3:
    KEY_ESC, KEY_F2, KEY_F4, KEY_PRINT_SCREEN,
    KEY_HOME, KEY_UP_ARROW, KEY_PAGE_UP, KEY_PRINT_SCREEN,
    KEY_LEFT_ARROW, ' ', KEY_RIGHT_ARROW, KEY_PRINT_SCREEN,
    KEY_LEFT_SHIFT, KEY_DOWN_ARROW, KEY_LEFT_GUI, ' ',
    KEY_LEFT_CTRL, ' ', KEY_DELETE, '\n' }
};

// SoftWireを渡してLCDオブジェクト生成
ST7032_asukiaaa lcd(LCD_ADDR);

unsigned long cnt0, cnt_cht;  // チャタリング防止のためタイマを設ける
int layer_ischange;           // 1:レイヤ変更がある場合 0:レイヤ変更が検出されない場合
int i;
int isclick, reload_time;
int isASCII;

int mode, old_mode;
int isEnternum;
double firstInputValue;
double secondInputValue;
char nowOperand = 0;  // '+', '-', '*', '/'
double calc_result = 0;

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
  /*
  for (int i = 0; i < (sizeof(LEDS) / sizeof(LEDS[0])); i++) {
    digitalWrite(LEDS[i], HIGH);
    delay(200);
  }
  delay(200);
  for (int i = 2; i > -1; i--) {
    digitalWrite(LEDS[2 - i], LOW);
    delay(100);
  }
  */

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

  /* イルミネーション(起動) */
  digitalWrite(LED_RIGHT, HIGH);
  delay(1000);
  digitalWrite(LED_RIGHT, LOW);

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
  lcd.print("10-KEY MODE:   ");
  digitalWrite(LCD_BACKLIGHT_EN, HIGH);  //LCDバックライトON
  lcdCursorPos = 0;
  isASCII = 0;
  isEnternum = 0;
  isclick = 0;
  mode = TENKEY;  // 初回はテンキーとして開始:
  starttime = millis();
}

void loop() {
  // put your main code here, to run repeatedly:
  debug_led(layer);
  nowtime = millis();
  now_sw = get_sw();

  //  Serial.println(pattern);
  switch (mode) {
    case TENKEY:
      // テンキーモードの場合:
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
          isclick = 0;  // 連続押し状態を初期化
          layer_ischange = get_layer_change();
          if (layer_ischange == 1) {  // レイヤ変更があった場合:
            old_layer = layer;
            layer++;                         // レイヤの階層を1進める
            if (layer > (MAX_LAYERS - 1)) {  // レイヤ数上限に達した場合は最初に戻す
              layer = LAYER_1;
            }
            sprintf(strbuf, "LAYER: %d>>%d ", old_layer, layer);
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
            Illmination();
            debug_led(layer);  // レイヤ切り替え状態を表示

            while (get_sw() > 0)
              ;                 // 同時押しが離されるまで待つ
            pattern = SCAN_SW;  // 検出状態を最初に戻す

          } else if (layer_ischange == 2) {  // レイヤ変更ではなくモード切替の場合:
            mode = CALC;                     // モード切キー入力へモード変更
            lcd.setCursor(0, 0);
            lcd.print("10-KEY -> CALC: ");
            lcd.setCursor(0, 1);
            for (int i = 0; i < 16; i++) {
              lcd.print(" ");
            }
            Illmination();
            lcd.setCursor(0, 0);
            lcd.print("CALC MODE:        ");
            while (get_sw() > 0)
              ;  // 同時押しが離されるまで待つ
            pattern = SCAN_SW;

          } else {
            if (nowtime - cnt0 > 5) {  // チャタリング防止:40ms
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
            if (now_sw & sw_det) {
              keyChar = layers_array[layer][i];
              lcd.setCursor(0, 0);
              lcd.print("Key Pressed:");
              if (keyChar >= 32 && keyChar <= 126) {
                if (isASCII == 0) {  // 以前が非ASCII文字の場合、下段をクリアする
                  clearLine(1);
                  lcdCursorPos = 0;
                }
                lcd.setCursor(lcdCursorPos, 1);
                lcd.print((char)keyChar);  // 表示可能なASCII文字
                isASCII = 1;
              } else {
                sprintf(strbuf, "Code: 0x%02X", keyChar);  // 非ASCIIキーコード
                clearLine(1);
                lcd.setCursor(0, 1);
                lcd.print(strbuf);
                isASCII = 0;  // 前回非ASCII文字とする
              }

              if (lcdCursorPos >= 16) {
                // 16文字を超えたら2行目クリア＆先頭に戻す
                clearLine(1);
                lcdCursorPos = 0;
              } else {
                lcdCursorPos++;
              }

              if (keyChar == '=') {
                Keyboard.press(KEY_LEFT_SHIFT);
                Keyboard.press('-');  // 実際は"="
              } else if (keyChar == ' ') {
                // do nothing
              } else {
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
          if (isclick == 1) reload_time = 200;
          else reload_time = 50;

          now_sw = get_sw();

          if (nowtime - cnt0 > reload_time) {
            cnt0 = cnt_cht;
            if (now_sw > 0) {
              pattern = DETECT_SW;
            } else {
              pattern = SCAN_SW;
            }
          } else {
            if (now_sw == 0) {
              pattern = SCAN_SW;
            }
          }
          break;

        default:
          /* NOT REACHED */
          break;
      }
      break;

    case CALC:
      debug_led(0);
      switch (pattern) {
        case SCAN_SW:
          if (now_sw != old_sw) {
            if (inputLen == 0 && isEnternum == 0) {
              lcd.setCursor(0, 0);
              lcd.print("CALC MODE:      ");
              clearBuffer();
            }
            pattern = TURN_ON;
            cnt0 = nowtime;
          }
          break;

        case TURN_ON:
          digitalWrite(LED_RIGHT, HIGH);
          nowtime = millis();
          isclick = 0;
          layer_ischange = get_layer_change();
          if (layer_ischange == 2) {  // モード切替
            mode = TENKEY;            // 電卓→10キー入力へモード変更
            lcd.setCursor(0, 0);
            lcd.print("CALC -> 10-KEY: ");
            lcd.setCursor(0, 1);
            for (int i = 0; i < 16; i++) {
              lcd.print(" ");
            }
            Illmination();

            lcd.setCursor(0, 0);
            lcd.print("10-KEY MODE:   ");
            layer_ischange = 0;
            clearBuffer();
            while (get_sw() > 0)
              ;
            pattern = SCAN_SW;
          } else {
            if (nowtime - cnt0 > 5) {
              pattern = DETECT_SW;
              cnt0 = cnt_cht;
            }
          }
          break;

        case DETECT_SW:
          sw_det = DET_SW1;
          for (int i = 0; i < 20; i++) {
            if (now_sw & sw_det) {
              keyChar = layers_array[0][i];  // CALCモードはレイヤ1固定

              // 数字・小数点・負号（先頭のみ）
              if ((keyChar >= '0' && keyChar <= '9') || keyChar == '.') {
                addChar(keyChar);
                lcdPrintRightAligned(1, inputBuffer);
              } else if (keyChar == '-') {
                // バッファが空なら負号として入力
                if (inputLen == 0) {
                  addChar('-');
                  lcdPrintRightAligned(1, inputBuffer);
                }
                // バッファに値がある場合は演算子として扱う
                else if (inputLen > 0 && isEnternum == 0) {
                  firstInputValue = getValue();
                  nowOperand = '-';
                  lcdPrintRightAligned(0, inputBuffer);
                  addChar(' ');
                  addChar(nowOperand);
                  lcdPrintRightAligned(0, inputBuffer);
                  clearBuffer();
                  lcd.setCursor(0, 1);
                  for (int j = 0; j < 16; j++) lcd.print(" ");  // 下段クリア
                  isEnternum = 1;
                }
              }
              // 演算子 (+ * /)
              else if ((keyChar == KEY_KP_PLUS || keyChar == '=' || keyChar == KEY_KP_ASTERISK || keyChar == '/')) {
                if (inputLen > 0 && isEnternum == 0) {
                  firstInputValue = getValue();
                  if (keyChar == KEY_KP_PLUS) nowOperand = '+';
                  else if (keyChar == '=') nowOperand = '+';
                  else if (keyChar == KEY_KP_ASTERISK) nowOperand = '*';
                  else if (keyChar == '/') nowOperand = '/';
                  lcdPrintRightAligned(0, inputBuffer);
                  addChar(' ');
                  addChar(nowOperand);
                  lcdPrintRightAligned(0, inputBuffer);
                  clearBuffer();
                  lcd.setCursor(0, 1);
                  for (int j = 0; j < 16; j++) lcd.print(" ");  // 下段クリア
                  isEnternum = 1;
                }
              }
              // バックスペース
              else if (keyChar == KEY_BACKSPACE) {
                removeChar();
                lcdPrintRightAligned(1, inputBuffer);
              }
              // = キー
              else if (keyChar == '\n') {
                if (isEnternum == 1 && inputLen > 0) {
                  secondInputValue = getValue();
                  switch (nowOperand) {
                    case '+': calc_result = firstInputValue + secondInputValue; break;
                    case '-': calc_result = firstInputValue - secondInputValue; break;
                    case '*': calc_result = firstInputValue * secondInputValue; break;
                    case '/':
                      if (secondInputValue != 0) calc_result = firstInputValue / secondInputValue;
                      else {
                        clearBuffer();
                        strcpy(inputBuffer, "ERR!");
                        inputLen = strlen(inputBuffer);
                        lcdPrintRightAligned(0, inputBuffer);
                        break;
                      }
                      break;
                  }

                  // 結果表示
                  char temp[17];
                  long intPart = (long)calc_result;
                  int intLen = 1;
                  long tmp = labs(intPart);
                  while (tmp >= 10) {
                    tmp /= 10;
                    intLen++;
                  }
                  int maxPrecision = 16 - intLen - 1;
                  if (maxPrecision < 0) maxPrecision = 0;
                  if (maxPrecision > 10) maxPrecision = 10;
                  dtostrf(calc_result, 0, maxPrecision, temp);
                  trimTrailingZeros(temp);
                  strncpy(inputBuffer, temp, 16);
                  inputBuffer[16] = '\0';
                  inputLen = strlen(inputBuffer);
                  lcd.setCursor(0, 0);
                  lcd.print("Result:         ");
                  lcdPrintRightAligned(1, inputBuffer);

                  isEnternum = 0;  // 計算終了
                }
              }
            }
            sw_det <<= 1;
          }
          isclick++;
          cnt0 = nowtime;
          pattern = RELOAD_SW;
          break;

        case RELOAD_SW:
          if (isclick == 1) reload_time = 200;
          else reload_time = 50;

          now_sw = get_sw();

          if (nowtime - cnt0 > reload_time) {
            cnt0 = cnt_cht;
            if (now_sw > 0) {
              pattern = DETECT_SW;
            } else {
              pattern = SCAN_SW;
            }
          } else {
            if (now_sw == 0) {
              pattern = SCAN_SW;
            }
          }
          break;
      }
      break;

    default:
      /*NOT REACHED*/
      break;
      old_sw = now_sw;
      old_mode = mode;
  }
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

  k = 0;                                                    // 現在のSW読み込み位置
  for (i = 0; i < (sizeof(rows) / sizeof(rows[0])); i++) {  // ROW側
    set_ROW(i);
    for (j = 0; j < (sizeof(cols) / sizeof(cols[0])); j++) {  //COL側
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
  if ((s & DET_SW2) && (s & DET_SW3)) {
    ret = 2;
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

  digitalWrite(LEDS[1], (0x02 & layer) >> 1);
  digitalWrite(LEDS[0], 0x01 & layer);
}

// 指定行をクリアする関数
void clearLine(int line) {
  lcd.setCursor(0, line);
  for (int i = 0; i < 16; i++) {  // 16文字のLCDの場合
    lcd.print(" ");
  }
}

// 文字入力（負号・小数点・桁数チェック）
void addChar(char c) {
  if (inputLen >= 16) return;

  if (c == '-' && inputLen == 0) {
    inputBuffer[inputLen++] = c;
    inputBuffer[inputLen] = '\0';
    return;
  }

  if ((c >= '0' && c <= '9') || c == '.') {
    if (c == '.' && strchr(inputBuffer, '.') != NULL) return;
    inputBuffer[inputLen++] = c;
    inputBuffer[inputLen] = '\0';
    return;
  }

  // 演算子とスペースも表示できるようにする
  if (c == '+' || c == '-' || c == '*' || c == '/' || c == ' ') {
    inputBuffer[inputLen++] = c;
    inputBuffer[inputLen] = '\0';
  }
}

// バックスペース
void removeChar(void) {
  if (inputLen > 0) {
    inputLen--;
    inputBuffer[inputLen] = '\0';
  }
}

// LCD右詰め表示
void lcdPrintRightAligned(uint8_t row, const char* str) {
  lcd.setCursor(0, row);
  lcd.print("                ");  // 行クリア
  char buf[17];
  snprintf(buf, sizeof(buf), "%16s", str);
  lcd.setCursor(0, row);
  lcd.print(buf);
}

// LCD左詰め表示
void lcdPrintLeftAligned(uint8_t row, const char* str) {
  lcd.setCursor(0, row);
  // まず行をクリア
  for (int i = 0; i < 16; i++) {
    lcd.print(" ");
  }
  // 先頭に戻して表示
  lcd.setCursor(0, row);
  lcd.print(str);
}

// 数値として取得
double getValue() {
  return atof(inputBuffer);
}

// バッファクリア
void clearBuffer() {
  inputLen = 0;
  inputBuffer[0] = '\0';
}

// 不要な末尾の0を削除
void trimTrailingZeros(char* str) {
  char* p = str + strlen(str) - 1;
  while (p > str && *p == '0') p--;
  if (*p == '.') p--;
  *(p + 1) = '\0';
}

void Illmination(void) {
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
}
