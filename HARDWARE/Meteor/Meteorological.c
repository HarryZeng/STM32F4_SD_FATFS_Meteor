/**
  ******************************************************************************
  * @file    Meteorological.c 
  * @author  ZKRT
  * @version V1.0
  * @date    12-March-2018
  * @brief   
	*					 + (1) init
	*                       
  ******************************************************************************
  * @attention
  *
  * ...
  *
  ******************************************************************************
  */

#include "Meteorological.h"
#include "delay.h" 								  
#include "usart.h"
#include "rs485.h"

/**
  * @brief   int to char
  * @param  None
  * @retval None
  */
char* itoa(int num,char*str,int radix)
{		/*������*/
		char index[]="0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
		unsigned unum;/*�м����*/
		char temp;
		int i=0,j,k;
		/*ȷ��unum��ֵ*/
		if(radix==10&&num<0)/*ʮ���Ƹ���*/
		{
		unum=(unsigned)-num;
		str[i++]='-';
		}
		else unum=(unsigned)num;/*�������*/
		/*ת��*/
		do{
		str[i++]=index[unum%(unsigned)radix];
		unum/=radix;
		}while(unum);
		str[i]='\0';
		/*����*/
		if(str[0]=='-')k=1;/*ʮ���Ƹ���*/
		else k=0;
		
		for(j=k;j<=(i-1)/2;j++)
		{
		temp=str[j];
		str[j]=str[i-1+k-j];
		str[i-1+k-j]=temp;
		}
		return str;
}
	
/**
  * @brief  Meteor Check
  * @param  None
  * @retval None
  */
bool CheckCommunication(void)
{
	uint8_t MeteorCheckData[5];
	uint16_t Len=5;
	uint8_t SendCmd[] = "Q\n";
	
	char CheckReceiveFlag[]="OK Q\n";
	char ReceiveData[5];
	
	RS485_Send_Data(SendCmd,2);
	
	delay_ms(10);

	RS485_Receive_Data(MeteorCheckData,&Len);

	if(strncmp(CheckReceiveFlag,MeteorCheckData,5))
		return false;
	return true;
}

/**
  * @brief  Meteor Start to measure
  * @param  None
  * @retval None
  */
bool StartMea(void)
{
		uint8_t Cmd[]="K\n";
	  
		RS485_Send_Data(Cmd, 2);

}

/**
  * @brief  Meteor Start to measure
  * @param  None
  * @retval None
  */
bool StopMea(void)
{
	uint8_t MeteorStopkData[5];
	uint16_t Len=0;
	uint8_t SendCmd[] = "S\n";
	
	char CheckReceiveFlag[]="OK S\n";
	char ReceiveData[5];
	
	RS485_Send_Data(SendCmd,2);
	
	delay_ms(10);

	RS485_Receive_Data(MeteorStopkData,&Len);

	if(strncmp(CheckReceiveFlag,MeteorStopkData,5))
		return false;
	return true;
}

/**
  * @brief  ��ȡ�������ֵ
  * @param  value������ֵ
  * @retval ��ȡ�ɹ�����TRUE ʧ�ܷ��� FALSE
  */
bool ReadMeteorVal (void)
{
	uint8_t Len=0;
	uint8_t SendCmd[] = "E\n";
		
	RS485_Send_Data(SendCmd,2);
	
	return true;
	
}
