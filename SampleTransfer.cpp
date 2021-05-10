#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/time.h>

#include "iconv.h"
#include "SampleTransfer.h"

//#include "mrloopbf_release.h"

#include <iostream>
#include <libusb-1.0/libusb.h>
#include "mcsl_api.h"
#include "speedtest.h"
#include "mcslwigig.h"
#include <inttypes.h>

using namespace std;

#define BUFSIZE  4096
#define PAYLOADSIZE (BUFSIZE - 96)
#define OFFSET (BUFSIZE - 4)
unsigned char count = 0;

uint8_t check_idx_tx = 1;
uint8_t check_idx_rx = 0;

uint8_t *recvBuf, *transferBuf;

bool start_idx = false;

FILE *tx_fp, *rx_fp;


int main(int argc, char *argv[])
{
	int comm;

	comm = getopt(argc, argv, "rt");
	if(comm != -1)
	{
		/*close debug message*/
		//MCSL_HiddenDebugMsg();
		
		/*open devices*/
		int status = MCSL_Init();
        
		switch(status)
		{
			case 0:
				MCSL_SetSpeed(7);
				break;
			case 1:
				printf("no devices\n");
				return 0;
				break;
			case 2:
				printf("connect fail\n");
				return 0;
				break;
			default:
				break;
		}
        
		MCSL_SetSector(7);
        
		/*Create tx, rx buffer, this size is 4096 byte*/
		transferBuf = (uint8_t*) malloc(BUFSIZE);
        memset(transferBuf, 0, BUFSIZE);
        recvBuf = (uint8_t*) malloc(BUFSIZE);
        memset(recvBuf, 0, BUFSIZE);

		switch(comm)
		{
			case 'r':
			case 'R':
				/*receive set STA mode*/
				MCSL_SetMode(1);
				start_idx = true;
				//pthread_create(&thread, NULL, Tx, NULL);
				Rx();
				break;
			case 't':
			case 'T':
				/*transfer set PCP mode*/
				MCSL_SetMode(2);
				start_idx = true;
				//pthread_create(&thread, NULL, Rx, NULL);
				Tx();
				break;
		}
	}
	else{
		printf("Command error\n");
	}

	if(recvBuf != NULL)	
		free(recvBuf);
	if(transferBuf != NULL)
		free(transferBuf);
	
	return 0;
}

void Tx()
{
	/*The Tx() is send sample.jpg to receive*/
	uint8_t * data = (uint8_t*)malloc(PAYLOADSIZE);
	tx_fp = fopen("./output.mp4", "r+");
	int data_length = 0;

	if(tx_fp == NULL)
	{
		start_idx = false;
		printf("Can not open file\n");
	}
	
	data_length = fread(data, 1, PAYLOADSIZE, tx_fp);
	
	while(start_idx)
	{
		if(CheckPktTx(data, data_length, 50) == 1)
		{

			if(data_length < PAYLOADSIZE)
			{
				start_idx = false;
				fclose(tx_fp);
				return;
			}else{
				data_length = fread(data, 1, PAYLOADSIZE, tx_fp);
			}
		}
	}
}

void Rx()
{
	uint8_t * data = (uint8_t*)malloc(PAYLOADSIZE);
        memset(data, 1, PAYLOADSIZE);
	int length;
	rx_fp = fopen("./output.mp4", "w+");
	if(rx_fp == NULL)
	{
		fclose(rx_fp);
		rx_fp = fopen("./output.mp4", "w+");
	}
	
	while(start_idx)
        {
		length = CheckPktRx(data, 50);
		if(length > 0)
		{
			fwrite(data, 1, length, rx_fp);
			if(length < PAYLOADSIZE)
			{
				fclose(rx_fp);
				start_idx = false;
				return;
			}
		}
        }
}

int CheckPktTx(uint8_t* data_buf, int data_length, int retryLimit)
{
	int status;
	int length = BUFSIZE;
	int pkt_length = BUFSIZE;

	/*put app index*/
	transferBuf[3] = 0xE5;
	/*push packet check index*/
	transferBuf[15] = check_idx_tx;

	/*copy data length*/
	memcpy(transferBuf + 16, &data_length, sizeof(int));
	
	/*copy data*/
	memcpy(transferBuf + 20, data_buf, data_length);

	for(int i = 0; i < 4; i++)
	{
		/*The trnasfer packet last 4 byte need to set value 0x00. If set other value the RF packet to lead to error. */
		transferBuf[OFFSET+i] = 0x00;
	}

	for(int i = 0; i< retryLimit; i++){
		status = MCSL_Transfer(transferBuf, pkt_length);
	        if(status > 0)
        	{
			/*Before use MCSL_Receiver need to set length. In this sample is receive 4096 data. */
			length = BUFSIZE;
			/*wait to receive rx ack and check packet index. */
			status = MCSL_Receiver(recvBuf, &length);
		        if(status > 0)
	    	        {
                		if(recvBuf[15] == check_idx_tx)
				{
                    check_idx_tx++;
                    //printf("Already sent successfule packet %ld\n", check_idx_tx);
					return 1;
				}else{
					return 0;
				}
		        }
		}
    	}

	printf("retry limit\n");
	return 0;
}

int CheckPktRx(uint8_t* data_buf,int retryLimit)
{
	int status;
	int length = BUFSIZE;
	int data_length;
	int pkt_length = BUFSIZE;
	int ack_count = 0;

	for(int i = 0; i< retryLimit; i++)
	{
		/*Before use MCSL_Receiver need to set length. In this sample is receive 4096 data. */
		length = BUFSIZE;
		status = MCSL_Receiver(recvBuf, &length);
	        if(status > 0)
        	{
			if(check_idx_rx != recvBuf[15] && recvBuf[3] == 0xE5)
		        {
                		check_idx_rx = recvBuf[15];
				/*push receive packet index to ack buffer*/
		                transferBuf[15] = recvBuf[15];
		                memcpy(&data_length, recvBuf+16, sizeof(int));
				if(data_length > PAYLOADSIZE || data_length <0)
					return 0;
				else{
		        		memcpy(data_buf, recvBuf+20, data_length);
				
					/* Send back ack */
                			status = MCSL_Transfer(transferBuf, pkt_length);

			                if(status > 0)
			                {
						/* return data length*/
	     					return data_length;
		                	}
				}
			 }else{
				/*handle repeat packet*/
				transferBuf[15] = check_idx_rx;
				/*Please do not often same back repeat packet ack. In this sample set ack count is 16. */
		                if(ack_count == 0 ){
					status = MCSL_Transfer(transferBuf, pkt_length);
			                if(status < 0)
			                {
						printf("USB error\n");
                        			return 0;
			                }
			                ack_count = 16;
		                }
		                ack_count--;
			}
	        }
	}
	printf("retry limit\n");
	return 0;	
}
