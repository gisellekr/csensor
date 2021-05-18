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
int g_run = 1;

#if 0 //using message queue
char g_sensor_mqueue_buffer[MESSAGE_QUEUE_BUFFER_SIZE];
pthread_mutex_t g_sensor_mutex;
mqd_t g_sensor_sender; //uart rx result -> parser
mqd_t g_sensor_receiver;

int sendResOfSensor(char* data, int length)
{
	bzero(g_sensor_mqueue_buffer, MESSAGE_QUEUE_BUFFER_SIZE);
	strncpy(g_sensor_mqueue_buffer, data, length);

	if (mq_send(g_sensor_sender, g_sensor_mqueue_buffer, length /*strlen(g_sensor_mqueue_buffer)*/, 0) != -1) {
	  printf("sensor data transfer is done: %s\n", g_sensor_mqueue_buffer);
	} else {
	  perror("Failed to send message to parser task");
	}

    if(length >0)
    {
    	printf("rx(%i) : ",length);
    	for(int i=0; i<length; i++)
         printf("%02x ",g_sensor_mqueue_buffer[i]);
    	printf("\n");
    }
}

void parser_thread(void *param)
{
#define reset_buffer() memset(rx,0,sizeof(char)*1024)
    char *rx = (char*)malloc(sizeof(char)*1024);
    int uartd = (int)param;
    int len=0;
    int msg_size;

    while(g_run)
    {
    	reset_buffer();
    	bzero(g_sensor_mqueue_buffer, MESSAGE_QUEUE_BUFFER_SIZE);

    	if ((msg_size = mq_receive(g_sensor_receiver, g_sensor_mqueue_buffer, MAX_MESSAGE_QUEUE_SIZE, NULL)) == -1) {
    	    printf("Error on receiving a queue message\n");
    	    sleep(1);
    	    continue; //break;
    	}

    	//printf("parser mqueue: size = %d, message = %s\n", msg_size, g_sensor_mqueue_buffer);
        if(msg_size >0)
        {
        	printf("parser rx(%i) : ",msg_size);
        	for(int i=0; i<len; i++)
             printf("%02x ",g_sensor_mqueue_buffer[i]);
        	printf("\n");
        }

    	//STOP message queue
    	if (msg_size == strlen(QUEUE_TERMINATION_MSG) && strcmp(g_sensor_mqueue_buffer, g_sensor_mqueue_buffer) == 0) {
    	     printf("Received termination message\n");
    	     break;
    	}


    	sprintf(rx,"%s",g_sensor_mqueue_buffer);
    	printf("%s\n",rx);
    }

    free(rx);
}
#endif

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
	struct Mesurement_Result mRes;
	unsigned char check_crc=0;
	char ResultOfSensor[128]={0,};

   	printf("parser(%i) : ",length);
   	for(int i=0; i<length; i++) {
   		ResultOfSensor[i] = data[i];
   		printf("%02x ",ResultOfSensor[i]);
   	}
   	printf("\n");

   	check_crc = make_crc(ResultOfSensor, length-1);//except crc data

   	//RES_READ_MEASUREMENT 16 '19' 01
   	if(data[1]== 0x19) {
   		unsigned int temprature=0;
   		printf("Read Measuremnt Result\n");
   	   	if(check_crc != ResultOfSensor[27]) {
   	   		printf("CRC failed\n");
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

   		printf("\n \
	CO2: 		%d \n \
	VOC:		%d \n \
	Humidity:	%3.1f \n \
	Temperature:	%3.1f \n \
	PM1.0:		%d \n \
	PM2.5:		%d \n \
	PM10:		%d \n \
	VOC Now/REF:	%d \n \
	VOC REF.R:	%d \n \
	VOC Now.R:	%d \n \
	State:		0x%x \n",mRes.CO2, mRes.VOC, mRes.Humidity, mRes.Temperature, mRes.PM1, mRes.PM2_5, mRes.PM10, mRes.VOCNowRef, mRes.VOCRefRValue, mRes.VOCNowRValue, mRes.State);

   	}

}


void rx_thread(void *param)
{
#define reset_buffer() memset(rx,0,sizeof(char)*1024)
    char *rx = (char*)malloc(sizeof(char)*1024);
    int uartd = (int)param;
    int len=0;

    while(g_run)
    {
    	printf("Waiting Rx data...\n");

        if(uart_readytoread(uartd,10000)>0)
        {
            len = 0;
            reset_buffer();
            len = uart_read(uartd, rx, 1024);
            /*
            if(len >0)
            {
            	printf("rx(%i) : ",len);
            	for(int i=0; i<len; i++)
                 printf("%02x ",rx[i]);
            	printf("\n");
            }
            */

#if 0 //using message queue
            sendResOfSensor(rx,len);
#endif
            if(len >0)
            	parsingResOfSensor(rx,len);
        }
        //sleep(1);
    }

    free(rx);
}

void tx_thread(void *param)
{
//#define reset_buffer() memset(tx,0,sizeof(char)*1024)
   // char *tx = (char*)malloc(sizeof(char)*1024);
    int uartd = (int)param;
    char choice;
    int res=-1;
    char send_cmd[128]={0,};

    while(g_run)
    {
    	choice = '\0';
    	bzero(send_cmd,sizeof(send_cmd));
    	printf("0.Read Measurement \n");
    	printf("1.Read PM Sensor software version \n");
    	printf("2.Read controller IP \n");
    	printf("9.Exit Program \n");
    	scanf("%c",&choice);
    	//printf(" %c\n",choice);
    	switch(choice) {
    	case '0':
    		printf("Read Measurement\n");
    		send_cmd[0]=0x11;
    		send_cmd[1]=0x02;
    		send_cmd[2]=0x01;
    		send_cmd[3]=0x00;
    		send_cmd[4] = make_crc(send_cmd,strlen(send_cmd));
    		//send_cmd[4]=0xec;
    		res = uart_write(uartd, send_cmd, 5); //strlen(send_cmd));
    		printf("writing count: %d\n",res);
			break;
    	case '1':
    		printf("Read PM Sensor software version number\n");
    		send_cmd[0]=0x11;
    		send_cmd[1]=0x02;
    		send_cmd[2]=0x2e;
    		send_cmd[3]=0x00;
    		send_cmd[4] = make_crc(send_cmd,strlen(send_cmd));
    		//send_cmd[4]=0xbf;
    		res = uart_write(uartd, send_cmd, 5); //strlen(send_cmd));
    		printf("writing count: %d\n",res);
			break;
    	case '2':
    		printf("Read controller IP\n");
    		send_cmd[0]=0x11;
    		send_cmd[1]=0x02;
    		send_cmd[2]=0xAC;
    		send_cmd[3]=0xFF;
    		send_cmd[4] = make_crc(send_cmd,strlen(send_cmd));
    		//send_cmd[4]=0x42;
    		res = uart_write(uartd, send_cmd, strlen(send_cmd));
    		printf("writing count: %d\n",res);
			break;
    	case '9':
    		printf("terminate program\n");
    		g_run = 0;
			break;
    	default :
    		break;

    	}
    }
}

int main(void)
{
  printf("Hello Arm World!" "\n");
  int uartd = 0;
  pthread_t rx_th;
  pthread_t tx_th;
  int rx_th_id;
  int tx_th_id;
  char send_cmd[128]={0,};
#if 0 //using message queue
  int parser_th_id;
  struct mq_attr attr;
#endif

  uartd = uart_open("/dev/ttyUSB0", 9600);
  if (uartd <= 0) {
	  printf("No /dev/ttyUSB0!!!\n");
      return 0;
  }

#if 0 //using message queue
  attr.mq_flags = 0;
  attr.mq_maxmsg = MAX_MESSAGES;
  attr.mq_msgsize = MAX_MESSAGE_QUEUE_SIZE;
  attr.mq_curmsgs = 0;

  if (pthread_mutex_init(&g_sensor_mutex, NULL) != 0) {
	  perror("Mutex init failed!\n");
      return -1;
  }

  if ((g_sensor_receiver = mq_open(SENSOR_QUEUE_NAME, O_CREAT | O_RDONLY, QUEUE_PERMISSIONS, &attr)) == -1) {
      perror("g_sensor_receiver: mq_open()");
      pthread_mutex_destroy(&g_sensor_mutex);

      return -1;
  }

  if ((g_sensor_sender = mq_open(SENSOR_QUEUE_NAME, O_WRONLY)) == -1) {
      perror("g_sensor_sender: mq_open()");
      mq_close(g_sensor_receiver);
      pthread_mutex_destroy(&g_sensor_mutex);

      return -1;
  }
#endif

  rx_th_id = pthread_create(&rx_th, NULL, (void*)rx_thread, (void*)uartd);
  tx_th_id = pthread_create(&tx_th, NULL, (void*)tx_thread, (void*)uartd);
#if 0 //using message queue
  parser_th_id = pthread_create(&tx_th, NULL, (void*)parser_thread, NULL);
#endif

  //sleep(10);
  //printf("tx : AT\n");


  while(g_run)
	  sleep(1);

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
#if 0 //using message queue
  mq_close(g_sensor_receiver);
  mq_close(g_sensor_sender);
  pthread_mutex_destroy(&g_sensor_mutex);
#endif

  return 0;
}
