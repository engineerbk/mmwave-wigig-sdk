#ifndef MCSL_WIGIG_H__
#define MCSL_WIGIG_H__

#include <libusb-1.0/libusb.h>

#define TC_PACKET_SIZE          4096

#define TC_FOOTER_SIZE          4
#define TC_PAYLOAD_SIZE         (TC_PACKET_SIZE - TC_FOOTER_SIZE)

#define TC_HEADER_SIZE          2
#define TC_BODY_SIZE            (TC_PAYLOAD_SIZE - TC_HEADER_SIZE)

static const uint8_t signature_field[8] =
{
    0x74, 0x65, 0x6e, 0x73, 0x6f, 0x73, 0x69, 0x67
};

enum class bf_role : unsigned {
    def = 0,
    pcp = 1,
    sta = 2,
};

struct __attribute__((__packed__)) tc_packet {
    union __attribute__((__packed__)) {
        struct __attribute__((__packed__)) {
            struct __attribute__((__packed__)) {
                uint16_t type   : 1;
                uint16_t id     : 15;
            } header;
            uint8_t body[TC_BODY_SIZE];
        } control;
        uint8_t data[TC_PAYLOAD_SIZE];
    } payload;
    struct __attribute__((__packed__)) { //Starts at offset 4092
        uint16_t reserve : 12;
        uint16_t channel : 4; //Control 0/1 Data 2/3
        uint16_t length;
    } footer;
};

struct __attribute__((__packed__)) tc_dev_info {
    uint8_t unicast_address[6];
    uint8_t multicast_address[6];
    uint8_t broadcast_address[6];
    uint8_t firmware_version[2];
};

struct __attribute__((__packed__)) tc_phy_stat {
    int8_t snr;
    int8_t evm;
    int16_t pwr;
    uint32_t total_tx;
    uint32_t total_rx;
    uint32_t fcs;
    uint32_t hcs;
    uint32_t stf;
    uint32_t det_sc;
    uint32_t det_cp;
    int8_t rssi;
    int8_t rcpi;
    uint16_t agc_gain_state;
};

struct __attribute__((__packed__)) tc_mac_stat {
    struct {
        uint32_t total_pkt_rcv;
        uint32_t total_mpdu_rcv;
        uint32_t total_ampdu_rcv;
        uint32_t total_fcs_err;
        uint32_t pkt_done_cnt;
        uint32_t data_pkt_done_cnt;
        uint32_t ack_counter;
        uint32_t cts_counter;
        uint32_t rts_counter;
        uint32_t back_counter;
        uint32_t bar_counter;
        uint32_t mic_pass_counter;
        uint32_t mic_fail_counter;
        uint32_t sector;

    } rx;

    struct {
        uint32_t fcs_err_rate;
        uint32_t fcs_value;
        uint32_t total_pkt_ct;
        uint32_t cp_pkt_ct;
        uint32_t sc_pkt_ct;
        uint32_t ampdu_ct;
        uint32_t done_ct;
        uint32_t fail_ct;
        uint32_t ack_ct;
        uint32_t sector;

    } tx;

};

struct __attribute__((__packed__)) tc_get_counters_rsp {
    tc_phy_stat phy_stat;
    tc_mac_stat mac_stat;
    int32_t temperature;
    uint32_t mcs;

};

struct __attribute__((__packed__)) tc_get_counters {
    uint8_t reset_phy_counters;
    uint8_t reset_mac_counters;

};

#define TK_LEN 16
#define QOS_DATA_MACH_LEN 26
struct __attribute__((__packed__)) tc_update_data_ch_settings {
    uint16_t streamid:4;//Ch to be configured
    uint16_t retranslimit:4;//retransmission limit
    uint16_t encryption:1;//1: encryption enable //0:disabled
    uint16_t mcs:5;//to be removed after implementing adaptation rate
    uint16_t heartbeat:1;
    uint16_t reserve:1;
    struct __attribute__((__packed__)) {
        uint8_t aid0;
        uint8_t aid1;
    } aid; //used by upper mac

    struct __attribute__((__packed__)) {
        uint16_t num:12; //0 ampdu disabled //
        uint16_t imp_ba:1;//0 explicit BAR // 1 implicit BAR
        uint16_t rsv:3;
    } ampdu;
    uint8_t tk[TK_LEN];
    uint8_t mach[QOS_DATA_MACH_LEN];//buffer containing mac header
    uint8_t sch_type:1;
    uint8_t ack_pol:2;
};

class mcslwigig {
public:
    mcslwigig(void);
    virtual ~mcslwigig(void);

public:
    int Init(void);
    int Send(unsigned char *data, int data_length);
    int Receive(unsigned char *data, int *data_length);
    int GetDeviceInfo(tc_dev_info *devinfo);
    int SetSpeed(int MCS_VALUE);
    int SetMode(int mode);
    int SetSector(int sector);
    int ReadHWCounter(void);
    int ReadCounter(tc_mac_stat* mStat, tc_phy_stat* pStat);
    void Close(void);
public:
    libusb_device **devs; //pointer to pointer of device, used to retrieve a list of devices
    libusb_device_handle *dev_handle; //a device handle
    libusb_context *ctx; //a libusb session
    tc_mac_stat* mStat;
    tc_phy_stat* pStat;
    //tc_get_counters* params;
    //tc_get_counters_rsp *getCounters;
private:
    int stop_dec;
    int stop;
    int video_stream_idx;
};

#endif
