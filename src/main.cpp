#include "Arduino.h"
#include "AccelStepper.h"
// #include "Oil.h"
#include "oliYuuahmad.h"
#include "EEPROM.h"

// definisi verbose output yang ingin ditampilkan
// #define verbose_proxy 1
// #define stepper_motor 1 // 1 stepper motor, 0 induksi motor
#define verbose_oli 1
// #define verbose_motor 1
// #define verbose_tools 1
// #define verbose_error 1
#define verbose_perintah_pc 1
#define verbose_mux_switch 1
#define program_mux_switch 1
// #define hanya_program_mux_switch 1

#ifndef hanya_program_mux_switch
// pin input ATC
#define prox_umb_maju PB9     // 0
#define prox_umb_mundur PB8   // 1
#define prox_clamp PA8        // 2
#define prox_unclamp PB10     // 3
#define prox_tools PB5        // 4
#define spindle_orient_ok PB4 // 5 dari board orientasi spindle (pi5)
#define oil_level PB3         // 6 (pi6)
#define input_emergency PB4   // 7

#ifdef program_mux_switch
const int input_pins[7] = {prox_umb_mundur, prox_umb_mundur, prox_clamp, prox_unclamp,
                           prox_tools, spindle_orient_ok, oil_level}; // Replace with your input pin numbers
#else
const int input_pins[7] = {prox_umb_maju, prox_umb_mundur, prox_clamp, prox_unclamp,
                           prox_tools, spindle_orient_ok, oil_level}; // Replace with your input pin numbers
#endif

// pin output ATC
#define step_stepper PB14   // 4
#define dir_stepper PA15    // 5
#define spindle_orient PC13 // 2 perintahkan spindle orient
#define tool_clamp PC14     // 0 clamp tools
#define move_umb PB1        // 1 aktuator maju-mundur umbrella
#define oil_pump PA5        // 6
// #define buzzer PB0         // 3
#ifdef program_mux_switch
const int output_pins[3] = {oil_pump, oil_pump,
                            oil_pump}; // Replace with your input pin numbers
#else
const int output_pins[3] = {tool_clamp, move_umb,
                            spindle_orient}; // Replace with your input pin numbers
#endif
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

// variabel untuk mux switch
#define pin0 PC13 // Pin untuk bit 0
#define pin1 PC14 // Pin untuk bit 1
#define pin2 PB1  // Pin untuk bit 2
#define input PB9 // Pin untuk bit 2
int nilai_toogle[8] = {0, 0, 0, 0, 0, 0, 0, 0};
int nilai_input_mux[8] = {0, 0, 0, 0, 0, 0, 0, 0};
int nilai_input__muxsebelumnya[8] = {0, 0, 0, 0, 0, 0, 0, 0};

void verbose_output()
{
#ifdef verbose_proxy
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
#endif

// untuk pergerakan motor
#ifdef verbose_motor
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
#ifdef verbose_tools
    Serial.print("|P:");
    nilai_input[4] == 1 ? Serial.print("1") : Serial.print("0");
#endif

    // untuk eroor
#ifdef verbose_error
    Serial.print("|E:");
    Serial.print("unknown");
#endif
// untuk respon oil level
#ifdef verbose_oli
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
#ifdef verbose_perintah_pc
    Serial.print("|R:");
    Serial.print(perintah_pc);
#endif

    // verbose baca nilai mux switch
#ifdef verbose_mux_switch
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
#ifdef stepper_motor
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

void baca_mux_switch()
{
    for (size_t i = 0; i < 8; i++)
    {
        digitalWriteFast(digitalPinToPinName(pin0), i & 1);
        digitalWriteFast(digitalPinToPinName(pin1), (i >> 1) & 1);
        digitalWriteFast(digitalPinToPinName(pin2), (i >> 2) & 1);
        // delay(10);

        // int reading;
        nilai_input_mux[i] = digitalReadFast(digitalPinToPinName(input));
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
    pinMode(pin0, OUTPUT);
    pinMode(pin1, OUTPUT);
    pinMode(pin2, OUTPUT);
    pinMode(input, INPUT_PULLUP);

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
    digitalWriteFast(digitalPinToPinName(spindle_orient), HIGH);
    digitalWriteFast(digitalPinToPinName(tool_clamp), LOW);
    digitalWriteFast(digitalPinToPinName(move_umb), LOW);
#endif

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
    baca_mux_switch();
    verbose_output();
    loop_oli();
    gerakkan_motor();
    // tes_oilpump_buzzer();
}
#else
#include <Arduino.h>

// verbose
#define verbose 1

// simulasi ic mux switch
// #define mux_switch 1

#if defined(mux_switch)
int nilai_input[13] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
bool nilai_toogle[13] = {false, false, false, false, false, false, false, false, false, false, false, false};
int nilai_input_sebelumnya[13] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
int nilai_debounce_input_sebelumnya[13] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
const int nilai_debounce = 20;

// pin output digital
#define output_1 13
#define output_2 12
#define output_3 11
#define output_4 10
#define output_5 9
#define output_6 8
#define output_7 7
#define output_8 6
#define output_mux 2
int output_pins_digital[9] = {output_1, output_2, output_3, output_4, output_5, output_6, output_7, output_8, output_mux};

// pin input digital
#define input_mux_1 5
#define input_mux_2 4
#define input_mux_3 3
#define input_1 A0
#define input_2 A1
#define input_3 A2
#define input_4 A3
#define input_5 A4
#define input_6 A5
int input_pins_digital[9] = {input_mux_1, input_mux_2, input_mux_3, input_1, input_2, input_3, input_4, input_5, input_6};
// pin input analog
#define input_7 A6
#define input_8 A7
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
#if defined(verbose)
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
#if defined(verbose)
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
#define pin0 PC13 // Pin untuk bit 0
#define pin1 PC14 // Pin untuk bit 1
#define pin2 PB1  // Pin untuk bit 2
#define input PB9 // Pin untuk bit 2

int nilai_toogle[8] = {0, 0, 0, 0, 0, 0, 0, 0};
int nilai_input[8] = {0, 0, 0, 0, 0, 0, 0, 0};
int nilai_input_sebelumnya[8] = {0, 0, 0, 0, 0, 0, 0, 0};

void setup()
{
    Serial.begin(115200); // Inisialisasi Serial Monitor pada baud rate 9600

    // Konfigurasi pin sebagai output
    pinMode(pin0, OUTPUT);
    pinMode(pin1, OUTPUT);
    pinMode(pin2, OUTPUT);
    pinMode(input, INPUT_PULLUP);
}

void loop()
{
    for (size_t i = 0; i < 8; i++)
    {
        digitalWriteFast(digitalPinToPinName(pin0), (i & 1));
        digitalWriteFast(digitalPinToPinName(pin1), ((i >> 1) & 1));
        digitalWriteFast(digitalPinToPinName(pin2), ((i >> 2) & 1));
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

#if defined(verbose)
        Serial.print(" ");
        // Serial.print(i, BIN); // Tampilkan nilai biner di Serial Monitor
        // Serial.print("|");
        Serial.print(nilai_input[i]);
#endif

        // delay(100); // Tambahkan delay 500 ms
    }

#if defined(verbose)
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