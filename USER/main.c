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

u8 USART1_TX_BUF[USART3_MAX_RECV_LEN]; 					//串口1,发送缓存区
nmea_msg gpsx; 											//GPS信息
__align(4) u8 dtbuf[50];   								//打印缓存器
const u8*fixmode_tbl[4]={"Fail","Fail"," 2D "," 3D "};	//fix mode字符串 
	  


//显示GPS定位信息 
void Gps_Msg_Show(void)
{
 	float tp;		   
	//POINT_COLOR=BLUE;  	 
	tp=gpsx.longitude;	   
	sprintf((char *)dtbuf,"Longitude:%.5f %1c   ",tp/=100000,gpsx.ewhemi);	//得到经度字符串
 	//LCD_ShowString(30,120,200,16,16,dtbuf);	 	   
	tp=gpsx.latitude;	   
	sprintf((char *)dtbuf,"Latitude:%.5f %1c   ",tp/=100000,gpsx.nshemi);	//得到纬度字符串
 	//LCD_ShowString(30,140,200,16,16,dtbuf);	 	 
	tp=gpsx.altitude;	   
 	sprintf((char *)dtbuf,"Altitude:%.1fm     ",tp/=10);	    			//得到高度字符串
 	//LCD_ShowString(30,160,200,16,16,dtbuf);	 			   
	tp=gpsx.speed;	   
 	sprintf((char *)dtbuf,"Speed:%.3fkm/h     ",tp/=1000);		    		//得到速度字符串	 
 //	LCD_ShowString(30,180,200,16,16,dtbuf);	 				    
	if(gpsx.fixmode<=3)														//定位状态
	{  
		sprintf((char *)dtbuf,"Fix Mode:%s",fixmode_tbl[gpsx.fixmode]);	
	  //LCD_ShowString(30,200,200,16,16,dtbuf);			   
	}	 	   
	sprintf((char *)dtbuf,"GPS+BD Valid satellite:%02d",gpsx.posslnum);	 		//用于定位的GPS卫星数
 //	LCD_ShowString(30,220,200,16,16,dtbuf);	    
	sprintf((char *)dtbuf,"GPS Visible satellite:%02d",gpsx.svnum%100);	 		//可见GPS卫星数
 //	LCD_ShowString(30,240,200,16,16,dtbuf);
	
	sprintf((char *)dtbuf,"BD Visible satellite:%02d",gpsx.beidou_svnum%100);	 		//可见北斗卫星数
 	//LCD_ShowString(30,260,200,16,16,dtbuf);
	
	sprintf((char *)dtbuf,"UTC Date:%04d/%02d/%02d   ",gpsx.utc.year,gpsx.utc.month,gpsx.utc.date);	//显示UTC日期
	//LCD_ShowString(30,280,200,16,16,dtbuf);		    
	sprintf((char *)dtbuf,"UTC Time:%02d:%02d:%02d   ",gpsx.utc.hour,gpsx.utc.min,gpsx.utc.sec);	//显示UTC时间
  //LCD_ShowString(30,300,200,16,16,dtbuf);		  
}

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
	
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);//设置系统中断优先级分组2
	delay_init(168);  //初始化延时函数
	uart_init(115200);		//初始化串口波特率为115200
	LED_Init();					//初始化LED 
	usmart_dev.init(84);		//初始化USMART
	usart3_init(38400);			//初始化串口3波特率为38400  ->FSMCD13  FSMC_D14
 	//LCD_Init();					//LCD初始化  
 	KEY_Init();					//按键初始化 
	RS485_Init(bound);				//初始化485
	W25QXX_Init();				//初始化W25Q128
	my_mem_init(SRAMIN);		//初始化内部内存池 
	my_mem_init(SRAMCCM);		//初始化CCM内存池
	
 	while(SD_Init())//检测不到SD卡
	{
		//LCD_ShowString(30,150,200,16,16,"SD Card Error!");
		delay_ms(500);					
		//LCD_ShowString(30,150,200,16,16,"Please Check! ");
		delay_ms(500);
		LED0=!LED0;//DS0闪烁
	}
 	exfuns_init();							//为fatfs相关变量申请内存				 
  f_mount(fs[0],"0:",1); 					//挂载SD卡 
 	res=f_mount(fs[1],"1:",1); 				//挂载FLASH.	
	if(res==0X0D)//FLASH磁盘,FAT文件系统错误,重新格式化FLASH
	{
		//LCD_ShowString(30,150,200,16,16,"Flash Disk Formatting...");	//格式化FLASH
		res=f_mkfs("1:",1,4096);//格式化FLASH,1,盘符;1,不需要引导区,8个扇区为1个簇
		if(res==0)
		{
			f_setlabel((const TCHAR *)"1:ALIENTEK");	//设置Flash磁盘的名字为：ALIENTEK
			//LCD_ShowString(30,150,200,16,16,"Flash Disk Format Finish");	//格式化完成
		}
//		else 
//			LCD_ShowString(30,150,200,16,16,"Flash Disk Format Error ");	//格式化失败
		delay_ms(1000);
	}													    
  
	while(exf_getfree("0",&total,&free))	//得到SD卡的总容量和剩余容量
	{
		delay_ms(200);
		LED0=!LED0;//DS0闪烁
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
			if(TIME_Check>=60)
			{
				TIME_Check =0;
				ReadMeteorVal();
			}
			RS485_Receive_Data(ReceiveBuf,&ReceiveBufCounter);
			delay_ms(20);
			if(ReceiveBufCounter)//接收到有数据
			{
				if(GPS_Save_File)
				{
					GPS_Save_File = 0;
					PutData2TXT(USART1_TX_BUF,rxlen);  /*保存GPS数据*/
					LED1 = 0;
				}
				
				PutData2TXT(ReceiveBuf,ReceiveBufCounter);
				LED0=0;
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
				LED1=1;
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
		/*******************GPS数据**********************/
		if(USART3_RX_STA&0X8000)		//接收到一次数据了
		{
			rxlen=USART3_RX_STA&0X7FFF;	//得到数据长度
			for(i=0;i<rxlen;i++)USART1_TX_BUF[i]=USART3_RX_BUF[i];	   
 			USART3_RX_STA=0;		   	//启动下一次接收
			USART1_TX_BUF[i]=0;			//自动添加结束符
			//GPS_Analysis(&gpsx,(u8*)USART1_TX_BUF);//分析字符串
			//Gps_Msg_Show();				//显示信息	
			GPS_Save_File = 1;
			if(upload)printf("\r\n%s\r\n",USART1_TX_BUF);//发送接收到的数据到串口1
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



