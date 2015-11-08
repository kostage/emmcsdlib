/**
 *  \file mmcsd_proto.c
 *
 *  \brief this file defines the MMC/SD standard operations
 *
 */

/*
* Copyright (C) 2010 Texas Instruments Incorporated - http://www.ti.com/
*/
/*
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions
*  are met:
*
*    Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
*
*    Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the
*    distribution.
*
*    Neither the name of Texas Instruments Incorporated nor the names of
*    its contributors may be used to endorse or promote products derived
*    from this software without specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
*  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
*  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
*  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
*  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
*  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
*  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
*  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
*  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
*/


#include "syl_mmcsd_proto.h"
#include "string.h"
#include "uartStdio.h"
#include "cache.h"
#include "hw_hs_mmcsd.h"
#include "hw_types.h"
#include "delay.h"
#include "hs_mmcsd.h"


#define DATA_RESPONSE_WIDTH       (4 * SOC_CACHELINE_SIZE)

/* Cache size aligned data buffer (minimum of 64 bytes) for command response */
#ifdef __TMS470__
#pragma DATA_ALIGN(dataBuffer, SOC_CACHELINE_SIZE);
static unsigned char dataBuffer[DATA_RESPONSE_WIDTH];

#elif defined(__IAR_SYSTEMS_ICC__)
#pragma data_alignment = SOC_CACHELINE_SIZE
static unsigned char dataBuffer[DATA_RESPONSE_WIDTH];

#elif defined(gcc)
static unsigned char dataBuffer[DATA_RESPONSE_WIDTH]
                               __attribute__((aligned(SOC_CACHELINE_SIZE)));

#else
#error "Unsupported compiler\n\r"
#endif


extern void delay(unsigned int milliSec);
/**
 * \brief   This function sends the command to MMCSD.
 *
 * \param    mmcsdCtrlInfo It holds the mmcsd control information.
 *
 * \param    mmcsdCmd It determines the mmcsd cmd
 *
 * \return   status of the command.
 *
 **/
#if 0
unsigned int MMCSDCmdSend(mmcsdCtrlInfo *ctrl, mmcsdCmd *c)
{
    return ctrl->cmdSend(ctrl, c);
}
#endif

/**
 * \brief   Configure the MMC/SD bus width
 *
 * \param    mmcsdCtrlInfo It holds the mmcsd control information.
 *
 * \param   buswidth   SD/MMC bus width.\n
 *
 *  buswidth can take the values.\n
 *     HS_MMCSD_BUS_WIDTH_4BIT.\n
 *     HS_MMCSD_BUS_WIDTH_1BIT.\n
 *
 * \return  None.
 *
 **/
unsigned int MMCSDBusWidthSet(mmcsdCtrlInfo *ctrl)
{
    mmcsdCardInfo *card = ctrl->card;
    unsigned int status = 0;
    mmcsdCmd capp;

    if (card->cardType == MMCSD_CARD_MMC)
    {//�� �������� ���� �������� ������ ��� �� ���������� ������ ��� ��������)
    	//� �� ������� �� ��� ��� �� ������������ � �� ����� ����? - �� �������)
		//���� ���� 4 ����
		if (card->busWidth & SD_BUS_WIDTH_4BIT)
		{
			if (ctrl->busWidth & SD_BUS_WIDTH_4BIT)
			{
				//���� ���� 4 ���� - ������ �� ����
				ctrl->busWidthConfig(ctrl, SD_BUS_WIDTH_4BIT);
				capp.idx = SD_CMD(6);
				capp.flags = SD_CMDRSP_BUSY;
				capp.arg = 0x03B70100;
				status = ctrl->cmdSend(ctrl, &capp);
				if (status == 0)  return 0;

			}
		}
		else if (card->busWidth & SD_BUS_WIDTH_8BIT) //����� 8 ���
		{
			if (ctrl->busWidth & SD_BUS_WIDTH_8BIT)
			{
				//���� ���� 8 ��� - ������ �� ����
				ctrl->busWidthConfig(ctrl, SD_BUS_WIDTH_8BIT);
				capp.idx = SD_CMD(6);
				capp.flags = SD_CMDRSP_BUSY;
				capp.arg = 0x03B70200;
				status = ctrl->cmdSend(ctrl, &capp);
				if (status == 0)  return 0;
				//����� ������ ����!! ��� ����� ��������!
				//���� ���� ����� �� ������ busy!
		        while (!(HWREG(ctrl->memBase + MMCHS_PSTATE) & (unsigned int)BIT(20)));

			}
		}



	}
    else
    {
    	return 0;
    }

    return 1; //����� �� �������!
}

/**
 * \brief    This function configures the transmission speed in MMCSD.
 *
 * \param    mmcsdCtrlInfo It holds the mmcsd control information.
 *
 * \returns  1 - successfull.
 *           0 - failed.
 **/
unsigned int MMCSDTranSpeedSet(mmcsdCtrlInfo *ctrl)
{
    mmcsdCardInfo *card = ctrl->card;
    int status;
    mmcsdCmd cmd;


    if (card->cardType == MMCSD_CARD_MMC)
    {
    	//��� ������������� eMMC ��� ���� ���������� ������������ �������
    	//�� ������� ����� ���� ��������� ����� ��� HS_TIMING �� TRAN_SPEED �� CSD
    	//������ ���� highspeed = 1 ������ 48 ���
    	if (card->sd_ver <= 1) return 1; //����� ��� �� ���� ��������
    	cmd.idx = SD_CMD(6);
    	cmd.flags = SD_CMDRSP_BUSY;
    	//����� �� ������������ eMMC
    	cmd.arg = 0x03B90100;
		status = ctrl->cmdSend(ctrl, &cmd);
		if (status == 0)  return 0;

        status = ctrl->busFreqConfig(ctrl, 48000000);
        if (status != 0) return 0;
        ctrl->opClk = 48000000;
        //��� ���������, �� ����� ����� ������ �� ���� ������� ����� ������
        //����� �������� �����-��
        //������ ���� ��������� ���� DAT0 �� ���������� � '1' ����� ����
        while (!(HWREG(ctrl->memBase + MMCHS_PSTATE) & (unsigned int)BIT(20)));
    }
    else
    {
    	return 0;
    }

    return 1;
}

/**
 * \brief   This function resets the MMCSD card.
 *
 * \param    mmcsdCtrlInfo It holds the mmcsd control information.
 *
 * \returns  1 - successfull reset of card.
 *           0 - fails to reset the card.
 **/
unsigned int MMCSDCardReset(mmcsdCtrlInfo *ctrl)
{
    unsigned int status = 0;
    mmcsdCmd cmd;

    cmd.idx = SD_CMD(0);

    cmd.flags = SD_CMDRSP_NONE;
    cmd.arg = 0;

    status = ctrl->cmdSend(ctrl, &cmd);

    return status;
}



/**
 * \brief   This function intializes the MMCSD Card.
 *
 * \param    mmcsdCtrlInfo It holds the mmcsd control information.
 *
 * \returns  1 - Intialization is successfull.
 *           0 - Intialization is failed.
 **/
unsigned int MMCSDCardInit(mmcsdCtrlInfo *ctrl)
{

    mmcsdCardInfo *card = ctrl->card;
    unsigned int retry = 0xFFFF;
    unsigned int status = 0;
    unsigned int khz;
    mmcsdCmd cmd;

    memset(ctrl->card, 0, sizeof(mmcsdCardInfo));

    card->ctrl = ctrl;

    /* CMD0 - reset card */
    status = MMCSDCardReset(ctrl);

    if (status == 0)
    {
        return 0;
    }

	//mmc card initialization


    ctrl->card->cardType = MMCSD_CARD_MMC;
    //open drain ��� ������ �������������
    HWREG(ctrl->memBase + MMCHS_CON) |= MMCHS_CON_OD;

    //Set SD_SYSCTL[25] SRC
    //bit to 0x1 and wait until it returns to 0x0
    HWREG(ctrl->memBase + MMCHS_SYSCTL) |= MMCHS_SYSCTL_SRC;
    while(!(HWREG(ctrl->memBase + MMCHS_SYSCTL) & MMCHS_SYSCTL_SRC));
    while(HWREG(ctrl->memBase + MMCHS_SYSCTL) & MMCHS_SYSCTL_SRC);

   /* CMD1 - SEND_OP_COND */
    retry = 10; //� �������
    cmd.idx = SD_CMD(1);
    cmd.flags = 0;
    cmd.arg = 0x00ff8080;/////����� �� 2 ��?
    cmd.rsp[0] = 0;
do{
	status = ctrl->cmdSend(ctrl, &cmd);
	if (status == 0) return status; //���� ��� ������, ����� ��������

} while (!(cmd.rsp[0] & ((unsigned int)BIT(31))) && retry--);

	if (0xffffffff == retry) //����� ������ 2 ��?
	{
		retry = 10; //c �������
        cmd.arg = 0x40ff8080; //��������� �������
		do{
			status = ctrl->cmdSend(ctrl, &cmd);
			if (status == 0) return status; //���� ��� ������, ����� ��������

		} while (!(cmd.rsp[0] & ((unsigned int)BIT(31))) && retry--);

	}
	if (0xffffffff == retry) return 0;

	//��������� OCR
    card->ocr = cmd.rsp[0];
    card->highCap = (card->ocr & SD_OCR_HIGH_CAPACITY) ? 1 : 0;

   /* CMD2 - ALL_SEND_CID */
    cmd.idx = SD_CMD(2);
    cmd.flags = SD_CMDRSP_136BITS;
    cmd.arg = 0;
	status = ctrl->cmdSend(ctrl, &cmd);
	if (status == 0) return status;

	//��������� CID �����
    memcpy(card->raw_cid, cmd.rsp, 16);

  /* CMD3 - SET_RELATIVE_ADDR */
    cmd.idx = SD_CMD(3);
    cmd.flags = 0;
    cmd.arg = 2 << 16;
	status = ctrl->cmdSend(ctrl, &cmd);
	if (status == 0) return status;

    card->rca = 2; //����

    //�������� open drain ��� ������ �������������
    HWREG(ctrl->memBase + MMCHS_CON) &= ~MMCHS_CON_OD;


    /* Send CMD9, to get the card specific data */
     cmd.idx = SD_CMD(9);
     cmd.flags = SD_CMDRSP_136BITS;
     cmd.arg = card->rca << 16;

		status = ctrl->cmdSend(ctrl, &cmd);
	if (status == 0) return status;

     memcpy(card->raw_csd, cmd.rsp, 16);

     card->sd_ver =  SD_CARD_CSD_VERSION(card);
     card->tranSpeed = SD_CARD0_TRANSPEED(card);

     //������ �������� ������� �� ������
     //���� ���-�� ��� ������������ - ��������� � �������
     switch (card->tranSpeed & 0x00000007) {
     case 0:
    	 khz = 100e3;
    	 break;
     case 1:
      	 khz = 1000e3;
       	 break;
     case 2:
       	 khz = 10000e3;
      	 break;
     case 3:
       	 khz = 100000e3;
      	 break;
     default:
         UARTPuts("TRAN_SPEED incorrect value read", -1);
    	 return 0;
     }
     switch ((card->tranSpeed) >> 3) {
     case 1:
    	 ctrl->opClk = 1 * khz;
    	 break;
     case 2:
    	 ctrl->opClk = 1.2 * khz;
    	 break;
     case 3:
    	 ctrl->opClk = 1.3 * khz;
    	 break;
     case 4:
    	 ctrl->opClk = 1.5 * khz;
    	 break;
     case 5:
    	 ctrl->opClk = 2 * khz;
    	 break;
     case 6:
    	 ctrl->opClk = 2.6 * khz;
    	 break;
     case 7:
    	 ctrl->opClk = 3 * khz;
    	 break;
     case 8:
    	 ctrl->opClk = 3.5 * khz;
    	 break;
     case 9:
    	 ctrl->opClk = 4 * khz;
    	 break;
     case 10:
    	 ctrl->opClk = 4.5 * khz;
    	 break;
     case 11:
    	 ctrl->opClk = 5.2 * khz;
    	 break;
     case 12:
    	 ctrl->opClk = 5.5 * khz;
    	 break;
     case 13:
    	 ctrl->opClk = 6 * khz;
    	 break;
     case 14:
    	 ctrl->opClk = 7 * khz;
    	 break;
     case 15:
    	 ctrl->opClk = 8 * khz;
    	 break;
     default:
         UARTPuts("TRAN_SPEED incorrect value read", -1);
    	 return 0;
     }
     status = ctrl->busFreqConfig(ctrl, ctrl->opClk);

     if (status != 0) //��� ������� ���������� ���� ��� ������
     {
         UARTPuts("HS MMC/SD TRAN_SPEED freqval set failed\n\r", -1);
     }
    //���� ������������ ���. 4.0 � ����
     if (card->sd_ver > 1)
	 {
         /* Send CMD7 select card */
          cmd.idx = SD_CMD(7);
          cmd.flags = 0; //����� R1
          cmd.arg = card->rca << 16;

  		status = ctrl->cmdSend(ctrl, &cmd);
 		if (status == 0) return status;

    	 //���� �������� EXT_CSD
		  ctrl->xferSetup(ctrl, 1, dataBuffer, 512, 1);
		  cmd.idx = SD_CMD(8);
		  cmd.flags = SD_CMDRSP_READ | SD_CMDRSP_DATA | (ctrl->dmaEnable << SD_CMDRSP_DMAEN_OFFSET);
		  cmd.arg = 0;
		  cmd.nblks = 1;
		  cmd.data = (signed char*)dataBuffer;
		  status = ctrl->cmdSend(ctrl, &cmd);
		  if (status == 0)  return 0;

		  status = ctrl->xferStatusGet(ctrl);
		  if (status == 0) return 0;

		  /* Invalidate the data cache. */
		  CacheDataInvalidateBuff((unsigned int)dataBuffer, DATA_RESPONSE_WIDTH);

	}
     else
     {
    	 UARTPuts("Unsupported eMMC\n\r", -1);
    	 return 0;
     }

     //������ ����������� ������� ����� ��� highCap ����  � ������� ����
     if (!(card->highCap))
     { //�� ����, ��� �������� ��� !highCap
		 card->blkLen = 1 << (SD_CARD0_RDBLKLEN(card));
		 card->size = SD_CARD0_SIZE(card);
		 card->nBlks = card->size / card->blkLen;

    /* Set data block length to 512 (for byte addressing cards) */
		 if (card->blkLen != 512)
		 {
			 cmd.idx = SD_CMD(16);
			 cmd.flags = 0; //resp R1
			 cmd.arg = 512;
			 status = ctrl->cmdSend(ctrl, &cmd);

			 if (status == 0)
			 {
				 return 0;
			 }
			 else
			 {
				 card->blkLen = 512;
			 }

		 }
     }
     else //highcap - ����� sector size �� EXT_CSD
     {
		  //���� ��������� ������ ���� �� EXT_CSD �� �������
		  //���� ������ SEC_COUNT
		 card->blkLen = 512;
		 card->nBlks = (dataBuffer[212] << 0) |
				  (dataBuffer[213] << 8) |
				  (dataBuffer[214] << 16) |
				  (dataBuffer[215] << 24);
		 card->size = card->nBlks; //��� highcap ����� ������ � ��������
     }
     card->busWidth = 1;
#if 0 //�������� ������� �� ������ �� ���� - ����� � ��� ����
     //deselect card
     /* Send CMD7 select card */
    cmd.idx = SD_CMD(7);
    cmd.flags = SD_CMDRSP_NONE; //����� R1
    cmd.arg = 0; //rca = 0
	status = ctrl->cmdSend(ctrl, &cmd);
		if (status == 0) return status;
#endif
	//end emmc initialization

    return 1;
}

/**
 * \brief   This function sends the write command to MMCSD card.
 *
 * \param    mmcsdCtrlInfo It holds the mmcsd control information.
 * \param    ptr           It determines the address from where data has to written
 * \param    block         It determines to which block data to be written
 * \param    nblks         It determines the number of blocks to be written
 *
 * \returns  1 - successfull written of data.
 *           0 - failure to write the data.
 **/
unsigned int MMCSDWriteCmdSend(mmcsdCtrlInfo *ctrl, void *ptr, unsigned int block,
                               unsigned int nblks)
{
    mmcsdCardInfo *card = ctrl->card;
    unsigned int status = 0;
    unsigned int address;
    mmcsdCmd cmd;

    /*
     * Address is in blks for high cap cards and in actual bytes
     * for standard capacity cards
     */

    if (card->highCap)
    {
        address = block;
    }
    else
    {
        address = block * card->blkLen;
    }

    /* Clean the data cache. */
    CacheDataCleanBuff((unsigned int) ptr, (512 * nblks));

    ctrl->xferSetup(ctrl, 0, ptr, 512, nblks);

    cmd.flags = SD_CMDRSP_WRITE | SD_CMDRSP_DATA | (ctrl->dmaEnable << SD_CMDRSP_DMAEN_OFFSET);
    cmd.arg = address;
    cmd.nblks = nblks;

    if (nblks > 1)
    {
        cmd.idx = SD_CMD(25);
    }
    else
    {
        cmd.idx = SD_CMD(24);
    }


    status = ctrl->cmdSend(ctrl, &cmd);

    if (status == 0)
    {
        return 0;
    }

    status = ctrl->xferStatusGet(ctrl);

    if (status == 0)
    {
        return 0;
    }

    while (!MMCSDWaitCardReadyForData(ctrl));

    return 1;
}

/**
 * \brief   This function sends the write command to MMCSD card.
 *
 * \param    mmcsdCtrlInfo It holds the mmcsd control information.
 * \param    ptr           It determines the address to where data has to read
 * \param    block         It determines from which block data to be read
 * \param    nblks         It determines the number of blocks to be read
 *
 * \returns  1 - successfull reading of data.
 *           0 - failure to the data.
 **/
unsigned int MMCSDReadCmdSend(mmcsdCtrlInfo *ctrl, void *ptr, unsigned int block,
                              unsigned int nblks)
{
    mmcsdCardInfo *card = ctrl->card;
    unsigned int status = 0;
    unsigned int address;
    mmcsdCmd cmd;

    /*
     * Address is in blks for high cap cards and in actual bytes
     * for standard capacity cards
     */
    if (card->highCap)
    {
        address = block;
    }
    else
    {
        address = block * card->blkLen;
    }

    ctrl->xferSetup(ctrl, 1, ptr, 512, nblks);

    cmd.flags = SD_CMDRSP_READ | SD_CMDRSP_DATA | (ctrl->dmaEnable << SD_CMDRSP_DMAEN_OFFSET);
    cmd.arg = address;
    cmd.nblks = nblks;

    if (nblks > 1)
    {
        cmd.idx = SD_CMD(18);
    }
    else
    {
        cmd.idx = SD_CMD(17);
    }

    status = ctrl->cmdSend(ctrl, &cmd);
    if (status == 0)
    {
        return 0;
    }

    status = ctrl->xferStatusGet(ctrl);

    if (status == 0)
    {
        return 0;
    }

    /* Invalidate the data cache. */
    CacheDataInvalidateBuff((unsigned int)(ptr), 512 * nblks);

    return 1;
}

//����� ����� ���������� ����� � ������� ������� �������� � ���������� 1 � ������ ������
//������� ��������� �� ��������� ��� ����� ����� �����, ����� ����� ������������� ��� ������
//����� �� ����� ������ � ����� ��������
unsigned int MMCSDWaitCardReadyForData(mmcsdCtrlInfo *ctrl)
{
    mmcsdCardInfo *card = ctrl->card;
    unsigned int status = 0;
    mmcsdCmd cmd;
    unsigned int retry = 0xffffffff;

    cmd.idx = SD_CMD(13);
    cmd.flags = 0;
    cmd.arg = card->rca << 16;

	do{
		status = ctrl->cmdSend(ctrl, &cmd);
		if (status == 0) return status; //���� ��� ������, ����� ��������

	} while (!(cmd.rsp[0] & ((unsigned int)BIT(8))) && --retry); //���-�� ������� �����, ���� ����� ��� READY_FOR_DATA

	if (0 == retry) return 0;

	return 1;
}
