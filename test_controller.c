#include <stdlib.h>
#include <math.h>
#include <stdio.h>

int battery_charge = 0;
int capacity = 0;
int battery_discharge = 0;
int control = 0;
double Irenewable = 0;
int status = 0;
int load1r = 0;
int load2r = 0;
int load3r = 0;
int load1s = 0;
int load2s = 0;
int load3s = 0;
double Imains = 0;
int mode = 0;
double before_voltage = 0;
double after_voltage = 0;

void controller(int mode, double* Imains, double* Irenewable, int* status_mains, int* load1_req, int* load2_req, int* load3_req, int* load1_set, int* load2_set, int* load3_set, int* control){
printf("enter function\n");
//double before_voltage = 0;
//before_voltage = 200;
//double after_voltage = 0;
double leftoverI = 0;
printf("Imains: %f", *Imains);

if(before_voltage < 240){
    if((*Imains)<2.9){
        (*Imains) = (*Imains) + 0.1;;  //adjusts Imains so it meets a threshold
    }
    printf("PWM Out: %f", *Imains);
    //set_pwm(*Imains);
    //_delay_ms(100);
    //after_voltage = 190;

    if(after_voltage < before_voltage){ //mains not working
        *status_mains = 0;

        switch(mode) {

            case 1:  if(battery_charge == 1){  //no break so it can go to case 2
                            battery_charge = 0; 
                            printf("\nentered loop 1\n");
                            //battery_control(1,0);
                        } //loop 1

            case 2:     printf("\nentering loop 2\n");
                        if(capacity > 10){  //deciding if we are using battery discharge or not
                            printf("enough capacity, discharge battery\n");
                            leftoverI = *Irenewable + 1;
                            printf("I: %lf\n", leftoverI);
                            if (battery_discharge == 0){
                                battery_discharge = 1;
                                //battery_control(0,1);
                            }

                        } else {
                            printf("not enough capacity, don't discharge\n");
                            leftoverI = (*Irenewable);
                            if(battery_discharge == 1){
                                battery_discharge = 0;
                                //battery_control(0,1);

                            }
                        }
                        *load1_set = 0;
                        *load2_set = 0;
                        *load3_set = 0;
                                                //turning loads on that we can power
                        if(*load1_req == 1){
                            if(leftoverI > 1.2){
                                *load1_set = 1;
                                leftoverI -= 1.2;

                                printf("load 1 on, left over I: %lf\n", leftoverI);
                            }
                            
                        } 
                        if(*load2_req == 1){
                            if(leftoverI > 2.0){
                                *load2_set = 1;
                                leftoverI -= 2.0;
                                printf("load 2 on, left over I: %lf\n", leftoverI);
                            }
                        }
                        if(*load3_req == 1){
                            if(leftoverI > 0.8){
                                *load3_set = 1;
                                printf("load 3 on\n");
                            }
                        }
                        //load_control(load1_set, load2_set, load3_set);

                        printf("load status(1,2,3): %d, %d, %d\n", *load1_set, *load2_set, *load3_set);
                        break;   //loop 2 & 1
            
            
            case 3:     battery_charge = 0;
                        //battery_control(1,0);
                        printf("\n entered loop 3\n");
                        break;     //loop 3
       
       
        }   

    *Imains -= 0.1;
    //set_pwm(*Imains);
    printf("PWM Out: %lf", *Imains);

    }

    else{

        *status_mains = 1;
        /* while((() < 240)){

            if((*Imains) > 2.9){ //Making sure Imains is enough, otherwise exit loop
                break;
            }

            *Imains += 0.1;
            set_pwm(*Imains); //requesting Imains
        }

        
        */

    }

}

*control = 1;

}

int main(){
    //scanf("control: %d\n", &control);
    printf("Irenewable: \n");
    scanf("%d", &mode);
    printf("battery_charge: \n");
    scanf("%d", &battery_charge);
    printf("battery_discharge: \n");
    scanf("%d", &battery_discharge);
    printf("Imains: \n");
    scanf("%lf", &Imains);
    printf("capacity: \n");
    scanf("%d", &capacity);
    printf("Irenewable: \n");
    scanf("%lf", &Irenewable);
    printf("mains status: \n");
    scanf("%d", &status); 
    printf("load1 request: \n");
    scanf("%d", &load1r);
    printf("load2 request: \n");
    scanf("%d", &load2r);
    printf("load3 request: \n");
    scanf("%d", &load3r);
    printf("load3 set: \n");
    scanf("%d", &load1s);
    printf("load2 set: \n");
    scanf("%d", &load2s);
    printf("load3 set: \n");
    scanf("%d", &load3s);  
    printf("Before voltage: \n");
    scanf("%lf", &before_voltage);  
    printf("After voltage: \n");
    scanf("%lf", &after_voltage);  
    controller(mode, &Imains, &Irenewable, &status, &load1r, &load2r, &load3r, &load1s, &load2s, &load3s, &control);

    return 0;
}