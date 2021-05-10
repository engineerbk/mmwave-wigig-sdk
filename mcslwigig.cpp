#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <iostream>
#include <libusb-1.0/libusb.h>
#include "mcslwigig.h"
#include "mcsl_api.h"
#include <inttypes.h>

using namespace std;

enum message_id : unsigned {
    /* common commands for both network mode and test mode */
    TC_MSG_SCAN_START = 0x0000,
    TC_MSG_SCAN_STOP,
    TC_MSG_STOP_DMG,
    TC_MSG_STOP_DMG_DONE,
    TC_MSG_LOAD_FILTER,
    TC_MSG_LOAD_FILTER_DONE,
    TC_MSG_JOIN_START,
    TC_MSG_JOIN_DONE,
    TC_MSG_ASSOC_START,
    TC_MSG_ASSOC_DONE,
    TC_MSG_SET_SCHEDULE,
    TC_MSG_CAL_MODE,
    TC_MSG_TEST_MODE,
    TC_MSG_NORMAL_MODE,
    TC_MSG_GET_DEV_INFO_REQ,
    TC_MSG_GET_DEV_INFO_RSP,
    TC_MSG_ADD_DATA_CH_SETTINGS,
    TC_MSG_DEL_DATA_CH_SETTINGS,
    TC_MSG_UPDATE_DATA_CH_SETTINGS,
    TC_MSG_SET_TXOP_REQ,
    TC_MSG_RESET_TXOP_REQ,
    TC_MSG_SLS_REQ,
    TC_MSG_BRP_REQ,
    TC_MSG_SET_LOCAL_MAC_ADDR, //< added on 4/27/16
    TC_MSG_LOG_CFG,
    TC_MSG_LOG_INFO,
    TC_MSG_SET_CHANNEL_REQ,
    TC_MSG_SET_CHANNEL_RSP,

    /* test mode only */
    TC_MSG_READ_REG_REQ = 0x0100,
    TC_MSG_READ_REG_RESP,
    TC_MSG_WRITE_REG,
    TC_MSG_READ_HW_COUNTERS_REQ,
    TC_MSG_READ_HW_COUNTERS_RESP,

    /* beamforming */
    TC_MSG_SET_TX_SECTOR_MAC    = 0x0f00,
    TC_MSG_SET_RX_SECTOR_MAC    = 0x0f01,
    TC_MSG_DEV_REQ              = 0x0f02,

    /* flashing */
    TC_MSG_FW_SELECTION         = 0x0fff,

};

void printdev(libusb_device *dev); //prototype of the function

mcslwigig::mcslwigig(void)
    :dev_handle(NULL)
    , stop(0)
    , devs(NULL)
    , ctx(NULL)
{
    stop = 1;
}

mcslwigig::~mcslwigig(void) {
    Close();
}

void printdev(libusb_device *dev) {
        libusb_device_descriptor desc;
        int r = libusb_get_device_descriptor(dev, &desc);
        if (r < 0) {
            printf("failed to get device descriptor");
            return;
        }

        //printf("Number of possible configurations: %d ",(int)desc.bNumConfigurations);
        printf("Device Class: %d ",(int)desc.bDeviceClass);
        printf("VendorID: %4x %d ", desc.idVendor, desc.idVendor);
        printf("ProductID: %4x %d\n",desc.idProduct, desc.idProduct);
        libusb_config_descriptor *config;
        libusb_get_config_descriptor(dev, 0, &config);
        printf("Interfaces: %d ", (int)config->bNumInterfaces);
        const libusb_interface *inter;
        const libusb_interface_descriptor *interdesc;
        const libusb_endpoint_descriptor *epdesc;
        for(int i=0; i<(int)config->bNumInterfaces; i++) {
            inter = &config->interface[i];
            //printf("Number of alternate settings: %d ", inter->num_altsetting);
            for(int j=0; j<inter->num_altsetting; j++) {
                interdesc = &inter->altsetting[j];
                //printf("Interface Number: %d ", (int)interdesc->bInterfaceNumber);
                printf("Number of endpoints: %d ", (int)interdesc->bNumEndpoints);
                for(int k=0; k<(int)interdesc->bNumEndpoints; k++) {
                    epdesc = &interdesc->endpoint[k];
                    printf("Descriptor Type: %d ",(int)epdesc->bDescriptorType);
                    printf("EP Address: %d\n",(int)epdesc->bEndpointAddress);
                }
            }
        }
        printf("\n\n\n");
        libusb_free_config_descriptor(config);
}

int mcslwigig::Init(void) {
        //libusb_device **devs; //pointer to pointer of device, used to retrieve a list of devices
        //libusb_device_handle *dev_handle; //a device handle
        //libusb_context *ctx = NULL; //a libusb session

        int r; //for return values

        ssize_t cnt; //holding number of devices in list

        r = libusb_init(&ctx); //initialize a library session

        if(r < 0) {
            printf("Init Error: %d", r); //there was an error
            return 1;
        }

        libusb_set_debug(ctx, 3); //set verbosity level to 3, as suggested in the documentation
        cnt = libusb_get_device_list(ctx, &devs); //get the list of devices
        if(cnt < 0) {
            printf("Get Device Error\n"); //there was an error
            return 1;
        }

        //printf("Devices in list: %d\n", cnt); //print total number of usb devices

        //dump device
        //ssize_t i; //for iterating through the list
       // for(i = 0; i < cnt; i++) {
        //    printdev(devs[i]); //print specs of this device
        //}

        dev_handle = libusb_open_device_with_vid_pid(ctx, 1204, 240); //these are vendorID and productID I found for my usb device
    if(dev_handle == NULL) {
            printf("Cannot open device\n");
        return 1;
    }
    else
        printf("Device Opened\n");

        libusb_free_device_list(devs, 1); //free the list, unref the devices in it

        //some dummy values
        int actual; //used to find out how many bytes were written
        if(libusb_kernel_driver_active(dev_handle, 0) == 1) { //find out if kernel driver is attached
            printf("Kernel Driver Active\n");
            if(libusb_detach_kernel_driver(dev_handle, 0) == 0) //detach it
                printf("Kernel Driver Detached!\n");
        }

        r = libusb_claim_interface(dev_handle, 0); //claim interface 0 (the first) of device (mine had jsut 1)
        if(r < 0) {
            printf("Cannot Claim Interface\n");
            return 1;
        } else
            printf("Claimed Interface\n");
        //printf("Data->%s\n", data); //just to see the data we want to write : abcd
        //printf("Writing Data...\n");

        /*while (1) {
            r = libusb_bulk_transfer(dev_handle, 1, data, 4096, &actual, 0); //my device's out endpoint was 2, found with trial- the device had 2 endpoints: 2 and 129
            if(r != 0 || actual != 4096) //we wrote the 4 bytes successfully
            {
                printf("Write Error\n");
                return;
            }
        }*/

        //r = libusb_release_interface(dev_handle, 0); //release the claimed interface
        //if(r!=0) {
        //    printf("Cannot Release Interface\n");
        //    return 1;
        //}
        //printf("Released Interface\n");

        //libusb_close(dev_handle); //close the device we opened

        //delete[] data;
        //libusb_free_device_list(devs, 1); //free the list, unref the devices in it
        //libusb_exit(ctx); //close the session
    return 0;

}

int mcslwigig::SetMode(int mode)
{
    int r = 0, actual = 0, received = 0;
    tc_packet *buffer = (tc_packet *)new char[TC_PACKET_SIZE];

    buffer->payload.control.header.type = 0;
    buffer->payload.control.header.id = TC_MSG_DEV_REQ;
    buffer->footer.reserve = 0;
    buffer->footer.channel = 0;
    buffer->footer.length = (TC_HEADER_SIZE + 1);
    memcpy((char*)buffer + 4000, signature_field, 8);

    buffer->payload.control.body[0] = mode; //bf_role; //Based on the bf_role

    if (mode == 1)
        printf("Set Mode: PCP\n");
    else if (mode == 2)
        printf("Set Mode: STA\n");
    else {
        printf("Please Set Mode: PCP(1) or STA(2)\n");
        return 1;
    }

    r = libusb_bulk_transfer(dev_handle, 1, (unsigned char*) buffer, TC_PACKET_SIZE, &actual, 0);
    if(r != 0 || actual != TC_PACKET_SIZE) //we wrote the 4 bytes successfully
    {
        printf("Write Error\n");
        return 1;
    }

    delete[] buffer;
    return 0;}

int mcslwigig::SetSpeed(int MCS_VALUE)
{
    int r = 0, actual = 0, received = 0;
    tc_packet *buffer = (tc_packet *)new char[TC_PACKET_SIZE];

    uint16_t param_size = sizeof(tc_update_data_ch_settings);
    buffer->payload.control.header.type = 0;
    buffer->payload.control.header.id = TC_MSG_UPDATE_DATA_CH_SETTINGS;
    buffer->footer.reserve = 0;
    buffer->footer.channel = 0;
    buffer->footer.length = (TC_HEADER_SIZE + param_size);
    memcpy((char*)buffer + 4000, signature_field, 8);

    tc_update_data_ch_settings *params =
    (tc_update_data_ch_settings*)buffer->payload.control.body;
    params->mcs = MCS_VALUE; //mcs can be from 1-7
    printf("Set MCS_Level: %d\n", params->mcs);
    params->ack_pol = 0;
    params->sch_type = 0;

    r = libusb_bulk_transfer(dev_handle, 1, (unsigned char*) buffer, TC_PACKET_SIZE, &actual, 0);
    if(r != 0 || actual != TC_PACKET_SIZE) //we wrote the 4 bytes successfully
    {
        printf("Write Error\n");
        return 1;
    }

    delete[] buffer;
    return 0;
}

int mcslwigig::SetSector(int sector)
{
    int r = 0 , actual = 0, received = 0;
    tc_packet *buffer = (tc_packet *)new char[TC_PACKET_SIZE];

    buffer->payload.control.header.type = 0;
    //buffer->payload.control.header.id = TC_MSG_SET_TX_SECTOR_MAC;
    buffer->payload.control.header.id = TC_MSG_SET_RX_SECTOR_MAC;
    buffer->footer.reserve = 0;
    buffer->footer.channel = 0;
    buffer->footer.length = (TC_HEADER_SIZE + 1);
    memcpy((char*)buffer + 4000, signature_field, 8);
    buffer->payload.control.body[0] = sector; //Sectors 0 - 15
    printf("Set sector: %d\n", sector);

    r = libusb_bulk_transfer(dev_handle, 1, (unsigned char*) buffer, TC_PACKET_SIZE, &actual, 0);
    if(r != 0 || actual != TC_PACKET_SIZE) //we wrote the 4 bytes successfully
    {
        printf("Write Error\n");
        return 1;
    }

    delete[] buffer;
    return 0;
}

int mcslwigig::ReadHWCounter(void)
{
    int r = 0 , actual = 0, received = 0;
    tc_packet *buffer = (tc_packet *)new char[TC_PACKET_SIZE];
    unsigned char recvBuffer[TC_PACKET_SIZE];
    tc_packet* packetBuffer;

    uint16_t param_size = sizeof(tc_get_counters);
    buffer->payload.control.header.type = 0;
    buffer->payload.control.header.id = TC_MSG_READ_HW_COUNTERS_REQ;
    buffer->footer.reserve = 0;
    buffer->footer.channel = 0;
    buffer->footer.length = (TC_HEADER_SIZE + param_size);
    memcpy((char*)buffer + 4000, signature_field, 8);

    tc_get_counters *params =
    (tc_get_counters*)buffer->payload.control.body;
    params->reset_mac_counters = 1;
    params->reset_phy_counters = 1;

    r = libusb_bulk_transfer(dev_handle, 1, (unsigned char*) buffer, TC_PACKET_SIZE, &actual, 0); //my device's out endpoint was1, found with trial- the device had 2 endpoints: 1 and 129
    if(r != 0 || actual != TC_PACKET_SIZE) //we wrote the 4 bytes successfully
    {
        printf("Write Error\n");
        return 1;
    }
    printf("Reset mac_counters and phy_counters\n");

    delete[] buffer;
    return 0;
}

int mcslwigig::ReadCounter(tc_mac_stat* mStat, tc_phy_stat* pStat) {
    int r = 0 , actual = 0, received = 0;
    tc_packet *buffer = (tc_packet *)new char[TC_PACKET_SIZE];
    unsigned char recvBuffer[TC_PACKET_SIZE];
    tc_packet* packetBuffer;

    uint16_t param_size = sizeof(tc_get_counters);
    buffer->payload.control.header.type = 0;
    buffer->payload.control.header.id = TC_MSG_READ_HW_COUNTERS_REQ;
    buffer->footer.reserve = 0;
    buffer->footer.channel = 0;
    buffer->footer.length = (TC_HEADER_SIZE + param_size);
    memcpy((char*)buffer + 4000, signature_field, 8);

    tc_get_counters *params =
    (tc_get_counters *)buffer->payload.control.body;
    params->reset_mac_counters = 1;
    params->reset_phy_counters = 1;

    r = libusb_bulk_transfer(dev_handle, 1, (unsigned char*)buffer, TC_PACKET_SIZE, &actual, 0); //my device's out endpoint was1, found with trial- the device had 2 endpoints: 1 and 129
    if(r != 0 || actual != TC_PACKET_SIZE) //we wrote the 4 bytes successfully
    {
        printf("Write Error\n");
        return 1;
    }

    r = libusb_bulk_transfer(dev_handle, 129, (unsigned char*)recvBuffer, TC_PACKET_SIZE, &received, 0); //my device's out endpoint was1, found with trial- the device had 2 endpoints: 1 and 129
    if(r != 0 || received!= TC_PACKET_SIZE) //we wrote the 4 bytes successfully
    {
        printf("Read Error\n");
        return 2;
    }

    packetBuffer = (tc_packet *)recvBuffer;
    if (packetBuffer->payload.control.header.id == TC_MSG_READ_HW_COUNTERS_RESP) {
    printf("got counter\n");
    tc_get_counters_rsp* getCounters =
    (tc_get_counters_rsp*)(packetBuffer->payload.control.body);
    if (mStat == NULL)
        mStat = new tc_mac_stat;
    if (pStat == NULL)
        pStat = new tc_phy_stat;
    memcpy(mStat, (tc_mac_stat*)(&getCounters->mac_stat), sizeof(tc_mac_stat));
    memcpy(pStat, (tc_phy_stat*)(&getCounters->phy_stat), sizeof(tc_phy_stat));
    }
    /*
    uint32_t n_detect = pStat->det_cp + pStat->det_sc;
    if (n_detect ==0) n_detect = 1;
    
    double PER = 0;
    if ( pStat->total_rx != 0)
        PER = pStat->fcs * 100 / pStat->total_rx;
    double STF = pStat->stf * 100 / n_detect;
    double HCS = pStat->hcs * 100 / n_detect;
    double FCS = pStat->fcs * 100 / n_detect;
    double snr = pStat->snr/4;
    double evm = pStat->evm/4;
    fprintf(stdout,"total_rx: %d | PER: %f| "
            "STF: %f | HCS: %f| "
            "FCS: %f | SNR: %f\n",
            pStat->total_rx, PER,
            STF, HCS,
            FCS, snr);
     */
     
    delete[] buffer;
    return 0;

}

int mcslwigig::GetDeviceInfo(tc_dev_info *devinfo)
{
    int r = 0 , actual, received;
    tc_packet *buffer = (tc_packet *)new char[TC_PACKET_SIZE];
    unsigned char recvBuffer[TC_PACKET_SIZE];
    tc_packet* packetBuffer;

    buffer->payload.control.header.type = 0;
    buffer->payload.control.header.id = TC_MSG_GET_DEV_INFO_REQ;
    buffer->footer.reserve = 0;
    buffer->footer.channel = 0;
    buffer->footer.length = TC_HEADER_SIZE;
    memcpy((char*)buffer + 4000, signature_field, 8);

    r = libusb_bulk_transfer(dev_handle, 1, (unsigned char*) buffer, TC_PACKET_SIZE, &actual, 0); //my device's out endpoint was1, found with trial- the device had 2 endpoints: 1 and 129
    if(r != 0 || actual != TC_PACKET_SIZE) //we wrote the 4 bytes successfully
    {
        printf("Write Error\n");
        return 1;
    }

    r = libusb_bulk_transfer(dev_handle, 129, (unsigned char*) recvBuffer, TC_PACKET_SIZE, &received, 0); //my device's out endpoint was1, found with trial- the device had 2 endpoints: 1 and 129
    if(r != 0 || received!= TC_PACKET_SIZE) //we wrote the 4 bytes successfully
    {
        printf("Read Error\n");
        return 2;
    }

    packetBuffer = (tc_packet*)recvBuffer;
    tc_dev_info *tmpdevinfo = (tc_dev_info*)packetBuffer->payload.control.body;

    if (devinfo == NULL)
    {
        devinfo = new tc_dev_info;
    }
    memcpy(devinfo, tmpdevinfo, sizeof(tc_dev_info));
    printf ("unicast_addr: 0x%02x \nmulticast_addr 0x%02x\n"
            "broadcast_addr: 0x%02x \nfirmware_version 0x%02x\n",
            tmpdevinfo->unicast_address,
            tmpdevinfo->multicast_address,
            tmpdevinfo->broadcast_address,
            tmpdevinfo->firmware_version);

    delete[] buffer;
    return 0;
}

int mcslwigig::Send(unsigned char *data, int data_length)
{
    int r = 0, actual, i =0;
    tc_packet *buffer = (tc_packet *)new char[TC_PACKET_SIZE];

    while (i < data_length) {
    buffer->footer.reserve = 0;
    buffer->footer.channel = 2;
    buffer->footer.length = TC_HEADER_SIZE + 1;
    memcpy((char *)buffer->payload.data, data +i, TC_PAYLOAD_SIZE);
    //memcpy((char*)buffer, data + i, TC_PACKET_SIZE);

    r = libusb_bulk_transfer(dev_handle, 1, (unsigned char *)buffer, TC_PACKET_SIZE, &actual, 0); //my device's out endpoint was1, found with trial- the device had 2 endpoints: 1 and 129
    if(r != 0 || actual != TC_PACKET_SIZE) //we wrote the 4 bytes successfully
    {
        printf("Write Error\n");
        return 1;
    }
        i += TC_PACKET_SIZE;
    }

    delete[] buffer;
    return 0;
}

int mcslwigig::Receive(unsigned char *data, int *data_length)
{
    tc_packet *buffer = (tc_packet *)new char[TC_PACKET_SIZE];
    unsigned char recvBuffer[TC_PACKET_SIZE];
    tc_packet *packetBuffer;
    int r, actual, i = 0;

    while ( i < *data_length) {
    r = libusb_bulk_transfer(dev_handle, 129, (unsigned char *)buffer, TC_PACKET_SIZE, &actual, 0); //my device's out endpoint was1, found with trial- the device had 2 endpoints: 1 and 129
    if(r != 0 || actual != TC_PACKET_SIZE) //we wrote the 4 bytes successfully
    {
        printf("Write Error\n");
        return 1;
    }

    //packetBuffer = (tc_packet *)recvBuffer;
    if (memcmp((char*)buffer + 4000, signature_field, 8) == 0)
    {
        /*
        if (buffer->payload.control.header.id == TC_MSG_READ_HW_COUNTERS_RESP)
        {
            //printf("check HW counts!\n");
            
            tc_get_counters_rsp* getCounters =
            (tc_get_counters_rsp*)(buffer->payload.control.body);
            tc_mac_stat* mStat = (tc_mac_stat*)(&getCounters->mac_stat);
            tc_phy_stat* pStat = (tc_phy_stat*)(&getCounters->phy_stat);
            uint32_t n_detect = pStat->det_cp + pStat->det_sc;
            //if (n_detect ==0) n_detect = 1;
            
            uint32_t PER = 0;
            if ( pStat->total_rx != 0)
                PER = pStat->fcs * 100 / pStat->total_rx;
            double STF = pStat->stf * 100 / n_detect;
            double HCS = pStat->hcs * 100 / n_detect;
            double FCS = pStat->fcs * 100 / n_detect;
            double snr = pStat->snr/4;
            double evm = pStat->evm/4;
            fprintf(stdout,"n_detect %" PRIu32 "\n"
                    "| total_rx: %ld | "
                    "PER: %" PRIu32 "| "
                    "STF: %lf | HCS: %lf| "
                    "FCS: %lf | SNR: %lf\n",
                    n_detect, pStat->total_rx, PER,
                    STF, HCS,
                    FCS, snr);
            
        }*/
        printf("control packet!\n");
        delete[] buffer;
        return 1;
    }
    //else {
        //memcpy(data, (const char*)buffer->payload.data, TC_PAYLOAD_SIZE);
      //  printf("data packet\n");
    //}

    //memcpy(data + i, (const char*)buffer, TC_PACKET_SIZE);
    memcpy(data + i, (const char*)buffer->payload.data, TC_PAYLOAD_SIZE);
    //memcpy(data, (const char*)buffer, TC_PACKET_SIZE);
    //*data_length = TC_PAYLOAD_SIZE;
    i += TC_PACKET_SIZE;
    }

    delete[] buffer;
    return 0;
}

void mcslwigig::Close(void) {
    int r;
    r = libusb_release_interface(dev_handle, 0); //release the claimed interface
    if(r!=0) {
        printf("Cannot Release Interface\n");
        return;
    }
    printf("Released Interface\n");

    libusb_close(dev_handle); //close the device we opened

    //libusb_free_device_list(devs, 1); //free the list, unref the devices in it
    libusb_exit(ctx); //close the session
}
