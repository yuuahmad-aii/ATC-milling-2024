#ifndef ORIENTATION_H
#define ORIENTATION_H
#define SPINDLE_ORIENT

int TOOL_POS();
class ATC
{
private:
    int pin;
    int bounce = 10;
    bool status;
    unsigned long timestamp;

public:
    ATC(int pinNumber, int mode);
    void setInputBounce(int value);
    void OFF();
    void ON();
    bool state();
};

void loop_orient();
void orientation_init();

extern ATC pinOut[];
#endif