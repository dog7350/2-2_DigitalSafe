#include <Servo.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>
#include <IRremote.h>

#define Rpow -23971 //전원버튼
#define Rreset -26521 //리셋버튼
#define R0 26775 //0 Key
#define R1 12495 //1 Key
#define R2 6375 //2 Key
#define R3 31365 //3 Key
#define R4 4335 //4 Key
#define R5 14535 //5 Key
#define R6 23205 //6 Key
#define R7 17085 //7 Key
#define R8 19125 //8 Key
#define R9 21165 //9 Key
#define Open_Door 10
#define Close_Door 105

int pow_button = 13;
int open_button = 12;
int gled1 = 4;
int gled2 = 5;
int gled3 = 6;
int buzzer = 7;
int receiver = 8;
int echo = 9;
int trig = 10;
int servoPin = 11;
int wled = A3;
int rled = A2;
int Lvar = A1;
int Rvar = A0;

Servo servo;
LiquidCrystal_I2C lcd(0x27, 16, 2);
SoftwareSerial HM10(3, 2);
IRrecv irrecv(receiver);
decode_results decoSignal;

void setup()
{
  HM10.begin(9600);
  Serial.begin(9600);
  irrecv.enableIRIn();
  servo.attach(servoPin);
  servo.write(Close_Door);
  lcd.begin();
  lcd.noBacklight();
  pinMode(echo, INPUT);
  pinMode(trig, OUTPUT);
  pinMode(buzzer, OUTPUT);
  pinMode(pow_button, INPUT);
  pinMode(open_button, INPUT);
  pinMode(gled1, OUTPUT);
  pinMode(gled2, OUTPUT);
  pinMode(gled3, OUTPUT);
  pinMode(wled, OUTPUT);
  pinMode(rled, OUTPUT);
}

///////////////////////// 전역 변수 부분 /////////////////////////

int POW_flag = false; // 전원 활성화 여부 플래그

int Admin_pw[5] = {2, 0, 2, 9, 9}; // 블루투스 관리자 코드
int Admin_use[5]; // 관리자 입력 배열
int Admin_con = 0; // 관리자 입력 수 카운트
int Admin_chg_pw = 0; // 관리자 코드 변경 카운트
int Admin_command; // 관리자 명령 번호

int User_key[3] = {2, 4, 6}; // 일반 사용자 비밀번호
int User_chg_pw = 0; // 일반 사용자 비밀번호 변경 카운트
int User_use[3]; // 일반 사용자 입력 배열
int User_con = 0; // 일반 사용자 입력 수 카운트
int User_usenum; // 일반 사용자 입력 번호 매핑
int User_Try_count = 0; // 일반 사용자 비밀번호 시도 횟수

int S = 0;
int B = 0;
int O = 0;

long duration; // 지속시간
long cm = 0; // cm 거리
int Lvel = 0; // 왼쪽 다이얼
int Rvel = 0; // 오른쪽 다이얼

boolean Door_flag = false; // 문 상태 플래그
boolean Admin_flag = false; // 관리자 확인 플래그
boolean User_flag = false; // 일반 사용자 확인 플래그
boolean US_flag = false; // 초음파 성공 여부 플래그
boolean Dial_flag = false; // 다이얼 성공 여부 플래그
boolean User_Numflag; // 일반 사용자 입력 번호 플래그
boolean UPW_flag = false; // 비밀번호 성공 여부 플래그

unsigned long last = millis(); // 리모콘 시간 컨트롤

///////////////////////// 함수 부분 /////////////////////////

void POWER_ON() // 전원 켜기 //
{
  All_Reset();
  POW_flag = true;
  lcd.clear();
  lcd.backlight();
  servo.attach(servoPin); // 연결이 끊겼을 경우 다시 연결
  servo.write(Close_Door);
  Default_Manual();
}

void POWER_OFF() // 전원 끄기 //
{
  POW_flag = false;
  lcd.clear();
  lcd.noBacklight();
  All_Reset();
}

void OpenDoor() // 문 열기 //
{
  lcd.clear();
  lcd.print("Door OPEN");
  servo.attach(servoPin);
  servo.write(Open_Door);
  digitalWrite(wled, HIGH);
  digitalWrite(buzzer, HIGH);
  delay(1000);
  digitalWrite(buzzer, LOW);
}

void CloseDoor() // 문 닫기 //
{
  lcd.clear();
  lcd.print("Door CLOSE");
  servo.attach(servoPin);
  servo.write(Close_Door);
  digitalWrite(wled, LOW);
  digitalWrite(buzzer, HIGH);
  delay(1000);
  digitalWrite(buzzer, LOW);
}

void All_Reset() // 모든 정보 초기화 //
{
  servo.attach(servoPin); // 연결이 끊겼을 경우 다시 연결
  servo.write(Close_Door);
  Door_flag = false;
  Admin_flag = false;
  User_flag = false;
  Admin_con = 0;
  User_con = 0;
  User_Try_count = 0;
  digitalWrite(gled1, LOW);
  digitalWrite(gled2, LOW);
  digitalWrite(gled3, LOW);
  digitalWrite(rled, LOW);
  digitalWrite(wled, LOW);
  US_flag = false;
  Dial_flag = false;
  UPW_flag = false;
  PW_Reset();
}

void lcdClear(int row) // LCD 한 줄 초기화 //
{
  lcd.setCursor(0, row);
  lcd.print("                ");
  lcd.setCursor(0, row);
}

void Default_Manual() // 기본 메뉴얼 //
{
  lcd.clear();
  lcd.println("Admin : BT Conn ");
  lcd.setCursor(0, 1);
  lcd.print("User : Comeon");
  lcd.home();
}

void Admin_Manual() // 관리자 메뉴얼 //
{
  lcd.clear();
  lcd.print("1 : User PW Chg");
  lcd.setCursor(0, 1);
  lcd.print("2 : Admin PW Chg");
  delay(1500);
  lcd.home();
  lcd.clear();
  lcd.print("3 : Open & Close");
  lcd.setCursor(0, 1);
  lcd.print("4 : Admin END");
  delay(1500);
  lcd.clear();
  lcd.print("5 : Manual");
  delay(1500);
  lcd.home();
}

int BT_INTreturn() // 관리자 BT 입력 받기 //
{
  Admin_command = HM10.read();
  Admin_command = Admin_command - '0';
  return Admin_command;
}

long microsecondsToCentimeters(long microseconds) // 초음파 센서 CM 측정 //
{
    return microseconds / 29 / 2;
}

void US_Check() // 초음파 거리 측정 //
{
    digitalWrite(trig, LOW);
    delayMicroseconds(2);
    digitalWrite(trig, HIGH);
    delayMicroseconds(10);
    digitalWrite(trig, LOW);
    duration = pulseIn(echo, HIGH);
    cm = microsecondsToCentimeters(duration);

    lcdClear(0);
    lcd.print("US Check");
    lcdClear(1);
    lcd.print("CM : ");
    lcd.print(cm);
    delay(500);

    if(cm <= 30)
    {
      US_flag = true;
      lcdClear(0);
      lcd.print("US Check OK");
      digitalWrite(buzzer, HIGH);
      digitalWrite(gled1, HIGH);
      delay(1000);
      digitalWrite(buzzer, LOW);
      lcd.clear();
    }
    else
    {
      US_flag = false;
      digitalWrite(gled1, LOW);
    }
}

void Dial_Check() // 다이얼 입력 측정 //
{
  Lvel = analogRead(Lvar);
  Rvel = analogRead(Rvar);
  Lvel = Lvel / 10;
  Rvel = Rvel / 10;

  lcd.clear();
  lcd.print("Dial Check");
  lcd.setCursor(0, 1);
  lcd.print("L : ");
  lcd.print(Lvel);
  lcd.print(", R : ");
  lcd.print(Rvel);
  delay(500);

  if((Lvel >= 54 && Lvel <= 57) && (Rvel >= 86 && Rvel <= 89))
  {
    Dial_flag = true;
    lcdClear(0);
    lcd.print("Dial Check OK");
    digitalWrite(buzzer, HIGH);
    digitalWrite(gled2, HIGH);
    delay(1000);
    digitalWrite(buzzer, LOW);
    lcd.clear();
    lcd.print("PW Check");
  }
  else
  {
    Dial_flag = false;
    digitalWrite(gled2, LOW);
  }
}

void PW_Reset() // 비밀번호 초기화
{
  for(int i = 0; i < 3; i++)
  {
    User_use[i] = NULL;
  }
  User_con = 0;
  S = 0;
  B = 0;
  O = 0;
}

void Strike() // 스트라이크 게임 //
{
  lcd.setCursor(5, 1);
  
  for(int i = 0; i < 3; i++)
  {
    if(User_key[i] == User_usenum)
    {
      if(i == User_con)
      {
        S++;
        break;
      }
      else
      {
        B++;
      }
    }
  }

  O = 3 - (S + B);

  lcd.print("S:");
  lcd.print(S);
  lcd.print(",B:");
  lcd.print(B);
  lcd.print(",O:");
  lcd.print(O);
}

void Remote_Check() // 비밀번호 체크 //
{
  if(User_Try_count >= 3)
  {
    POWER_OFF();
  }
  if(irrecv.decode(&decoSignal) == true)
  {
    if(millis() - last > 250)
    {
      User_usenum = decoSignal.value;
      if(User_usenum == Rpow)
      {
        if(User_con != 3)
        {
          PW_Reset();
          lcd.clear();
          lcd.print("PW Reset");
          User_Try_count++;
          lcd.setCursor(0, 1);
          lcd.print("Try : ");
          lcd.print(User_Try_count);
          digitalWrite(rled, HIGH);
          digitalWrite(buzzer, HIGH);
          delay(1000);
          digitalWrite(rled, LOW);
          digitalWrite(buzzer, LOW);
          lcd.clear();
          lcd.print("PW Check 3 NUM");
        }
        else if(User_con == 3)
        {
          UPW_flag = true;
          for(int i = 0; i < 3; i++)
          {
            if(User_use[i] != User_key[i])
            {
              UPW_flag = false;
              break;
            }
            else
            {
              continue;
            }
          }
          if(UPW_flag == true)
          {
            digitalWrite(gled3, HIGH);
            digitalWrite(buzzer, HIGH);
            lcdClear(0);
            lcd.print("PW OK");
            delay(1000);
            digitalWrite(buzzer, LOW);
            lcd.clear();
            lcd.print("OPEN BUTTON PUSH");
          }
          else if(UPW_flag == false)
          {
            PW_Reset();
            lcd.clear();
            lcd.print("PW Reset");
            User_Try_count++;
            lcd.setCursor(0, 1);
            lcd.print("Try : ");
            lcd.print(User_Try_count);
            digitalWrite(rled, HIGH);
            digitalWrite(buzzer, HIGH);
            delay(1000);
            digitalWrite(rled, LOW);
            digitalWrite(buzzer, LOW);
            lcd.clear();
            lcd.print("PW Check");
          }
        }
      }
      else
      {
        User_Numflag = false;
        if(User_con >= 3)
        {
          PW_Reset();
          lcd.clear();
          lcd.print("PW Reset");
          User_Try_count++;
          lcd.setCursor(0, 1);
          lcd.print("Try : ");
          lcd.print(User_Try_count);
          digitalWrite(rled, HIGH);
          digitalWrite(buzzer, HIGH);
          delay(1000);
          digitalWrite(rled, LOW);
          digitalWrite(buzzer, LOW);
          lcd.clear();
          lcd.print("PW Check");
          last = millis();
          irrecv.resume();
          return;
        }
        switch(User_usenum)
        {
          case R0:
           User_usenum = 0;
           User_Numflag = true;
           break;
          case R1:
           User_usenum = 1;
           User_Numflag = true;
           break;
          case R2:
           User_usenum = 2;
           User_Numflag = true;
           break;
          case R3:
           User_usenum = 3;
           User_Numflag = true;
           break;
          case R4:
           User_usenum = 4;
           User_Numflag = true;
           break;
          case R5:
           User_usenum = 5;
           User_Numflag = true;
           break;
          case R6:
           User_usenum = 6;
           User_Numflag = true;
           break;
          case R7:
           User_usenum = 7;
           User_Numflag = true;
           break;
          case R8:
           User_usenum = 8;
           User_Numflag = true;
           break;
          case R9:
           User_usenum = 9;
           User_Numflag = true;
           break;
          case Rreset:
           PW_Reset();
           lcd.clear();
           lcd.print("PW Reset");
           digitalWrite(rled, HIGH);
           digitalWrite(buzzer, HIGH);
           delay(1000);
           digitalWrite(rled, LOW);
           digitalWrite(buzzer, LOW);
           lcd.clear();
           lcd.print("PW Check");
           break;
          default :
           PW_Reset();
           lcd.clear();
           lcd.print("PW Reset");
           digitalWrite(rled, HIGH);
           digitalWrite(buzzer, HIGH);
           delay(1000);
           digitalWrite(rled, LOW);
           digitalWrite(buzzer, LOW);
           lcd.clear();
           lcd.print("PW Check");
           lcd.clear();
           lcd.print("NOT NUMBER");
           digitalWrite(buzzer, HIGH);
           digitalWrite(rled, HIGH);
           delay(1000);
           digitalWrite(buzzer, LOW);
           digitalWrite(rled, LOW);
           lcd.clear();
           lcd.print("PW Check");
        }
        if(User_Numflag == true)
        {
          // 스트라이크 게임 //
          Strike();
          
          lcd.setCursor(User_con, 1);
          lcd.print(User_usenum);
          User_use[User_con] = User_usenum;
          User_con++;
        }
      }
    }
    last = millis();
    irrecv.resume();
  }
}

///////////////////////// 실행 부분 /////////////////////////
void loop()
{
  if(digitalRead(pow_button) == HIGH && POW_flag == false)
  {
    POWER_ON();
    
    delay(500);
  } // 파워 ON //
  else if(digitalRead(pow_button) == HIGH && POW_flag == true)
  {
    POWER_OFF();
    
    delay(500);
  } // 파워 OFF //

  if(POW_flag == true) // 파워 연결 실행 //
  {
    servo.detach(); // 서보모터 연결 해제 //

    if(HM10.available() && Admin_flag == false) // 관리자 연결 여부 //
    {
      lcd.clear();
      lcd.home();
      lcd.print("Admin Conn");
      Admin_use[Admin_con] = BT_INTreturn();
      lcd.setCursor(Admin_con, 1);
      lcd.print(Admin_use[Admin_con]);
      Admin_con++;
      if(Admin_con >= 5)
      {
        lcd.clear();
        Admin_flag = true;
        for(int i = 0; i < 5; i++)
        {
          if(Admin_pw[i] != Admin_use[i])
          {
            Admin_flag = false;
            lcd.clear();
            lcd.print("Admin : Fail");
            delay(1000);
            Default_Manual();
          }
        }
        if(Admin_flag == true)
        {
          User_flag = false;
          lcd.clear();
          lcd.print("Welcome Admin");
          digitalWrite(gled1, HIGH);
          digitalWrite(gled2, HIGH);
          digitalWrite(gled3, HIGH);
          digitalWrite(rled, HIGH);
          digitalWrite(buzzer, HIGH);
          delay(1000);
          digitalWrite(buzzer, LOW);
          Admin_Manual();
          lcd.clear();
          lcd.print("INPUT MENU");
        }
        Admin_con = 0;
      } // 관리자 검증 //
    }
      
    if(Admin_flag == true && User_flag == false) // 관리자 접속 //
    {
      Admin_command = BT_INTreturn();
      if(Admin_command == 1)
      {
        User_chg_pw = 0;
        lcd.clear();
        lcd.print("Chang PW 3 NUM");
        lcd.setCursor(0, 1);
        while(User_chg_pw < 3)
        {
          if(HM10.available())
          {
            Admin_command = BT_INTreturn();
            User_key[User_chg_pw] = Admin_command;
            lcd.print(Admin_command);
            User_chg_pw++;
          }
        }
        digitalWrite(buzzer, HIGH);
        lcd.clear();
        lcd.print("User PW Chg");
        delay(1000);
        digitalWrite(buzzer, LOW);
        lcd.clear();
        lcd.print("INPUT MENU");
      }
      else if(Admin_command == 2) // 관리자 코드 변경 //
      {
        Admin_chg_pw = 0;
        lcd.clear();
        lcd.print("Change PW 5 NUM");
        lcd.setCursor(0, 1);
        while(Admin_chg_pw < 5)
        {
          if(HM10.available())
          {
            Admin_command = BT_INTreturn();
            Admin_pw[Admin_chg_pw] = Admin_command;
            lcd.print(Admin_command);
            Admin_chg_pw++;
          }
        }
        digitalWrite(buzzer, HIGH);
        lcd.clear();
        lcd.print("Admin PW Chg");
        delay(1000);
        digitalWrite(buzzer, LOW);
        All_Reset();
        Default_Manual();
      }
      else if(Admin_command == 3) // 관리자 도어 열기/닫기 //
      {
        Door_flag = !Door_flag;
        if(Door_flag == false)
        {
          CloseDoor();
        }
        else if(Door_flag == true)
        {
          OpenDoor();
        }
        lcd.clear();
        lcd.print("INPUT MENU");
      }
      else if(Admin_command == 4) // 관리자 모드 종료 //
      {
        All_Reset();
        Default_Manual();
      }
      else if(Admin_command == 5) // 관리자 메뉴얼 출력 //
      {
        Admin_Manual();
        lcd.clear();
        lcd.print("INPUT MENU");
      }
      // 관리자 모드 //
    }
    else if(User_flag == false)
    {
      if(digitalRead(open_button) == HIGH)
      {
        User_flag = true;
        lcd.clear();
        lcd.print("User CONNECT");
        delay(1000);
        lcd.clear();
        lcd.print("US Check");
      }
    }
    else if(User_flag == true && Admin_flag == false) // 일반 사용자 접속 //
    {
      if(US_flag == false)
      {
        US_Check();
      }
      else if(US_flag == true)
      {
        if(Dial_flag == false)
        {
          Dial_Check();
        }
        else if(Dial_flag == true)
        {
          if(UPW_flag == false)
          {
            Remote_Check();
          }
          else if(UPW_flag == true)
          {
            if(digitalRead(open_button) == HIGH && Door_flag == false)
            {
              OpenDoor();
              Door_flag = !Door_flag;
            }
            else if(digitalRead(open_button) == HIGH && Door_flag == true)
            {
              CloseDoor();
              Door_flag = !Door_flag;
              All_Reset();
              Default_Manual();
            }
          }
        }
      }
    }
  } // 파워 ON 상태의 실행 //
}
