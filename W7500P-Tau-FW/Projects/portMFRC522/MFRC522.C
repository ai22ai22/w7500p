//#include <intrins.h>
//#include "lpcreg.h"
//#include "main.h"
#include "MFRC522.h"
#include "W7500x.h"
#include "W7500x_ssp.h"

////////////////////////////////////////////////////////////////////////////////
#define MAXRLEN 18
#define USED_MFRC522_PA

#define MFRC522_RST_HIGH()     GPIOB->LB_MASKED[0x01] = 0x01
#define MFRC522_RST_LOW()      GPIOB->LB_MASKED[0x01] = 0x00

#define MFRC522_NSS_HIGH()     GPIOC->LB_MASKED[0x10] = 0x10
#define MFRC522_NSS_LOW()      GPIOC->LB_MASKED[0x10] = 0x00

////////////////////////////////////////////////////////////////////////////////
char PcdReset(void);
void PcdAntennaOn(void);
void PcdAntennaOff(void);
char PcdRequest(unsigned char req_code,unsigned char *pTagType);
char PcdAnticoll(unsigned char *pSnr);
char PcdSelect(unsigned char *pSnr);
char PcdAuthState(unsigned char auth_mode,unsigned char addr,unsigned char *pKey,unsigned char *pSnr);
char PcdRead(unsigned char addr,unsigned char *pData);
char PcdWrite(unsigned char addr,unsigned char *pData);
char PcdValue(unsigned char dd_mode,unsigned char addr,unsigned char *pValue);
char PcdBakValue(unsigned char sourceaddr, unsigned char goaladdr);
char PcdHalt(void);
char PcdComMF522(unsigned char Command,
                 unsigned char *pInData,
                 unsigned char InLenByte,
                 unsigned char *pOutData,
                 unsigned int  *pOutLenBit);
void CalulateCRC(unsigned char *pIndata,unsigned char len,unsigned char *pOutData);
void WriteRawRC(unsigned char Address,unsigned char value);
unsigned char ReadRawRC(unsigned char Address);
void SetBitMask(unsigned char reg,unsigned char mask);
void ClearBitMask(unsigned char reg,unsigned char mask);

////////////////////////////////////////////////////////////////////////////////
//��    �ܣ�Ѱ��
//����˵��: req_code[IN]:Ѱ����ʽ
//                0x52 = Ѱ��Ӧ�������з���14443A��׼�Ŀ�
//                0x26 = Ѱδ��������״̬�Ŀ�
//          pTagType[OUT]����Ƭ���ʹ���
//                0x4400 = Mifare_UltraLight
//                0x0400 = Mifare_One(S50)
//                0x0200 = Mifare_One(S70)
//                0x0800 = Mifare_Pro(X)
//                0x4403 = Mifare_DESFire
//��    ��: �ɹ�����MI_OK
////////////////////////////////////////////////////////////////////////////////
char PcdRequest(unsigned char req_code,unsigned char *pTagType)
{
  char status;
  unsigned int  unLen;
  unsigned char ucComMF522Buf[MAXRLEN];
  
  ClearBitMask(Status2Reg,0x08);
  WriteRawRC(BitFramingReg,0x07);
  SetBitMask(TxControlReg,0x03);

  ucComMF522Buf[0] = req_code;

  status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,1,ucComMF522Buf,&unLen);

  if ((status == MI_OK) && (unLen == 0x10))
  {
    *pTagType     = ucComMF522Buf[0];
    *(pTagType+1) = ucComMF522Buf[1];
  }
  else
  {   
    status = MI_ERR;
  }

  return status;
}

////////////////////////////////////////////////////////////////////////////////
//��    �ܣ�����ײ
//����˵��: pSnr[OUT]:��Ƭ���кţ�4�ֽ�
//��    ��: �ɹ����� MI_OK
////////////////////////////////////////////////////////////////////////////////
char PcdAnticoll(unsigned char *pSnr)
{
  char status;
  unsigned char i,snr_check=0;
  unsigned int  unLen;
  unsigned char ucComMF522Buf[MAXRLEN];

  ClearBitMask(Status2Reg,0x08);
  WriteRawRC(BitFramingReg,0x00);
  ClearBitMask(CollReg,0x80);

  ucComMF522Buf[0] = PICC_ANTICOLL1;
  ucComMF522Buf[1] = 0x20;

  status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,2,ucComMF522Buf,&unLen);

  if (status == MI_OK)
  {
    for (i=0; i<4; i++)
    {
      *(pSnr+i)  = ucComMF522Buf[i];
      snr_check ^= ucComMF522Buf[i];

    }
    if (snr_check != ucComMF522Buf[i])
    {   status = MI_ERR;    }
  }

  SetBitMask(CollReg,0x80);
  return status;
}

////////////////////////////////////////////////////////////////////////////////
//��    �ܣ�ѡ����Ƭ
//����˵��: pSnr[IN]:��Ƭ���кţ�4�ֽ�
//��    ��: �ɹ����� MI_OK
////////////////////////////////////////////////////////////////////////////////
char PcdSelect(unsigned char *pSnr)
{
  char status;
  unsigned char i;
  unsigned int  unLen;
  unsigned char ucComMF522Buf[MAXRLEN];

  ucComMF522Buf[0] = PICC_ANTICOLL1;
  ucComMF522Buf[1] = 0x70;
  ucComMF522Buf[6] = 0;
  for (i=0; i<4; i++)
  {
    ucComMF522Buf[i+2] = *(pSnr+i);
    ucComMF522Buf[6]  ^= *(pSnr+i);
  }
  CalulateCRC(ucComMF522Buf,7,&ucComMF522Buf[7]);

  ClearBitMask(Status2Reg,0x08);

  status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,9,ucComMF522Buf,&unLen);

  if ((status == MI_OK) && (unLen == 0x18))
  {   status = MI_OK;  }
  else
  {   status = MI_ERR;    }

  return status;
}

////////////////////////////////////////////////////////////////////////////////
//��    �ܣ���֤��Ƭ����
//����˵��: auth_mode[IN]: ������֤ģʽ
//                 0x60 = ��֤A��Կ
//                 0x61 = ��֤B��Կ
//          addr[IN]�����ַ
//          pKey[IN]������
//          pSnr[IN]����Ƭ���кţ�4�ֽ�
//��    ��: �ɹ�����MI_OK
////////////////////////////////////////////////////////////////////////////////
char PcdAuthState(unsigned char auth_mode,unsigned char addr,unsigned char *pKey,unsigned char *pSnr)
{
  char status;
  unsigned int  unLen;
  unsigned char i,ucComMF522Buf[MAXRLEN];

  ucComMF522Buf[0] = auth_mode;
  ucComMF522Buf[1] = addr;
  for (i=0; i<6; i++)
  {    ucComMF522Buf[i+2] = *(pKey+i);   }
  for (i=0; i<6; i++)
  {    ucComMF522Buf[i+8] = *(pSnr+i);   }
 //   memcpy(&ucComMF522Buf[2], pKey, 6);
 //   memcpy(&ucComMF522Buf[8], pSnr, 4);

  status = PcdComMF522(PCD_AUTHENT,ucComMF522Buf,12,ucComMF522Buf,&unLen);
  if ((status != MI_OK) || (!(ReadRawRC(Status2Reg) & 0x08)))
  {   status = MI_ERR;   }

  return status;
}

////////////////////////////////////////////////////////////////////////////////
//��    �ܣ���ȡ M1 ��һ������
//����˵��: addr[IN]�����ַ
//          pData[OUT]�����������ݣ�16�ֽ�
//��    ��: �ɹ�����MI_OK
////////////////////////////////////////////////////////////////////////////////
char PcdRead(unsigned char addr,unsigned char *pData)
{
  char status;
  unsigned int  unLen;
  unsigned char i,ucComMF522Buf[MAXRLEN];

  ucComMF522Buf[0] = PICC_READ;
  ucComMF522Buf[1] = addr;
  CalulateCRC(ucComMF522Buf,2,&ucComMF522Buf[2]);

  status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,4,ucComMF522Buf,&unLen);
 //   {   memcpy(pData, ucComMF522Buf, 16);   }
  if ((status == MI_OK) && (unLen == 0x90))
  {
    for (i=0; i<16; i++)
    { *(pData+i) = ucComMF522Buf[i];   }
  }
  else
  { status = MI_ERR;   }

  return status;
}

////////////////////////////////////////////////////////////////////////////////
//��    �ܣ�д���ݵ� M1 ��һ��
//����˵��: addr[IN]�����ַ
//          pData[IN]��д������ݣ�16�ֽ�
//��    ��: �ɹ�����MI_OK
////////////////////////////////////////////////////////////////////////////////
char PcdWrite(unsigned char addr,unsigned char *pData)
{
  char status;
  unsigned int  unLen;
  unsigned char i,ucComMF522Buf[MAXRLEN];

  ucComMF522Buf[0] = PICC_WRITE;
  ucComMF522Buf[1] = addr;
  CalulateCRC(ucComMF522Buf,2,&ucComMF522Buf[2]);

  status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,4,ucComMF522Buf,&unLen);

  if ((status != MI_OK) || (unLen != 4) || ((ucComMF522Buf[0] & 0x0F) != 0x0A))
  {   status = MI_ERR;   }

  if (status == MI_OK)
  {
    //memcpy(ucComMF522Buf, pData, 16);
    for (i=0; i<16; i++)
    {    ucComMF522Buf[i] = *(pData+i);   }
    CalulateCRC(ucComMF522Buf,16,&ucComMF522Buf[16]);

    status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,18,ucComMF522Buf,&unLen);
    if ((status != MI_OK) || (unLen != 4) || ((ucComMF522Buf[0] & 0x0F) != 0x0A))
    {   status = MI_ERR;   }
  }

  return status;
}

////////////////////////////////////////////////////////////////////////////////
//��    �ܣ��ۿ�ͳ�ֵ
//����˵��: dd_mode[IN]��������
//               0xC0 = �ۿ�
//               0xC1 = ��ֵ
//          addr[IN]��Ǯ����ַ
//          pValue[IN]��4�ֽ���(��)ֵ����λ��ǰ
//��    ��: �ɹ�����MI_OK
////////////////////////////////////////////////////////////////////////////////
char PcdValue(unsigned char dd_mode,unsigned char addr,unsigned char *pValue)
{
  char status;
  unsigned int  unLen;
  unsigned char i,ucComMF522Buf[MAXRLEN];

  ucComMF522Buf[0] = dd_mode;
  ucComMF522Buf[1] = addr;
  CalulateCRC(ucComMF522Buf,2,&ucComMF522Buf[2]);

  status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,4,ucComMF522Buf,&unLen);

  if ((status != MI_OK) || (unLen != 4) || ((ucComMF522Buf[0] & 0x0F) != 0x0A))
  {   status = MI_ERR;   }

  if (status == MI_OK)
  {
    // memcpy(ucComMF522Buf, pValue, 4);
    for (i=0; i<16; i++)
    {    ucComMF522Buf[i] = *(pValue+i);   }
    CalulateCRC(ucComMF522Buf,4,&ucComMF522Buf[4]);
    unLen = 0;
    status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,6,ucComMF522Buf,&unLen);
    if (status != MI_ERR)
    {    status = MI_OK;    }
  }

  if (status == MI_OK)
  {
    ucComMF522Buf[0] = PICC_TRANSFER;
    ucComMF522Buf[1] = addr;
    CalulateCRC(ucComMF522Buf,2,&ucComMF522Buf[2]);

    status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,4,ucComMF522Buf,&unLen);

    if ((status != MI_OK) || (unLen != 4) || ((ucComMF522Buf[0] & 0x0F) != 0x0A))
    {   status = MI_ERR;   }
  }
  return status;
}

////////////////////////////////////////////////////////////////////////////////
//��    �ܣ�����Ǯ��
//����˵��: sourceaddr[IN]��Դ��ַ
//          goaladdr[IN]��Ŀ���ַ
//��    ��: �ɹ�����MI_OK
////////////////////////////////////////////////////////////////////////////////
char PcdBakValue(unsigned char sourceaddr, unsigned char goaladdr)
{
  char status;
  unsigned int  unLen;
  unsigned char ucComMF522Buf[MAXRLEN];

  ucComMF522Buf[0] = PICC_RESTORE;
  ucComMF522Buf[1] = sourceaddr;
  CalulateCRC(ucComMF522Buf,2,&ucComMF522Buf[2]);

  status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,4,ucComMF522Buf,&unLen);

  if ((status != MI_OK) || (unLen != 4) || ((ucComMF522Buf[0] & 0x0F) != 0x0A))
  {   status = MI_ERR;   }

  if (status == MI_OK)
  {
    ucComMF522Buf[0] = 0;
    ucComMF522Buf[1] = 0;
    ucComMF522Buf[2] = 0;
    ucComMF522Buf[3] = 0;
    CalulateCRC(ucComMF522Buf,4,&ucComMF522Buf[4]);

    status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,6,ucComMF522Buf,&unLen);
    if (status != MI_ERR)
    {    status = MI_OK;    }
  }

  if (status != MI_OK)
  { return MI_ERR;   }

  ucComMF522Buf[0] = PICC_TRANSFER;
  ucComMF522Buf[1] = goaladdr;

  CalulateCRC(ucComMF522Buf,2,&ucComMF522Buf[2]);

  status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,4,ucComMF522Buf,&unLen);

  if ((status != MI_OK) || (unLen != 4) || ((ucComMF522Buf[0] & 0x0F) != 0x0A))
  {   status = MI_ERR;   }

  return status;
}


////////////////////////////////////////////////////////////////////////////////
//��    �ܣ����Ƭ��������״̬
//��    ��: �ɹ����� MI_OK
////////////////////////////////////////////////////////////////////////////////
char PcdHalt(void)
{
  char status;
  unsigned int  unLen;
  unsigned char ucComMF522Buf[MAXRLEN];

  ucComMF522Buf[0] = PICC_HALT;
  ucComMF522Buf[1] = 0;
  CalulateCRC(ucComMF522Buf,2,&ucComMF522Buf[2]);

  status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,4,ucComMF522Buf,&unLen);

  return MI_OK;
}

////////////////////////////////////////////////////////////////////////////////
//�� MFRC522 ���� CRC16 ����
////////////////////////////////////////////////////////////////////////////////
void CalulateCRC(unsigned char *pIndata,unsigned char len,unsigned char *pOutData)
{
  unsigned char i,n;
  ClearBitMask(DivIrqReg,0x04);
  WriteRawRC(CommandReg,PCD_IDLE);
  SetBitMask(FIFOLevelReg,0x80);
  for (i=0; i<len; i++)
  {   WriteRawRC(FIFODataReg, *(pIndata+i));   }
  WriteRawRC(CommandReg, PCD_CALCCRC);
  i = 0xFF;
  do
  {
    n = ReadRawRC(DivIrqReg);
    i--;
  }
  while ((i!=0) && !(n&0x04));
  pOutData[0] = ReadRawRC(CRCResultRegL);
  pOutData[1] = ReadRawRC(CRCResultRegM);
}

////////////////////////////////////////////////////////////////////////////////
//��    �ܣ���λ MFRC522
//��    ��: �ɹ����� MI_OK
////////////////////////////////////////////////////////////////////////////////
char PcdReset(void)
{
  volatile unsigned char i;

  MFRC522_RST_HIGH();
  for(i=0;i<127;i++)  __NOP();

  MFRC522_RST_LOW();
  for(i=0;i<255;i++)  __NOP();
  for(i=0;i<255;i++)  __NOP();

  MFRC522_RST_HIGH();
  for(i=0;i<255;i++)  __NOP();
  for(i=0;i<255;i++)  __NOP();
  for(i=0;i<255;i++)  __NOP();

//  WriteRawRC(CommandReg,PCD_RESETPHASE);
//  for(i=0;i<255;i++)  __NOP();
//  for(i=0;i<255;i++)  __NOP();
//  for(i=0;i<255;i++)  __NOP();

  WriteRawRC(ModeReg,0x3D);            // ��Mifare��ͨѶ��CRC��ʼֵ0x6363
  WriteRawRC(TReloadRegL,30);
  WriteRawRC(TReloadRegH,0);
  WriteRawRC(TModeReg,0x8D);
  WriteRawRC(TPrescalerReg,0x3E);
  WriteRawRC(TxAutoReg,0x40);
#ifdef USED_MFRC522_PA
  /* �����������棬����翹�Ȳ�����ʹ MFRC522-PA ģ���и�Զ�Ķ�д������ */
  /* MFRC522-PA, https://item.taobao.com/item.htm?id=552002236051       */
  /* ���²������ܲ����������������Ƶ�ģ�飬������Ϊ�ο�                 */
  /* ����Ϊʲô������������ο� MFRC522 оƬ�ֲ��Լ� NXP ������ĵ�     */
  WriteRawRC(RFCfgReg, 0x50);       // RxGain = 38dB
  //WriteRawRC(RFCfgReg, 0x70);       // RxGain = 48dB
  WriteRawRC(GsNReg, 0xF4);         // default = 0x88
  WriteRawRC(CWGsCfgReg, 0x3F);     // default = 0x20
  WriteRawRC(ModGsCfgReg, 0x11);    // default = 0x20
#endif
  return MI_OK;
}

////////////////////////////////////////////////////////////////////////////////
//��    �ܣ��� MFRC522 �Ĵ���
//����˵����Address[IN]:�Ĵ�����ַ
//��    �أ�������ֵ
////////////////////////////////////////////////////////////////////////////////
unsigned char ReadRawRC(unsigned char Address)
{
  unsigned char i, ucAddr;
  unsigned short retry;

  MFRC522_NSS_LOW();
  __NOP();  __NOP();
  __NOP();  __NOP();
//  for(i=0;i<127;i++)  __NOP();

  retry = 0;
  /* Loop while DR register in not emplty */
  while((SSP1->SR & SSP_FLAG_TNF) == RESET);
//  while((SSP1->SR & SSP_FLAG_TFE) == RESET);
//  {
//    retry++;
//    if(retry>40000){
//      return 0;
//    }
//  }
  SSP1->DR = ((Address<<1)&0x7E)|0x80;

  retry = 0;
  /* Wait to receive a byte */
  // while (SSP_GetFlagStatus (SSP1, SSP_FLAG_RNE) == RESET)
//  while((SSP1->SR & SSP_FLAG_TNF) == RESET);
  while((SSP1->SR & SSP_FLAG_RNE) == RESET);
//  {
//    retry++;
//    if(retry>40000){
//      return 0;
//    }
//  }
  i = SSP1->DR;

  retry=0;
  /* Loop while DR register in not emplty */
  while((SSP1->SR & SSP_FLAG_TNF) == RESET);
//  while((SSP1->SR & SSP_FLAG_TNF) == RESET);
//  {
//    retry++;
//    if(retry>40000){
//      return 0;
//    }
//  }
  SSP1->DR = 0x00;

  retry=0;
  /* Wait to receive a byte */
  // while (SSP_GetFlagStatus (SSP1, SSP_FLAG_RNE) == RESET)
//  while((SSP1->SR & SSP_FLAG_TNF) == RESET);
  while((SSP1->SR & SSP_FLAG_RNE) == RESET);
//  {
//    retry++;
//    if(retry>40000){
//      return 0;
//    }
//  }

//  for(i=0;i<127;i++)  __NOP();
  __NOP();  __NOP();
  __NOP();  __NOP();
  i = SSP1->DR;
  MFRC522_NSS_HIGH();
  return i;
}

////////////////////////////////////////////////////////////////////////////////
//��    �ܣ�д MFRC522 �Ĵ���
//����˵����Address[IN]:�Ĵ�����ַ
//          value[IN]:д���ֵ
////////////////////////////////////////////////////////////////////////////////
void WriteRawRC(unsigned char Address, unsigned char value)
{
  unsigned char i, ucAddr;
  unsigned short retry=0;

  MFRC522_NSS_LOW();
  __NOP();  __NOP();
  __NOP();  __NOP();
//  for(i=0;i<127;i++)  __NOP();
  /* Loop while DR register in not emplty */
  while((SSP1->SR & SSP_FLAG_TNF) == RESET);
//  while((SSP1->SR & SSP_FLAG_TFE) == RESET);
//  {
//    retry++;
//    if(retry>40000){
//      break;
//    }
//  }
  SSP1->DR = ((Address<<1)&0x7E);
  retry=0;
//  while((SSP1->SR & SSP_FLAG_BSY) == SSP_FLAG_BSY);
  while((SSP1->SR & SSP_FLAG_RNE) == RESET);
//  {
//    retry++;
//    if(retry>40000){
//      break;
//    }
//  }
  i = SSP1->DR;

  while((SSP1->SR & SSP_FLAG_TNF) == RESET);
  SSP1->DR = value;
//  retry=0;
//  while((SSP1->SR & SSP_FLAG_TNF) == RESET)
//  {
//    retry++;
//    if(retry>40000){
//      break;
//    }
//  }
  retry=0;
//  while((SSP1->SR & SSP_FLAG_BSY) == SSP_FLAG_BSY);
  while((SSP1->SR & SSP_FLAG_RNE) == RESET);
//  while((SSP1->SR & SSP_FLAG_TFE) == RESET);
//  {
//    retry++;
//    if(retry>40000){
//      break;
//    }
//  }
  i = SSP1->DR;

//  for(i=0;i<127;i++)  __NOP();
  __NOP();  __NOP();
  __NOP();  __NOP();
  MFRC522_NSS_HIGH();

//  MF522_SCK = 0;
//  MF522_NSS = 0;
//  ucAddr = ((Address<<1)&0x7E);

//  for(i=8;i>0;i--)
//  {
//    MF522_SI = ((ucAddr&0x80)==0x80);
//    MF522_SCK = 1;
//    ucAddr <<= 1;
//    MF522_SCK = 0;
//  }

//  for(i=8;i>0;i--)
//  {
//    MF522_SI = ((value&0x80)==0x80);
//    MF522_SCK = 1;
//    value <<= 1;
//    MF522_SCK = 0;
//  }
//  MF522_NSS = 1;
//  MF522_SCK = 1;
}

////////////////////////////////////////////////////////////////////////////////
//��    �ܣ��� MFRC522 �Ĵ���λ
//����˵����reg[IN]:�Ĵ�����ַ
//          mask[IN]:��λֵ
////////////////////////////////////////////////////////////////////////////////
void SetBitMask(unsigned char reg,unsigned char mask)
{
  char tmp = 0x0;
  tmp = ReadRawRC(reg);
  WriteRawRC(reg,tmp | mask);  // set bit mask
}

////////////////////////////////////////////////////////////////////////////////
//��    �ܣ��� MFRC522 �Ĵ���λ
//����˵����reg[IN]:�Ĵ�����ַ
//          mask[IN]:��λֵ
////////////////////////////////////////////////////////////////////////////////
void ClearBitMask(unsigned char reg,unsigned char mask)
{
  char tmp = 0x0;
  tmp = ReadRawRC(reg);
  WriteRawRC(reg, tmp & ~mask);  // clear bit mask
}

////////////////////////////////////////////////////////////////////////////////
//��    �ܣ�ͨ��RC522��ISO14443��ͨѶ
//����˵����Command[IN]: MFRC522 ������
//          pInData[IN]: ͨ�� MFRC522 ���͵���Ƭ������
//          InLenByte[IN]: �������ݵ��ֽڳ���
//          pOutData[OUT]: ���յ��Ŀ�Ƭ��������
//          *pOutLenBit[OUT]: �������ݵ�λ����
////////////////////////////////////////////////////////////////////////////////
char PcdComMF522(unsigned char Command,
                 unsigned char *pInData,
                 unsigned char InLenByte,
                 unsigned char *pOutData,
                 unsigned int  *pOutLenBit)
{
  char status = MI_ERR;
  unsigned char irqEn   = 0x00;
  unsigned char waitFor = 0x00;
  unsigned char lastBits;
  unsigned char n;
  unsigned long int i;

  switch (Command)
  {
    case PCD_AUTHENT:
      irqEn   = 0x12;
      waitFor = 0x10;
      break;
    case PCD_TRANSCEIVE:
      irqEn   = 0x77;
      waitFor = 0x30;
      break;
    default:
      break;
  }

  WriteRawRC(ComIEnReg,irqEn|0x80);
  ClearBitMask(ComIrqReg,0x80);
  WriteRawRC(CommandReg,PCD_IDLE);
  SetBitMask(FIFOLevelReg,0x80);

  for (i=0; i<InLenByte; i++)
  {   WriteRawRC(FIFODataReg, pInData[i]);    }
  WriteRawRC(CommandReg, Command);

  if (Command == PCD_TRANSCEIVE)
  {    SetBitMask(BitFramingReg,0x80);  }

//  i = 600;//����ʱ��Ƶ�ʵ���������M1�����ȴ�ʱ��25ms
  i = 600000;//����ʱ��Ƶ�ʵ���������M1�����ȴ�ʱ��25ms
  do
  {
    n = ReadRawRC(ComIrqReg);
    i--;
  } while ((i!=0) && !(n&0x01) && !(n&waitFor));

  ClearBitMask(BitFramingReg,0x80);

  if (i!=0)
  {
    if(!(ReadRawRC(ErrorReg)&0x1B))
    {
      status = MI_OK;
      if (n & irqEn & 0x01)
      {   status = MI_NOTAGERR;   }
      if (Command == PCD_TRANSCEIVE)
      {
        n = ReadRawRC(FIFOLevelReg);
        lastBits = ReadRawRC(ControlReg) & 0x07;
        if (lastBits)
        {   *pOutLenBit = (n-1)*8 + lastBits;   }
        else
        {   *pOutLenBit = n*8;   }
        if (n == 0)
        {   n = 1;    }
        if (n > MAXRLEN)
        {   n = MAXRLEN;   }
        for (i=0; i<n; i++)
        {   pOutData[i] = ReadRawRC(FIFODataReg);    }
      }
    }
    else
    {   status = MI_ERR;   }

 }

  SetBitMask(ControlReg,0x80);           // stop timer now
  WriteRawRC(CommandReg,PCD_IDLE);
  return status;
}

////////////////////////////////////////////////////////////////////////////////
//��������
//ÿ��������ر����߷���֮��Ӧ������1ms�ļ��
////////////////////////////////////////////////////////////////////////////////
void PcdAntennaOn()
{
  unsigned char i;
  i = ReadRawRC(TxControlReg);
  if (!(i & 0x03))
  {
    SetBitMask(TxControlReg, 0x03);
  }
}

////////////////////////////////////////////////////////////////////////////////
//�ر�����
////////////////////////////////////////////////////////////////////////////////
void PcdAntennaOff()
{
  ClearBitMask(TxControlReg, 0x03);
}