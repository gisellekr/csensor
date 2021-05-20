#ifndef __uart_h__
#define __uart_h__

#ifdef __cplusplus
extern "C"{
#endif
        int uart_open(char *dev, int baud_rate);
        void uart_close(int uartd);
        int uart_readytoread(int uartd,int timeoutms);
        int uart_waitnread(int uartd, char *data, int size,int timeoutms);
        int uart_read(int uartd, char *data, int size);
        int uart_write(int uartd, char *data, int size);
#ifdef __cplusplus
}
#endif
#if 0 //using message queue
#define SENSOR_QUEUE_NAME   					"/sensor-mqueue"
#define QUEUE_TERMINATION_MSG   				"AllTaskSTOP"

#define QUEUE_PERMISSIONS 						0660
#define MAX_MESSAGES 							10
#define MAX_MESSAGE_QUEUE_SIZE 					1024
#define MESSAGE_QUEUE_BUFFER_SIZE 				MAX_MESSAGE_QUEUE_SIZE + 10
#endif //0 using message queue

#endif
