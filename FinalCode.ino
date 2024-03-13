#include <SPI.h>
#include <MFRC522.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <Servo.h> 
#include <Key.h>
#include <Keypad.h>

#define RST_PIN 9
#define SS_PIN 10
#define SERVO_PIN A1
#define BUZZER_PIN A0
#define CARD_MAX 10
#define PASSWORD_LENGTH 3


const byte numRows = 4;
const byte numCols = 3;

char keymap[numRows][numCols] =
{
  { '1','2','3' },
  { '4', '5','6' },
  { '7' ,'8' , '9'},
  {'*','0','#'}
};

byte rowPin[numRows] = {8,7,6,5};
byte colPin[numCols] = {4,3,2};

Keypad keypad = Keypad(makeKeymap(keymap),rowPin,colPin,numRows,numCols);


LiquidCrystal_I2C lcd(0x27, 16, 2);
Servo myservo;          


MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance

void setup() {
	Serial.begin(9600);		// Initialize serial communications with the PC
	while (!Serial);		// Do nothing if no serial port is opened (added for Arduinos based on ATMEGA32U4)
	SPI.begin();			// Init SPI bus
	mfrc522.PCD_Init();		// Init MFRC522
	delay(4);				// Optional delay. Some board do need more time after init to be ready, see Readme
	mfrc522.PCD_DumpVersionToSerial();	// Show details of PCD - MFRC522 Card Reader details
	Serial.println(F("Scan PICC to see UID, SAK, type, and data blocks..."));

  lcd.init();
  lcd.backlight();
  
  pinMode(BUZZER_PIN, OUTPUT);
  myservo.attach(SERVO_PIN);
}

byte cardUID[10][4];
char cardPwd[10][6];
int cardPointer = 0;

bool checkCardRegisted() {
  for (int i = 0; i < CARD_MAX; i++) {
    if (cardUID[i][0] == mfrc522.uid.uidByte[0] && cardUID[i][1] == mfrc522.uid.uidByte[1]) {
      if (cardUID[i][2] == mfrc522.uid.uidByte[2] && cardUID[i][3] == mfrc522.uid.uidByte[3]) {
        return true;
      }
    }
  }
  return false;
}

bool enterPassword(char enterPwd[]) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Enter password: ");
  lcd.setCursor(0, 1);
  
  //Password code
  int i = 0;
  while (i < PASSWORD_LENGTH) {
    char keypressed = keyPressed();
    if(keypressed == NO_KEY) return false;
    if (keypressed != NO_KEY && keypressed!='*' && keypressed!='#') {
      Serial.print(keypressed);
      lcd.setCursor(i, 1);
      lcd.print("*");
      enterPwd[i] = keypressed;
      i++;
    }

  }
  return true;
}
bool checkPassword() {
  char enterPwd[PASSWORD_LENGTH];
  bool result = enterPassword(enterPwd);
  if (!result) return false;
  for (int i = 0; i < CARD_MAX; i++) {
    if (cardUID[i][0] == mfrc522.uid.uidByte[0] && cardUID[i][1] == mfrc522.uid.uidByte[1]) {
      if (cardUID[i][2] == mfrc522.uid.uidByte[2] && cardUID[i][3] == mfrc522.uid.uidByte[3]) {
        for (int j = 0; j < PASSWORD_LENGTH; j++) {
          if (cardPwd[i][j] != enterPwd[j]) {

            break;
          }
          if (j == PASSWORD_LENGTH - 1) return true;
        }
      }
    }
  }
  return false;
}

char keyPressed() {
  unsigned long StartTime = millis();

  char key = keypad.getKey();
  while (key == NO_KEY) {
    unsigned long CurrentTime = millis();
    if( CurrentTime - StartTime >= 3000) return NO_KEY;
    if (key != NO_KEY) {
      break;
    }
    key = keypad.getKey();
  }
  return key;
}

bool assignCard(){
  char enterPwd[PASSWORD_LENGTH] ;
  bool result = enterPassword(enterPwd);
  if(!result) return false;
  // store card uid
  int i = 0;
  while(i<CARD_MAX){
    if(cardUID[i][0] == 0 && cardUID[i][1] == 0 && cardUID[i][2] == 0 && cardUID[i][3] == 0){
      //card index empty
      cardUID[i][0] = mfrc522.uid.uidByte[0];
      cardUID[i][1] = mfrc522.uid.uidByte[1];
      cardUID[i][2] = mfrc522.uid.uidByte[2];
      cardUID[i][3] = mfrc522.uid.uidByte[3];
      break;
    }
    i++;
  }
  //Store password
  for(int j = 0;j < PASSWORD_LENGTH;j++){
    cardPwd[i][j] = enterPwd[j];
  }
  return true;
}

bool authorizeCard() {
  while (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
  }
  // Check if card is registed
  if(checkCardRegisted()){
    //Card is registed
    if(!checkPassword()){
      lcd.clear();
      lcd.print("Password wrong!");
      delay(1000);
      return false;
    }
    else return true;
  }
  else{
    lcd.setCursor(0, 0);
    lcd.print("New card regist?");
    lcd.setCursor(0, 1);
    lcd.print("1.Yes   2.No    ");
    char choice = keyPressed();
    if(choice == '1'){
      //Password code
      if(assignCard()){
        lcd.clear();
      lcd.print("Assign success!");
      delay(1000);
      }
      else {
         lcd.clear();
      lcd.print("Assign fail!");
      delay(1000);
      }
    }
  }
  return false;
}

void loop() {
  myservo.write(0);
  lcd.clear();
  lcd.print("Swipe the card!");

 
  if(authorizeCard()){
    lcd.clear();
    lcd.print("Login success!");
    digitalWrite(BUZZER_PIN, HIGH);
    myservo.write(90);
    delay(1000);
    digitalWrite(BUZZER_PIN, LOW);

    removeCard();
    
  }
  
  
}

void removeCard() {
  lcd.clear();
  lcd.print("Remove card?");
  lcd.setCursor(0, 1);
  lcd.print("1.Yes   2.No    ");
  char choice = keyPressed();

  if (choice == '1' && checkPassword()) {
    int i = 0;
    while (i < CARD_MAX) {
      if (cardUID[i][0] == mfrc522.uid.uidByte[0] && cardUID[i][1] == mfrc522.uid.uidByte[1]) {
        if (cardUID[i][2] == mfrc522.uid.uidByte[2] && cardUID[i][3] == mfrc522.uid.uidByte[3]) {
          cardUID[i][0] = 0;
          cardUID[i][1] = 0;
          cardUID[i][2] = 0;
          cardUID[i][3] = 0;

          lcd.clear();
          lcd.print("Remove success!");
          delay(1000);
          break;
        }
      }
      i++;
    }
  }
}
