#ifndef OLI_YUUAHMAD
#define OLI_YUUAHMAD

// pinout oil lubricants
#define pompa_oli PA5
#define timer_oli_on PA6 // dari m3 
#define sensor_oli PA7
#define buzzer PB0

void init_oli();
void loop_oli();
int get_naik_timer_ke();
bool get_keadaan_pompa_oli();
bool get_keadaan_timer_oli_on();
bool get_keadaan_level_oli();
void set_nyalakan_pompa(bool on_pompa_serial);
#endif