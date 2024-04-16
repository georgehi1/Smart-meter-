
#include <avr/io.h>
#include <util/delay.h>




int main(){
    init_pwm();
    while(1){
    set_pwm(2.5);
    
    }

}


void init_pwm() {

DDRD |= _BV(PD7);
TCCR2A |= _BV(COM2A1) | _BV(WGM21) | _BV(WGM20); //non-inverting compare A mode, fast pwm top=0xFF
TCCR2B |= _BV(CS20);


}
void set_pwm(double val){

OCR2A = (uint16_t) ((val/3.3)*255); //convert 0-3.3V output (amplified using interface circuitry) to 0-255
}

