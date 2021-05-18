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
#include "uart.h"

/*=========================================
 LOCAL DEFINE
>=========================================*/
typedef struct _uart_ctx_t
{
    pthread_mutex_t mutex;
    int fd;
    struct termios oldtio,newtio;
}uart_ctx_t;


/*=========================================
 LOCAL FUNCTIONS
=========================================*/
int open_fd(char *dev)
{
    int fd = 0;
    if (!dev) return -1;

    fd = open( dev, O_RDWR | O_NOCTTY | O_NONBLOCK );
    if (fd < 0)
    {
        return -1;
    }
    return fd;
}

int uart_config(uart_ctx_t *ctx, int baud_rate)
{
    if (!ctx || ctx->fd <= 0) return -1;
    tcgetattr(ctx->fd,&ctx->oldtio); // save current port setting.
    //set new port settings
    switch(baud_rate)
    {
        case 9600:
            ctx->newtio.c_cflag = B9600;
            break;
        case 19200:
            ctx->newtio.c_cflag = B19200;
            break;
        case 38400:
            ctx->newtio.c_cflag = B38400;
            break;
        case 57600:
            ctx->newtio.c_cflag = B57600;
            break;
        case 115200:
        default:
            ctx->newtio.c_cflag = B115200;
            break;
    }
    /* 8N1 Mode */
    ctx->newtio.c_cflag &= ~PARENB;  /* Disables the Parity Enable bit(PARENB),So No Parity */
    ctx->newtio.c_cflag &= ~CSTOPB;  /* CSTOPB = 2 Stop bits,here it is cleared so 1 Stop bit */
    ctx->newtio.c_cflag &= ~CSIZE;   /* Clears the mask for setting the data size */
    ctx->newtio.c_cflag |= CS8;  /* Set the data bits = 8 */
    ctx->newtio.c_cflag &= ~CRTSCTS;  /* No Hardware flow Control */
    ctx->newtio.c_cflag |= CREAD | CLOCAL;  /* Enable receiver,Ignore Modem Control lines */
    ctx->newtio.c_iflag &= ~(IXON | IXOFF | IXANY);  /* Disable XON/XOFF flow control both i/p and o/p */
    ctx->newtio.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);  /* Non Cannonical mode */
    ctx->newtio.c_oflag &= ~OPOST;/*No Output Processing*/

    /* Setting Time outs */
    ctx->newtio.c_cc[VMIN] = 1; /* Read at least 1 characters */
    ctx->newtio.c_cc[VTIME] = 0; /* Wait indefinetly */

    int ret = tcsetattr(ctx->fd, TCSANOW, &ctx->newtio); /* Set the attributes to the termios structure*/
    if (ret != 0) {
      printf("> ERROR ! in Setting UART (%d)\n", ret);
    }
    /*
    ctx->newtio.c_cflag |= (CRTSCTS | CS8 | CLOCAL | CREAD);
    ctx->newtio.c_iflag = IGNPAR;
    ctx->newtio.c_oflag = 0;
    ctx->newtio.c_lflag = 0;//non-Canonical mode ... if you want Canonical mode, add ICANON;
    ctx->newtio.c_cc[VMIN]= 0;  // non blocking...
    ctx->newtio.c_cc[VTIME]=0;
    tcflush(ctx->fd, TCIFLUSH);
    tcsetattr(ctx->fd,TCSANOW,&ctx->newtio);
     */
    return 0;
}

int uart_fd_select(int fd, int timeout_ms)
{
    fd_set io_fds;
    struct timeval timeout;

    if (fd <=0) return -1;
    if (timeout_ms<0) timeout_ms = 0;

    FD_ZERO(&io_fds);
    FD_SET(fd,&io_fds);

    timeout.tv_sec = 0;
    timeout.tv_usec = 1000*timeout_ms;

    if(select(fd+1,&io_fds,0,0,&timeout) == -1)      {
    	printf("Error fd_select\n");
        return -1;
    }

    if(FD_ISSET(fd,&io_fds))
    {
    	//printf("received data \n");
        return fd;
    }

    //printf("timeout\n");
    return -1;
}


/*=========================================
GLOBAL FUNCTIONS
=========================================*/
int uart_open(char *dev, int baud_rate)
{
    uart_ctx_t *ctx = NULL;
    if (!dev) return 0;

    ctx = (uart_ctx_t*)malloc(sizeof(uart_ctx_t));
    if (ctx)
    {
        memset(ctx,0,sizeof(uart_ctx_t));
        pthread_mutex_init(&ctx->mutex,NULL);

        ctx->fd = open_fd(dev);
        if(ctx->fd <= 0)
        {
            uart_close((int)ctx);
            return 0;
        }

        uart_config(ctx,baud_rate);
    }

    return (int)ctx;
}

void uart_close(int uartd)
{
    uart_ctx_t *ctx = (uart_ctx_t *)uartd;
    if(ctx)
    {
        if(ctx->fd > 0)
        {
            tcsetattr(ctx->fd,TCSANOW,&ctx->oldtio);
            close(ctx->fd);
            ctx->fd = 0;
        }

        free(ctx);
    }
}

int uart_readytoread(int uartd,int timeoutms)
{
    uart_ctx_t *ctx = (uart_ctx_t *)uartd;

    if(!ctx) return -1;

    pthread_mutex_lock(&ctx->mutex);
    if(uart_fd_select(ctx->fd,timeoutms) == ctx->fd)
    {
        pthread_mutex_unlock(&ctx->mutex);
        return 1;
    }
    pthread_mutex_unlock(&ctx->mutex);

    return 0;
}

int uart_read(int uartd, char *data, int size)
{
    int readlen = 0;
    uart_ctx_t *ctx = (uart_ctx_t *)uartd;

    if(!ctx || ctx->fd <= 0 || !data || size <= 0) return 0;

    pthread_mutex_lock(&ctx->mutex);
    readlen = read(ctx->fd,data,size*sizeof(char));
    pthread_mutex_unlock(&ctx->mutex);

    return readlen;
}

int uart_waitnread(int uartd, char *data, int size,int timeoutms)
{
    int readlen = 0;
    uart_ctx_t *ctx = (uart_ctx_t *)uartd;

    if(!ctx || ctx->fd <= 0 || !data || size <= 0) return 0;

    pthread_mutex_lock(&ctx->mutex);
    if(uart_fd_select(ctx->fd,timeoutms) == ctx->fd)
    {
        readlen = read(ctx->fd,data,size*sizeof(char));
    }
    pthread_mutex_unlock(&ctx->mutex);

    return readlen;
}

int uart_write(int uartd, char *data, int size)
{
    int wrotelen = 0;
    int failcnt = 0;
    uart_ctx_t *ctx = (uart_ctx_t *)uartd;

    if(!ctx || ctx->fd <= 0 || !data || size <= 0) return 0;

    //pthread_mutex_lock(&ctx->mutex);

    while(wrotelen < size && failcnt < 10)
    {
        int ret = 0;
        int towritelen = size - wrotelen;
        char *ptr = data + wrotelen;

        ret = write(ctx->fd,ptr,towritelen*sizeof(char));

        if (ret > 0)
        {
            wrotelen += ret;
            //printf("written count : %d",ret);
        }
        else
        {
            failcnt ++;
            //printf("written fail count : %d",ret);
        }
    }
    //pthread_mutex_unlock(&ctx->mutex);

    return wrotelen;
}
