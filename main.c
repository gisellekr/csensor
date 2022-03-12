/*
 ============================================================================
 Name        : main.c
 Author      : ykkim
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C
 ============================================================================
 */
/*=========================================
    INCLUDE
=========================================*/
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/select.h>
#include <string.h>
#if 0 //using message queue
#include <mqueue.h>
#endif
#include "uart.h"
#include "command.h"
struct Mesurement_Result mRes;

int g_fdUart=-1;
#define SENSOR_MONITORING_TIME 10
int num_choice = CMD_READ_MEASUREMENT;


unsigned char make_crc(char *data,int length)
{
	unsigned char crc=0x0;
	unsigned char sum=0x0;

	//printf("data length : %d\n",length);
	for(int i=0; i<length; i++) {
		sum = (unsigned char)sum+data[i];
		//printf("%02x ",sum);
	}
	crc = 256-sum;
	//printf("crc : %x\n",crc);

	return crc;
}

int parsingResOfSensor(char* data, int length)
{
	unsigned char check_crc=0;
	char ResultOfSensor[128]={0,};
	int uartd1,pm25,dim_mode;
    char send_cmd[10]={0,};
	struct json_object * pJsonObject = NULL;
    struct json_object * mode_object = NULL;

   	for(int i=0; i<length; i++) {
   		ResultOfSensor[i] = data[i];
   	}

   	check_crc = make_crc(ResultOfSensor, length-1);//except crc data

   	if(data[0]== 0x16)
	{
   		mRes.PM1 = ResultOfSensor[4]*0xFFFFFF + ResultOfSensor[5]*0xFFFF + ResultOfSensor[6]*0xFF + ResultOfSensor[7] ;
   		mRes.PM2_5 = ResultOfSensor[8]*0xFFFFFF + ResultOfSensor[9]*0xFFFF + ResultOfSensor[10]*0xFF + ResultOfSensor[11] ;
		mRes.PM10 = ResultOfSensor[12]*0xFFFFFF + ResultOfSensor[13]*0xFFFF + ResultOfSensor[14]*0xFF + ResultOfSensor[15] ;
		
		mRes.Reserve = ResultOfSensor[16] * 0xFF + ResultOfSensor[17];

		mRes.Reserve = (ResultOfSensor[18] * 256 + ResultOfSensor[19]);
   		mRes.Humidity = (mRes.Reserve)/10 ;
   		mRes.Reserve = (ResultOfSensor[20]*256 + ResultOfSensor[21] -500) ;
		mRes.Temperature = (mRes.Reserve)/10 ;
   	}

	pJsonObject = json_object_from_file("./hellow.json");
  	json_object_object_get_ex(pJsonObject,"mode",&mode_object);
    dim_mode = json_object_get_int(mode_object);

	if(dim_mode)
	{
		if( (mRes.PM2_5 <= 15))
			pm25= 8;
		else if((16 <= mRes.PM2_5) && (mRes.PM2_5 <= 35))
			pm25= 7;
		else if((36 <= mRes.PM2_5) && (mRes.PM2_5 <= 75))
			pm25= 6;
		else 
			pm25= 5;	
	}
	else
	{
		if( (mRes.PM2_5 <= 15))
			pm25= 4;
		else if((16 <= mRes.PM2_5) && (mRes.PM2_5 <= 35))
			pm25= 3;
		else if((36 <= mRes.PM2_5) && (mRes.PM2_5 <= 75))
			pm25= 2;
		else 
			pm25= 1;	
	}
		
   	uartd1 = uart_open("/dev/ttyUSB1", 9600);
	if (uartd1 <= 0) 
	{
		printf("Don't open /dev/ttyUSB1!!!\n");
	}
	send_cmd[0]='S';
	send_cmd[1]=pm25;
	send_cmd[2]='E';
	uart_write(uartd1, send_cmd, 3);

 	printf("dim_mode(%d): Humidity:	%3.2f Temperature:	%3.2f PM1.0: %d, PM2.5:	%d,	PM10:%d send_cmd:(%c,%d,%c)\n",dim_mode,mRes.Humidity, mRes.Temperature, mRes.PM1, mRes.PM2_5, mRes.PM10,send_cmd[0],send_cmd[1],send_cmd[2]);
	
	uart_close(uartd1);
	return 1;
}


void rx_thread(void *param)
{
#define reset_buffer() memset(rx,0,sizeof(char)*1024)
    char *rx = (char*)malloc(sizeof(char)*1024);
    int uartd = (int)param;
    int len=0;

    while(1)
    {
    	//printf("Waiting Rx data...\n");

        if(uart_readytoread(uartd,10000)>0)
        {
            len = 0;
            reset_buffer();
            len = uart_read(uartd, rx, 1024);

            if(len >0)
            	parsingResOfSensor(rx,len);
        }
        //sleep(1);
    }

    free(rx);
}

void tx_thread(void *param)
{

    int uartd = (int)param;
    char send_cmd[128]={0,};

    while(1)
    {
  
    	bzero(send_cmd,sizeof(send_cmd));

    	switch(num_choice) 
		{
			case CMD_READ_MEASUREMENT:
				send_cmd[0]=0x11;
				send_cmd[1]=0x02;
				send_cmd[2]=0x0B;
				send_cmd[3]=0x00;
				send_cmd[4] = make_crc(send_cmd,strlen(send_cmd));
				//send_cmd[4]=0xec;
				uart_write(uartd, send_cmd, 5); //strlen(send_cmd));
				
				break;

			case CMD_READ_IP:
				
				send_cmd[0]=0x11;
				send_cmd[1]=0x02;
				send_cmd[2]=0xAC;
				send_cmd[3]=0xFF;
				send_cmd[4] = make_crc(send_cmd,strlen(send_cmd));
				//send_cmd[4]=0x42;
				uart_write(uartd, send_cmd, strlen(send_cmd));
				
				break;

			default :
				break;

    	}
		sleep(SENSOR_MONITORING_TIME);

    }
}

int main(void)
{
	printf("Starting Sensor monitoring every %d Second \n",SENSOR_MONITORING_TIME);
	int uartd = 0;
	pthread_t rx_th;
	pthread_t tx_th;
	int rx_th_id;
	int tx_th_id;

	num_choice = CMD_READ_MEASUREMENT;
	//Sensordata_Data_Parsing();

	uartd = uart_open("/dev/ttyUSB0", 9600);
	if (uartd <= 0) 
	{
		//printf("No /dev/ttyUSB0!!!\n");
		return 0;
	}

	rx_th_id = pthread_create(&rx_th, NULL, (void*)rx_thread, (void*)uartd);
	tx_th_id = pthread_create(&tx_th, NULL, (void*)tx_thread, (void*)uartd);


	while(1)
	{
		sleep(1);
	}

	if(rx_th_id > 0)
	{
		int status;
		pthread_join(rx_th, (void **)&status);
		rx_th = 0;
		rx_th_id = 0;
	}

	if(tx_th_id > 0)
	{
		int status;
		pthread_join(tx_th, (void **)&status);
		tx_th = 0;
		tx_th_id = 0;
	}

	uart_close(uartd);


  return 0;
}
