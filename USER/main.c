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
#include "usart3.h"
#include "gps.h"

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

u8 USART1_TX_BUF[USART3_MAX_RECV_LEN]; 					//����1,���ͻ�����
nmea_msg gpsx; 											//GPS��Ϣ
__align(4) u8 dtbuf[50];   								//��ӡ������
const u8*fixmode_tbl[4]={"Fail","Fail"," 2D "," 3D "};	//fix mode�ַ��� 
	  


//��ʾGPS��λ��Ϣ 
void Gps_Msg_Show(void)
{
 	float tp;		   
	//POINT_COLOR=BLUE;  	 
	tp=gpsx.longitude;	   
	sprintf((char *)dtbuf,"Longitude:%.5f %1c   ",tp/=100000,gpsx.ewhemi);	//�õ������ַ���
 	//LCD_ShowString(30,120,200,16,16,dtbuf);	 	   
	tp=gpsx.latitude;	   
	sprintf((char *)dtbuf,"Latitude:%.5f %1c   ",tp/=100000,gpsx.nshemi);	//�õ�γ���ַ���
 	//LCD_ShowString(30,140,200,16,16,dtbuf);	 	 
	tp=gpsx.altitude;	   
 	sprintf((char *)dtbuf,"Altitude:%.1fm     ",tp/=10);	    			//�õ��߶��ַ���
 	//LCD_ShowString(30,160,200,16,16,dtbuf);	 			   
	tp=gpsx.speed;	   
 	sprintf((char *)dtbuf,"Speed:%.3fkm/h     ",tp/=1000);		    		//�õ��ٶ��ַ���	 
 //	LCD_ShowString(30,180,200,16,16,dtbuf);	 				    
	if(gpsx.fixmode<=3)														//��λ״̬
	{  
		sprintf((char *)dtbuf,"Fix Mode:%s",fixmode_tbl[gpsx.fixmode]);	
	  //LCD_ShowString(30,200,200,16,16,dtbuf);			   
	}	 	   
	sprintf((char *)dtbuf,"GPS+BD Valid satellite:%02d",gpsx.posslnum);	 		//���ڶ�λ��GPS������
 //	LCD_ShowString(30,220,200,16,16,dtbuf);	    
	sprintf((char *)dtbuf,"GPS Visible satellite:%02d",gpsx.svnum%100);	 		//�ɼ�GPS������
 //	LCD_ShowString(30,240,200,16,16,dtbuf);
	
	sprintf((char *)dtbuf,"BD Visible satellite:%02d",gpsx.beidou_svnum%100);	 		//�ɼ�����������
 	//LCD_ShowString(30,260,200,16,16,dtbuf);
	
	sprintf((char *)dtbuf,"UTC Date:%04d/%02d/%02d   ",gpsx.utc.year,gpsx.utc.month,gpsx.utc.date);	//��ʾUTC����
	//LCD_ShowString(30,280,200,16,16,dtbuf);		    
	sprintf((char *)dtbuf,"UTC Time:%02d:%02d:%02d   ",gpsx.utc.hour,gpsx.utc.min,gpsx.utc.sec);	//��ʾUTCʱ��
  //LCD_ShowString(30,300,200,16,16,dtbuf);		  
}

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
	u16 i=0,rxlen;
	u16 lenx;
	u32 TIME_Check=0;
 	u32 total,free;
	u8 keypress=0;
	u8 res=0;	
	u8 upload=0; 
	fp = &fileTXT;
	u8 GPS_Save_File=0;
	FRESULT res_sd;
	
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);//����ϵͳ�ж����ȼ�����2
	delay_init(168);  //��ʼ����ʱ����
	uart_init(115200);		//��ʼ�����ڲ�����Ϊ115200
	LED_Init();					//��ʼ��LED 
	usmart_dev.init(84);		//��ʼ��USMART
	usart3_init(38400);			//��ʼ������3������Ϊ38400  ->FSMCD13  FSMC_D14
 	//LCD_Init();					//LCD��ʼ��  
 	KEY_Init();					//������ʼ�� 
	RS485_Init(bound);				//��ʼ��485
	W25QXX_Init();				//��ʼ��W25Q128
	my_mem_init(SRAMIN);		//��ʼ���ڲ��ڴ�� 
	my_mem_init(SRAMCCM);		//��ʼ��CCM�ڴ��
	
 	while(SD_Init())//��ⲻ��SD��
	{
		//LCD_ShowString(30,150,200,16,16,"SD Card Error!");
		delay_ms(500);					
		//LCD_ShowString(30,150,200,16,16,"Please Check! ");
		delay_ms(500);
		LED0=!LED0;//DS0��˸
	}
 	exfuns_init();							//Ϊfatfs��ر��������ڴ�				 
  f_mount(fs[0],"0:",1); 					//����SD�� 
 	res=f_mount(fs[1],"1:",1); 				//����FLASH.	
	if(res==0X0D)//FLASH����,FAT�ļ�ϵͳ����,���¸�ʽ��FLASH
	{
		//LCD_ShowString(30,150,200,16,16,"Flash Disk Formatting...");	//��ʽ��FLASH
		res=f_mkfs("1:",1,4096);//��ʽ��FLASH,1,�̷�;1,����Ҫ������,8������Ϊ1����
		if(res==0)
		{
			f_setlabel((const TCHAR *)"1:ALIENTEK");	//����Flash���̵�����Ϊ��ALIENTEK
			//LCD_ShowString(30,150,200,16,16,"Flash Disk Format Finish");	//��ʽ�����
		}
//		else 
//			LCD_ShowString(30,150,200,16,16,"Flash Disk Format Error ");	//��ʽ��ʧ��
		delay_ms(1000);
	}													    
  
	while(exf_getfree("0",&total,&free))	//�õ�SD������������ʣ������
	{
		delay_ms(200);
		LED0=!LED0;//DS0��˸
	}													  			    

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
						if(upload)
								upload = 0;
						else 
								upload = 1;
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
			if(TIME_Check>=60)
			{
				TIME_Check =0;
				ReadMeteorVal();
			}
			RS485_Receive_Data(ReceiveBuf,&ReceiveBufCounter);
			delay_ms(20);
			if(ReceiveBufCounter)//���յ�������
			{
				if(GPS_Save_File)
				{
					GPS_Save_File = 0;
					PutData2TXT(USART1_TX_BUF,rxlen);  /*����GPS����*/
					LED1 = 0;
				}
				
				PutData2TXT(ReceiveBuf,ReceiveBufCounter);
				LED0=0;
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
				LED1=1;
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
		/*******************GPS����**********************/
		if(USART3_RX_STA&0X8000)		//���յ�һ��������
		{
			rxlen=USART3_RX_STA&0X7FFF;	//�õ����ݳ���
			for(i=0;i<rxlen;i++)USART1_TX_BUF[i]=USART3_RX_BUF[i];	   
 			USART3_RX_STA=0;		   	//������һ�ν���
			USART1_TX_BUF[i]=0;			//�Զ���ӽ�����
			//GPS_Analysis(&gpsx,(u8*)USART1_TX_BUF);//�����ַ���
			//Gps_Msg_Show();				//��ʾ��Ϣ	
			GPS_Save_File = 1;
			if(upload)printf("\r\n%s\r\n",USART1_TX_BUF);//���ͽ��յ������ݵ�����1
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



