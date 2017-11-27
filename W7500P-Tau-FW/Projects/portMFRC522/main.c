/**********************RC522ʵ�鿪��������************************
*  MCU: STC89C52RC
*  ����: 11.0592MHz
*  ������: 4800
*  �� MCU Ϊ�����ͺţ��Լ�����Ϊ����Ƶ��ʱ�������ʻ��б䶯��
*  ����ʹ�� STC89C54RD+ & 22.1184MHz & 6T/˫���� ʱ��������Ϊ 19200��
*  �������Ŷ���μ� main.h
******************************************************************/

/**
  ******************************************************************************
  * @file    portMFRC522/main.c 
  * @author  popctrl@163.com
  * @version V1.0.0
  * @date    2017/11/24
  * @brief   Main program body
  ******************************************************************************
  * @attention
  *
  * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, WIZnet SHALL NOT BE HELD LIABLE FOR ANY
  * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
  * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  * <h2><center>&copy; COPYRIGHT 2017 </center></h2>
  ******************************************************************************
  */ 
/* Includes ------------------------------------------------------------------*/
#include "W7500x.h"
#include "W7500x_ssp.h"
#include "MFRC522.h"
#include "print_x.h"

/* Defines -------------------------------------------------------------------*/
#define LED_OFF()     GPIOC->LB_MASKED[1] = 1
#define LED_ON()      GPIOC->LB_MASKED[1] = 0

/* Private variables ---------------------------------------------------------*/
unsigned char data1[16] = {0x12,0x34,0x56,0x78,0xED,0xCB,0xA9,0x87,0x12,0x34,0x56,0x78,0x01,0xFE,0x01,0xFE};
//M1����ĳһ��дΪ���¸�ʽ����ÿ�ΪǮ�����ɽ��տۿ�ͳ�ֵ����
//4�ֽڽ����ֽ���ǰ����4�ֽڽ��ȡ����4�ֽڽ�1�ֽڿ��ַ��1�ֽڿ��ַȡ����1�ֽڿ��ַ��1�ֽڿ��ַȡ��
unsigned char data2[4]  = {0,0,0,0x01};
unsigned char DefaultKey[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

/* Private function prototypes -----------------------------------------------*/
//void InitializeSystem();
void SPI1_Init(void);


unsigned char g_ucTempbuf[20];
void delay1(unsigned int z)
{
  unsigned int x,y;
  for(x=z;x>0;x--)
  for(y=110;y>0;y--);
}


void main( )
{
  unsigned char status,i;
  unsigned int temp;

  /* Set System init */
  SystemInit();
  SPI1_Init();

  /* Configure UART2, Baudrate = 115200 */
//    S_UART_Init(115200);
  UART2->BAUDDIV = 417;   /* (48000000 / 115200) */
  UART2->CTRL |= (S_UART_CTRL_RX_EN | S_UART_CTRL_TX_EN);

  prt_str("\r\nW7500P ---MFRC522 (read & write M1 card)--- Test!\r\n\r\n");

  prt_str("Read MFRC522's Reg Default Value.\r\n\r\n");
  for(i=0;i<48;i++){
    status = ReadRawRC(i);
    prt_str("["); prt_hb(i); prt_str("] = "); prt_hb(status); prt_str("\r\n");
  }

  PcdReset();
  PcdAntennaOff();
  PcdAntennaOn();

  prt_str("Read MFRC522's Reg Set Value.\r\n\r\n");
  for(i=0;i<48;i++){
    status = ReadRawRC(i);
    prt_str("["); prt_hb(i); prt_str("] = "); prt_hb(status); prt_str("\r\n");
  }


  prt_str("PcdAntennaOn();\r\n\r\n");

  while ( 1 )
  {
    status = PcdRequest(PICC_REQALL, g_ucTempbuf);//Ѱ��
    if (status != MI_OK)
    {
//     InitializeSystem( );
//     PcdReset();
     PcdAntennaOff();
     PcdAntennaOn();
     continue;
    }
  
    prt_str("��������: ");
    for(i=0;i<2;i++)
    {
      temp=g_ucTempbuf[i];
      prt_hb(temp); prt_str(" ");
    }
    prt_str("\r\n\r\n");
  
    status = PcdAnticoll(g_ucTempbuf);//����ײ
    if (status != MI_OK)
    {    continue;    }
  
  
    ////////����Ϊ�����ն˴�ӡ��������////////////////////////
    prt_str("�����к�: "); //�����ն���ʾ,
    for(i=0;i<4;i++)
    {
      temp=g_ucTempbuf[i];
      prt_hb(temp); prt_str(" ");
    }
    prt_str("\r\n\r\n");
    ///////////////////////////////////////////////////////////
  
    status = PcdSelect(g_ucTempbuf);//ѡ����Ƭ
    if (status != MI_OK)
    {    continue;    }
  
    status = PcdAuthState(PICC_AUTHENT1A, 1, DefaultKey, g_ucTempbuf);//��֤��Ƭ����
    if (status != MI_OK)
    {    continue;    }
  
    status = PcdWrite(1, data1);//д��
    if (status != MI_OK)
    {    continue;    }

    while(1)
    {
      status = PcdRequest(PICC_REQALL, g_ucTempbuf);//Ѱ��
      if (status != MI_OK)
      {
        //InitializeSystem( );
        //PcdReset();
        PcdAntennaOff();
        PcdAntennaOn();
        continue;
      }
      status = PcdAnticoll(g_ucTempbuf);//����ײ
      if (status != MI_OK)
      {    continue;    }
      status = PcdSelect(g_ucTempbuf);//ѡ����Ƭ
      if (status != MI_OK)
      {    continue;    }
  
      status = PcdAuthState(PICC_AUTHENT1A, 1, DefaultKey, g_ucTempbuf);//��֤��Ƭ����
      if (status != MI_OK)
      {    continue;    }
  
      status = PcdValue(PICC_DECREMENT,1,data2);//�ۿ�
      if (status != MI_OK)
      {    continue;    }
  
      status = PcdBakValue(1, 2);//�鱸��
      if (status != MI_OK)
      {    continue;    }
  
      status = PcdRead(2, g_ucTempbuf);//����
      if (status != MI_OK)
      {    continue;    }
      prt_str("��Ƭ����: "); //�����ն���ʾ,
      for(i=0;i<16;i++)
      {
        temp=g_ucTempbuf[i];
        prt_hb(temp); prt_str(" ");
        if((i==3)||(i==7)||(i==11)) prt_str(" ");
      }
      prt_str("\r\n");

      LED_ON();
      delay1(8000);
      LED_OFF();
      delay1(8000);
      LED_ON();
      delay1(8000);
      LED_OFF();
      delay1(8000);

//      LED_GREEN = 0;
//      delay1(800);
//      LED_GREEN = 1;
//      delay1(800);
//      LED_GREEN = 0;
//      delay1(800);
//      LED_GREEN = 1;
//      delay1(800);
      PcdHalt();
    }
  }
}


/////////////////////////////////////////////////////////////////////
//ϵͳ��ʼ��
/////////////////////////////////////////////////////////////////////
//void InitializeSystem()
//{
//  P0M1 = 0x0; P0M2 = 0x0;
//  P1M1 = 0x0; P1M2 = 0x0;
//  P3M1 = 0x0; P3M2 = 0xFF;
//  P0 = 0xFF; P1 = 0xFF; P3 = 0xFF;P2 = 0xFF;

//  TMOD=0x21;       //��T0Ϊ��ʽ1��GATE=1��
//  SCON=0x50;
//  TH1=0xFa;          //������Ϊ4800bps
//  TL1=0xFa;
//  TH0=0;
//  TL0=0;
//  TR0=1;
//  ET0=1;             //����T0�ж�
//  TR1=1;         //������ʱ��
//  TI=1;
//  EA=1;         //�������ж�

//  ES = 1;
//  RI = 1;
//}
/**
  * @brief  SPI1_Init Function
  */
void SPI1_Init(void)
{
    SSP1->CR0 |= (uint32_t)( SSP_FrameFormat_MO | SSP_CPHA_1Edge | SSP_CPOL_Low | SSP_DataSize_8b );
    SSP1->CR1 |= (uint32_t)(SSP_SOD_RESET | SSP_Mode_Master | SSP_NSS_Hard | SSP_SSE_SET | SSP_LBM_RESET );
//    SSP1->CPSR = SSP_BaudRatePrescaler_2;
    SSP1->CPSR = 24;   // 48,000,000 / 24 = 2,000,000 (2MHz)

    /* GPIO PC_04 SPI1 nCS Set */
    GPIOC->OUTENSET = (0x01<<4);
    PC_PCR->Port[4] = 0x06;         // High driving strength & pull-up
    PC_AFSR->Port[4] = 0x01;        // GPIOC_4
    GPIOC->LB_MASKED[0x10] = 0x10;  // set GPIOC_4 high

    /* GPIO PB_00 nReset MFRC522 Set */
    GPIOB->OUTENSET = 0x01;
    PB_PCR->Port[0] = 0x06;         // High driving strength & pull-up
    PB_AFSR->Port[0] = 0x01;        // GPIOB_0
    GPIOB->LB_MASKED[0x01] = 0x01;  // set GPIOB_0 high

//    GPIO_InitDef.GPIO_Pin = GPIO_Pin_0; // Set to PC_00 (LED(R))
//    GPIO_InitDef.GPIO_Mode = GPIO_Mode_OUT; // Set to Mode Output
//    GPIO_Init(GPIOC, &GPIO_InitDef);
//    PAD_AFConfig(PAD_PC,GPIO_Pin_0, PAD_AF1); // PAD Config - LED used 2nd Function
//    GPIO_SetBits(GPIOC, GPIO_Pin_0); // LED(R) Off

    GPIOC->OUTENSET = 0x01;
    PC_PCR->Port[0] = 0x06;
    PC_AFSR->Port[0] = 0x01;
    GPIOC->LB_MASKED[1] = 1;
}






