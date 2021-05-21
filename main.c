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

int g_fdUart=-1;
// EVERY 1HOUR
#define SENSOR_MONITORING_TIME 60
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
	struct Mesurement_Result mRes, mWes;
	unsigned char check_crc=0;
	char ResultOfSensor[128]={0,};
	int sd_fb, re;
	char filename[BUFFER_SIZE];
	
	sprintf(filename,"%s%s",SENSORDATA_PATH,SENSORDATA_FILE);
   	//printf("parser(%i) : ",length);
   	for(int i=0; i<length; i++) {
   		ResultOfSensor[i] = data[i];
   		//printf("%02x ",ResultOfSensor[i]);
   	}
//   	printf("\n");

   	check_crc = make_crc(ResultOfSensor, length-1);//except crc data

   	//RES_READ_MEASUREMENT 16 '19' 01
   	if(data[1]== 0x19)
	{
   		//printf("Read Measuremnt Result\n");
   	   	if(check_crc != ResultOfSensor[27])
		{
   	   	//	printf("CRC failed\n");
   	   		return -1;
   	   	}

   		mRes.CO2 = ResultOfSensor[4];
   		mRes.CO2 = mRes.CO2<<8;
   		mRes.CO2 = mRes.CO2 + ResultOfSensor[5];

   		mRes.VOC = ResultOfSensor[6];
   		mRes.VOC = mRes.VOC<<8;
   		mRes.VOC = mRes.VOC + ResultOfSensor[7];

   		mRes.Humidity = ResultOfSensor[8];
   		mRes.Humidity = mRes.Humidity * 256; //<<8
   		mRes.Humidity = mRes.Humidity + ResultOfSensor[9];
   		mRes.Humidity = mRes.Humidity/10;

   		mRes.Temperature = ResultOfSensor[10];
   		mRes.Temperature = mRes.Temperature*256; //<<8
   		mRes.Temperature = mRes.Temperature + ResultOfSensor[11];
   		mRes.Temperature = (mRes.Temperature - 500)/10;

   		mRes.PM1 = ResultOfSensor[12];
   		mRes.PM1 = mRes.PM1<<8;
   		mRes.PM1 = mRes.PM1 + ResultOfSensor[13];

   		mRes.PM2_5 = ResultOfSensor[14];
   		mRes.PM2_5 = mRes.PM2_5<<8;
   		mRes.PM2_5 = mRes.PM2_5 + ResultOfSensor[15];

   		mRes.PM10 = ResultOfSensor[16];
   		mRes.PM10 = mRes.PM10<<8;
   			mRes.PM10 = mRes.PM10 + ResultOfSensor[17];

   		mRes.VOCNowRef = ResultOfSensor[18];
   		mRes.VOCNowRef = mRes.VOCNowRef<<8;
   		mRes.VOCNowRef = mRes.VOCNowRef + ResultOfSensor[19];

   		mRes.VOCRefRValue = ResultOfSensor[20];
   		mRes.VOCRefRValue = mRes.VOCRefRValue<<8;
   		mRes.VOCRefRValue = mRes.VOCRefRValue + ResultOfSensor[21];

   		mRes.VOCNowRValue = ResultOfSensor[22];
   		mRes.VOCNowRValue = mRes.VOCNowRValue<<8;
   		mRes.VOCNowRValue = mRes.VOCNowRValue + ResultOfSensor[23];

   		mRes.Reserve = ResultOfSensor[24];
   		mRes.Reserve = mRes.Reserve<<8;
   		mRes.Reserve = mRes.Reserve + ResultOfSensor[25];

   		mRes.State = ResultOfSensor[26];

   		mRes.crc = ResultOfSensor[27];


		sd_fb = open(filename,O_WRONLY|O_CREAT|O_TRUNC ,0644);

		if (sd_fb == -1)
		{
 			perror("failed open file.");
        	return sd_fb;
		}
		
		re = write(sd_fb, &mRes, sizeof(mRes));
		if(re == -1)
		{
			perror("failed write ");
			return re;
		}

		printf("write %d bytes\n",re);
		close(sd_fb);	
   	}

    sd_fb = open(filename,O_RDONLY);//읽기 전용
    if(sd_fb == -1)
    {
        perror("failed open dummy file.");
        return sd_fb;
    }
   
    re = read(sd_fb, &mWes, sizeof(mWes));
	if(re>0)
    {
   		printf("read: CO2: %d ,VOC:%d ,Humidity:	%3.2f Temperature:	%3.2f PM1.0: %d, PM2.5:	%d,	PM10:%d,VOC Now/REF:%d,	VOC REF.R:%d, VOC Now.R:%d State:0x%x \n",
			mWes.CO2, mWes.VOC, mWes.Humidity, mWes.Temperature, mWes.PM1, mWes.PM2_5, mWes.PM10, mWes.VOCNowRef, mWes.VOCRefRValue, mWes.VOCNowRValue, mWes.State);
    }
	printf("read %d bytes\n",re);

	close(sd_fb);
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
				send_cmd[2]=0x01;
				send_cmd[3]=0x00;
				send_cmd[4] = make_crc(send_cmd,strlen(send_cmd));
				//send_cmd[4]=0xec;
				uart_write(uartd, send_cmd, 5); //strlen(send_cmd));
				
				break;
			case CMD_PM_SW_VERSION:
				
				send_cmd[0]=0x11;
				send_cmd[1]=0x02;
				send_cmd[2]=0x2e;
				send_cmd[3]=0x00;
				send_cmd[4] = make_crc(send_cmd,strlen(send_cmd));
				//send_cmd[4]=0xbf;
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
