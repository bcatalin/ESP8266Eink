/*
 2012 Copyright (c) Seeed Technology Inc.

 Author: Zhangkun  
 
 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc.,51 Franklin St,Fifth Floor, Boston, MA 02110-1301 USA

 Converted from STM32 to ESP8266 by Catalin Batrinu 
 2016 Copyright (c) bcatalin@gmail.com

*/

#ifndef Eink_h
#define Eink_h
 
#ifndef INT8U
#define INT8U unsigned char
#endif
#ifndef INT16U
#define INT16U unsigned int
#endif
#ifndef INT32U
#define INT32U unsigned long
#endif

#define CS 15
#define DC 5

#define Eink_CS1_LOW  digitalWrite(CS,LOW) //{DDRD |= 0x40;PORTD &=~ 0x40;}
#define Eink_CS1_HIGH digitalWrite(CS,HIGH)//{DDRD |= 0x40;PORTD |=  0x40;}
#define Eink_DC_LOW   digitalWrite(DC,LOW) //{DDRD |= 0x20;PORTD &=~ 0x20;}
#define Eink_DC_HIGH  digitalWrite(DC,HIGH) //{DDRD |= 0x20;PORTD |=  0x20;}


class E_ink_ESP8266
{
public:
 void clearScreen(void);
 void refreshScreen(void);
 void getCharMatrixData(INT16U unicode_Char);
 void converCharMatrixData (void);
 void converChineseMatrixData(void);
 void displayChar(INT8U x1,INT8U y1,INT16U unicode_Char);
 void displayTwoDimensionalCode(INT8U x,INT8U y);
 void E_Ink_P8x16Str(INT8U y,INT8U x,char ch[]);
private:
 INT8U matrixdata[32];
 INT8U matrixdata_conver[200]; 
 
 void  writeComm(INT8U Command);
 void  writeData(INT8U data);
 INT16U  GTRead(INT32U Address);
 INT16U convertData(INT8U Original_data); 
 void  converDimensionalCode(void);
 
 void  initEink(void);
 void  closeBump(void);
 void  configureLUTRegister(void);
 void  setPositionXY(INT8U Xs, INT8U Xe,INT8U Ys,INT8U Ye);
 
};

//extern E_ink_ESP8266 Eink_ESP8266;
extern INT8U dimensionalData[];
#endif
/*********************************************************************************************************
  END FILE
*********************************************************************************************************/
  
