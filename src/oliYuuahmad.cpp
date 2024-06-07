#include "oliYuuahmad.h"
#include "EEPROM.h"

extern int waktu_delay;
extern int waktu_on;
extern int banyak_cycle;

unsigned short int keadaan_level_oli;
unsigned short int keadaan_timer_oli_on;
unsigned short int keadaan_pompa_oli;

int naik_timer_ke = 0; // 15 menit yang ke berapa?
bool nyalakan_pompa = false;
unsigned long prev_millis_tulis_eeprom_oli = 0; // Waktu terakhir kalkulasi oli
unsigned long prev_millis_pompa_oli = 0;        // Waktu terakhir pompa oli dinyalakan

void init_oli()
{
    // Inisialisasi pin io dan state pertama
    pinMode(pompa_oli, OUTPUT);
    pinMode(timer_oli_on, INPUT);
    pinMode(sensor_oli, INPUT_PULLUP);
    pinMode(buzzer, OUTPUT);
    digitalWriteFast(digitalPinToPinName(buzzer), LOW);
    digitalWriteFast(digitalPinToPinName(pompa_oli), LOW);

    // Membaca waktu terakhir dari EEPROM
    // naik_timer_ke = EEPROM.read(0);

    // // Jika EEPROM kosong (nilai awal), set waktu ke 0
    // if (naik_timer_ke == 0xFFFFFFFF || naik_timer_ke > 10 || naik_timer_ke < 0)
    //     naik_timer_ke = 0;
}

void loop_oli() // perhitungan waktu dan komputasi oli
{
    // nyalakan buzzer ketika oli habis
    if (digitalRead(sensor_oli))
        digitalWrite(buzzer, HIGH);
    // nyalakan pompa ketika perintah pc == Q
    else if (nyalakan_pompa)
    {
        if (millis() - prev_millis_pompa_oli >= (waktu_on * 1000))
        {
            if (!digitalRead(pompa_oli))
                digitalWrite(pompa_oli, HIGH);
            else
            {
                digitalWrite(pompa_oli, LOW);
                nyalakan_pompa = false;
            }
            prev_millis_pompa_oli = millis();
        }
    }
    // nyalakan pompa setiap selang waktu_delay dan resume ketika spindle menyala lagi
    else if (!digitalRead(sensor_oli) && digitalRead(timer_oli_on))
    {
        digitalWrite(buzzer, LOW);
        if (millis() - prev_millis_tulis_eeprom_oli >= ((waktu_delay * 60 * 1000) / banyak_cycle))
        {
            // nyalakan oli setiap 1 jam sekali
            naik_timer_ke > (banyak_cycle - 1) ? naik_timer_ke = 0 : naik_timer_ke++;
            if (naik_timer_ke == 0)
                nyalakan_pompa = true;
            // EEPROM.write(0, naik_timer_ke); // komen ketika debugging
            prev_millis_tulis_eeprom_oli = millis();
        }
        if (nyalakan_pompa)
            if (millis() - prev_millis_pompa_oli >= (waktu_on * 1000))
            {
                if (!digitalRead(pompa_oli))
                    digitalWrite(pompa_oli, HIGH);
                else
                {
                    digitalWrite(pompa_oli, LOW);
                    nyalakan_pompa = false;
                }
                prev_millis_pompa_oli = millis();
            }
    }
    else
        digitalWrite(buzzer, LOW);
}

bool get_keadaan_pompa_oli()
{
    keadaan_pompa_oli = digitalRead(pompa_oli);
    return keadaan_pompa_oli;
};

int get_naik_timer_ke()
{
    return naik_timer_ke;
};

bool get_keadaan_timer_oli_on()
{
    keadaan_timer_oli_on = digitalRead(timer_oli_on);
    return keadaan_timer_oli_on;
};

bool get_keadaan_level_oli()
{
    keadaan_level_oli = digitalRead(sensor_oli);
    return keadaan_level_oli;
};

void set_nyalakan_pompa(bool on_pompa_serial)
{
    nyalakan_pompa = on_pompa_serial;
}