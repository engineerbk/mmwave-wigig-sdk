
#include "mcslwigig.h"
#include "mcsl_api.h"
#include <iostream>
using namespace std;

mcslwigig *ml_wigig = new mcslwigig();

int MCSL_Init(void)
{
    return ml_wigig->Init();
}

int MCSL_Transfer(unsigned char *data, int data_length)
{
    if (ml_wigig->Send(data, data_length) == 0)
        return 1;
    return 0;
}

int MCSL_Receiver(unsigned char *data, int *data_length)
{
    if (ml_wigig->Receive(data, data_length) == 0)
        return 1;
    return 0;
}

int MCSL_SetMode(int mode)
{
        return ml_wigig->SetMode(mode);
}

int MCSL_SetSpeed(int speed)
{
        return ml_wigig->SetSpeed(speed);
}

int MCSL_SetSector(int sector)
{
    return ml_wigig->SetSector(sector);
}

int MCSL_GetCounter(void)
{
    return ml_wigig->ReadHWCounter();
}

int MCSL_GetReadCounter(void)
{
    tc_mac_stat mStat;
    tc_phy_stat pStat;
    ml_wigig->ReadCounter(&mStat, &pStat);
    //MCSL_PrintCounters(mStat, pStat);
    double n_detect = (double)pStat.det_cp + (double)pStat.det_sc;
    if (n_detect ==0) n_detect = 1;
    double PER = 0;
    if ( pStat.total_rx != 0)
        PER = (double)pStat.fcs * 100 / (double)pStat.total_rx;
    double STF = (double)pStat.stf * 100 / (double)n_detect;
    double HCS = (double)pStat.hcs * 100 / (double)n_detect;
    double FCS = (double)pStat.fcs * 100 /(double)n_detect;
    double snr = (double)pStat.snr/4;
    double evm = (double)pStat.evm/4;
    
    cout << " n_detect: " << n_detect << endl;
    cout << " fcs: " << (double)pStat.fcs * 100 << endl;
    cout << " total_rx: " << (double)pStat.total_rx <<  endl;
    cout << " PER: " << PER << endl;
    cout << " FCS: " << FCS << endl;
    cout << " HCS: " << HCS << endl;
    cout << " STF: " << STF << endl;
    cout << " snr: " << snr << endl;
    cout << " evm: " << evm << endl;
    cout << "sector: " << mStat.rx.sector << endl;
}

int MCSL_GetDeviceInfo(void)
{
    tc_dev_info* dev_info = NULL;
    return ml_wigig->GetDeviceInfo(dev_info);
}

void MCSL_Close(void)
{
        return ml_wigig->Close();
}
