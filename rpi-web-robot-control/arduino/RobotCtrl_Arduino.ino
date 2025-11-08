/*
  從 RPi 收 CSV: "L,R,V\n"
    L/R: -255..255  (左右馬達 PWM；負號代表反轉)
    V  : 0/1        (吸塵關/開)

  接線腳位：
    L298N ENA ← D5  (PWM)
    L298N IN1 ← D7
    L298N IN2 ← D8
    L298N IN3 ← D9
    L298N IN4 ← D10
    L298N ENB ← D6  (PWM)
    吸塵繼電器 ← D4  (HIGH=開；若相反，將 VAC_ACTIVE_HIGH 改為 false)
*/

/// ====== 腳位設定 ======
const int VAC_PIN = 4;                 // 吸塵開關輸出腳
const bool VAC_ACTIVE_HIGH = true;     // true: HIGH=開；false: LOW=開

// L298N 左馬達
const int L_PWM = 5;                   // ENA (PWM 腳)
const int L_IN1 = 7;                   // IN1
const int L_IN2 = 8;                   // IN2

// L298N 右馬達
const int R_PWM = 6;                   // ENB (PWM 腳)
const int R_IN1 = 9;                   // IN3
const int R_IN2 = 10;                  // IN4

/// ====== 其他參數 ======
const int PWM_MAX = 255;

/// ====== 狀態變數 ======
int curL = 0, curR = 0;
bool curVac = false;

/// ====== 工具函式 ======
int clampPWM(int x){
  if (x > PWM_MAX)  x = PWM_MAX;
  if (x < -PWM_MAX) x = -PWM_MAX;
  return x;
}

void setVacuum(bool on){
  curVac = on;
  if (VAC_ACTIVE_HIGH) {
    digitalWrite(VAC_PIN, on ? HIGH : LOW);
  } else {
    digitalWrite(VAC_PIN, on ? LOW : HIGH);
  }
}

void driveOne_L298N(int pwmPin, int in1, int in2, int val){
  int v = abs(val);
  if (v > PWM_MAX) v = PWM_MAX;

  if (val >= 0){
    digitalWrite(in1, HIGH);
    digitalWrite(in2, LOW);
  }else{
    digitalWrite(in1, LOW);
    digitalWrite(in2, HIGH);
  }
  analogWrite(pwmPin, v);
}

void applyMotors(int L, int R){
  driveOne_L298N(L_PWM, L_IN1, L_IN2, L);
  driveOne_L298N(R_PWM, R_IN1, R_IN2, R);
  curL = L; curR = R;
}

void stopAll(){
  applyMotors(0, 0);
  setVacuum(false);
}

// 解析一行 "L,R,V"
bool parseLine(String line, int &L, int &R, bool &V){
  line.trim();
  if (line.length() == 0) return false;

  int c1 = line.indexOf(',');
  int c2 = line.indexOf(',', c1+1);
  if (c1 < 0 || c2 < 0) return false;

  L = line.substring(0, c1).toInt();
  R = line.substring(c1+1, c2).toInt();
  int vraw = line.substring(c2+1).toInt();
  V = (vraw != 0);
  return true;
}

/// ====== 進入點 ======
void setup(){
  Serial.begin(115200);

  pinMode(VAC_PIN, OUTPUT);
  setVacuum(false);

  pinMode(L_IN1, OUTPUT); pinMode(L_IN2, OUTPUT);
  pinMode(R_IN1, OUTPUT); pinMode(R_IN2, OUTPUT);
  pinMode(L_PWM, OUTPUT); pinMode(R_PWM, OUTPUT);
  applyMotors(0,0);

  Serial.println("ARDUINO READY (L298N, no timeout)");
}

void loop(){
  static String buf;

  // 讀取一行
  while (Serial.available()){
    char ch = (char)Serial.read();
    if (ch == '\n'){
      int L, R; bool V;
      if (parseLine(buf, L, R, V)){
        L = clampPWM(L);
        R = clampPWM(R);
        applyMotors(L, R);
        setVacuum(V);
        // Serial.print("OK "); Serial.print(L); Serial.print(","); Serial.print(R); Serial.print(","); Serial.println(V?1:0);
      }
      buf = "";
    }else{
      buf += ch;
      if (buf.length() > 48) buf = ""; // 防爆
    }
  }
}
