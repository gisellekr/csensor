/*
 * command.h
 *
 *  Created on: 2021. 5. 6.
 *      Author: dragonball
 */

#ifndef SRC_COMMAND_H_
#define SRC_COMMAND_H_

#define START_SYMBOL		0x11

#define BUFFER_SIZE 80

#define SENSORDATA_FILE "sdata.log"
#define SENSORDATA_PATH "./"

//Command
#define CMD_READ_MEASUREMENT 	0x01
#define CMD_CAL_CO2 			0x03
#define CMD_SETUP_READ			0x07
#define CMD_SET_IP				0xAB
#define CMD_READ_IP				0xAC
#define CMD_SW_VERSION			0x1E
#define CMD_PM_SW_VERSION		0x2E
#define CMD_CO2_SW_VERSION		0x3E
#define CMD_SERIAL_NUM			0x1F
#define CMD_RESET				0x55

#define RES_READ_MEASUREMENT	0x19

struct Mesurement_Result {
	unsigned int CO2;
	unsigned int VOC;
	float Humidity;
	float Temperature;
	unsigned int PM1;
	unsigned int PM2_5;
	unsigned int PM10;
	unsigned int VOCNowRef;
	unsigned int VOCRefRValue;
	unsigned int VOCNowRValue;
	unsigned int Reserve;
	char State;
	char crc;
};

#endif /* SRC_COMMAND_H_ */
