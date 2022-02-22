/*
 * File:   ProjectTask1HKN.c
 * Author: Huy Khoi Nguyen 500954699
 * Created on November 29, 2021, 1:34 AM
 */
/*Last digit = 9 
 * -> I2C1 Module
 * BaudRate = 19200 
 * Fscl = 400kHz
 * I2C1BRG  = (((1/(2*Fscl))-TPGD)*FCY) - 2 = 16
 * Typical TPGD is 104ns = 1.04e-7
 */  

#include "xc.h"
#include "i2c.h"
#define USE_AND_OR //enable AND OR mask setting
#include <p24FJ128GA010.h>  
#include <uart.h>
#include <string.h>
#include <stdio.h>

#pragma config POSCMOD = NONE // Primary Oscillator Select -> disabled
#pragma config FNOSC = FRCPLL /*Oscillator select -> fast RC Oscillator
with PLL module, FOSC = 32MHz -> FCY = 16MHz*/
#pragma config FCKSM = CSDCMD 
#pragma config FWDTEN = OFF
#pragma config JTAGEN = OFF
#define _SUPPRESS_PLIB_WARNING

//2-wire serial temperature sensor
#define SLAVE1_I2C_ADDRESS 0x90 
#define SLAVE2_I2C_ADDRESS 0x92 

INT16 temperature; // hold 16-bit binary value
INT16 read_temperature(UINT16 SLAVE_I2C_ADDRESS); //function prototype. returns 16-bit temperature readings 
UINT8 buf[1024];

//Send data prototype
void SendDataBuffer(const char *buffer, UINT16 size);

int main(void) {
    INT16 Temp_C; //hold 16-bit temperature value
    float f_Temp_C; // hold values containing decimal places
    INT16 Temp_C2; //hold 16-bit temperature value
    float f_Temp_C2;
    OpenI2C1(I2C_ON, 16); //Enable I2C1 with BRG = 16
    
    //write here Timer and PORTs configuration and initialization
    T2CON = 0x8030; //TMR2 on, prescale 1:256
    PR2 = 6249; // set period register for delay = 100msec
    
    //Write here UART configuration
    CloseUART1();
    OpenUART1(UART_EN| UART_IDLE_CON | UART_NO_PAR_8BIT | UART_BRGH_SIXTEEN | UART_1STOPBIT, UART_TX_ENABLE, 51);
    // 51 = (Fcy/(Baud rate * 16)) - 1
    
    sprintf(buf, "\r\nHuy Khoi Nguyen 500954699\r\n\r\n");
    SendDataBuffer(buf, strlen(buf));
    
    while(1){
        Temp_C = read_temperature(SLAVE1_I2C_ADDRESS); //read the sensor
        Temp_C2 = read_temperature(SLAVE2_I2C_ADDRESS);
        while(TMR2);
        f_Temp_C = (float)(Temp_C)/256;
        f_Temp_C2 = (float)(Temp_C2)/256;
        //f_Temp_C =(float)(((Temp_C & 0xF0)>>4)*16+(Temp_C & 0x0F))*0.5;
        //if (temperature << 1 & 0x0100) //check if it is in 2's complement (for negative values)
        //f_Temp_C = -1 *(128 - f_Temp_C);
        if ( (f_Temp_C >= 15 && f_Temp_C <= 30) || (f_Temp_C <= -15 && f_Temp_C >= -30)){
            sprintf(buf, "\r\nRoom 1 Current Temperature is: %3.4f Celsius\r\n\r\n", f_Temp_C);
            SendDataBuffer(buf, strlen(buf));
        }
        else{
            sprintf(buf, "\r\nRoom 1 Temperature out of range.\r\n\r\n");
            SendDataBuffer(buf, strlen(buf));
        }
        if ( (f_Temp_C2 >= 15 && f_Temp_C2 <= 30) || (f_Temp_C2 <= -15 && f_Temp_C2 >= -30)){
            sprintf(buf, "\r\nRoom 2 Current Temperature is: %3.4f Celsius\r\n\r\n", f_Temp_C2);
            SendDataBuffer(buf, strlen(buf));
        }
        else{
            sprintf(buf, "\r\nRoom 2 Temperature out of range.\r\n\r\n");
            SendDataBuffer(buf, strlen(buf));
        }
        while(TMR2);
    }
CloseUART1(); //disable UART1
return 0;
} 

INT16 read_temperature(UINT16 SLAVE_I2C_ADDRESS){
    //Write to configuration Register
    //Send start
    IdleI2C1(); //wait for I2C bus to be ready
    StartI2C1(); //generate a start signal
    while(I2C1CONbits.SEN){}; //wait transmitting start signal in progress
    //write slave address and with R/W bit 0 for write
    IdleI2C1();
    MasterWriteI2C1(SLAVE_I2C_ADDRESS); //writing to slave address 90
    while(I2C1STATbits.TRSTAT){}; //wait while master transmitting in progress
    if(I2C1STATbits.ACKSTAT == 1) //if ACK from the slave is not received
        goto read_temperature_fail;
    
    //To access configuration register
    IdleI2C1();
    MasterWriteI2C1(0xAC); //Writing 0xAC to pointer register
    while(I2C1STATbits.TRSTAT){};
    if(I2C1STATbits.ACKSTAT == 1) 
        goto read_temperature_fail;
    
    IdleI2C1();
    //02 = 0.5, 06 = 0.25, 0A = 0.125, 0E = 0.0625
    MasterWriteI2C1(0x06); //Resolution of 0.5C. int mode, active high 
    while(I2C1STATbits.TRSTAT){};
    if(I2C1STATbits.ACKSTAT == 1) 
        goto read_temperature_fail;
    
    //Send Stop
    IdleI2C1();
    StopI2C1();
    while(I2C1CONbits.PEN){}; // wait while transmitting stop in progress
   
    //Send start
    IdleI2C1();
    StartI2C1();
    while(I2C1CONbits.SEN){};
    
    //write slave address and with R/W bit 0 for write
    IdleI2C1();
    MasterWriteI2C1(SLAVE_I2C_ADDRESS); //writing to slave address 90
    while(I2C1STATbits.TRSTAT){}; //wait while master transmitting in progress
    if(I2C1STATbits.ACKSTAT == 1) //if ACK from the slave is not received
        goto read_temperature_fail;
    
    //write pointer to Temperature register
    IdleI2C1();
    MasterWriteI2C1(0x51);
    while(I2C1STATbits.TRSTAT){};
    if(I2C1STATbits.ACKSTAT == 1)
        goto read_temperature_fail;
    // send stop
    IdleI2C1();
    StopI2C1();
    while(I2C1CONbits.PEN) {}; 
    
    //send start
    IdleI2C1();
    StartI2C1();
    while(I2C1CONbits.SEN){}; //wait while transmitting Restart in progress
    
    //Write Slave address and with RW bit 0 for write
    IdleI2C1();
    MasterWriteI2C1(SLAVE_I2C_ADDRESS); //Writing to slave address 90
    while(I2C1STATbits.TRSTAT){}; //Wait while Master transmitting in progress
    if(I2C1STATbits.ACKSTAT ==1) //If ACK from the slave is not received
      goto read_temperature_fail;
    
    //write pointer to upper alarm trip point register
    IdleI2C1();
    MasterWriteI2C1(0xA1); //Writing 0x01 to pointer register
    while (I2C1STATbits.TRSTAT) {};
    if (I2C1STATbits.ACKSTAT == 1)
      goto read_temperature_fail;
    
    IdleI2C1();
    MasterWriteI2C1(0x1E); //maximum temp is 30
    while (I2C1STATbits.TRSTAT) {};
    if (I2C1STATbits.ACKSTAT == 1)
      goto read_temperature_fail;
    
    IdleI2C1();
    MasterWriteI2C1(0x00); // Resolution of 0. 5°C , int mode, active high
    while (I2C1STATbits.TRSTAT) {};
    if (I2C1STATbits.ACKSTAT == 1)
      goto read_temperature_fail;
    
    // send stop
    IdleI2C1();
    StopI2C1();
    while(I2C1CONbits.PEN) {}; 
    
    //send start
    IdleI2C1();
    StartI2C1();
    while(I2C1CONbits.SEN){}; //wait while transmitting Restart in progress
     
    //Write Slave address and with RW bit 0 for write
    IdleI2C1();
    MasterWriteI2C1(SLAVE_I2C_ADDRESS); //Writing to slave address 90
    while(I2C1STATbits.TRSTAT){}; //Wait while Master transmitting in progress
    if(I2C1STATbits.ACKSTAT ==1) //If ACK from the slave is not received
      goto read_temperature_fail;
    
    //write pointer to lower alarm trip point register
    IdleI2C1();
    MasterWriteI2C1(0xA2); //Writing 0x01 to pointer register
    while (I2C1STATbits.TRSTAT) {};
    if (I2C1STATbits.ACKSTAT == 1)
      goto read_temperature_fail;
    
    IdleI2C1();
    MasterWriteI2C1(0x0F); //minimum temp is 15
    while (I2C1STATbits.TRSTAT) {};
    if (I2C1STATbits.ACKSTAT == 1)
      goto read_temperature_fail;
    
    IdleI2C1();
    MasterWriteI2C1(0x00); // Resolution of 0. 5°C , int mode, active high
    while (I2C1STATbits.TRSTAT) {};
    if (I2C1STATbits.ACKSTAT == 1)
      goto read_temperature_fail;
    
    // send stop
    IdleI2C1();
    StopI2C1();
    while(I2C1CONbits.PEN) {}; 
    
    //send start
    IdleI2C1();
    StartI2C1();
    while(I2C1CONbits.SEN){}; //wait while transmitting Restart in progress
    
    IdleI2C1();
    MasterWriteI2C1(SLAVE_I2C_ADDRESS); //Writing to slave address 90
    while(I2C1STATbits.TRSTAT){}; //Wait while Master transmitting in progress
    if(I2C1STATbits.ACKSTAT ==1) //If ACK from the slave is not received
      goto read_temperature_fail;
    
    //Write pointer to temperature register
    IdleI2C1();
    MasterWriteI2C1(0xAA); //AA is the address of temperature
    while(I2C1STATbits.TRSTAT) {};
    if(I2C1STATbits.ACKSTAT == 1)
      goto read_temperature_fail;
    
    //Send Restart
    IdleI2C1();
    RestartI2C1();
    while(I2C1CONbits.RSEN){};
    
    //Write Slave address and with RW bit 1 for read
    IdleI2C1();
    MasterWriteI2C1(SLAVE_I2C_ADDRESS | 0x01); //address=90+1 for write
    while(I2C1STATbits.TRSTAT) {};
    if(I2C1STATbits.ACKSTAT == 1)
      goto read_temperature_fail;
    
    //Get byte 0 (Most Significant Byte)
    IdleI2C1();
    temperature = MasterReadI2C1();  //Read the high byte  of the Temperature register

    //Send Ack
    IdleI2C1();
    AckI2C1();
    while(I2C1CONbits.ACKEN) {};  //Master transmit ACK
    //Get byte 1 (Least significant Byte)
    IdleI2C1();
    temperature = (temperature << 8) | MasterReadI2C1();
    //Send NacK
    IdleI2C1();
    NotAckI2C1();
    while(I2C1CONbits.ACKEN) {}; 
    //Send Stop
    IdleI2C1();
    StopI2C1();
    while(I2C1CONbits.PEN) {}; //Wait while transmitting Stop in progress
    return(temperature);

    read_temperature_fail:
        //Send Stop
        IdleI2C1();
        StopI2C1();
        while(I2C1CONbits.PEN) {};
        temperature = 0;
        return(0);
}

void SendDataBuffer(const char *buffer, UINT16 size){
    while(size){
        while(BusyUART1());
        WriteUART1(*buffer);
        buffer++;
        size--;
    }
}

