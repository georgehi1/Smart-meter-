//#include "pwm.h"

#include <stdlib.h>
#include <math.h>
#include <stdio.h>

#define STEP 0.1
#define pi 3.141592654
double i = 0;
double sine;
int count = 0;
double vamp;

double getV_amplitude() { //busbar voltage


    double bus_sample = 0;
    double bus_amplitude = 0;
    int peak_end_t = count + 60; //val = how long it will take to measure the peak
    while (count < peak_end_t){
        //bus_sample = abs(adc(PINV)-511); //(1024/2)-1 = 511 which is midpoint
            sine=sin(i);

        bus_sample = sine;

        if(bus_sample > bus_amplitude){
            
            bus_amplitude = bus_sample;

        }
        printf("sin(%f) = %f   Vamp = %f\n", i, sine, bus_amplitude);
        count += 1;
        i += STEP;


    }
        return ((bus_amplitude/512.0)/sqrt(2));
}
int main(){
    for(i=0; i<pi; i+=STEP){
    sine=sin(i);
    vamp = getV_amplitude();
    }

    //vamp = getV_amplitude();

    printf("Vamp = %f\n", vamp);

    return 0;
}



