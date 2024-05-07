#include <Wire.h>                 // I2C haberleşmesi için gerekli kütüphane
#include <Adafruit_GFX.h>         // OLED ekranda grafik çizmek için gerekli kütüphane
#include <Adafruit_SSD1306.h>     // SSD1306 tabanlı OLED ekranı kullanmak için kütüphane

#define ANCHO 128                // Ekranın genişliği
#define ALTO 64                  // Ekranın yüksekliği

Adafruit_SSD1306 display(ANCHO, ALTO, &Wire, -1); // OLED ekran nesnesi

#define SELECT_PIN 0 // Seçim butonu pin numarası
#define UP_PIN 1     // Yukarı butonu pin numarası
#define DOWN_PIN 13   // Aşağı butonu pin numarası
#define LED1_PIN 8  // Birinci LED'in pin numarası
#define LED2_PIN 9  // İkinci LED'in pin numarası
#define LED3_PIN 10 // Üçüncü LED'in pin numarası
#define POTENTIOMETER_PIN A0 // Potansiyometre pin numarası

int paddlePosition = ANCHO - 7; // Panelin başlangıç pozisyonunu hesapla
int paddleYPosition = (ALTO - 20)/2; // Panelin başlangıç y dikey pozisyonunu ayarla
unsigned long previousMillis = 0;
const long interval = 10; // Her bir döngü adımının zaman aralığı (ms)

// 7 segment displaylerin pin tanımlamaları
const int digitPins[] = {13, 12}; // 7 segment displaylerin sıra pinleri (common cathode/anode)
const int segmentPins[] = {2,3,4,5,6,7,11}; // 7 segment displaylerin segment pinleri (A, B, C, D, E, F, G)

// 7 segment displaylerin basılabilir rakamları
const byte digits[][7] = {
  // 0bABCDEFG
  {1, 1, 1, 1, 1, 1, 0}, // 0
  {0, 1, 1, 0, 0, 0, 0}, // 1
  {1, 1, 0, 1, 1, 0, 1}, // 2
  {1, 1, 1, 1, 0, 0, 1}, // 3
  {0, 1, 1, 0, 0, 1, 1}, // 4
  {1, 0, 1, 1, 0, 1, 1}, // 5
  {1, 0, 1, 1, 1, 1, 1}, // 6
  {1, 1, 1, 0, 0, 0, 0}, // 7
  {1, 1, 1, 1, 1, 1, 1}, // 8
  {1, 1, 1, 1, 0, 1, 1}  // 9
};

int bolum = 1; // Oyunun başlangıç bölümü
int lives = 3;         // Can sayısı
int topBoyutu = 3;
int ballSpeedX = 3; // Topun X ekseni boyunca hızı
int ballSpeedY = 3; // Topun Y ekseni boyunca hızı
int score = 0;

enum MenuState { START, EXIT }; // Menü durumları

MenuState menuState = START; // Başlangıçta menü durumu başlat

const int NUM_TUGLALAR1 = 6; // Tuğla sayısı
int tuglaX1[NUM_TUGLALAR1] = {10, 10, 10, 25, 25, 40}; // Tuğlaların X koordinatları
int tuglaY1[NUM_TUGLALAR1] = {0, 21, 42, 10, 31, 22}; // Tuğlaların Y koordinatları
bool tuglaIsActive1[NUM_TUGLALAR1] = {true, true, true, true, true, true}; // Tuğlaların aktif durumu

const int NUM_TUGLALAR2 = 6; // Tuğla sayısı
int tuglaX2[NUM_TUGLALAR2] = {40, 40, 40, 25, 25, 10}; // Tuğlaların X koordinatları
int tuglaY2[NUM_TUGLALAR2] = {0, 21, 42, 10, 31, 22}; // Tuğlaların Y koordinatları
bool tuglaIsActive2[NUM_TUGLALAR2] = {true, true, true, true, true, true}; // Tuğlaların aktif durumu

const int NUM_TUGLALAR3 = 5; // Tuğla sayısı
int tuglaX3[NUM_TUGLALAR3] = {25, 40, 40, 10, 10}; // Tuğlaların X koordinatları
int tuglaY3[NUM_TUGLALAR3] = {21, 10, 31, 10, 31}; // Tuğlaların Y koordinatları
bool tuglaIsActive3[NUM_TUGLALAR3] = {true, true, true, true, true}; // Tuğlaların aktif durumu

void setup() {
  Serial.begin(9600);   // Seri haberleşmeyi başlatır
  Wire.begin();         // I2C haberleşmesini başlatır

  // OLED ekranını başlatır
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 ekranı bulunamadı!"));
    for (;;); // Sonsuz döngüde kalır
  }

  display.clearDisplay();     // Ekranı temizler
  display.setTextSize(1);     // Yazı boyutunu ayarlar
  display.setTextColor(WHITE);// Yazı rengini ayarlar
  display.setCursor(0, 0);    // Yazıyı ekranın sol üst köşesine konumlandırır
  display.println("Baslat");  // Başlangıç menüsü
  display.println("Cikis");
  display.display();          // Yazıyı ekrana gönderir
  delay(100);                 // 0.1 saniye bekler

  pinMode(SELECT_PIN, INPUT_PULLUP); // Seçim butonu için INPUT_PULLUP modunu ayarlar
  pinMode(UP_PIN, INPUT_PULLUP);     // Yukarı butonu için INPUT_PULLUP modunu ayarlar
  pinMode(DOWN_PIN, INPUT_PULLUP);   // Aşağı butonu için INPUT_PULLUP modunu ayarlar

  pinMode(LED1_PIN, OUTPUT); // LED pinlerini çıkış olarak ayarla
  pinMode(LED2_PIN, OUTPUT);
  pinMode(LED3_PIN, OUTPUT);

   // Pin modlarını ayarla
  for (int i = 0; i < sizeof(digitPins) / sizeof(digitPins[0]); i++) {
    pinMode(digitPins[i], OUTPUT); // Sıra pinlerini çıkış olarak ayarla
  }
  for (int i = 0; i < 7; i++) {
    pinMode(segmentPins[i], OUTPUT); // Segment pinlerini çıkış olarak ayarla
  }
}

void loop() {
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

     // Potansiyometre değerini oku
        int potValue = analogRead(POTENTIOMETER_PIN);
        // Potansiyometre değerini paletin yüksekliğine dönüştür
        paddleYPosition = map(potValue, 0, 1023, 0, ALTO - 20);

// Buton durumlarını oku
    bool selectButtonState = digitalRead(SELECT_PIN);
    bool upButtonState = digitalRead(UP_PIN);
    bool downButtonState = digitalRead(DOWN_PIN);

    // Menü durumunu güncelle
    if (selectButtonState == LOW) { // Seçim butonuna basıldığında
      if (menuState == START) {
        startGame(); // Oyunu başlat
      } else if (menuState == EXIT) {
        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(WHITE);
        display.setCursor(0, 0);
        display.println("Oyunumuza");
        display.println("gosterdiginiz");
        display.println("ilgi icin");
        display.println("tesekkurler");
        display.display();
        delay(2000);
        while (true); // Sonsuz döngüde kalır
      }
    } else if (upButtonState == LOW) { // Yukarı butonuna basıldığında
      menuState = START;
      updateMenu();
    } else if (downButtonState == LOW) { // Aşağı butonuna basıldığında
      menuState = EXIT;
      updateMenu();
    }
  }

updateLivesLEDs();
 
}

void menu(){
  
}

void startGame() {
    lives = 3; // Can sayısını 3'e ayarla
    score = 0;
    bolum = 1 ;
    updateScore(score);
    updateLivesLEDs();
    // Oyun başladığında paletin başlangıç pozisyonunu belirle
    paddleYPosition = (ALTO - 20) / 2;
    
    // Ekranı temizle
    display.clearDisplay(); 
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.println("Oyun basladi!");
    display.display();
    delay(300); // 0.3 saniye bekler
    display.clearDisplay();

    int ballXPosition = 60;
    int ballYPosition = 22;
    // Oyun döngüsünü başlat
    playGame(ballXPosition, ballYPosition);
    
}

void activateAllBricks() {
    // Tuğlaları aktif hale getir
    for (int i = 0; i < NUM_TUGLALAR1; i++) {
        tuglaIsActive1[i] = true;
    }

    for (int i = 0; i < NUM_TUGLALAR2; i++) {
        tuglaIsActive2[i] = true;
    }

    for (int i = 0; i < NUM_TUGLALAR3; i++) {
        tuglaIsActive3[i] = true;
    }
}

void contiuneGame()
{

// Oyun başladığında paletin başlangıç pozisyonunu belirle
    paddleYPosition = (ALTO - 20) / 2;
    
    // Ekranı temizle
    display.clearDisplay(); 
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.println("Oyun devam ediyor!");
    display.println(score);
    display.display();
    delay(500); // 0.3 saniye bekler
    display.clearDisplay();

    int ballXPosition = 60;
    int ballYPosition = 22;
    drawBall(ballXPosition, ballYPosition);

    // Oyun döngüsünü başlat
    playGame(ballXPosition, ballYPosition);

}

void playGame(int ballXPosition, int ballYPosition) {
    // Topun hızını ve yönünü belirle
    

    while (1) {
        // Paleti hareket ettir
        movePaddle();

        // Topu hareket ettir
        ballXPosition += ballSpeedX;
        ballYPosition += ballSpeedY;

        // Duvar ve palet çarpışmalarını kontrol et
        checkCollisions(ballXPosition, ballYPosition, ballSpeedX, ballSpeedY);

        // Topu çiz
        drawBall(ballXPosition, ballYPosition);

        
    }
}

void drawBall(int x, int y) {
    display.fillRect(0, 0, ANCHO, ALTO, BLACK); // Ekranı temizle
    display.fillRect(x, y, topBoyutu, topBoyutu, WHITE); // Topu çiz
    display.fillRect(paddlePosition, paddleYPosition, 2, 20, WHITE); // Paneli çiz

 // Her bölüm başında ilgili haritayı çiz
    if (bolum == 1) {
        harita1();
    } else if (bolum == 2) {
        harita2();
    } else if (bolum == 3) {
        harita3();
    }

    display.display(); // Ekranı güncelle
}
void harita1()
{
    // Tuğlaları çiz
    int tuglaGenislik = 10; // Tuğla genişliği
    int tuglaYukseklik = 18; // Tuğla yüksekliği
    
    for (int i = 0; i < NUM_TUGLALAR1; i++) {
        if (tuglaIsActive1[i]) {
            display.fillRect(tuglaX1[i], tuglaY1[i], tuglaGenislik, tuglaYukseklik, WHITE); // Aktif tuğlayı çiz
        } else {
            display.fillRect(tuglaX1[i], tuglaY1[i], tuglaGenislik, tuglaYukseklik, BLACK); // Aktif olmayan tuğlayı çiz
        }
    }
}

void harita2()
{
    // Tuğlaları çiz
    int tuglaGenislik = 10; // Tuğla genişliği
    int tuglaYukseklik = 18; // Tuğla yüksekliği
    
    for (int i = 0; i < NUM_TUGLALAR2; i++) {
        if (tuglaIsActive2[i]) {
            display.fillRect(tuglaX2[i], tuglaY2[i], tuglaGenislik, tuglaYukseklik, WHITE); // Aktif tuğlayı çiz
        } else {
            display.fillRect(tuglaX2[i], tuglaY2[i], tuglaGenislik, tuglaYukseklik, BLACK); // Aktif olmayan tuğlayı çiz
        }
    }
}

void harita3()
{
    // Tuğlaları çiz
    int tuglaGenislik = 10; // Tuğla genişliği
    int tuglaYukseklik = 18; // Tuğla yüksekliği
    
    for (int i = 0; i < NUM_TUGLALAR3; i++) {
        if (tuglaIsActive3[i]) {
            display.fillRect(tuglaX3[i], tuglaY3[i], tuglaGenislik, tuglaYukseklik, WHITE); // Aktif tuğlayı çiz
        } else {
            display.fillRect(tuglaX3[i], tuglaY3[i], tuglaGenislik, tuglaYukseklik, BLACK); // Aktif olmayan tuğlayı çiz
        }
    }
}

void checkCollisions(int &ballXPosition, int &ballYPosition, int &ballSpeedX, int &ballSpeedY) {
    // Ekranın kenarlarına çarpışmayı kontrol et
    if (ballXPosition <= 0 || ballXPosition >= ANCHO - topBoyutu) {
        if (ballXPosition >= ANCHO - topBoyutu) {
            lives--; // Can azalt
            if (lives == 0) {
              updateLivesLEDs();
                gameOver(); // Canlar tükendiğinde oyunu sonlandır
            } else {
              updateLivesLEDs();
                contiuneGame(); // Canlar varsa oyunu yeniden başlat
            }
        }
        ballSpeedX = -ballSpeedX; // Topun X hızını tersine çevir
    }
    if (ballYPosition <= 0 || ballYPosition >= ALTO - topBoyutu) {
        ballSpeedY = -ballSpeedY; // Topun Y hızını tersine çevir
    }

    // Palet ile çarpışmayı kontrol et
    if (ballXPosition >= paddlePosition && ballXPosition <= paddlePosition + 2 && 
        ballYPosition >= paddleYPosition && ballYPosition <= paddleYPosition + 20) {
        ballSpeedX = -ballSpeedX; // Topun X hızını tersine çevir
    }

if (bolum == 1) {
    // Çarpışma kontrolü
    for (int i = 0; i < NUM_TUGLALAR1; i++) {
        if (tuglaIsActive1[i] && ballXPosition + topBoyutu >= tuglaX1[i] && ballXPosition <= tuglaX1[i] + 10 &&
            ballYPosition + topBoyutu >= tuglaY1[i] && ballYPosition <= tuglaY1[i] + 18) {
            // Çarpışma varsa, topun yönünü tersine çevir
            if (ballXPosition + topBoyutu >= tuglaX1[i] || ballXPosition <= tuglaX1[i] + 10) {
                ballSpeedX = -ballSpeedX;
            }
            if (ballYPosition + topBoyutu >= tuglaY1[i] || ballYPosition <= tuglaY1[i] + 18) {
                ballSpeedY = -ballSpeedY;
            }
            // Çarpılan tuğlayı ekrandan kaldır
            display.fillRect(tuglaX1[i], tuglaY1[i], 30, 30, BLACK); // Tuğlayı siyahla boyar
            tuglaIsActive1[i] = false; // Tuğla artık aktif değil
            score++;
            updateScore(score);

            // Çizim sırasında önce siyah bir dikdörtgen çizerek tuğlanın üzerini temizle
            display.fillRect(tuglaX1[i], tuglaY1[i], 30, 30, BLACK);
            break; // Tek bir tuğla ile çarpışmayı kontrol etmek yeterli, diğerlerini kontrol etmeye gerek yok
        }
    }
    bolumGecisi1(ballXPosition, ballYPosition); // Bölüm geçişini kontrol et
}

if(bolum ==2)
{// Çarpışma kontrolü
for (int i = 0; i < NUM_TUGLALAR2; i++) {
    if (tuglaIsActive2[i] && ballXPosition + topBoyutu >= tuglaX2[i] && ballXPosition <= tuglaX2[i] + 10 &&
        ballYPosition + topBoyutu >= tuglaY2[i] && ballYPosition <= tuglaY2[i] + 18) {
        // Çarpışma varsa, topun yönünü tersine çevir
        if (ballXPosition + topBoyutu >= tuglaX2[i] || ballXPosition <= tuglaX2[i] + 10) {
            ballSpeedX = -ballSpeedX;
        }
        if (ballYPosition + topBoyutu >= tuglaY2[i] || ballYPosition <= tuglaY2[i] + 18) {
            ballSpeedY = -ballSpeedY;
        }
        // Çarpılan tuğlayı ekrandan kaldır
        display.fillRect(tuglaX2[i], tuglaY2[i], 30, 30, BLACK); // Tuğlayı siyahla boyar
        tuglaIsActive2[i] = false; // Tuğla artık aktif değil
        score++;
            updateScore(score);


        // Çizim sırasında önce siyah bir dikdörtgen çizerek tuğlanın üzerini temizle
        display.fillRect(tuglaX2[i], tuglaY2[i], 30, 30, BLACK);
        break; // Tek bir tuğla ile çarpışmayı kontrol etmek yeterli, diğerlerini kontrol etmeye gerek yok
         }
     }
    bolumGecisi2(ballXPosition, ballYPosition); // Bölüm geçişini kontrol et
}
    
if(bolum ==3)
{// Çarpışma kontrolü
for (int i = 0; i < NUM_TUGLALAR3; i++) {
    if (tuglaIsActive3[i] && ballXPosition + topBoyutu >= tuglaX3[i] && ballXPosition <= tuglaX3[i] + 10 &&
        ballYPosition + topBoyutu >= tuglaY3[i] && ballYPosition <= tuglaY3[i] + 18) {
        // Çarpışma varsa, topun yönünü tersine çevir
        if (ballXPosition + topBoyutu >= tuglaX3[i] || ballXPosition <= tuglaX3[i] + 10) {
            ballSpeedX = -ballSpeedX;
        }
        if (ballYPosition + topBoyutu >= tuglaY3[i] || ballYPosition <= tuglaY3[i] + 18) {
            ballSpeedY = -ballSpeedY;
        }
        // Çarpılan tuğlayı ekrandan kaldır
        display.fillRect(tuglaX3[i], tuglaY3[i], 30, 30, BLACK); // Tuğlayı siyahla boyar
        tuglaIsActive3[i] = false; // Tuğla artık aktif değil
        score++;
            updateScore(score);

        // Çizim sırasında önce siyah bir dikdörtgen çizerek tuğlanın üzerini temizle
        display.fillRect(tuglaX3[i], tuglaY3[i], 30, 30, BLACK);
        break; // Tek bir tuğla ile çarpışmayı kontrol etmek yeterli, diğerlerini kontrol etmeye gerek yok
         }
     }
     bolumGecisi3(); // Bölüm geçişini kontrol et
}

}
void bolumGecisi1(int &ballXPosition, int &ballYPosition) {
    bool aktifTuqlaKaldi1 = false;
    
    // Aktif tuğla var mı kontrol et
    for (int i = 0; i < NUM_TUGLALAR1; i++) {
        if (tuglaIsActive1[i]) {
            aktifTuqlaKaldi1 = true;
            break;
        }
    }
    
    // Aktif tuğla kalmadıysa bölümü geç
    if (!aktifTuqlaKaldi1) {
        // Bölüm geçişi ekranını göster
        display.fillScreen(BLACK);
        display.setTextSize(1);
        display.setTextColor(WHITE);
        display.setCursor(10, 20);
        display.println("Bolum 2!");
        
        display.display();
        
        delay(500); // 3 saniye bekle
        
        // Sonraki bölümü yükle
        bolum++; // Bölüm numarasını arttır
       
                harita2();
                ballXPosition = 60; // Topun yeni başlangıç X pozisyonu
                ballYPosition = 22; // Topun yeni başlangıç Y pozisyonu
          
    }
}

void bolumGecisi2(int &ballXPosition, int &ballYPosition) {
    bool aktifTuqlaKaldi2 = false;
    
    // Aktif tuğla var mı kontrol et
    for (int i = 0; i < NUM_TUGLALAR2; i++) {
        if (tuglaIsActive2[i]) {
            aktifTuqlaKaldi2 = true;
            break;
        }
    }
    
    // Aktif tuğla kalmadıysa bölümü geç
    if (!aktifTuqlaKaldi2) {
        // Bölüm geçişi ekranını göster
        display.fillScreen(BLACK);
        display.setTextSize(1);
        display.setTextColor(WHITE);
        display.setCursor(10, 20);
        display.println("Bolum 3!");
        
        display.display();
        
        delay(500); // 3 saniye bekle
        
        // Sonraki bölümü yükle
        bolum++; // Bölüm numarasını arttır
        
                harita3();
                ballXPosition = 60; // Topun yeni başlangıç X pozisyonu
                ballYPosition = 22; // Topun yeni başlangıç Y pozisyonu
               
        
        
    }
}

void bolumGecisi3() {
    bool aktifTuqlaKaldi3 = false;
    
    // Aktif tuğla var mı kontrol et
    for (int i = 0; i < NUM_TUGLALAR3; i++) {
        if (tuglaIsActive3[i]) {
            aktifTuqlaKaldi3 = true;
            break;
        }
    }
    
    // Aktif tuğla kalmadıysa bölümü geç
    if (!aktifTuqlaKaldi3) {
       
        display.clearDisplay(); 
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.println("Oyun bitti!");
    display.println(score);
    display.display();
    delay(300); // 0.3 saniye bekler

    menuState = START; // Menü durumunu başlat
lives=3;
bolum=1;
score=0;
updateLivesLEDs();
    // Menüyü ekrana yazdır
    updateMenu();

    // Menü seçimini bekleyen döngü
    while (true) {
        // Buton durumlarını oku
        bool selectButtonState = digitalRead(SELECT_PIN);
        bool upButtonState = digitalRead(UP_PIN);
        bool downButtonState = digitalRead(DOWN_PIN);

        if (selectButtonState == LOW) { // Seçim butonuna basıldığında
            if (menuState == START) {
              activateAllBricks();
              
                startGame(); // Oyunu başlat
            } else if (menuState == EXIT) {
                // Çıkış seçeneği seçildiğinde ekrana veda mesajı yazdır ve beklet
                display.clearDisplay();
                display.setTextSize(1);
                display.setTextColor(WHITE);
                display.setCursor(0, 0);
                display.println("Oyunumuza");
                display.println("gosterdiginiz");
                display.println("ilgi icin");
                display.println("tesekkurler");
                display.display();
                delay(2000);
                while (true); // Sonsuz döngüde kalır
            }
        } else if (upButtonState == LOW) { // Yukarı butonuna basıldığında
            menuState = START;
            updateMenu();
        } else if (downButtonState == LOW) { // Aşağı butonuna basıldığında
            menuState = EXIT;
            updateMenu();
        }
    }
    }
}


void movePaddle() {
  int potValue = analogRead(A0); // Potansiyometre değerini oku
  
// Yukarı veya aşağı hareketi potansiyometrenin değerine göre tersine ayarla
paddleYPosition = map(potValue, 0, 1023, ALTO - 20, 0);
}

void drawPaddle() {
    display.clearDisplay(); // Ekranı temizle
  display.fillRect(paddlePosition, paddleYPosition, 2, 20, WHITE); // Paneli çiz
  display.display(); // Ekranı güncelle
}

void gameOver() {
    display.clearDisplay(); 
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.println("Oyun bitti!");
    display.println(score);
    display.display();
    delay(300); // 0.3 saniye bekler
lives=3;
bolum=1;
score=0;
   
   menuState = START;
    // Menüyü ekrana yazdır
    updateMenu();

    // Menü seçimini bekleyen döngü
    while (true) {
       // Buton durumlarını oku
    bool selectButtonState = digitalRead(SELECT_PIN);
    bool upButtonState = digitalRead(UP_PIN);
    bool downButtonState = digitalRead(DOWN_PIN);

    // Menü durumunu güncelle
    if (selectButtonState == LOW) { // Seçim butonuna basıldığında
      if (menuState == START) {
        activateAllBricks();
        startGame(); // Oyunu başlat
      } else if (menuState == EXIT) {
        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(WHITE);
        display.setCursor(0, 0);
        display.println("Oyunumuza");
        display.println("gosterdiginiz");
        display.println("ilgi icin");
        display.println("tesekkurler");
        display.display();
        delay(2000);
        while (true); // Sonsuz döngüde kalır
      }
    } 
    
    if (upButtonState == LOW) { // Yukarı butonuna basıldığında
      menuState = START;
      updateMenu();
    } else if (downButtonState == LOW) { // Aşağı butonuna basıldığında
      menuState = EXIT;
      updateMenu();
    }
    }
}


void updateLivesLEDs() {
    // Can sayısına göre LED'leri kontrol et
    switch (lives) {
        case 3:
            digitalWrite(LED1_PIN, HIGH);
            digitalWrite(LED2_PIN, HIGH);
            digitalWrite(LED3_PIN, HIGH);
            break;
        case 2:
            digitalWrite(LED1_PIN, HIGH);
            digitalWrite(LED2_PIN, HIGH);
            digitalWrite(LED3_PIN, LOW);
            break;
        case 1:
            digitalWrite(LED1_PIN, HIGH);
            digitalWrite(LED2_PIN, LOW);
            digitalWrite(LED3_PIN, LOW);
            break;
        case 0:
            digitalWrite(LED1_PIN, LOW);
            digitalWrite(LED2_PIN, LOW);
            digitalWrite(LED3_PIN, LOW);
            break;
    }
}

// Displaye bir rakam basan fonksiyon
void displayDigit(int digit, int value) {
  // Sıra pinlerini seç
  for (int i = 0; i < sizeof(digitPins) / sizeof(digitPins[0]); i++) {
    digitalWrite(digitPins[i], HIGH); // Önceki sıra pinini kapat
  }
  digitalWrite(digitPins[digit], LOW); // İstenen sıra pinini aç
  
  // Rakamı göster
  for (int i = 0; i < 7; i++) {
    digitalWrite(segmentPins[i], digits[value][i]);
  }
}

void updateScore(int score) {
  int units = score % 10; // Birler basamağı
  int tens = score / 10; // Onlar basamağı

  // Onlar basamağını göster
  displayDigit(2, tens);
  // Birler basamağını göster
  displayDigit(1, units);
}

void updateMenu() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  if (menuState == START) {
    display.println("> Baslat");
    display.println("  Cikis"); // Seçili menü öğesini işaretler
    
  } else if (menuState == EXIT) {
    display.println("  Baslat");
    display.println("> Cikis"); // Seçili menü öğesini işaretler
  }
  display.display();
  delay(200); // 0.2 saniye bekler
}