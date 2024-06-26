#include "Arduino.h"
#include "AccelStepper.h"
// #include "Oil.h"
#include "oliYuuahmad.h"
#include "EEPROM.h"

// definisi verbose output yang ingin ditampilkan
#define stepper_motor 0 // 1 stepper motor, 0 induksi motor
#define verbose_oli 1
#define verbose_motor 1
#define verbose_tools 1
#define verbose_error 0
#define verbose_perintah_pc 1

// pin input ATC
#define prox_umb_maju PB9     // 0
#define prox_umb_mundur PB8   // 1
#define prox_clamp PA8        // 2
#define prox_unclamp PB10     // 3
#define prox_tools PB5        // 4
#define spindle_orient_ok PB4 // 5 dari board orientasi spindle (pi5)
#define oil_level PB3         // 6 (pi6)
#define input_emergency PB4   // 7
const int input_pins[7] = {prox_umb_maju, prox_umb_mundur, prox_clamp, prox_unclamp,
                           prox_tools, spindle_orient_ok, oil_level}; // Replace with your input pin numbers

// pin output ATC
#define step_stepper PB14   // 4
#define dir_stepper PA15    // 5
#define spindle_orient PC13 // 2 perintahkan spindle orient
#define tool_clamp PC14     // 0 clamp tools
#define move_umb PB1        // 1 aktuator maju-mundur umbrella
// #define oil_pump PA5        // 6
// #define buzzer PB0         // 3
const int output_pins[3] = {tool_clamp, move_umb,
                            spindle_orient}; // Replace with your input pin numbers

// setup motor stepper
AccelStepper my_stepper = AccelStepper(1, step_stepper);
unsigned char gerak_motor = 'C'; // A=CW, B=CCW, C=STOP

// variabel untuk memproses nilai input
bool nilai_input[] = {0, 0, 0, 0, 0, 0, 0, 0};
bool last_nilai_input[] = {0, 0, 0, 0, 0, 0, 0, 0};
unsigned int bounce = 10;
bool status;
unsigned long lastDebounceTimes[8];

// // variabel untuk parsing perintah serial
const byte numChars = 32;     // Panjang maksimal pesan yang diterima
char receivedChars[numChars]; // Buffer untuk menyimpan pesan yang diterima
bool newData = false;         // Menunjukkan apakah ada data baru yang diterima
char karakter_awal;
char perintah_pc = '0';

// variabel parameter oli
int waktu_delay = 1;   // waktu delay tiap menit tulis ke eeprom
int banyak_cycle = 60; // banyak segmen untuk jeda spindle mati
int waktu_on = 5;      // Variabel untuk menyimpan nilai waktu_on
// waktu delay = 15 waktu_on = 5, artinya waktu_on 5 detik selama 15 menit sekali

void verbose_output()
{
    // untuk proxy dan lainnya
    Serial.print("T:");
    // if (nilai_input[5])
    //     Serial.print("O"); // spindle orientation ok
    // ok hanya ketika nilai_input[5] benar lalu low kembali
    if (nilai_input[0])
        Serial.print("F"); // umbrella didepan
    if (nilai_input[3])
        Serial.print("U"); // tool unclamp
    if (nilai_input[1])
        Serial.print("B"); // umbrella dibelakang
    if (nilai_input[2])
        Serial.print("L"); // tool clamp
    if (nilai_input[5])
        Serial.print("O"); // orient ok

    // untuk pergerakan motor
    if (verbose_motor)
    {
        Serial.print("|M:");
        switch (gerak_motor)
        {
        case 'A':
            Serial.print("A");
            break;
        case 'B':
            Serial.print("B");
            break;
        case 'C':
            Serial.print("C");
            break;
        default:
            break;
        }
    }

    // untuk tools
    if (verbose_tools)
    {
        Serial.print("|P:");
        nilai_input[4] == 1 ? Serial.print("1") : Serial.print("0");
    }

    // untuk eroor
    if (verbose_error)
    {
        Serial.print("|E:");
        Serial.print("unknown");
    }

    // untuk respon oil level
    if (verbose_oli)
    {
        Serial.print("|O:");
        Serial.print(waktu_delay);
        // Serial.print(",");
        // Serial.print(get_naik_timer_ke());
        if (get_keadaan_level_oli()) // oli habis
            Serial.print('E');
        if (get_keadaan_pompa_oli()) // bagaimana keadaan pompa oli
            Serial.print('H');
        if (get_keadaan_timer_oli_on()) // bagaimana keadaan timer oli on
            Serial.print('M');
    }

    // verbose tulis kembali perintah pc
    if (verbose_perintah_pc)
    {
        Serial.print("|R:");
        Serial.print(perintah_pc);
    }
    Serial.println(""); // enter, baris selanjutnya
}

void recvWithEndMarker()
{
    static byte ndx = 0;
    char endMarker = '\n';
    char rc;

    while (Serial.available() > 0 && newData == false)
    {
        rc = Serial.read();
        rc = toUpperCase(rc);

        if (rc != endMarker)
        {
            receivedChars[ndx] = rc;
            ndx++;
            if (ndx >= numChars)
                ndx = numChars - 1;
        }
        else
        {
            receivedChars[ndx] = '\0'; // Menandai akhir string
            ndx = 0;
            newData = true;
        }
    }
}

void parsing_perintah_pc()
{
    char *strtokIndx;

    // Mendapatkan karakter pertama dari buffer
    strtokIndx = strtok(receivedChars, ",");
    karakter_awal = strtokIndx[0];
    switch (strtokIndx[0])
    {
        // -----LIST PERINTAH DARI PC-----
        // Spindle Orientation (H) (T:O) (Err:O)
        // Umbrella goes forward (P) (T:F) (Err:F)
        // Spindle tool unclamp (U) (T:U) (Err:U)
        // Tool Rotation (A)
        // Check Tool Position Counting (Software) (P:0/1)
        // Stop Rotation (C)
        // Spindle tool clamping (L) (T:L) (Err:L)
        // Umbrella goes backward (K) (T:B) (Err:B)
    case 'H':
        perintah_pc = 'H';
        digitalWriteFast(digitalPinToPinName(spindle_orient), LOW); // spindle orient aktif low
        break;
    case 'J':
        perintah_pc = 'J';
        digitalWriteFast(digitalPinToPinName(spindle_orient), HIGH);
        break;
    case 'P':
        perintah_pc = 'P';
        digitalWriteFast(digitalPinToPinName(move_umb), HIGH); // relay aktif high
        break;
    case 'U':
        perintah_pc = 'U';
        digitalWriteFast(digitalPinToPinName(tool_clamp), HIGH); // relay aktif high
        break;
    case 'K':
        perintah_pc = 'K';
        digitalWriteFast(digitalPinToPinName(move_umb), LOW);
        break;
    case 'L':
        perintah_pc = 'L';
        digitalWriteFast(digitalPinToPinName(tool_clamp), LOW);
        break;
    default:
        break;
    case 'A':
        perintah_pc = 'A';
        gerak_motor = 'A';
        break;
    case 'B':
        perintah_pc = 'B';
        gerak_motor = 'B';
        break;
    case 'C':
        perintah_pc = 'C';
        gerak_motor = 'C';
        break;
    case 'O':
        perintah_pc = 'O';
        // Mendapatkan nilai kedua
        strtokIndx = strtok(NULL, ",");
        waktu_delay = atoi(strtokIndx);
        EEPROM.write(5, waktu_delay);

        // Mendapatkan nilai ketiga
        // strtokIndx = strtok(NULL, ",");
        // banyak_cycle = atoi(strtokIndx);
        // EEPROM.write(10, banyak_cycle);

        // Mendapatkan nilai keempat
        strtokIndx = strtok(NULL, ",");
        waktu_on = atoi(strtokIndx);
        EEPROM.write(15, waktu_on);
        break;
    case 'Q':
        perintah_pc = 'Q';
        set_nyalakan_pompa(true);
        break;
    }
}

void baca_sinyal_input()
{
    // baca status pin input, kemudian kirim data ke PC
    for (int i = 0; i < 7; i++)
    {
        int reading = !digitalReadFast(digitalPinToPinName(input_pins[i]));
        // karena logicnya terbalik
        if (reading != last_nilai_input[i])
            lastDebounceTimes[i] = millis();
        if ((millis() - lastDebounceTimes[i]) > bounce)
            if (reading != nilai_input[i])
                nilai_input[i] = reading;
        last_nilai_input[i] = reading;
        // Serial.print(nilai_input[i]);
    }
}

void gerakkan_motor()
{
    switch (gerak_motor)
    {
#if stepper_motor
    case 'A':
        my_stepper.setSpeed(3500);
        digitalWriteFast(digitalPinToPinName(dir_stepper), HIGH);
        my_stepper.runSpeed();
        break;
    case 'B':
        my_stepper.setSpeed(3500);
        digitalWriteFast(digitalPinToPinName(dir_stepper), LOW);
        my_stepper.runSpeed();
        break;
    case 'C':
        my_stepper.stop();
        my_stepper.setCurrentPosition(0);
        break;
    default:
        break;
#else
    case 'A':
        digitalWriteFast(digitalPinToPinName(step_stepper), HIGH);
        digitalWriteFast(digitalPinToPinName(dir_stepper), LOW);
        break;
    case 'B':
        digitalWriteFast(digitalPinToPinName(step_stepper), LOW);
        digitalWriteFast(digitalPinToPinName(dir_stepper), HIGH);
        break;
    case 'C':
        digitalWriteFast(digitalPinToPinName(step_stepper), LOW);
        digitalWriteFast(digitalPinToPinName(dir_stepper), LOW);
        break;
    default:
        break;
#endif
    }
}

void setup()
{
    Serial.begin(9600);
    // set pin input dan output
    for (int i = 0; i < 7; i++)
        pinMode(input_pins[i], INPUT_PULLDOWN);
    for (int i = 0; i < 3; i++)
        pinMode(output_pins[i], OUTPUT);

    // inisialisasi oli
    init_oli();
    // Membaca waktu terakhir dari EEPROM
    waktu_delay = EEPROM.read(5);
    // banyak_cycle = EEPROM.read(10);
    waktu_on = EEPROM.read(15);

    // Jika EEPROM kosong (nilai awal), set waktu ke default test
    if (waktu_delay == 0xFFFFFFFF)
        waktu_delay = 1;
    // if (banyak_cycle == 0xFFFFFFFF)
    //     banyak_cycle = 1;
    if (waktu_on == 0xFFFFFFFF)
        waktu_on = 5;

    // pastikan in output dalam keadaan default
    digitalWriteFast(digitalPinToPinName(spindle_orient), HIGH);
    digitalWriteFast(digitalPinToPinName(tool_clamp), LOW);
    digitalWriteFast(digitalPinToPinName(move_umb), LOW);

    // inisialisasi stepper
    my_stepper.setMaxSpeed(5000);
    my_stepper.setPinsInverted(false, false);
    my_stepper.stop();
    my_stepper.setCurrentPosition(0);
}

void loop()
{
    recvWithEndMarker();
    if (newData == true)
    {
        parsing_perintah_pc();
        newData = false;
    }
    baca_sinyal_input();
    verbose_output();
    loop_oli();
    gerakkan_motor();
    // tes_oilpump_buzzer();
}