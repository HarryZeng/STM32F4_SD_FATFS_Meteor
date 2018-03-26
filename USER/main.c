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


//ALIENTEK 探索者STM32F407开发板 实验39
//FATFS 实验 -库函数版本
//技术支持：www.openedv.com
//淘宝店铺：http://eboard.taobao.com
//广州市星翼电子科技有限公司    
//作者：正点原子 @ALIENTEK 
 
 
#define  WORK_MODE  1//0->气象设备，1->核辐射设备
 
FIL   	fileTXT;
FIL* 		fp;
BYTE 			buffer[]="hello world!";//	写入数据
BYTE 			buffer2[]="";//	写入数据
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
1-风向
2-真实风向
3-风速
4-真实风速
5-温度
6-湿度
7-大气压
8-露点温度
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
	
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);//设置系统中断优先级分组2
	delay_init(168);  //初始化延时函数
	uart_init(115200);		//初始化串口波特率为115200
	LED_Init();					//初始化LED 
	usmart_dev.init(84);		//初始化USMART
 	LCD_Init();					//LCD初始化  
 	KEY_Init();					//按键初始化 
	RS485_Init(bound);				//初始化485
	W25QXX_Init();				//初始化W25Q128
	my_mem_init(SRAMIN);		//初始化内部内存池 
	my_mem_init(SRAMCCM);		//初始化CCM内存池
	
 	POINT_COLOR=RED;//设置字体为红色 
	LCD_ShowString(30,50,200,16,16,"Explorer STM32F4");	
	LCD_ShowString(30,70,200,16,16,"FATFS TEST");	
	LCD_ShowString(30,90,200,16,16,"ATOM@ALIENTEK");
	LCD_ShowString(30,110,200,16,16,"2014/5/15");   
	LCD_ShowString(30,130,200,16,16,"Use USMART for test");	   
 	while(SD_Init())//检测不到SD卡
	{
		LCD_ShowString(30,150,200,16,16,"SD Card Error!");
		delay_ms(500);					
		LCD_ShowString(30,150,200,16,16,"Please Check! ");
		delay_ms(500);
		LED0=!LED0;//DS0闪烁
	}
 	exfuns_init();							//为fatfs相关变量申请内存				 
  	f_mount(fs[0],"0:",1); 					//挂载SD卡 
 	res=f_mount(fs[1],"1:",1); 				//挂载FLASH.	
	if(res==0X0D)//FLASH磁盘,FAT文件系统错误,重新格式化FLASH
	{
		LCD_ShowString(30,150,200,16,16,"Flash Disk Formatting...");	//格式化FLASH
		res=f_mkfs("1:",1,4096);//格式化FLASH,1,盘符;1,不需要引导区,8个扇区为1个簇
		if(res==0)
		{
			f_setlabel((const TCHAR *)"1:ALIENTEK");	//设置Flash磁盘的名字为：ALIENTEK
			LCD_ShowString(30,150,200,16,16,"Flash Disk Format Finish");	//格式化完成
		}else LCD_ShowString(30,150,200,16,16,"Flash Disk Format Error ");	//格式化失败
		delay_ms(1000);
	}													    
	LCD_Fill(30,150,240,150+16,WHITE);		//清除显示			  
	while(exf_getfree("0",&total,&free))	//得到SD卡的总容量和剩余容量
	{
		LCD_ShowString(30,150,200,16,16,"SD Card Fatfs Error!");
		delay_ms(200);
		LCD_Fill(30,150,240,150+16,WHITE);	//清除显示			  
		delay_ms(200);
		LED0=!LED0;//DS0闪烁
	}													  			    
 	POINT_COLOR=BLUE;//设置字体为蓝色	   
	LCD_ShowString(30,150,200,16,16,"FATFS OK!");	 
	LCD_ShowString(30,170,200,16,16,"SD Total Size:     MB");	 
	LCD_ShowString(30,190,200,16,16,"SD  Free Size:     MB"); 	    
 	LCD_ShowNum(30+8*14,170,total>>10,5,16);				//显示SD卡总容量 MB
 	LCD_ShowNum(30+8*14,190,free>>10,5,16);					//显示SD卡剩余容量 MB			
	
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
					RS485_Init(115200);				//初始化485
					printf("Set Mode Bound to 115200\r\n");
					break;
				case KEY2_PRES:
//					WorkInFlag=1;
//					ReadyFlag=0;
					RS485_Init(4800);				//初始化485
					printf("Set Mode Bound to 4800\r\n");
					break;
			}
		}
		/*****************核辐射设备通讯---测试*******************/
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
		/*****************核辐射设备数据---读取*******************/
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
			if(ReceiveBufCounter)//接收到有数据
			{
				LED0=0;
				
				PutData2TXT(ReceiveBuf,ReceiveBufCounter);
				
				/**清零缓存数据**/
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
		/*****************气象设备数据----读取******************/
		else if(WorkInFlag==0)
		{
			RS485_Receive_Data(ReceiveBuf,&ReceiveBufCounter);
			if(ReceiveBufCounter)//接收到有数据
			{
				SyncTimeCounter = 0;
				LED0=0;
				
				PutData2TXT(ReceiveBuf,ReceiveBufCounter);
				
				/**清零缓存数据**/
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
将读取到的数据保存到SD卡里
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
将SD里的TXT读取出来，通过串口打印出去，最大只能打印1000个字节
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
测试SD卡，往SD卡里面写入helloworld 测试
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



