/*
Project for "Paczkuś"
ESP32 Dev Module
Code created by Igor Barć
Student of University of Technology in Rzeszow
*/


#include <Keypad.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

const char* ssid = "moto g(7) plus 9900";
const char* password = "mateusz1";

LiquidCrystal_I2C lcd(0x27, 16, 2);

const byte ROWS = 4;
const byte COLS = 3;
char key[ROWS][COLS] = {
  { '1', '2', '3' },
  { '4', '5', '6' },
  { '7', '8', '9' },
  { '*', '0', '#' }
};
byte rowPins[ROWS] = { 18, 5, 17, 16 };
byte colPins[COLS] = { 4, 0, 2 };

Keypad keypad = Keypad(makeKeymap(key), rowPins, colPins, ROWS, COLS);

int stan = 99;
int machinePassword = 111111;
String serverName = "https://paczkomatdatabaseapi.azurewebsites.net/api/paczkomat/PDKP1/package/";
String serverPDKP1S1 = "https://paczkomatdatabaseapi.azurewebsites.net/api/paczkomat/PDKP1/PDKP1S1/package/";
String serverPDKP1S2 = "https://paczkomatdatabaseapi.azurewebsites.net/api/paczkomat/PDKP1/PDKP1S2/package/";
String serverService = "https://paczkomatdatabaseapi.azurewebsites.net/api/paczkomat/inspection/";
String servicePhone = "555226622";
String serviceCode = "445566";
String phone, code, locker, serverPath;
int stan_, order;
char znak;
bool zamek1, zamek2, alert, full, errorSending, checkin, checkinService, message, alarmMessage, codeAlarm, alarmCodeMessage, noConnectionMessage;
unsigned long start, now;



//------------------------------------------------------------------- SETUP I LOOP ----------------------------------------------------//

void setup() {
  pinMode(13, OUTPUT);        // brzeczyk - aktualnie nieuzywany
  pinMode(15, OUTPUT);        // dioda alarmowa
  pinMode(12, OUTPUT);        // dioda sygnalizujace zamek skrytki 1
  pinMode(14, OUTPUT);        // dioda sygnalizujaca zamek skrytki 2
  pinMode(26, INPUT_PULLUP);  // kontaktron skrytki 1
  pinMode(27, INPUT_PULLUP);  // kontaktron skrytki 2
  Serial.begin(115200);
  lcd.init();
  lcd.backlight();
  lcd.home();
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  lcd.print("Laczenie z WiFi");
  lcd.setCursor(0, 1);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  lcd.clear();
  lcd.home();
  lcd.print("Adres IP:");
  lcd.setCursor(0, 1);
  lcd.print(WiFi.localIP());
  delay(2000);
}

void loop() {
  if (!alert && !codeAlarm) {
    if (WiFi.status() == WL_CONNECTED) {
      switch (stan) {
        case 99:  //wybor uzytkownik/obsluga
          zamek1 = false;
          zamek2 = false;
          code = "";
          phone = "";
          locker = " ";
          if (!message) {
            msgWho();
            message = true;
          }
          numer(1);
          if (znak == '1') {  //uzytkownik
            nowTime();
            stan = 29;
          } else if (znak == '2') {  //obsluga
            nowTime();
            stan = 0;
          }
          break;
        // ------> OBSŁUGA <------ //
        case 0:  //wybor obslugi
          zamek1 = false;
          zamek2 = false;
          code = "";
          phone = "";
          if (!message) {
            msgService();
            message = true;
          }
          numer(1);
          if (znak == '1') {  //kurier
            nowTime();
            stan = 1;
          } else if (znak == '2') {  //serwis
            nowTime();
            stan = 24;
          } else if (znak == '*' || !tim())
            stan = 99;
          break;
        // ------> KURIER <------ //
        case 1:  //wybor opcji kuriera
          zamek1 = false;
          zamek2 = false;
          code = "";
          phone = "";
          if (!message) {
            msgCourier();
            message = true;
          }
          numer(1);
          if (znak == '1') {  //odbior
            nowTime();
            stan = 2;
          } else if (znak == '2') {  //nadanie
            nowTime();
            stan = 13;
          } else if (znak == '*' || !tim()) {
            nowTime();
            stan = 0;
          }
          break;
        // ------> ODBIÓR PACZEK PRZEZ KURIERA <------ //
        case 2:  //podaj numer telefonu
          zamek1 = false;
          zamek2 = false;
          if (!message) {
            msgOptions(4);
            message = true;
          }
          numer(2);
          if (znak == '#') {
            nowTime();
            stan = 3;
          } else if (znak == '*' || !tim()) {
            nowTime();
            stan = 1;
          }
          break;
        case 3:  //podaj kod odbioru
          zamek1 = false;
          zamek2 = false;
          if (!message) {
            msgOptions(1);
            message = true;
          }
          numer(3);
          if (znak == '#')
            stan = 4;
          else if (znak == '*' || !tim()) {
            nowTime();
            stan = 1;
          }
          break;
        case 4:  //sprawdzanie kodu odbioru
          zamek1 = false;
          zamek2 = false;
          if (!message) {
            msgChecking();
            message = true;
          }
          serverPath = serverName + "picking";
          check(serverPath);
          if (checkin && locker == "PDKP1S1") {
            nowTime();
            stan = 5;
          } else if (checkin && locker == "PDKP1S2") {
            nowTime();
            stan = 9;
          } else if (!checkin) {
            msgInvalidCode();
            nowTime();
            stan = 1;
            delay(2000);
          }
          break;
        case 5:  //otwarcie drzwi skrytki 1
          zamek1 = true;
          zamek2 = false;
          if (!message) {
            lockers(1);
            message = true;
          }
          if (digitalRead(26) == HIGH) {
            nowTime();
            stan = 6;
          } else if (!tim())
            stan = 99;
          break;
        case 6:  //opcje odbioru paczki ze skrytki 1 (wyjście/otwórz ponownie)
          zamek1 = true;
          zamek2 = false;
          lcd.autoscroll();
          lcd.scrollDisplayLeft();
          if (!message) {
            msgPackage();
            message = true;
          }
          numer(1);
          if (znak == '*') {
            nowTime();
            stan = 5;
          } else if (digitalRead(26) == HIGH && !tim())
            stan = 7;
          else if (((znak == '#') || !tim()) && digitalRead(26) == LOW)
            stan = 8;
          delay(500);
          break;
        case 7:  //zamknij skrytke 1
          zamek1 = true;
          zamek2 = false;
          if (!message) {
            msgCloseDoor();
            message = true;
          }
          if (digitalRead(26) == LOW) {
            nowTime();
            stan = 6;
          }
          break;
        case 8:  //wiadomość o wyjęciu paczki ze skrytki 1
          zamek1 = false;
          zamek2 = false;
          msgEnd();
          serverPath = serverPDKP1S1 + String(order) + "/picked";
          check(serverPath);
          delay(2000);
          stan = 99;
          break;
        case 9:  //otwarcie drzwi skrytki 2
          zamek1 = false;
          zamek2 = true;
          if (!message) {
            lockers(2);
            message = true;
          }
          if (digitalRead(27) == HIGH) {
            nowTime();
            stan = 10;
          } else if (!tim())
            stan = 99;
          break;
        case 10:  //opcje odbioru paczki ze skrytki 2 (wyjście/otwórz ponownie)
          zamek1 = false;
          zamek2 = true;
          lcd.autoscroll();
          lcd.scrollDisplayLeft();
          if (!message) {
            msgPackage();
            message = true;
          }
          numer(1);
          if (znak == '*') {
            nowTime();
            stan = 9;
          } else if (digitalRead(27) == HIGH && !tim())
            stan = 11;
          else if ((znak == '#' || !tim()) && digitalRead(27) == LOW)
            stan = 12;
          delay(500);
          break;
        case 11:  //zamknij skrytke 2
          zamek1 = false;
          zamek2 = true;
          if (!message) {
            msgCloseDoor();
            message = true;
          }
          if (digitalRead(27) == LOW) {
            nowTime();
            stan = 10;
          }
          break;
        case 12:  //wiadomość o wyjęciu paczki ze skrytki 2
          zamek1 = false;
          zamek2 = false;
          msgEnd();
          serverPath = serverPDKP1S2 + String(order) + "/picked";
          check(serverPath);
          delay(2000);
          stan = 99;
          break;
        // ------> NADANIE PACZEK PRZEZ KURIERA <------ //
        case 13:  //podaj numer telefonu
          zamek1 = false;
          zamek2 = false;
          if (!message) {
            msgOptions(4);
            message = true;
          }
          numer(2);
          if (znak == '#') {
            nowTime();
            stan = 14;
          } else if (znak == '*' || !tim()) {
            nowTime();
            stan = 1;
          }
          break;
        case 14:  //podaj kod nadania
          zamek1 = false;
          zamek2 = false;
          if (!message) {
            msgOptions(2);
            message = true;
          }
          numer(3);
          if (znak == '#') {
            nowTime();
            stan = 15;
          } else if (znak == '*' || !tim()) {
            nowTime();
            stan = 1;
          }
          break;
        case 15:  //sprawdzanie kodu nadania
          zamek1 = false;
          zamek2 = false;
          if (!message) {
            msgChecking();
            message = true;
          }
          serverPath = serverName + "delivering";
          check(serverPath);
          if (checkin && locker == "PDKP1S1") {
            nowTime();
            stan = 16;
          } else if (checkin && locker == "PDKP1S2") {
            nowTime();
            stan = 20;
          } else if (!checkin) {
            msgInvalidCode();
            nowTime();
            stan = 1;
            delay(2000);
          }
          break;
        case 16:  //otwarcie drzwi skrytki 1
          zamek1 = true;
          zamek2 = false;
          if (!message) {
            lockers(1);
            message = true;
          }
          if (digitalRead(26) == HIGH) {
            nowTime();
            stan = 17;
          } else if (!tim())
            stan = 99;
          break;
        case 17:  //opcje nadania paczki ze skrytki 1 (wyjście/otwórz ponownie)
          zamek1 = true;
          zamek2 = false;
          lcd.autoscroll();
          lcd.scrollDisplayLeft();
          if (!message) {
            msgPackage();
            message = true;
          }
          numer(1);
          if (znak == '*') {
            nowTime();
            stan = 16;
          } else if (digitalRead(26) == HIGH && !tim())
            stan = 18;
          else if (((znak == '#') || !tim()) && digitalRead(26) == LOW)
            stan = 19;
          delay(500);
          break;
        case 18:  //zamknij skrytke 1
          zamek1 = true;
          zamek2 = false;
          if (!message) {
            msgCloseDoor();
            message = true;
          }
          if (digitalRead(26) == LOW) {
            nowTime();
            stan = 17;
          }
          break;
        case 19:  //wiadomość o włożeniu paczki do skrytki 1
          zamek1 = false;
          zamek2 = false;
          msgEnd();
          serverPath = serverPDKP1S1 + String(order) + "/delivered";
          check(serverPath);
          delay(2000);
          stan = 99;
          break;
        case 20:  //otwarcie drzwi skrytki 2
          zamek1 = false;
          zamek2 = true;
          if (!message) {
            lockers(2);
            message = true;
          }
          if (digitalRead(27) == HIGH) {
            nowTime();
            stan = 21;
          } else if (!tim())
            stan = 99;
          break;
        case 21:  //opcje nadania paczki ze skrytki 2 (wyjście/otwórz ponownie)
          zamek1 = false;
          zamek2 = true;
          lcd.autoscroll();
          lcd.scrollDisplayLeft();
          if (!message) {
            msgPackage();
            message = true;
          }
          numer(1);
          if (znak == '*') {
            nowTime();
            stan = 20;
          } else if (digitalRead(27) == HIGH && !tim())
            stan = 22;
          else if ((znak == '#' || !tim()) && digitalRead(27) == LOW)
            stan = 23;
          delay(500);
          break;
        case 22:  //zamknij skrytke 2
          zamek1 = false;
          zamek2 = true;
          if (!message) {
            msgCloseDoor();
            message = true;
          }
          if (digitalRead(27) == LOW) {
            nowTime();
            stan = 21;
          }
          break;
        case 23:  //wiadomość o wlozeniu paczki do skrytki 2
          zamek1 = false;
          zamek2 = false;
          msgEnd();
          serverPath = serverPDKP1S2 + String(order) + "/delivered";
          check(serverPath);
          delay(2000);
          stan = 99;
          break;
        // ------> SERWIS PACZKUSIA <------ //
        case 24:  //podaj numer telefonu serwisanta
          zamek1 = false;
          zamek2 = false;
          if (!message) {
            msgOptions(4);
            message = true;
          }
          numer(2);
          if (znak == '#') {
            nowTime();
            stan = 25;
          } else if (znak == '*' || !tim()) {
            nowTime();
            stan = 0;
          }
          break;
        case 25:  //podaj kod serwisanta
          zamek1 = false;
          zamek2 = false;
          if (!message) {
            msgOptions(3);
            message = true;
          }
          numer(3);
          if (znak == '#')
            stan = 26;
          else if (znak == '*' || !tim())
            stan = 0;
          break;
        case 26:  //sprawdzanie kodów
          zamek1 = false;
          zamek2 = false;
          if (!message) {
            msgChecking();
            message = true;
          }
          serverPath = serverService + "start";
          checkService(serverPath, 1);
          if (checkinService) {
            nowTime();
            stan = 27;
          } else if (!checkinService) {
            full = false;
            nowTime();
            msgInvalidCode();
            stan = 0;
            delay(2000);
          }
          break;
        case 27:  //otwarte skrytki, czas na serwis
          zamek1 = true;
          zamek2 = true;
          if (!message) {
            lcd.clear();
            lcd.home();
            lcd.print("     SERWIS");
            message = true;
          }
          serviceTime();
          numer(1);
          if (digitalRead(26) == LOW && digitalRead(27) == LOW && (znak == '#' || !serviceTime()))
            stan = 28;
          else if (znak == '*'/* || ((digitalRead(26) == HIGH || digitalRead(27) == HIGH) && !serviceTime())*/)
            nowTime();
          break;
        case 28:  //wiadomosc o koncu serwisu
          zamek1 = false;
          zamek2 = false;
          serverPath = serverService + String(order) + "/ended";
          checkService(serverPath, 0);
          msgEnd();
          delay(2000);
          stan = 99;
          break;
        // ------> UŻYTKOWNIK <------ //
        case 29:  //wybor opcji uzytkownika
          zamek1 = false;
          zamek2 = false;
          code = "";
          phone = "";
          if (!message) {
            msgUser();
            message = true;
          }
          numer(1);
          if (znak == '1') {  //odbior
            nowTime();
            stan = 30;
          } else if (znak == '2') {  //nadanie
            nowTime();
            stan = 41;
          } else if (znak == '*' || !tim())
            stan = 99;
          break;
        // ------> ODBIÓR PACZEK PRZEZ UŻYTKOWNIKA <------ //
        case 30:  //podaj numer telefonu
          zamek1 = false;
          zamek2 = false;
          if (!message) {
            msgOptions(4);
            message = true;
          }
          numer(2);
          if (znak == '#') {
            nowTime();
            stan = 31;
          } else if (znak == '*' || !tim()) {
            nowTime();
            stan = 29;
          }
          break;
        case 31:  //podaj kod odbioru
          zamek1 = false;
          zamek2 = false;
          if (!message) {
            msgOptions(1);
            message = true;
          }
          numer(3);
          if (znak == '#')
            stan = 32;
          else if (znak == '*' || !tim()) {
            nowTime();
            stan = 29;
          }
          break;
        case 32:  //sprawdzanie kodu odbioru
          zamek1 = false;
          zamek2 = false;
          if (!message) {
            msgChecking();
            message = true;
          }
          serverPath = serverName + "receiving";
          check(serverPath);
          if (checkin && locker == "PDKP1S1") {
            nowTime();
            stan = 33;
          } else if (checkin && locker == "PDKP1S2") {
            nowTime();
            stan = 37;
          } else if (!checkin) {
            msgInvalidCode();
            nowTime();
            stan = 29;
            delay(2000);
          }
          break;
        case 33:  //otwarcie drzwi skrytki 1
          zamek1 = true;
          zamek2 = false;
          if (!message) {
            lockers(1);
            message = true;
          }
          if (digitalRead(26) == HIGH) {
            nowTime();
            stan = 34;
          } else if (!tim())
            stan = 99;
          break;
        case 34:  //opcje odbioru paczki ze skrytki 1 (wyjście/otwórz ponownie)
          zamek1 = true;
          zamek2 = false;
          lcd.autoscroll();
          lcd.scrollDisplayLeft();
          if (!message) {
            msgPackage();
            message = true;
          }
          numer(1);
          if (znak == '*') {
            nowTime();
            stan = 33;
          } else if (digitalRead(26) == HIGH && !tim())
            stan = 35;
          else if (((znak == '#') || !tim()) && digitalRead(26) == LOW)
            stan = 36;
          delay(500);
          break;
        case 35:  //zamknij skrytke 1
          zamek1 = true;
          zamek2 = false;
          if (!message) {
            msgCloseDoor();
            message = true;
          }
          if (digitalRead(26) == LOW) {
            nowTime();
            stan = 34;
          }
          break;
        case 36:  //wiadomość o wyjęciu paczki ze skrytki 1
          zamek1 = false;
          zamek2 = false;
          msgEnd();
          serverPath = serverPDKP1S1 + String(order) + "/received";
          check(serverPath);
          delay(2000);
          stan = 99;
          break;
        case 37:  //otwarcie drzwi skrytki 2
          zamek1 = false;
          zamek2 = true;
          if (!message) {
            lockers(2);
            message = true;
          }
          if (digitalRead(27) == HIGH) {
            nowTime();
            stan = 38;
          } else if (!tim())
            stan = 99;
          break;
        case 38:  //opcje odbioru paczki ze skrytki 2 (wyjście/otwórz ponownie)
          zamek1 = false;
          zamek2 = true;
          lcd.autoscroll();
          lcd.scrollDisplayLeft();
          if (!message) {
            msgPackage();
            message = true;
          }
          numer(1);
          if (znak == '*') {
            nowTime();
            stan = 37;
          } else if (digitalRead(27) == HIGH && !tim())
            stan = 39;
          else if ((znak == '#' || !tim()) && digitalRead(27) == LOW)
            stan = 40;
          delay(500);
          break;
        case 39:  //zamknij skrytke 2
          zamek1 = false;
          zamek2 = true;
          if (!message) {
            msgCloseDoor();
            message = true;
          }
          if (digitalRead(27) == LOW) {
            nowTime();
            stan = 38;
          }
          break;
        case 40:  //wiadomość o wyjęciu paczki ze skrytki 2
          zamek1 = false;
          zamek2 = false;
          msgEnd();
          serverPath = serverPDKP1S2 + String(order) + "/received";
          check(serverPath);
          delay(2000);
          stan = 99;
          break;
        // ------> NADANIE PACZEK PRZEZ UŻYTKOWNIKA <------ //
        case 41:  //podaj numer telefonu
          zamek1 = false;
          zamek2 = false;
          if (!message) {
            msgOptions(4);
            message = true;
          }
          numer(2);
          if (znak == '#') {
            nowTime();
            stan = 42;
          } else if (znak == '*' || !tim()) {
            nowTime();
            stan = 29;
          }
          break;
        case 42:  //podaj kod nadania
          zamek1 = false;
          zamek2 = false;
          if (!message) {
            msgOptions(2);
            message = true;
          }
          numer(3);
          if (znak == '#')
            stan = 43;
          else if (znak == '*' || !tim()) {
            nowTime();
            stan = 29;
          }
          break;
        case 43:  //sprawdzanie kodu nadania
          zamek1 = false;
          zamek2 = false;
          if (!message) {
            msgChecking();
            message = true;
          }
          serverPath = serverName + "insertion";
          check(serverPath);
          if (checkin && locker == "PDKP1S1") {
            nowTime();
            stan = 44;
          } else if (checkin && locker == "PDKP1S2") {
            nowTime();
            stan = 48;
          } else if (!checkin) {
            msgInvalidCode();
            nowTime();
            stan = 29;
            delay(2000);
          }
          break;
        case 44:  //otwarcie drzwi skrytki 1
          zamek1 = true;
          zamek2 = false;
          if (!message) {
            lockers(1);
            message = true;
          }
          if (digitalRead(26) == HIGH) {
            nowTime();
            stan = 45;
          } else if (!tim())
            stan = 99;
          break;
        case 45:  //opcje nadania paczki ze skrytki 1 (wyjście/otwórz ponownie)
          zamek1 = true;
          zamek2 = false;
          lcd.autoscroll();
          lcd.scrollDisplayLeft();
          if (!message) {
            msgPackage();
            message = true;
          }
          numer(1);
          if (znak == '*') {
            nowTime();
            stan = 44;
          } else if (digitalRead(26) == HIGH && !tim())
            stan = 46;
          else if (((znak == '#') || !tim()) && digitalRead(26) == LOW)
            stan = 47;
          delay(500);
          break;
        case 46:  //zamknij skrytke 1
          zamek1 = true;
          zamek2 = false;
          if (!message) {
            msgCloseDoor();
            message = true;
          }
          if (digitalRead(26) == LOW) {
            nowTime();
            stan = 45;
          }
          break;
        case 47:  //wiadomość o włożeniu paczki do skrytki 1
          zamek1 = false;
          zamek2 = false;
          msgEnd();
          serverPath = serverPDKP1S1 + String(order) + "/inserted";
          check(serverPath);
          delay(2000);
          stan = 99;
          break;
        case 48:  //otwarcie drzwi skrytki 2
          zamek1 = false;
          zamek2 = true;
          if (!message) {
            lockers(2);
            message = true;
          }
          if (digitalRead(27) == HIGH) {
            nowTime();
            stan = 49;
          } else if (!tim()) {
            stan = 99;
          }
          break;
        case 49:  //opcje nadania paczki ze skrytki 2 (wyjście/otwórz ponownie)
          zamek1 = false;
          zamek2 = true;
          lcd.autoscroll();
          lcd.scrollDisplayLeft();
          if (!message) {
            msgPackage();
            message = true;
          }
          numer(1);
          if (znak == '*') {
            nowTime();
            stan = 48;
          } else if (digitalRead(27) == HIGH && !tim())
            stan = 50;
          else if ((znak == '#' || !tim()) && digitalRead(27) == LOW)
            stan = 51;
          delay(500);
          break;
        case 50:  //zamknij skrytke 2
          zamek1 = false;
          zamek2 = true;
          if (!message) {
            msgCloseDoor();
            message = true;
          }
          if (digitalRead(27) == LOW) {
            nowTime();
            stan = 49;
          }
          break;
        case 51:  //wiadomość o wlozeniu paczki do skrytki 2
          zamek1 = false;
          zamek2 = false;
          msgEnd();
          serverPath = serverPDKP1S2 + String(order) + "/inserted";
          check(serverPath);
          delay(2000);
          stan = 99;
          break;
      }
      alarmMessage = false;
      alarmCodeMessage = false;
      noConnectionMessage = false;
      //noTone(13);
      digitalWrite(15, LOW);
    } else {  // brak internetu
      digitalWrite(15, LOW);
      if (!noConnectionMessage) {
        lcd.noAutoscroll();
        lcd.clear();
        lcd.home();
        lcd.print("BRAK POLACZENIA");
        lcd.setCursor(0, 1);
        lcd.print("   Z SERWEREM");
        noConnectionMessage = true;
      }
      message = false;
      alarmCodeMessage = false;
      alarmMessage = false;
      WiFi.mode(WIFI_STA);
      WiFi.begin(ssid, password);
      for (int i = 0; i < 1000; i++) {
        Serial.print(".");
      }
    }
  } else {  // alarm wlaczony
    digitalWrite(15, HIGH);
    if (!alarmMessage) {
      //tone(13, 440);
      lcd.noAutoscroll();
      lcd.clear();
      lcd.home();
      lcd.print("     ALARM");
      lcd.setCursor(0, 1);
      lcd.print("WEZWANO OCHRONE");
      Serial.println("\nAlarm");
      if (WiFi.status() == WL_CONNECTED)
        alarmSend();
      else
        delay(2000);
      alarmMessage = true;
    }
    if (!(digitalRead(26) == HIGH && !zamek1) && !(digitalRead(27) == HIGH && !zamek2)) {  // wpisywanie kodu aby wylaczyc alarm
      if (!alarmCodeMessage) {
        code = "";
        lcd.clear();
        lcd.print("PODAJ KOD:");
        lcd.setCursor(1, 1);
        alarmCodeMessage = true;
      }
      numer(3);
      if (znak == '#') {
        if (code == "1234") {
          code = "";
          codeAlarm = false;
        } else {
          lcd.clear();
          lcd.home();
          lcd.print(" BLEDNY KOD!");
          delay(2000);
          alarmCodeMessage = false;
        }
      } else if (znak == '*')
        alarmCodeMessage = false;
    }
    message = false;
    noConnectionMessage = false;
  }
  if (stan != stan_)  // zmiana stanu
    message = false;
  stan_ = stan;
  znak = ' ';
  if (!codeAlarm) {  // zalaczenie alarmu
    if ((digitalRead(26) == HIGH && !zamek1) || (digitalRead(27) == HIGH && !zamek2)) {
      alert = true;
      codeAlarm = true;
    } else
      alert = false;
  }
  if (stan == 5 || stan == 16 || stan == 27 || stan == 33 || stan == 44)
    digitalWrite(12, HIGH);  // skrytka 1 otwarta
  else
    digitalWrite(12, LOW);  // skrytka 1 zamknieta

  if (stan == 9 || stan == 20 || stan == 27 || stan == 37 || stan == 48)
    digitalWrite(14, HIGH);  // skrytka 2 otwarta
  else
    digitalWrite(14, LOW);  // skrytka 2 zamknieta
  delay(200);
}

//------------------------------------------------------------ FUNKCJE -------------------------------------------------------//

void msgService() {
  lcd.clear();
  lcd.home();
  lcd.print("1 -> KURIER");
  lcd.setCursor(0, 1);
  lcd.print("2 -> SERWIS");
}

void msgCourier() {
  lcd.clear();
  lcd.home();
  lcd.print("1 -> WYJECIE");
  lcd.setCursor(0, 1);
  lcd.print("2 -> WLOZENIE");
}

void msgUser() {
  lcd.clear();
  lcd.home();
  lcd.print("1 -> ODBIOR");
  lcd.setCursor(0, 1);
  lcd.print("2 -> NADANIE");
}

void msgWho() {
  lcd.noAutoscroll();
  lcd.clear();
  lcd.home();
  lcd.print("1 -> UZYTKOWNIK");
  lcd.setCursor(0, 1);
  lcd.print("2 -> OBSLUGA");
}

void msgOptions(int a) {
  lcd.noAutoscroll();
  lcd.clear();
  lcd.home();
  if (a == 1)
    lcd.print("KOD ODBIORU:");
  else if (a == 2)
    lcd.print("KOD NADANIA:");
  else if (a == 3)
    lcd.print("HASLO:");
  else if (a == 4) {
    lcd.print(" WPROWADZ DANE");
    lcd.setCursor(6, 1);
    lcd.print("-->");
    delay(2000);
    lcd.clear();
    lcd.home();
    lcd.print("NUMER TELEFONU:");
  }
  lcd.setCursor(1, 1);
}

void lockers(int a) {
  lcd.noAutoscroll();
  lcd.clear();
  lcd.home();
  lcd.print("SKRYTKA NUMER ");
  lcd.print(a);
}

void msgInvalidCode() {
  code = "";
  phone = "";
  lcd.clear();
  lcd.home();
  if (!errorSending) {
    if (full) {
      lcd.print("  BRAK MIEJSCA");
      lcd.setCursor(0, 1);
      lcd.print(" W PACZKOMACIE!");
    } else {
      lcd.print("  BLEDNE DANE");
      lcd.setCursor(0, 1);
      lcd.print("SPROBUJ POZNIEJ!");
    }
  } else {
    lcd.print(" BLAD WYSYLANIA");
    lcd.setCursor(0, 1);
    lcd.print("SPROBUJ PONOWNIE");
  }
}

void msgChecking() {
  lcd.clear();
  lcd.home();
  lcd.print(" SPRAWDZANIE...");
}

void msgCloseDoor() {
  lcd.noAutoscroll();
  lcd.clear();
  lcd.home();
  lcd.print("Prosze zamknac");
  lcd.setCursor(0, 1);
  lcd.print("szafke...");
}

void msgEnd() {
  lcd.noAutoscroll();
  lcd.clear();
  lcd.home();
  lcd.print("  DZIEKUJEMY!");
  lcd.setCursor(0, 1);
  lcd.print(" DO ZOBACZENIA!");
}

void msgPackage() {
  lcd.clear();
  lcd.home();
  lcd.print("  * - OTWORZ PONOWNIE");
  lcd.setCursor(0, 1);
  lcd.print("  # - ZAKONCZ");
}

void nowTime() {
  start = millis();
}

bool tim() {
  now = millis();
  if ((now - start) >= 60000)
    return false;
  else
    return true;
}

void numer(int a) {
  char key = keypad.getKey();
  if (a == 1) {
    if (key)
      znak = key;
  } else if (a == 2) {
    if (key) {
      znak = key;
      switch (key) {
        case '*':
          break;
        case '#':
          break;
        default:
          nowTime();
          lcd.print(key);
          phone += key;
      }
    }
  } else if (a == 3) {
    if (key) {
      znak = key;
      switch (key) {
        case '*':
          break;
        case '#':
          break;
        default:
          nowTime();
          lcd.print(key);
          code += key;
      }
    }
  }
}

void checkService(String serverPath, int a) {
  HTTPClient http;
  http.begin(serverPath);
  http.addHeader("Content-Type", "application/json");

  StaticJsonDocument<200> send;
  send["phoneNumber"] = phone;
  send["code"] = code;
  send["machine"] = "PDKP1";
  if (a == 1)
    send["description"] = "Serwis";
  serializeJsonPretty(send, Serial);
  String requestBody;
  serializeJson(send, requestBody);
  int httpRequestCode = http.POST(requestBody);
  Serial.println(httpRequestCode);

  if (httpRequestCode > 0) {
    errorSending = false;
    String response = http.getString();
    Serial.println(httpRequestCode);
    Serial.println(response);
    StaticJsonDocument<200> receive;
    deserializeJson(receive, response);
    DeserializationError error = deserializeJson(receive, response);
    // Test if parsing succeeds.
    if (error) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      return;
    }
    order = receive["id"];
    Serial.println("Numer serwisu: " + String(order));
  } else {
    errorSending = true;
    Serial.println("Error while sending");
  }
  http.end();
  if (httpRequestCode == 200)
    checkinService = true;
  else
    checkinService = false;
}

void check(String serverPath) {
  HTTPClient http;
  http.begin(serverPath);
  http.addHeader("Content-Type", "application/json");

  StaticJsonDocument<200> send;
  send["phoneNumber"] = phone;
  send["code"] = code;
  send["machinePassword"] = machinePassword;
  serializeJsonPretty(send, Serial);
  String requestBody;
  serializeJson(send, requestBody);

  int httpRequestCode = http.POST(requestBody);
  Serial.println(httpRequestCode);
  if (httpRequestCode > 0) {
    errorSending = false;
    if (httpRequestCode == 200)
      checkin = true;
    else
      checkin = false;
    String response = http.getString();
    Serial.println(httpRequestCode);
    Serial.println(response);
    if (response == "Nie ma miejsca w paczkomacie!")
      full = true;
    else
      full = false;
    StaticJsonDocument<200> receive;
    deserializeJson(receive, response);
    DeserializationError error = deserializeJson(receive, response);
    // Test if parsing succeeds.
    if (error) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      return;
    }
    String lock = receive["locker"];
    order = receive["order"];
    locker = lock;
    Serial.println("Locker: " + locker + " & id: " + String(order));

  } else {
    errorSending = true;
    Serial.println("Error while sending");
  }
  http.end();
}

void alarmSend() {
  HTTPClient http;
  String serverPath = "https://paczkomatdatabaseapi.azurewebsites.net/api/paczkomat/failure/report";
  http.begin(serverPath);
  http.addHeader("Content-Type", "application/json");

  StaticJsonDocument<200> send;
  send["machine"] = "PDKP1";
  send["machinePassword"] = machinePassword;
  if ((digitalRead(26) == HIGH && !zamek1) || !(digitalRead(27) == HIGH && !zamek2))
    send["description"] = "Awaria/otwarcie pierwszej skrytki";
  if (!(digitalRead(26) == HIGH && !zamek1) || (digitalRead(27) == HIGH && !zamek2))
    send["description"] = "Awaria/otwarcie drugiej skrytki";
  if ((digitalRead(26) == HIGH && !zamek1) && (digitalRead(27) == HIGH && !zamek2))
    send["description"] = "Awaria/otwarcie obu skrytek";
  serializeJsonPretty(send, Serial);
  String requestBody;
  serializeJson(send, requestBody);

  int httpRequestCode = http.POST(requestBody);
  Serial.println(httpRequestCode);
  if (httpRequestCode > 0) {
    String response = http.getString();
    Serial.println(httpRequestCode);
    Serial.println(response);
  } else
    Serial.println("Error while sending");
  http.end();
}

bool serviceTime() {
  int time = (600000 - (millis() - start)) / 1000;
  int minuty = time / 60;
  int sekundy = time % 60;
  lcd.setCursor(6, 1);
  if (sekundy >= 10 && minuty < 10)
    lcd.print("0" + String(minuty) + ":" + String(sekundy));
  else if (sekundy < 10 && minuty >= 10)
    lcd.print(String(minuty) + ":0" + String(sekundy));
  else if (sekundy < 10 && minuty < 10)
    lcd.print("0" + String(minuty) + ":0" + String(sekundy));
  else if (sekundy >= 10 && minuty >= 10)
    lcd.print(String(minuty) + ":" + String(sekundy));
  if ((millis() - start) >= 600000)
    return false;
  else
    return true;
}