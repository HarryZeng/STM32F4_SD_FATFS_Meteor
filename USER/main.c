#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "led.h"
#include "lcd.h"
#include "key.h"  
#include "sram.h"   
#include "malloc.h" 
#include "usmart.h"  
#include "sdio_sdcard.h"    
#include "malloc.h" 
#include "w25qxx.h"    
#include "ff.h"  
#include "exfuns.h"    
#include "rs485.h" 
#include <Meteorological.h>


//ALIENTEK ̽����STM32F407������ ʵ��39
//FATFS ʵ�� -�⺯���汾
//����֧�֣�www.openedv.com
//�Ա����̣�http://eboard.taobao.com
//������������ӿƼ����޹�˾    
//���ߣ�����ԭ�� @ALIENTEK 
 
 
#define  WORK_MODE  1//0->�����豸��1->�˷����豸
 
FIL   	fileTXT;
FIL* 		fp;
BYTE 			buffer[]="hello world!";//	д������
BYTE 			buffer2[]="";//	д������
BYTE			TXTdata2UartBuffer[2000];

void PutTXTdata2Uart(void);
void PutTXTdata2TFCardTest(void);

void PutData2TXT(u8 *databuffer,uint16_t length);
u8 			 ReceiveBuf[1000]; 
uint16_t  ReceiveBufCounter; 
uint32_t SyncTimeCounter=0;
#if (WORK_MODE==0)
uint8_t WorkInFlag=0;
uint8_t ReadyFlag=1;
u32 bound=4800;
#else
uint8_t WorkInFlag=1;
uint8_t ReadyFlag=0;
u32 bound=115200;
#endif


/*
1-����
2-��ʵ����
3-����
4-��ʵ����
5-�¶�
6-ʪ��
7-����ѹ
8-¶���¶�
*/
int main(void)
{        
	u32 TIME_Check=0;
 	u32 total,free;
	u8 keypress=0;
	u8 res=0;	
	int i=0;
	fp = &fileTXT;
	FRESULT res_sd;
	
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);//����ϵͳ�ж����ȼ�����2
	delay_init(168);  //��ʼ����ʱ����
	uart_init(115200);		//��ʼ�����ڲ�����Ϊ115200
	LED_Init();					//��ʼ��LED 
	usmart_dev.init(84);		//��ʼ��USMART
 	LCD_Init();					//LCD��ʼ��  
 	KEY_Init();					//������ʼ�� 
	RS485_Init(bound);				//��ʼ��485
	W25QXX_Init();				//��ʼ��W25Q128
	my_mem_init(SRAMIN);		//��ʼ���ڲ��ڴ�� 
	my_mem_init(SRAMCCM);		//��ʼ��CCM�ڴ��
	
 	POINT_COLOR=RED;//��������Ϊ��ɫ 
	LCD_ShowString(30,50,200,16,16,"Explorer STM32F4");	
	LCD_ShowString(30,70,200,16,16,"FATFS TEST");	
	LCD_ShowString(30,90,200,16,16,"ATOM@ALIENTEK");
	LCD_ShowString(30,110,200,16,16,"2014/5/15");   
	LCD_ShowString(30,130,200,16,16,"Use USMART for test");	   
 	while(SD_Init())//��ⲻ��SD��
	{
		LCD_ShowString(30,150,200,16,16,"SD Card Error!");
		delay_ms(500);					
		LCD_ShowString(30,150,200,16,16,"Please Check! ");
		delay_ms(500);
		LED0=!LED0;//DS0��˸
	}
 	exfuns_init();							//Ϊfatfs��ر��������ڴ�				 
  	f_mount(fs[0],"0:",1); 					//����SD�� 
 	res=f_mount(fs[1],"1:",1); 				//����FLASH.	
	if(res==0X0D)//FLASH����,FAT�ļ�ϵͳ����,���¸�ʽ��FLASH
	{
		LCD_ShowString(30,150,200,16,16,"Flash Disk Formatting...");	//��ʽ��FLASH
		res=f_mkfs("1:",1,4096);//��ʽ��FLASH,1,�̷�;1,����Ҫ������,8������Ϊ1����
		if(res==0)
		{
			f_setlabel((const TCHAR *)"1:ALIENTEK");	//����Flash���̵�����Ϊ��ALIENTEK
			LCD_ShowString(30,150,200,16,16,"Flash Disk Format Finish");	//��ʽ�����
		}else LCD_ShowString(30,150,200,16,16,"Flash Disk Format Error ");	//��ʽ��ʧ��
		delay_ms(1000);
	}													    
	LCD_Fill(30,150,240,150+16,WHITE);		//�����ʾ			  
	while(exf_getfree("0",&total,&free))	//�õ�SD������������ʣ������
	{
		LCD_ShowString(30,150,200,16,16,"SD Card Fatfs Error!");
		delay_ms(200);
		LCD_Fill(30,150,240,150+16,WHITE);	//�����ʾ			  
		delay_ms(200);
		LED0=!LED0;//DS0��˸
	}													  			    
 	POINT_COLOR=BLUE;//��������Ϊ��ɫ	   
	LCD_ShowString(30,150,200,16,16,"FATFS OK!");	 
	LCD_ShowString(30,170,200,16,16,"SD Total Size:     MB");	 
	LCD_ShowString(30,190,200,16,16,"SD  Free Size:     MB"); 	    
 	LCD_ShowNum(30+8*14,170,total>>10,5,16);				//��ʾSD�������� MB
 	LCD_ShowNum(30+8*14,190,free>>10,5,16);					//��ʾSD��ʣ������ MB			
	
	printf("Press WKUP_PRES to PutTXTdata2TFCardTest \r\n");
	printf("Press KEY0_PRES to PutTXTdata2Uart \r\n");
	printf("Press KEY1_PRES to RS485 bound set 115200; \r\n");
	printf("Press KEY2_PRES to RS485 bound set 4800; \r\n");
	
	res_sd = f_open(fp, "0:/ReceiveData.txt", FA_READ|FA_WRITE|FA_OPEN_ALWAYS);
	if (res_sd==FR_OK) {
		printf("Open file successfully\r\n");
		printf("FATFS OK!\r\n");
		printf("SD Total Size:%d 	MB\r\n",total>>10);
		printf("SD  Free Size:%d	MB\r\n",free>>10);
	}
	else
	{
		printf("Open file failed\r\n");
	}

	res_sd = f_lseek(fp,fp->fsize);  
	
	delay_ms(5);
	
	LED0 = 0;

	while(1)
	{
		keypress = KEY_Scan(0);
		if(keypress)
		{						   
			switch(keypress)
			{				 
				case WKUP_PRES:
//							WorkInFlag = 0;
//							ReadyFlag = 0;
//							printf("Work In Nuclear Station -- WorkFlag:%d \r\n",WorkInFlag);
					//PutTXTdata2TFCardTest();
					break;
				case KEY0_PRES:
//							WorkInFlag = 1;
//							ReadyFlag = 1;
//							printf("Work In Meteor Station -- WorkFlag:%d \r\n",WorkInFlag);
					//PutTXTdata2Uart();
					break;
				case KEY1_PRES:
//					WorkInFlag=1;
//					ReadyFlag=0;
					RS485_Init(115200);				//��ʼ��485
					printf("Set Mode Bound to 115200\r\n");
					break;
				case KEY2_PRES:
//					WorkInFlag=1;
//					ReadyFlag=0;
					RS485_Init(4800);				//��ʼ��485
					printf("Set Mode Bound to 4800\r\n");
					break;
			}
		}
		/*****************�˷����豸ͨѶ---����*******************/
		if(WorkInFlag==1&&ReadyFlag==0)   
		{
			//if(1)
			if(CheckCommunication()==true)
			{
				printf("Communication is OK\r\n");
				printf("Ready to read...");
				StartMea();
				delay_ms(10);
				ReadyFlag = 1;
				LED0 = 1;
			}
			delay_ms(1000);
		}
		/*****************�˷����豸����---��ȡ*******************/
		else if(WorkInFlag==1&&ReadyFlag==1)
		{
				TIME_Check++;
			if(TIME_Check>=50)
			{
				TIME_Check =0;
				ReadMeteorVal();
			}
			delay_ms(20);
			RS485_Receive_Data(ReceiveBuf,&ReceiveBufCounter);
			if(ReceiveBufCounter)//���յ�������
			{
				LED0=0;
				
				PutData2TXT(ReceiveBuf,ReceiveBufCounter);
				
				/**���㻺������**/
				for(i=0;i<ReceiveBufCounter;i++)
				{
					ReceiveBuf[i] = 0;
				}
				ReceiveBufCounter = 0;
			}	
			else
			{
				LED0=1;
			}			
		}
		/*****************�����豸����----��ȡ******************/
		else if(WorkInFlag==0)
		{
			RS485_Receive_Data(ReceiveBuf,&ReceiveBufCounter);
			if(ReceiveBufCounter)//���յ�������
			{
				SyncTimeCounter = 0;
				LED0=0;
				
				PutData2TXT(ReceiveBuf,ReceiveBufCounter);
				
				/**���㻺������**/
				for(i=0;i<ReceiveBufCounter;i++)
				{
					ReceiveBuf[i] = 0;
				}
				ReceiveBufCounter = 0;
			}
			else
			{
				LED0=1;
			}
		}
	} 
}
uint32_t ByteCounter=0;
/*******************************************
����ȡ�������ݱ��浽SD����
*******************************************/
void PutData2TXT(u8 *databuffer,uint16_t length)
{
	FRESULT res_sd;

	res_sd = f_write(fp, databuffer, ReceiveBufCounter, &bw);
	if (res_sd == FR_OK)
	{
		f_sync(fp);
		if (res_sd==FR_OK) 
		{
				ByteCounter = ByteCounter+bw;
				printf("%8d\r\n",ByteCounter);
		}
	}	
}

/*******************************************
��SD���TXT��ȡ������ͨ�����ڴ�ӡ��ȥ�����ֻ�ܴ�ӡ1000���ֽ�
*******************************************/
void PutTXTdata2Uart(void)
{
	FRESULT res_sd;
	UINT fnum;
	f_close(fp);
	printf("\r\n******************Read TXT data*****************\r\n");
	res_sd = f_open(fp, "0:/ReceiveData.txt", FA_READ|FA_OPEN_EXISTING);
	if (res_sd == FR_OK) {
		printf("Open file successfully,ready to read...\r\n");
	}
	res_sd = f_read(fp, TXTdata2UartBuffer, sizeof(TXTdata2UartBuffer), &fnum);
	if (res_sd==FR_OK) {
		printf("Reveive Data Counter:%d\r\n",fnum);
		printf("Data:\r\n %s \r\n", TXTdata2UartBuffer);
	}
	else
	{
		printf("Read file failed\r\n");
	}
	f_close(fp);
	if (res_sd==FR_OK) {
		printf("close file successfully\r\n");
	}
	else
	{
		printf("close file failed\r\n");
	}
}

/*******************************************
����SD������SD������д��helloworld ����
*******************************************/
void PutTXTdata2TFCardTest(void)
{
	FRESULT res_sd;
	u32 total,free;
	int fputsCounter;
	
	exf_getfree("0",&total,&free);
	
	delay_ms(200);
	res_sd = f_open(fp, "0:/ReceiveData.txt", FA_READ|FA_WRITE|FA_OPEN_ALWAYS);
	if (res_sd==FR_OK) {
		printf("Open file successfully\r\n");
		printf("FATFS OK!\r\n");
		printf("SD Total Size:%d 	MB\r\n",total>>10);
		printf("SD  Free Size:%d	MB\r\n",free>>10);
	}
	else
	{
		printf("Open file failed\r\n");
	}
	delay_ms(200);
	fputsCounter = f_puts((char *)buffer2,fp); 
	
	printf("write data %d \r\n",fputsCounter);

	res_sd = f_close(fp);
	if (res_sd==FR_OK) {
		printf("close file successfully\r\n");
	}
	else
	{
		printf("close file failed\r\n");
	}
}



