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
#include "pthread.h"
//#include "include/speedtest.h"
//#include "include/mrloopbf_release.h"

#include <iostream>
#include <libusb-1.0/libusb.h>
#include "mcsl_api.h"
#include "speedtest.h"
#include "mcslwigig.h"

#include <inttypes.h>
using namespace std;

#define BUFSIZE  4096 * 4
bool is_open, s_index = false, running = false;
//libusb_device_handle *usb_handle;
unsigned char count = 0;
long bits;
//tc_mac_stat* mStat;// = (tc_mac_stat*) malloc(sizeof(tc_mac_stat));
//tc_phy_stat* pStat;// = (tc_phy_stat*) malloc(sizeof(tc_phy_stat));

pthread_t thread;
pthread_t show_bitrate;

int main(int argc, char *argv[])
{
    bool tx_idx = false, rx_idx = false;

#if 1
    int menu, tmp_num, tmp_sp, tmp_rule;
    uint8_t *speed = NULL, *rule = NULL;

    while(true){

        do{
            fprintf(stdout,"\033[2J");
            fprintf(stdout,"\033[1;0H");
            if(tx_idx){
                fprintf(stdout,"Select Function: \n");
                fprintf(stdout,"        1. Speed Set\n");
                fprintf(stdout,"        0. Quit\n");
            }else if(rx_idx){
                fprintf(stdout,"Select Function: \n");
                fprintf(stdout,"        0. Quit\n");
            }else{
                fprintf(stdout,"Select Function: \n");
                fprintf(stdout,"    1. Speed Set\n");
                fprintf(stdout,"    2. Rule Set\n");
                fprintf(stdout,"    3. Start SpeedTest\n");
                fprintf(stdout,"    0. Quit\n");
            }
            scanf("%d", &menu);

            switch(menu){
                case 1:
                    if(is_open){
                        fprintf(stdout,"\033");
                        fprintf(stdout,"\033");
                    }
                    fprintf(stdout,"Select Speed 1 - 7\n");
                    scanf("%d", &tmp_sp);
                    if(tmp_sp >= 1 && tmp_sp < 8){
                        if(!is_open)
                            speed = (uint8_t*) &tmp_sp;
                        else{
                            MCSL_SetSpeed(*speed);
                            fprintf(stdout,"Set Speed!\n");
                        }

                    }else{
                        fprintf(stdout,"Error Number\n");
                        speed = NULL;
                    }
                    break;
                case 2:
                    if(!is_open){
                        fprintf(stdout,"Select Rule : 1. PCP, 2. STA \n");
                        scanf("%d", &tmp_rule);
                        if(tmp_rule == 1 || tmp_rule == 2){
                            rule = (uint8_t*) &tmp_rule;
                            //printf("Error Number\n");
                            //rule = NULL;
                        }
                        //else{
                        //    printf("Error Number\n");
                        //}
                    }
                    break;
                case 3:
#if 1
                    if(!running){
                        if( speed != NULL && rule != NULL){
                            //printf(" %d, %d\n", *speed, *rule);
                            if(MCSL_Init() != 0){
                                return 0;
                            }
                            else{
                                //MCSL_GetDeviceInfo();
                                MCSL_SetSector(7);
                                MCSL_SetSpeed(*speed);
                                MCSL_SetMode(*rule);
                                //MCSL_GetCounter();
                                is_open = true;

                                fprintf(stdout,"Select Function 1. Tx, 2.Rx\n");
                                scanf("%d", &tmp_num);
                                if(tmp_num == 1){
                                    s_index = true;
                                    pthread_create(&thread, NULL, SpeedTx, NULL);
                                    pthread_create(&show_bitrate, NULL, ShowBitrate, NULL);
                                    running = true;
                                    tx_idx = true;
                                    fprintf(stdout,"Speed Test Tx Start!\n");
                                }
                                else if(tmp_num == 2){
                                    s_index = true;
                                    pthread_create(&thread, NULL, SpeedRx, NULL);
                                    pthread_create(&show_bitrate, NULL, ShowBitrate, NULL);
                                    running = true;
                                    rx_idx = true;
                                }
                                else{
                                    fprintf(stdout,"Fail, Please select funcion");
                                }
                            }
                        }else{
                            fprintf(stdout,"Cannot Start, Please set speed & rule\n");
                        }
                    }else{
                        fprintf(stdout,"Please close function\n");
                    }
                    break;
#endif
                case 0:
                    if(running){
                        s_index = false;
                        pthread_join(thread, NULL);
                        pthread_join(show_bitrate, NULL);
                        running = false;
                    }
                    break;
                default:
                    fprintf(stdout,"error\n");
                    continue;
            }
        }while(menu != 0 );
        break;
    }

    //device_close();
    printf("\033[2J");
#endif
    return 0;
}

void* ShowBitrate(void *ptr){

    long bitrate = 0;
    tc_mac_stat mStat;
    tc_phy_stat pStat;

    while(s_index){
        bitrate = ((bits * 8) / (1024 *1024));
        fprintf(stdout,"\033  %ld Mbp/s\n", bitrate);
        MCSL_GetReadCounter();
        /*double n_detect = (double)pStat.det_cp + (double)pStat.det_sc;
        if (n_detect ==0) n_detect = 1;
        
        double PER = 0;
        if ( pStat.total_rx != 0)
            PER = (double)pStat.fcs * 100 / (double)pStat.total_rx;
        double STF = (double)pStat.stf * 100 / n_detect;
        double HCS = (double)pStat.hcs * 100 / n_detect;
        double FCS = (double)pStat.fcs * 100 / n_detect;
        double snr = (double)pStat.snr/4;
        double evm = (double)pStat.evm/4;
        
        cout << "total_rx: " << (double)pStat.total_rx <<  endl;
        cout << "fcs: " << (double)pStat.fcs << endl;
        cout << "PER: " << (double)PER << endl;*/
        
        
        fflush(stdout);
        bits = 0;
        sleep(1);
    }
    return ((void *)0);
}

#if 1
void* SpeedTx(void *ptr){

    unsigned char* buf = (unsigned char*) malloc(BUFSIZE  * sizeof(char));
    memset(buf, 0, BUFSIZE);
    int length = BUFSIZE , status;
    //FILE *tx_fp = fopen("./sample.jpg", "r+");
    //int data_length = 0;
    
    //if(tx_fp == NULL)
    //{
    //    s_index = false;
    //    printf("Can not open file\n");
   // }
    
    //data_length = fread(buf, 1, BUFSIZE - 4, tx_fp);

    while(s_index){
        status = MCSL_Transfer(buf, length);
        if(status > 0){
            bits += length;
            /*if (data_length < BUFSIZE -4) {
                s_index = false;
                fclose(tx_fp);
                return ((void*) 0);
            } else {
                data_length = fread(buf, 1, BUFSIZE -4, tx_fp);
            }*/
        }
    }

    free(buf);
    return ((void *)0);
}

void* SpeedRx(void *ptr){

    unsigned char* buf = (unsigned  char*) malloc(BUFSIZE * sizeof(char));
    int status;
    int length;
    //uint8_t *data = (uint8_t*)malloc(32000);
    //memset(data, 1,32000);
    //FILE *rx_fp = fopen("/Users/kunnie/MCSL/tc_mcsl/output.yuv","a+");
     //if (rx_fp == NULL)
    // {
    // fclose(rx_fp);
    // rx_fp = fopen("/Users/kunnie/MCSL/tc_mcsl/output.yuv","a+");
    // }

    while(s_index){
        length = BUFSIZE;
        status = MCSL_Receiver(buf, &length);
        if(status > 0){
            bits += BUFSIZE;

            //fwrite(buf, 1, BUFSIZE * 4, rx_fp);
        }
        else{
            fprintf(stdout,"Rx fail return:%d", status);
        }
    }

    //fclose(rx_fp);
    free(buf);
    return ((void *)0);
}
#endif
/*
#include <stdio.h>
#include <stdlib.h>
#include <libusb-1.0/libusb.h>
#include "mcsl_api.h"
#include <string.h>

using namespace std;

#define BUFSIZE 4096

void printdev(libusb_device *dev); //prototype of the function

int main(int argc, const char * argv[]) {
    //mcslwigig *dongle = new mcslwigig();
    unsigned char *recvBuffer = (unsigned char*)malloc(BUFSIZE * 4);
    unsigned char  *transBuffer = (unsigned char*)malloc(BUFSIZE * 4);
    memset(transBuffer, 0, BUFSIZE * 4);

    int data_length = BUFSIZE *4;
    //tc_dev_info *dev_info = NULL;
    //tc_update_data_ch_settings params;
    //params.mcs = 7;
    MCSL_Init();

    //dongle->GetDeviceInfo(dev_info);

    //printf ("unicast_addr: 0x%02x \nmulticast_addr 0x%02x\n"
    //        "broadcast_addr: 0x%02x \nfirmware_version 0x%02x\n",
    //        dev_info->unicast_address,
    //        dev_info->multicast_address,
    //        dev_info->broadcast_address,
    //        dev_info->firmware_version);
    MCSL_SetMode(1);
    MCSL_SetSpeed(7);
    MCSL_SetSector(7);
    while (1) {
        MCSL_Receive(recvBuffer, &data_length);
        //MCSL_Transfer(transBuffer, data_length);
    }

    MCSL_GetCounter();
    MCSL_Close();
    return 0;
}*/
