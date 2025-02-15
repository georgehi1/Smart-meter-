#include "pwm.h"//read inputs function, switch up code a bit 


/*  Values that might be needed

Busbar V: look for getV_amplitude() function

Imains: look for set_pwm(), usually line above is setting Imains to a certain value

PV & Wind Capacity: adc() function, variables Iwind & Isolar give 0-5V range

Battery Charge/Discharge: look for battery_charge & battery_discharge variables
                          or battery_control(). battery_control(1,0) changes 
                          battery_charge value to opposite value, battery_control(0,1) 
                          does the same for battery_discharge. Good place to put screen
                          update function after battery_control function. 


*/

volatile uint32_t count = 0;

volatile uint32_t capacity = 0;
volatile uint32_t charge_start_t = 0;
volatile uint32_t charge_end_t = 0;
volatile uint32_t discharge_start_t = 0;
volatile uint32_t discharge_end_t = 0;
volatile uint8_t battery_charge = 1;
volatile uint8_t battery_discharge = 1;

ISR(TIMER1_COMPA_vect)
{

  count += 1;	//increments every ms


}

int main(){

double busbar_v = 0;
double busbar_i = 0;
double Imains = 0;
double Iwind = 0;
double Isolar = 0;
double Irequired = 0;
double Irenewable = 0;
double check_renewable = 0;
uint8_t control = 1;
uint8_t load_off = 0;
uint8_t status_mains = 1;
uint8_t load1_set = 1;
uint8_t load2_set = 1;
uint8_t load3_set = 1;
uint8_t load1_req = 1;
uint8_t load2_req = 1;
uint8_t load3_req = 1;
uint32_t battery_drain = 0;
uint32_t check_start_mains = 0;
uint32_t check_end_mains = 0;

init_lcd();
init_pwm();
init_adc();
init_digital();
init_timer();

sei();

init_loadsPwm();

while(1){

    draw_screen(1);
    update(&Iwind, &Isolar);
    
   while(control == 1){ // begin control loop
        
        if(load1_req != get_digital(RLOAD1) || load2_req != get_digital(RLOAD2) || load3_req != get_digital(RLOAD3) ) {
            draw_screen(1);
            //loads requested differ from load request values on pins
            control = 0;



        }

        if(battery_discharge == 1){

            battery_drain = capacity - (count - discharge_start_t); //capacity measurement
            if((status_mains ==1) && (battery_drain < 200000)){ //if mains is on & battery capacity < 3mins
                draw_screen(1);
                control = 0;

            }
        } if((status_mains == 0) && (battery_drain < 10)){ //mains on & not enough capacity

            control = 0;
        }


    
        check_renewable = ((adc(WINDI))/1023.0) * 5; //wind current
        //printf("%f", check_renewable);
        if((Iwind < (check_renewable - 0.02)) || (Iwind > (check_renewable + 0.02))){ //wind current changed?

            control = 0;
        }

        check_renewable = ((adc(PVI))/1023) * 5; //solar current
        
        if((Isolar < (check_renewable - 0.02)) || (Isolar > (check_renewable + 0.02))){

            control = 0;
        }

        
        
        
            update(&Iwind, &Isolar); 
         
    }

    load1_req = (get_digital(RLOAD1)) ? 1 : 0;  //saves load signals from interface to local req variables
    load2_req = (get_digital(RLOAD2)) ? 1 : 0;
    load3_req = (get_digital(RLOAD3)) ? 1 : 0;

    Irequired = (load1_req * ILOAD1) + (load2_req * ILOAD2) + (load3_req * ILOAD3);

    Iwind = ((adc(WINDI))/1023.0) * 5; //wind current
    Isolar = ((adc(PVI))/1023.0) * 5; //PV current

    Irenewable = Iwind + Isolar;

    update( &Iwind, &Isolar); 
    _delay_ms(1000);

    //DECISION BLOCK

    if((Irequired == 0) && (Irenewable == 0)) { //something wrong as Irequired is 0

        load1_set = load1_req; 
        load2_set = load2_req;
        load3_set = load3_req;
        load_control(&load1_set, &load2_set, &load3_set);
        control = 1;
    }

    else if (Irenewable > Irequired){ //current surplus from renewable

        Imains = 0;
        set_pwm(Imains); //no need for mains

        if(battery_discharge == 1) { //battery stops discharging

            battery_discharge = 0;
            battery_control(0,1);
        }

        if((Irenewable - Irequired) > 1) { // 1A current surplus

            if(battery_charge == 1) { //set up loads while battery charging

                load1_set = load1_req; 
                load2_set = load2_req;
                load3_set = load3_req;
                load_control(&load1_set, &load2_set, &load3_set);
            }

            else { //charge battery via surplus

                battery_charge = 1;
                battery_control(1,0);
                load1_set = load1_req; 
                load2_set = load2_req;
                load3_set = load3_req;
                load_control(&load1_set, &load2_set, &load3_set);

            }

            control = 1;
        } 

        else if (capacity > 600000) { // 600,000 = 10mins

            if(battery_charge == 1) { //no need to charge more

                battery_charge = 0;
                battery_control(1,0);
            }
            load1_set = load1_req; 
            load2_set = load2_req;
            load3_set = load3_req;
            load_control(&load1_set, &load2_set, &load3_set);
            control = 1;
        }

        else {

            if(battery_charge == 0) { //not enough capacity, charge battery

                battery_charge = 1;
                battery_control(1,0);

            }

            Imains = 1-(Irenewable - Irequired); // mains supplements current so we have 1A extra

            set_pwm(Imains);
            load1_set = load1_req; 
            load2_set = load2_req;
            load3_set = load3_req;
            load_control(&load1_set, &load2_set, &load3_set);
            controller(2, &Imains, &Irenewable, &status_mains, &load1_req, &load2_req, &load3_req, &load1_set, &load2_set, &load3_set,&control);
            //loop 3: stops charging when mains turns on
        
        }
    }

    else {
        if((Irequired - Irenewable) < 1) { //not enough current surplus to charge battery

            if(battery_discharge == 1) { //stop discharging, check capacity

                battery_discharge = 0;
                battery_control(0,1);
            } 

            if(capacity > 600000) { // more than 6mins of capacity

                if(battery_charge == 1) { //no need to charge

                    battery_charge = 0;
                    battery_control(1,0);
                }

                Imains = Irequired - Irenewable; //mains used to fill deficit

                set_pwm(Imains);
                load1_set = load1_req; 
                load2_set = load2_req;
                load3_set = load3_req;
                load_control(&load1_set, &load2_set, &load3_set);
                controller(1, &Imains, &Irenewable, &status_mains, &load1_req, &load2_req, &load3_req, &load1_set, &load2_set, &load3_set,&control);
                //loop 2 on controller: Deciding what loads to turn on 
            }

            else { //less than 6mins capacity, charge battery

                if(battery_charge != 1) {

                    battery_charge = 1;
                    battery_control(1,0);
                }

                Imains = (Irequired - Irenewable) + 1; //mains provides 1A excess to charge battery with

                set_pwm(Imains);
                load1_set = load1_req; 
                load2_set = load2_req;
                load3_set = load3_req;
                load_control(&load1_set, &load2_set, &load3_set);
                controller(2, &Imains, &Irenewable, &status_mains, &load1_req, &load2_req, &load3_req, &load1_set, &load2_set, &load3_set,&control);
                controller(1, &Imains, &Irenewable, &status_mains, &load1_req, &load2_req, &load3_req, &load1_set, &load2_set, &load3_set,&control);
                //loop 2: turn off charging, then loop 1
            }
        }

        else { //no surplus of renewable current

            if(capacity > 180000) { //if enough capacity, begin discharging to supplement deficit

                if(battery_charge == 1) {

                    battery_charge = 0;
                    battery_control(1,0);
                }

                if(battery_discharge != 1) {

                    battery_discharge = 1;
                    battery_control(0,1);
                }

                Imains = Irequired - Irenewable - 1; // -1 becuase battery is discharging

                set_pwm(Imains);
                load1_set = load1_req; 
                load2_set = load2_req;
                load3_set = load3_req;
                load_control(&load1_set, &load2_set, &load3_set);
                controller(1, &Imains, &Irenewable, &status_mains, &load1_req, &load2_req, &load3_req, &load1_set, &load2_set, &load3_set,&control);
            
            }

            else { //not enough capacity in battery for discharging

                if(battery_discharge == 1) {

                    battery_discharge = 0;
                    battery_control(0,1);
                }

                Imains = Irequired - Irenewable;

                if((Imains + 1) < 3) {

                    Imains += 1; // Increase Imains enough to get 1A extra to charge battery
                    if(battery_charge != 1) { 

                        battery_charge = 1;
                        battery_control(1,0);
                    }

                }
                else if(battery_charge == 1) {

                    battery_charge = 0;
                    battery_control(1,0);
                }


                set_pwm(Imains);
                load1_set = load1_req; 
                load2_set = load2_req;
                load3_set = load3_req;
                load_control(&load1_set, &load2_set, &load3_set);
                controller(2, &Imains, &Irenewable, &status_mains, &load1_req, &load2_req, &load3_req, &load1_set, &load2_set, &load3_set,&control);
                controller(1, &Imains, &Irenewable, &status_mains, &load1_req, &load2_req, &load3_req, &load1_set, &load2_set, &load3_set,&control);
            }
        }
    }

  return 0;
}


}
void init_pwm() {

    DDRD |= _BV(PD5);
    TCCR1A |= _BV(COM1A0) | _BV(WGM10); //non-inverting compare A mode, fast pwm top=0xFF
    TCCR1B |= _BV(CS11);
    TCCR1B |= _BV(WGM13);

    //OCR2A = 0; //Compare match A register, holds max value PWM will go to
}

void init_digital(){  //inputs on port A, outputs on port D

    DDRIN &= ~(_BV(RLOAD1) | _BV(RLOAD2) | _BV(RLOAD3));
    PORTIN &= ~(_BV(RLOAD1) | _BV(RLOAD2) | _BV(RLOAD3));
    DDROUT |= _BV(CHARGEPIN) | _BV(DISCHARGEPIN) | _BV(SLOAD1) | _BV(SLOAD2) | _BV(SLOAD3); //loads in empty spots, turning loads on signals
}

void init_loadsPwm(){

    set_switch(SLOAD1,0);
    set_switch(SLOAD2,0);
    set_switch(SLOAD3,0);
    set_switch(CHARGEPIN,0);
    set_switch(DISCHARGEPIN,0);
    set_pwm(0);
}

void init_adc(){

DDRIN &= ~(_BV(PINI) | _BV(PINV) | _BV(WINDI) | _BV(PVI) ); //ADC inputs
ADCSRA |= _BV(ADPS2) | _BV(ADPS1) | _BV(ADEN); //turn on ADC, F_ADC = F_CPU/64

}

void set_pwm(double val){

OCR1A = (uint16_t) ((val/3.3)*255); //convert 0-3.3V output (amplified using interface circuitry) to 0-255
}

void init_timer(){

TCCR1B |= _BV(WGM12); //ctc mode.
TCCR1B |= _BV(CS11); //setting prescaler to 8.
OCR1A = 1499; //Max value to count every 1ms.

TIMSK1 |= _BV(OCIE1A); //enable interrupt.

}



uint16_t adc(int channel){

    ADMUX = channel;
    ADCSRA |= _BV(ADSC);
    while(ADCSRA & _BV(ADSC)) {};

    return ADC; //returns value once conversion is done, repeated via while(1) loop


}

uint8_t get_digital(uint8_t pin){ //tells you what value is on a pin

    return PININ & _BV(pin);
}

double getV_amplitude() { //busbar voltage


    double bus_sample = 0;
    double bus_amplitude = 0;
    uint32_t peak_end_t = count + 600; //val = how long it will take to measure the peak
    while (count < peak_end_t){
        bus_sample = abs(adc(PINV)-511); //(1024/2)-1 = 511 which is midpoint


        if(bus_sample > bus_amplitude){
            
            bus_amplitude = bus_sample;

        }


    }
        return ((bus_amplitude/512.0)/sqrt(2));
}


double getI_amplitude() { //busbar current

    
    double bus_sample = 0;
    double bus_amplitude = 0;
    uint32_t peak_end_t = count + 600; //val = how long it will take to measure the peak
    while (count < peak_end_t){
        bus_sample = abs(adc(PINI)-511); //(1024/2)-1 = 511 which is midpoint


        if(bus_sample > bus_amplitude){
            
            bus_amplitude = bus_sample;

        }


    }
        return ((bus_amplitude/1024)/sqrt(2));
}


void set_switch(uint8_t pin, uint8_t state) { //setting value at a pin to a value, used for outputs to testbench

    if (state){
        PORTOUT |= _BV(pin); 
    } else {

        PORTOUT &= ~ _BV(pin);
    }



}

void battery_control(uint8_t charge_c, uint8_t discharge_c){

    if((charge_c) == 1){  //might be way to optimize this, combine charge_c & battery_charge
        if(battery_charge == 1){
            set_switch(CHARGEPIN, battery_charge);
            charge_start_t = count;
        } 
        else if(battery_charge == 0){
            set_switch(CHARGEPIN, battery_charge);
            charge_end_t = count;
            capacity += (charge_end_t-charge_start_t); //working out how long we have charged for, adding to capacity
        }
    }

    if(discharge_c == 1){
        if(battery_discharge == 1){
            set_switch(DISCHARGEPIN, battery_discharge);
            discharge_start_t = count;
        }
        if(battery_discharge == 0){
            set_switch(DISCHARGEPIN, battery_discharge);
            discharge_end_t = count;

            capacity -= (discharge_end_t - discharge_start_t); //works out how much time we have discharged, subtracts from capacity
        }

    }


}



void load_control(uint8_t* load1_set,uint8_t* load2_set, uint8_t* load3_set) { 
    
    set_switch(SLOAD1, *load1_set); //sets loads to their values from pins
    set_switch(SLOAD2, *load2_set);
    set_switch(SLOAD3, *load3_set);


}

void controller(uint8_t mode, double* Imains, double* Irenewable, uint8_t* status_mains, uint8_t* load1_req, uint8_t* load2_req, uint8_t* load3_req, uint8_t* load1_set, uint8_t* load2_set, uint8_t* load3_set, uint8_t* control){

double before_voltage = 0;
before_voltage = getV_amplitude();
double after_voltage = 0;
double leftoverI = 0;

if(before_voltage < THRESHOLD_V){
    if((*Imains)<THRESHOLD_I){
        *Imains+= 0.1;  //adjusts Imains so it meets a threshold
    }

    set_pwm(*Imains);
    _delay_ms(100);
    after_voltage = getV_amplitude();

    if(!(after_voltage > before_voltage)){ //mains not working
        *status_mains = 0;

        switch(mode) {

            case 1:  if(capacity > 10){   //deciding if we are using battery discharge or not
                            leftoverI = *Irenewable + BATTERYI;
                            if (battery_discharge == 0){
                                battery_discharge = 1;
                                battery_control(0,1);
                            }
                        } else {

                            leftoverI = *Irenewable;
                            if(battery_discharge == 1){
                                battery_discharge = 0;
                                battery_control(0,1);
                            }
                        }
                        *load1_set = 0;
                        *load2_set = 0;
                        *load3_set = 0;
                                                //turning loads on that we can power
                        if(*load1_req == 1){
                            if(leftoverI > ILOAD1){
                                *load1_set = 1;
                                leftoverI -= ILOAD1;
                            }
                            
                        } 
                        if(*load2_req == 1){
                            if(leftoverI > ILOAD2){
                                *load2_set = 1;
                                leftoverI -= ILOAD2;
                            }
                        }
                        if(*load3_req == 1){
                            if(leftoverI > ILOAD3){
                                *load3_set = 1;
                            }
                        }
                        load_control(load1_set, load2_set, load3_set);
                        break;   //loop 2 & 1
            
            
            case 2:     battery_charge = 0;
                        battery_control(1,0);
                        break;     //loop 3
       
       
        }   

    *Imains -= 0.1;
    set_pwm(*Imains);

    }

    else{

        *status_mains = 1;
        while(((getV_amplitude()) < THRESHOLD_V)){

            if((*Imains) > THRESHOLD_I){ //Making sure Imains is enough, otherwise exit loop
                break;
            }

            *Imains += 0.1;
            set_pwm(*Imains); //requesting Imains
        }




    }

}

*control = 1;

}

void draw_screen(int state)
{
    // Set orientation
    set_orientation(West);

    double v_test = 2.432;
    double i_test = 2.567;
    double Isolar = 0;

    
    double Iwind = ((adc(WINDI))/1023.0) * 5; //wind current
    //double Isolar = ((adc(PVI))/1023.0) * 5; //PV current
    PORTOUT = Iwind;
    // Clear the screen
    clear_screen();

    // Converting result to string so it can be printed
    char buffer[8];


    //Vbus
    display.x = XBORDER;
    display.y = ROW1;
    display_string("Busbar Voltage(V):");


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
    dtostrf(Iwind , 6, 2, buffer); // Convert double to string with 2 decimal places
    display_string(buffer);

    //PV
    display.x = XBORDER;
    display.y = ROW4;
    display_string("PV Current (A):");
    
    display.x = 120;
    dtostrf(Isolar, 6, 2, buffer); // Convert double to string with 2 decimal places
    display_string(buffer);

    display.x = XBORDER;
    display.y = ROW5;
    display_string("Time(s):");

    display.x = 120;
   // dtostrf(count , 6, 2, buffer);
   // display_string(buffer);
    

   if(battery_charge == 1)
   {
    display.x = XBORDER;
    display.y = ROW6;
    display_string("Battery Charging");

    display.x = 120; 
    status_bar(100, GREEN);
   }
   else if (battery_discharge == 1)
   {
    display.x = XBORDER;
    display.y = ROW6;
    display_string("Battery Discharging");

    display.x = 135; 
    status_bar(100, RED);
   }
   else
    display.x = XBORDER;
    display.y = ROW6;
    display_string("Battery Idle");

    display.x = 120; 
    status_bar(100, YELLOW);
}


void status_bar(uint16_t length, uint16_t colour) {

    // To make the bar fit the screen
    //uint16_t busbar_v_length = (getV_amplitude * 1000) / THRESHOLD_V ;
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

void inputs(double* Iwind, double* Isolar, double* v, double* i, uint8_t* load1_req, uint8_t* load2_req, uint8_t* load3_req) {

*v = getV_amplitude();
*i = getI_amplitude();
*Iwind = ((adc(WINDI))/1023.0) * 5; //wind current
*Isolar = ((adc(PVI))/1023.0) * 5; //PV current

*load1_req = get_digital(RLOAD1);
*load2_req = get_digital(RLOAD2);
*load3_req = get_digital(RLOAD3);


}
