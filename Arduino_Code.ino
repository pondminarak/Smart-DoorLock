#include <EEPROM.h>
#include "Keypad.h"
#include "LiquidCrystal_I2C.h"
#include <Adafruit_Fingerprint.h>
#include <avr/wdt.h>
#include <Servo.h>

/* LCD I2C */
LiquidCrystal_I2C lcd(0x27, 16, 2);   // I2C address 0x27, 16 column and 2 rows

/* Fingerprint sensor */
SoftwareSerial mySerial(10, 11);      // Tx = pin 10 , RX = 11
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

#define buzzer 13
#define button 2

/* Servo */  
Servo myservo;

/* Keypad */
const byte ROWS = 4; // Rows
const byte COLS = 4; // Columns
char keys[ROWS][COLS] = {
 {'1','2','3','A'},
 {'4','5','6','B'},
 {'7','8','9','C'},
 {'*','0','#','D'}
};
byte rowPins[ROWS] = {3, 4, 5, 6};        // connect to the row pinouts of the keypad
byte colPins[COLS] = {17, 16, 15, 14};    // connect to the column pinouts of the keypad
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS ); // attach keypad

char password[4];
char initial_password[4],new_password[4];
char key_pressed = 0;
int cnt=0, Check=0;
int set_time = 8;     // stay open 8 second when unlock the door
volatile bool buttonPressed = false;  // button state

void setup(){
  pinMode(button,INPUT); // รับข้อมูลจากขา button
  pinMode(buzzer,OUTPUT); // ส่งออกข้อมูลไปที่ buzzer

  /* Interrupt PIN 2 */
  attachInterrupt(digitalPinToInterrupt(2), INT0_ISR, FALLING);
  Serial.begin(9600);
  finger.begin(57600);
  delay(5);
  if (finger.verifyPassword()) 
  {
  } 
  else 
  {
    while (1) { delay(1); }
  }

  myservo.attach(9);
  myservo.write(90);
  myservo.detach();

  /* แสดงข้อความบน LCD เมื่อเริ่มต้นรันโปรแกรม */
  lcd.begin();
  lcd.backlight();
  lcd.setCursor(0,0);
  lcd.print("     Mairu      ");  //Group name
  lcd.setCursor(0,1);
  lcd.print("Door Lock System");
  delay(2000); // Waiting for a while
  lcd.clear(); 
  lcd.print("  Press # for  ");
  lcd.setCursor(0,1);
  lcd.print("Change  Password");
  delay(2000); // Waiting for a while
  lcd.clear(); 
  lcd.print("Enter Password: ");

  if(EEPROM.read(100)==0){}
  else{
    for(int i=0;i<4;i++){
      EEPROM.write(i, i+49);
    }  
  EEPROM.write(100, 0);
  } 

  /* อ่าน code จาก EEPROM */
  for(int i=0;i<4;i++){ 
    initial_password[i]=EEPROM.read(i);
  }
}

void INT0_ISR() {
  buttonPressed = true;
}

/* ตรวจสอบลายนิ้วมือที่แสกนว่าตรงกับข้อมูลที่อยู่ในฐานข้อมูลหรือไม่ */
int getFingerPrint() 
{
  int p = finger.getImage();
  if (p != FINGERPRINT_OK)  return -1;

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK)  return -1;

  p = finger.fingerFastSearch();
  if (p != FINGERPRINT_OK)  return -1;

  // found a match
  return finger.fingerID;
}

/* ฟังก์ชั่นเปลี่ยนรหัสผ่าน */
void change(){
  int j=0;
  lcd.clear();
  lcd.print("Current Password");
  lcd.setCursor(5,1);
  tone(buzzer, 2000);
  delay(100);
  noTone(buzzer);
  while(j<4){
   char key=keypad.getKey();
    if(key){
      tone(buzzer, 2000);
      delay(200);
      noTone(buzzer);
      new_password[j++]=key;
      lcd.print(key);   
    }
    key=0;
  }
  delay(500);
  if((strncmp(new_password, initial_password, 4))){
    lcd.clear();
    lcd.print("Wrong Password");
    lcd.setCursor(0,1);
    lcd.print("Try Again");
    tone(buzzer, 90);
    delay(200);
    noTone(buzzer);
    delay(200);
    tone(buzzer, 90);
    delay(200);
  }
  else{
    j=0;
    lcd.clear();
    lcd.print("New Password:");
    lcd.setCursor(5,1);
    while(j<4){
      char key=keypad.getKey();
      if(key){
        tone(buzzer, 2000);
        delay(200);
        noTone(buzzer);
        initial_password[j]=key;
        lcd.print(key);
        EEPROM.write(j,key);
        j++;
      }
    }
    lcd.clear();
    lcd.print("Pass Changed");
    delay(1000);
  }
  
  lcd.clear();
  lcd.print("Enter Password:");
  key_pressed=0;
}

void loop(){
  // get a key
  key_pressed = keypad.getKey();

  /* กดปุ่ม restart */
  if (buttonPressed) {
    lcd.clear();
    delay(500);

    lcd.setCursor(0, 0);
    lcd.print("Restarting....");

    // Reset the buttonPressed flag
    buttonPressed = false;

    // Enable the watchdog timer 15ms timeout
    wdt_enable(WDTO_15MS);
    while (true) {} // Wait for the watchdog timer to reset the Arduino
  }

  /* กดปุ่ม 'D' เพื่อลบ password ทีละตัว */
  if(key_pressed=='D'){
  if(cnt>0)password[cnt--];
    lcd.setCursor(5+cnt,1);
    if(cnt>0)lcd.print("*   ");
        else lcd.print("    ");
    tone(buzzer, 2000);
    delay(100);  
  }
  /* กดปุ่ม '#' เพื่อเปลี่ยน password */
  else if(key_pressed == '#'){
    change();
  }
  else{
  /* เมื่อมีการกดปุ่มจะแสดง password เป็น '*' */
  if(key_pressed){
    password[cnt++] = key_pressed;
    lcd.setCursor(5+cnt,1);
    lcd.print("*  ");  
    tone(buzzer, 2000);
    delay(100);
  }  
  }
  delay(100);  
  noTone(buzzer);

  /* ประตูเปิดเมื่อลายนิ้วมือถูกต้อง */
  if(getFingerPrint() != -1){
    lcd.clear();
    lcd.print("Please Wait... ");
    lcd.setCursor(0,1);
    lcd.print(" Door is Opening");
    myservo.attach(9);
    myservo.write(0);
    tone(buzzer, 1000);
    delay(500);
    tone(buzzer, 800);
    delay(500);
    noTone(buzzer);
    lcd.clear();
    lcd.print("    Welcome   ");
    lcd.setCursor(0,1);
    lcd.print("  Door is Open ");

    for(int i=0;i<set_time;i++){
      delay(1000);
    }
    
    lcd.clear();
    lcd.print("Please Wait... ");
    lcd.setCursor(0,1);
    lcd.print(" Door is Closing");
    myservo.write(90);
    delay(2000); 
    lcd.clear();
    lcd.print("Enter Password:");
    myservo.detach();
  }
  /* ประตูเปิดเมื่อ password ถูกต้อง */
  if(cnt>=4){
    delay(200);
    for(int j=0;j<4;j++){
      initial_password[j]=EEPROM.read(j);
    }
    if(!(strncmp(password, initial_password,4))){
      Check=0;
      lcd.clear();
      lcd.print("Please Wait... ");
      lcd.setCursor(0,1);
      lcd.print(" Door is Opening");
      myservo.attach(9);
      myservo.write(0);
      tone(buzzer, 1000);
      delay(500);
      tone(buzzer, 800);
      delay(500);
      noTone(buzzer);
      lcd.clear();
      lcd.print("    Welcome   ");
      lcd.setCursor(0,1);
      lcd.print("  Door is Open ");

      for(int i=0;i<set_time;i++){
        delay(1000);
      }

      lcd.clear();
      lcd.print("Please Wait... ");
      lcd.setCursor(0,1);
      lcd.print(" Door is Closing");
      myservo.write(90);
      delay(2000); 
      lcd.clear();
      lcd.print("Enter Password:");
      myservo.detach();
      cnt=0;
    }

    /* กด password ไม่ถูกต้อง */
    else{ 
      Check = Check+1;
      lcd.clear();
      lcd.print("Wrong Password");
      lcd.setCursor(0,1);
      lcd.print("Pres # to Change");

    /* กด password ไม่ถูกต้องครบ 3 ครั้ง นับถอยหลัง 10 วินาที */
    if(Check>=3){ 
      Check=0;
      lcd.clear();
      lcd.print("Please Wait... ");
      for(int i=10; i>0; i--){
        lcd.setCursor(0,1);
        lcd.print("    Time: ");  
        lcd.print((i/10)%10);
        lcd.print(i%10);
        noTone(buzzer);
        delay(500);
      } 
    /* ส่งเสียงเมื่อกด password ไม่ถูกต้อง */ 
    }else{
      tone(buzzer, 90);
      delay(200);
      noTone(buzzer);
      delay(200);
      tone(buzzer, 90);
      delay(200);
    }
    lcd.clear();
    lcd.print("Enter Password:");
    cnt=0;
  }

}
}