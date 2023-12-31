#include <SPI.h>
#include <MFRC522.h>
#include "RTClib.h"
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <SD.h>

#define SS_PIN 10
#define RST_PIN 9

LiquidCrystal_I2C lcd(0x27, 16, 2);

int buzzer = 2;
int pb_mode = A0;

byte readCard[4];
int cards1[4] = {52, 98, 155, 81};
int cards2[4] = {131, 69, 134, 29};

int ID;
String nama;
bool status_kartu = false;
bool sudah_presensi = false;
boolean mode_pulang = false;

RTC_DS3231 rtc;
MFRC522 mfrc522(SS_PIN, RST_PIN);
char daysOfTheWeek[7][12] = {"Minggu", "Senin", "Selasa", "Rabu", "Kamis", "Jum'at", "Sabtu"};

// Objek untuk mengakses file SD
File dataFile;

int kartu_tidak_terdaftar_count = 0; // Jumlah kartu yang tidak terdaftar yang telah ditempelkan
bool kartu_terdaftar = false; // Status kartu terdaftar

void setup() {
  Serial.begin(9600);
  SPI.begin();
  mfrc522.PCD_Init();
  lcd.begin();
  pinMode(pb_mode, INPUT_PULLUP);
  pinMode(buzzer, OUTPUT);
  digitalWrite(buzzer, LOW);

  rtc.begin();

  // Inisialisasi SD card
  if (!SD.begin(4)) {
    lcd.print("Gagal memulai SD Card");
    while (1);
  }

  lcd.setCursor(1, 0);
  lcd.print("Sistem Presensi");
  lcd.setCursor(0, 1);
  lcd.print("Tempelkan Kartu");

  delay(1500);
  lcd.clear();
  delay(50);
}

void loop() {
  DateTime now = rtc.now();

  if (digitalRead(pb_mode) == LOW) {
    mode_pulang = !mode_pulang;
    Serial.println("Pergantian mode");
    delay(500);
  }

  lcd.setCursor(4, 1);
  printposisilcd(now.hour());
  lcd.print(":");
  printposisilcd(now.minute());
  lcd.print(":");
  printposisilcd(now.second());

  if (mode_pulang == true) {
    lcd.setCursor(1, 0);
    lcd.print("Presensi Pulang");
  }
  else {
    lcd.setCursor(1, 0);
    lcd.print("Presensi Masuk ");
  }

  // Membaca kartu RFID yang baru dipresentasikan
  if (!mfrc522.PICC_IsNewCardPresent()) {
    return;
  }

  if (!mfrc522.PICC_ReadCardSerial()) {
    return;
  }

  if (mfrc522.uid.uidByte[0] != readCard[0] || mfrc522.uid.uidByte[1] != readCard[1]) {
    Serial.println("");
    Serial.print("UID : ");
    for (byte i = 0; i < mfrc522.uid.size; i++) {
      readCard[i] = mfrc522.uid.uidByte[i];
      Serial.print(readCard[i]);
      if (i < mfrc522.uid.size - 1) {
        Serial.print(" ,");
      }
      else {
        Serial.println("");
      }

      sudah_presensi = false;
      status_kartu = true;

      // Verifikasi ID kartu dan menentukan status kartu
      if (readCard[i] == cards1[i]) {
        ID = 1;
        nama = "Sukarmini";
      }
      else if (readCard[i] == cards2[i]) {
        ID = 2;
        nama = "Sumarno";
      }
      else {
        status_kartu = false;
        mode_pulang = false; // Menonaktifkan mode pulang jika kartu tidak terdaftar
      }
    }
  }
  else {
    sudah_presensi = true;
    Serial.println("sudah presensi");
    lcd.setCursor(1, 1);
    lcd.print("Sudah Presensi");
  }

  kartu_terdaftar = (status_kartu && !sudah_presensi);

  if (kartu_terdaftar) {
    if (mode_pulang == false) {
      Serial.println("-----Presensi Masuk------");
      Serial.print("ID Kartu : ");
      Serial.println(ID);
      printtanggal();
      Serial.println("");
      Serial.print("Nama : ");
      Serial.println(nama);
      Serial.print("Waktu Presensi : ");
      printwaktu();
      Serial.println("");
      lcd.setCursor(2, 1);
      lcd.print("Terima Kasih");

      // Simpan data presensi ke SD card
      dataFile = SD.open("Presensi.txt", FILE_WRITE);
      if (dataFile) {
        dataFile.print("Presensi Masuk - ");
        dataFile.print(",");
        dataFile.print(ID);
        dataFile.print(",");
        dataFile.print(nama);
        dataFile.print(",");
        dataFile.print(now.day());
        dataFile.print('/');
        dataFile.print(now.month());
        dataFile.print('/');
        dataFile.print(now.year());
        dataFile.print(",");
        dataFile.print(now.hour());
        dataFile.print(":");
        dataFile.print(now.minute());
        dataFile.print(":");
        dataFile.print(now.second());
        dataFile.println();
        dataFile.close();
      }
      else {
        Serial.println("Gagal membuka file SD");
      }
    }

    if (mode_pulang == true) {
      Serial.println("-----Presensi Keluar------");
      Serial.print("ID Kartu : ");
      Serial.println(ID);
      printtanggal();
      Serial.println("");
      Serial.print("Nama : ");
      Serial.println(nama);
      Serial.print("Waktu Presensi : ");
      printwaktu();
      Serial.println("");
      lcd.setCursor(2, 1);
      lcd.print("Terima Kasih");

      // Simpan data presensi ke SD card
      dataFile = SD.open("Presensi.txt", FILE_WRITE);
      if (dataFile) {
        dataFile.print("Presensi Keluar - ");
        dataFile.print(",");
        dataFile.print(ID);
        dataFile.print(",");
        dataFile.print(nama);
        dataFile.print(",");
        dataFile.print(now.day());
        dataFile.print('/');
        dataFile.print(now.month());
        dataFile.print('/');
        dataFile.print(now.year());
        dataFile.print(",");
        dataFile.print(now.hour());
        dataFile.print(":");
        dataFile.print(now.minute());
        dataFile.print(":");
        dataFile.print(now.second());
        dataFile.println();
        dataFile.close();
      }
      else {
        Serial.println("Gagal membuka file SD");
      }
    }
  }
  else if (!status_kartu) {
    kartu_tidak_terdaftar_count = 1; // Reset jumlah kartu yang tidak terdaftar jika kartu baru ditempelkan
    lcd.setCursor(0, 1);
    lcd.print("Tidak terdaftar");

    Serial.println("Kartu tidak terdaftar");
    Serial.println(kartu_tidak_terdaftar_count);
  }
  else {
    kartu_tidak_terdaftar_count++;
    Serial.println(kartu_tidak_terdaftar_count);
  }

  digitalWrite(buzzer, HIGH);
  delay(200);
  digitalWrite(buzzer, LOW);
  delay(2000);
  lcd.clear();
  delay(50);
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();

}

// Fungsi untuk mencetak angka pada LCD dengan posisi dua digit
void printposisilcd(int number) {
  if (number < 10) {
    lcd.print("0");
  }
  lcd.print(number);
}

// Fungsi untuk mencetak tanggal pada Serial Monitor
void printtanggal() {
  DateTime now = rtc.now();
  Serial.print(daysOfTheWeek[now.dayOfTheWeek()]);
  Serial.print(", ");
  Serial.print(now.day());
  Serial.print("/");
  Serial.print(now.month());
  Serial.print("/");
  Serial.println(now.year());
}

// Fungsi untuk mencetak waktu pada Serial Monitor
void printwaktu() {
  DateTime now = rtc.now();
  printposisilcd(now.hour());
  Serial.print(":");
  printposisilcd(now.minute());
  Serial.print(":");
  printposisilcd(now.second());
}
