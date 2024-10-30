#include "Arduino.h"
#include "AccelStepper.h"
// #include "Oil.h"
#include "oliYuuahmad.h"
#include "EEPROM.h"

// definisi verbose output yang ingin ditampilkan
#define VERBOSE_PROXY 1
// #define STEPPER_MOTOR 1 // 1 stepper motor, 0 induksi motor
// #define VERBOSE_OLI 1
#define VERBOSE_MOTOR 1
#define VERBOSE_TOOLS 1
// #define VERBOSE_ERROR 1
#define VERBOSE_PERINTAH_PC 1
// #define VERBOSE_MUX_SWITCH 1
// #define PROGRAM_MUX_SWITCH 1
// #define HANYA_PROGRAM_MUX_SWITCH 1
#define CILEUNGSI 1

#ifndef HANYA_PROGRAM_MUX_SWITCH
// pin input ATC
#ifdef CILEUNGSI
#define PROX_UMB_MAJU PA0     // 0
#define PROX_UMB_MUNDUR PA1   // 1
#define PROX_CLAMP PA2        // 2
#define PROX_UNCLAMP PA3      // 3
#define PROX_TOOLS PA4        // 4
#define SPINDLE_ORIENT_OK PA6 // 5
// #define OIL_LEVEL PB3         // 6
// #define INPUT_EMERGENCY PB4   // 7
#else
#define PROX_UMB_MAJU PB9     // 0
#define PROX_UMB_MUNDUR PB8   // 1
#define PROX_CLAMP PA8        // 2
#define PROX_UNCLAMP PB10     // 3
#define PROX_TOOLS PB5        // 4
#define SPINDLE_ORIENT_OK PB4 // 5
// #define OIL_LEVEL PB3         // 6
// #define INPUT_EMERGENCY PB4   // 7
#endif

#ifdef PROGRAM_MUX_SWITCH
const int input_pins[6] = {
    PROX_UMB_MUNDUR,
    PROX_UMB_MUNDUR,
    PROX_CLAMP,
    PROX_UNCLAMP,
    PROX_TOOLS,
    SPINDLE_ORIENT_OK,
    OIL_LEVEL}; // Replace with your input pin numbers
#else
const int input_pins[6] = {
    PROX_UMB_MAJU,
    PROX_UMB_MUNDUR,
    PROX_CLAMP,
    PROX_UNCLAMP,
    PROX_TOOLS,
    SPINDLE_ORIENT_OK}; // Replace with your input pin numbers
#endif

// pin output ATC
#ifdef CILEUNGSI
#define STEP_STEPPER PB4   // 0
#define DIR_STEPPER PB5    // 1
#define MOVE_UMB PB6       // 4
#define TOOL_CLAMP PB7     // 3
#define SPINDLE_ORIENT PB8 // 2
// #define OIL_PUMP PA5     // 5
// #define BUZZER PB0       // 6
#else
#define STEP_STEPPER PB14   // 0
#define DIR_STEPPER PA15    // 1
#define SPINDLE_ORIENT PC13 // 2
#define TOOL_CLAMP PC14     // 3
#define MOVE_UMB PB1        // 4
// #define OIL_PUMP PA5     // 5
// #define BUZZER PB0       // 6
#endif
#ifdef PROGRAM_MUX_SWITCH
const int output_pins[3] = {
    OIL_PUMP,
    OIL_PUMP,
    OIL_PUMP}; // Replace with your input pin numbers
#else
#ifdef CILEUNGSI
const int output_pins[5] = {
    STEP_STEPPER,
    DIR_STEPPER,
    SPINDLE_ORIENT,
    TOOL_CLAMP,
    MOVE_UMB,
}; // Replace with your input pin numbers
#else
const int output_pins[3] = {
    TOOL_CLAMP,
    MOVE_UMB,
    SPINDLE_ORIENT}; // Replace with your input pin numbers
#endif
#endif

// setup motor stepper dan motor induksi
unsigned char gerak_motor = 'C'; // A=CW, B=CCW, C=STOP
#ifdef STEPPER_MOTOR
AccelStepper my_stepper = AccelStepper(1, STEP_STEPPER);
#endif

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

// variabel untuk mux switch
#define PIN0 PC13 // Pin untuk bit 0
#define PIN1 PC14 // Pin untuk bit 1
#define PIN2 PB1  // Pin untuk bit 2
#define INPUT PB9 // Pin untuk bit 2
int nilai_toogle[8] = {0, 0, 0, 0, 0, 0, 0, 0};
int nilai_input_mux[8] = {0, 0, 0, 0, 0, 0, 0, 0};
int nilai_input__muxsebelumnya[8] = {0, 0, 0, 0, 0, 0, 0, 0};

void verbose_output()
{
#ifdef VERBOSE_PROXY
    // untuk proxy dan lainnya
    Serial.print("T:");
    if (nilai_input[0])
        Serial.print("F"); // umbrella didepan
    if (nilai_input[1])
        Serial.print("B"); // umbrella dibelakang
    if (nilai_input[2])
        Serial.print("L"); // tool clamp
    if (nilai_input[3])
        Serial.print("U"); // tool unclamp
    if (nilai_input[5])
        Serial.print("O"); // orient ok
#endif

// untuk pergerakan motor
#ifdef VERBOSE_MOTOR
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
#endif

// untuk tools
#ifdef VERBOSE_TOOLS
    Serial.print("|P:");
    nilai_input[4] == 1 ? Serial.print("1") : Serial.print("0");
#endif

    // untuk eroor
#ifdef VERBOSE_ERROR
    Serial.print("|E:");
    Serial.print("unknown");
#endif
// untuk respon oil level
#ifdef VERBOSE_OLI
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
#endif

        // verbose tulis kembali perintah pc
#ifdef VERBOSE_PERINTAH_PC
    Serial.print("|R:");
    Serial.print(perintah_pc);
#endif

    // verbose baca nilai mux switch
#ifdef VERBOSE_MUX_SWITCH
    Serial.print("|X:");
    for (size_t i = 0; i < 8; i++)
        Serial.print(nilai_input_mux[i]);
#endif

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
        digitalWriteFast(digitalPinToPinName(SPINDLE_ORIENT), LOW); // spindle orient aktif low
        break;
    case 'J':
        perintah_pc = 'J';
        digitalWriteFast(digitalPinToPinName(SPINDLE_ORIENT), HIGH);
        break;
    case 'P':
        perintah_pc = 'P';
        digitalWriteFast(digitalPinToPinName(MOVE_UMB), HIGH); // relay aktif high
        break;
    case 'U':
        perintah_pc = 'U';
        digitalWriteFast(digitalPinToPinName(TOOL_CLAMP), HIGH); // relay aktif high
        break;
    case 'K':
        perintah_pc = 'K';
        digitalWriteFast(digitalPinToPinName(MOVE_UMB), LOW);
        break;
    case 'L':
        perintah_pc = 'L';
        digitalWriteFast(digitalPinToPinName(TOOL_CLAMP), LOW);
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
#ifdef STEPPER_MOTOR
    case 'A':
        my_stepper.setSpeed(3500);
        digitalWriteFast(digitalPinToPinName(DIR_STEPPER), HIGH);
        my_stepper.runSpeed();
        break;
    case 'B':
        my_stepper.setSpeed(3500);
        digitalWriteFast(digitalPinToPinName(DIR_STEPPER), LOW);
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
        digitalWriteFast(digitalPinToPinName(STEP_STEPPER), HIGH);
        digitalWriteFast(digitalPinToPinName(DIR_STEPPER), LOW);
        break;
    case 'B':
        digitalWriteFast(digitalPinToPinName(STEP_STEPPER), LOW);
        digitalWriteFast(digitalPinToPinName(DIR_STEPPER), HIGH);
        break;
    case 'C':
        digitalWriteFast(digitalPinToPinName(STEP_STEPPER), LOW);
        digitalWriteFast(digitalPinToPinName(DIR_STEPPER), LOW);
        break;
    default:
        break;
#endif
    }
}

void baca_mux_switch()
{
    for (size_t i = 0; i < 8; i++)
    {
        digitalWriteFast(digitalPinToPinName(PIN0), i & 1);
        digitalWriteFast(digitalPinToPinName(PIN1), (i >> 1) & 1);
        digitalWriteFast(digitalPinToPinName(PIN2), (i >> 2) & 1);
        // delay(10);

        // int reading;
        nilai_input_mux[i] = digitalReadFast(digitalPinToPinName(INPUT));
        // Tampilkan nilai biner pada pin output
        // nilai_input[i] = reading;

        // Update the button state
        // if (reading != nilai_input[i])
        // {
        //   if (nilai_input[i] == HIGH)
        //     nilai_toogle[i] = !nilai_toogle[i];
        // }
        // nilai_input_sebelumnya[i] = reading;
    }
    // delay(100); // Tambahkan delay 500 ms
}

void setup()
{
    Serial.begin(115200);
    // set pin input dan output
    for (int i = 0; i < 7; i++)
        pinMode(input_pins[i], INPUT_PULLDOWN);
    for (int i = 0; i < 3; i++)
        pinMode(output_pins[i], OUTPUT);
    // Konfigurasi pin I/O baca mux switch
    pinMode(PIN0, OUTPUT);
    pinMode(PIN1, OUTPUT);
    pinMode(PIN2, OUTPUT);
    pinMode(INPUT, INPUT_PULLUP);

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
#ifndef program_mux_switch
    digitalWriteFast(digitalPinToPinName(SPINDLE_ORIENT), HIGH);
    digitalWriteFast(digitalPinToPinName(TOOL_CLAMP), LOW);
    digitalWriteFast(digitalPinToPinName(MOVE_UMB), LOW);
#endif

    // inisialisasi stepper
#ifdef STEPPER_MOTOR
    my_stepper.setMaxSpeed(5000);
    my_stepper.setPinsInverted(false, false);
    my_stepper.stop();
    my_stepper.setCurrentPosition(0);
#endif
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
#ifdef HANYA_PROGRAM_MUX_SWITCH
    baca_mux_switch();
#endif
    verbose_output();
    loop_oli();
    gerakkan_motor();
    // tes_oilpump_buzzer();
}
#else
#include <Arduino.h>

// verbose
#define VERBOSE 1

// simulasi ic mux switch
// #define MUX_SWITCH 1

#if defineD(mux_switch)
int nilai_input[13] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
bool nilai_toogle[13] = {false, false, false, false, false, false, false, false, false, false, false, false};
int nilai_input_sebelumnya[13] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
int nilai_debounce_input_sebelumnya[13] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
const int nilai_debounce = 20;

// pin output digital
#define OUTPUT_1 13
#define OUTPUT_2 12
#define OUTPUT_3 11
#define OUTPUT_4 10
#define OUTPUT_5 9
#define OUTPUT_6 8
#define OUTPUT_7 7
#define OUTPUT_8 6
#define OUTPUT_MUX 2
int output_pins_digital[9] = {output_1, output_2, output_3, output_4, output_5, output_6, output_7, output_8, output_mux};

// pin input digital
#define INPUT_MUX_1 5
#define INPUT_MUX_2 4
#define INPUT_MUX_3 3
#define INPUT_1 A0
#define INPUT_2 A1
#define INPUT_3 A2
#define INPUT_4 A3
#define INPUT_5 A4
#define INPUT_6 A5
int input_pins_digital[9] = {input_mux_1, input_mux_2, input_mux_3, input_1, input_2, input_3, input_4, input_5, input_6};
// pin input analog
#define INPUT_7 A6
#define INPUT_8 A7
int input_pins_analog[2] = {input_7, input_8};

int selectorValue = 0;
int selectorValue_sebelumnya = 0;

void setup()
{
    for (size_t i = 0; i < 9; i++) // ada 8 buah output
        pinMode(output_pins_digital[i], OUTPUT);
    for (size_t i = 0; i < 2; i++) // ada 8 buah output
        pinMode(input_pins_analog[i], INPUT);
    for (size_t i = 0; i < 9; i++) // ada 10 buah output
        pinMode(input_pins_digital[i], INPUT_PULLUP);

// init serial
#if defineD(verbose)
    Serial.begin(115200);
#endif // verbose
}

void loop()
{
    selectorValue = 0;
    for (int i = 0; i < 11; i++)
    {
        int reading;
        if (i < 9) // tombol digital
            reading = digitalRead(input_pins_digital[i]);
        else // tombool analog
        {
            reading = analogRead(input_pins_analog[i - 9]);
            reading < 10 ? reading = 0 : reading = 1;
        }

        // Check if the button state has changed
        if (reading != nilai_input_sebelumnya[i])
            nilai_debounce_input_sebelumnya[i] = millis();

        // If the button state has remained the same for longer than the debounce delay
        else if ((millis() - nilai_debounce_input_sebelumnya[i]) > nilai_debounce)
        {
            // Update the button state
            if (reading != nilai_input[i])
            {
                nilai_input[i] = reading;
                if (nilai_input[i] == LOW)
                    nilai_toogle[i] = !nilai_toogle[i];
            }
        }
        if (i < 3)
            selectorValue |= (nilai_input[i] << i);
        nilai_input_sebelumnya[i] = reading;
    }

    for (int j = 0; j < 8; j++) // tampilkan nilai input ke led
        digitalWrite(output_pins_digital[j], !nilai_input[j + 3]);

    // mux switch
    int selectedInput = digitalRead(output_pins_digital[selectorValue]);
    digitalWrite(output_mux, selectedInput);

// tampilkan nilai
#if defineD(verbose)
    Serial.print("inp: ");
    for (size_t i = 0; i < 11; i++)
        Serial.print(nilai_input[i]);
    // Serial.print(" inps: ");
    // for (size_t i = 0; i < 11; i++)
    //   Serial.print(nilai_input_sebelumnya[i]);
    Serial.print(" nil_led: ");
    for (size_t i = 0; i < 11; i++)
        Serial.print(nilai_toogle[i]);
    Serial.print(" sel: ");
    Serial.print(selectorValue);
    Serial.println();
#endif // verbose
}
#else
// variabel untuk mux switch
#define PIN0 PC13 // Pin untuk bit 0
#define PIN1 PC14 // Pin untuk bit 1
#define PIN2 PB1  // Pin untuk bit 2
#define INPUT PB9 // Pin untuk bit 2

int nilai_toogle[8] = {0, 0, 0, 0, 0, 0, 0, 0};
int nilai_input[8] = {0, 0, 0, 0, 0, 0, 0, 0};
int nilai_input_sebelumnya[8] = {0, 0, 0, 0, 0, 0, 0, 0};

void setup()
{
    Serial.begin(115200); // Inisialisasi Serial Monitor pada baud rate 9600

    // Konfigurasi pin sebagai output
    pinMode(PIN0, OUTPUT);
    pinMode(PIN1, OUTPUT);
    pinMode(PIN2, OUTPUT);
    pinMode(input, INPUT_PULLUP);
}

void loop()
{
    for (size_t i = 0; i < 8; i++)
    {
        digitalWriteFast(digitalPinToPinName(PIN0), (i & 1));
        digitalWriteFast(digitalPinToPinName(PIN1), ((i >> 1) & 1));
        digitalWriteFast(digitalPinToPinName(PIN2), ((i >> 2) & 1));
        delay(10);

        // int reading;
        nilai_input[i] = digitalReadFast(digitalPinToPinName(input));
        // Tampilkan nilai biner pada pin output
        // nilai_input[i] = reading;

        // Update the button state
        // if (reading != nilai_input[i])
        // {
        //   if (nilai_input[i] == HIGH)
        //     nilai_toogle[i] = !nilai_toogle[i];
        // }
        // nilai_input_sebelumnya[i] = reading;

#if defineD(verbose)
        Serial.print(" ");
        // Serial.print(i, BIN); // Tampilkan nilai biner di Serial Monitor
        // Serial.print("|");
        Serial.print(nilai_input[i]);
#endif

        // delay(100); // Tambahkan delay 500 ms
    }

#if defineD(verbose)
    // Serial.print(" input: ");
    // for (size_t i = 0; i < 8; i++)
    //   Serial.print(nilai_input[i]);
    // Serial.print(" toogle: ");
    // for (size_t i = 0; i < 8; i++)
    //   Serial.print(nilai_toogle[i]);
    Serial.println();
#endif
}
#endif // mux_switch

#endif