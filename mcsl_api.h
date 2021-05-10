/*
 * This source is a part of MCSL core.
 * Copyright by MCSL - Gachon University
 * by mcsl.gachon@gmail.com
 */

#ifndef MCSL_API_H__
#define MCSL_API_H__
#include "mcslwigig.h"

int MCSL_Transfer(unsigned char *data, int data_length);

int MCSL_Receiver(unsigned char *data, int *data_length);

int MCSL_Init(void);

int MCSL_SetMode(int Mode);

int MCSL_SetSpeed(int Speed);

int MCSL_SetSector(int Sector);

int MCSL_GetCounter(void);

int MCSL_GetReadCounter(void);

int MCSL_GetDeviceInfo(void);

void MCSL_Close(void);

#endif
