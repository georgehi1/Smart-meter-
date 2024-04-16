#include "pwm.h"

volatile uint32_t count = 0;



void draw_screen(int state)
{
    // Set orientation
    set_orientation(West);

    double v_test = 2.432;
    double i_test = 2.567;
    double Isolar = 0;
   

    

    // Clear the screen
    clear_screen();

    // Converting result to string so it can be printed
    char buffer[8];


    //Vbus
    display.x = XBORDER;
    display.y = ROW1;
    display_string("Busbar Voltage:");


    display.x = 120;
    //dtostrf, 6, 2, buffer); // Convert double to string with 2 decimal places
    display_string(buffer);
    //status_bar(100, GREEN);
    
    
    //Ibus
    display.x = XBORDER;
    display.y = ROW2;
    display_string("Busbar Current(A):");
    
    display.x = 120;
    dtostrf(i_test, 6, 2, buffer); // Convert double to string with 2 decimal places
    display_string(buffer);

    //Wind
    display.x = XBORDER;
    display.y = ROW3;
    display_string("Wind Current(A):");
    
    display.x = 120;
    //dtostrf(Iwind , 6, 2, buffer); // Convert double to string with 2 decimal places
    display_string(buffer);

    //PV
    display.x = XBORDER;
    display.y = ROW4;
    display_string("PV Current (A):");
    
    display.x = 120;
    dtostrf(Isolar, 6, 2, buffer); // Convert double to string with 2 decimal places
    display_string(buffer);


    display.x = XBORDER;
    display.y = ROW6;
    display_string("Power:");
   
}


void status_bar(uint16_t length, uint16_t colour) {

    // To make the bar fit the screen
    length = length/ 8;

    // drawing the bar
    rectangle bar;
    bar.top = display.y;
    bar.left = display.x;
    bar.bottom = display.y + 7;
    bar.right = display.x + length;

    fill_rectangle(bar, colour);
}
void print_numbers(double val, int row){
   
    display.x = 120; 
    display.y = row;
    char buffer[8];
    dtostrf(val, 6, 2, buffer);
    display_string(buffer);

}

void update(double* Iwind, double* Isolar, double* busbar_power){


*busbar_power = Iwind * Isolar;

print_numbers((*Iwind), 90);
print_numbers((*Isolar), 120);
print_numbers((*busbar_power), 180);

//print_numbers(*v, ROW1);

}

int main(){
init_lcd();

double i = 0;
double j = 0;
double p = i*j;
init_timer();


sei();
draw_screen(1);
while(1) {
    display_time();
    

    i +=1;

    j+=0.2;

   update(&i, &j, &p); 

_delay_ms(500);


}
return 0;
}


void display_time(){
    char buffer[20];
    display.x = XBORDER;
    display.y = ROW5;
    snprintf(buffer, sizeof(buffer), "TIME: %lu", count / 1000); // Convert count to seconds
    display_string(buffer); // Display the string on the screen
     
}


void init_timer() {
    TCCR1B |= _BV(WGM12); // Set CTC mode
    TCCR1B |= _BV(CS11); // Prescaler to 8
    OCR1A = 1499; // Match value for 1ms at 12MHz clock with prescaler of 8
    TIMSK1 |= _BV(OCIE1A); // Enable Timer 1 CTC interrupt
}

ISR(TIMER1_COMPA_vect) {
    count++; // Increment count every millisecond
}
