// **********************************************************
// Baychi soft 2013
// **      RFM22B/23BP/Si4432 Transmitter with Expert protocol **
// **      This Source code licensed under GPL            **
// **********************************************************
// Latest Code Update : 2013-10-22
// Supported Hardware : Expert Tiny, Orange/OpenLRS Tx/Rx boards (store.flytron.com)
// Project page       : https://github.com/baychi/OpenExpertTX
// **********************************************************


// Функции меню терминала
//
static unsigned char regs[] = {1, 2, 3, 4, 5, 11,12,13,14,15,16,17,18,19,20,21,22 }; // номера отображаемых регистров
static char *help[] = {
  "Bind N",
  "Freq Corr",
  "Term corr enable",
  "FS check enable",
  "Debug output (1-PPMbuf, 2-performance)",
  "Hope F1",
  "Hope F2",
  "Hope F3",
  "Hope F4",
  "Hope F5",
  "Hope F6",
  "Hope F7",
  "Hope F8",
  "Power switch chan (1-12) 0=off )",
  "Power min (0-7)",  
  "Power middle (0-7)",
  "Power max (0-7)"
};  
  

void showRegs(void)         // показать значения регистров
{
  unsigned char i,j=0;
  for(int i=1; i<=REGS_NUM; i++) {
    if(regs[j] == i) {
      Serial.print(i);
      Serial.print("=");
      Serial.print(read_eeprom_uchar(i));
      Serial.print("\t");
      Serial.println(help[j]);
      j++;
    }
  }
}


bool checkMenu(void)   // проверка на вход в меню
{
   int in; 
   
   if (Serial.available() > 0) {
      in= Serial.read();             // все, что пришло, отображаем
      if(in == 'c' || in == 'C') mppmDif=maxDif=0; // !!!!!!!
      if(in == 'm' || in == 'M') return true; // есть вход в меню
   } 
   return false;                        // не дождались 
}


void getStr(char str[])             // получение строки, завершающейся Enter от пользователя
{
  int in,sn=0;
  str[0]=0;
  while(1) {
    wdt_reset();               //  поддержка сторожевого таймера
    if (Serial.available() > 0) {
       in= Serial.read();             // все, что пришло, отображаем
       if(in > 0) {
          Serial.write(in);
          if(in == 0xd || in == 0xa) {
            Serial.println("");
            mppmDif=maxDif=0;           // !!!!!!!
            return;                     // нажали Enter
          }
          if(in == 8) {                 // backspace, удаляем последний символ
            if(sn) sn--;
            continue;
          } 
          str[sn]=in; str[sn+1]=0;
          if(sn < 6) sn++;              // не более 6 символов
        }
     } else delay(1);
  }
}

#define R_AVR 199               // усреднение RSSI

byte margin(byte v)
{
   if(v < 10) return 0; 
   else if(v>71) return 61;

   return  v-10;
}
void print3(unsigned char val)  // печать 3-цифр с выравниваем пробелами
{
  if(val < 10) Serial.print("  ");
  else if(val <100) Serial.print(" ");
  Serial.print(val);
  Serial.print(" ");
}  

byte _spi_read(byte address); 
void _spi_write(byte address,byte val); 

void showNoise(char str[])             // отображаем уровень шумов по каналам
{
  byte fBeg=0, fMax=254;
  byte rMin, rMax;
  word rAvr;
  byte i,j,k;

  rAvr=atoi(str+1);           // считаем параметры, если есть в виде Nbeg-end
  if(rAvr > 0 && rAvr < 255) {
     fBeg=rAvr;
     for(i=2; i<10; i++) {
      if(str[i] == 0) break;
      if(str[i] == '-') {
        rAvr=atoi(str+i+1);
        if(rAvr > fBeg && rAvr < 255) fMax=rAvr;
        break;
      }
    }
  }
  
  RF22B_init_parameter();      // подготовим RFMку 
  to_rx_mode(); 
 
  Serial.println("FHn: Min Avr Max");
  
  for(i=fBeg; i<=fMax; i++) {    // цикл по каналам
     _spi_write(0x79, i);       // ставим канал
     delayMicroseconds(749);
     rMin=255; rMax=0; rAvr=0;
     for(j=0; j<R_AVR; j++) {   // по каждому каналу 
       delayMicroseconds(99);
       k=_spi_read(0x26);         // Read the RSSI value
       rAvr+=k;
       if(k<rMin) rMin=k;         // min/max calc
       if(k>rMax) rMax=k;
     }
     if(i < 10) Serial.print("  ");
     else if(i <100) Serial.print(" ");
     Serial.print(i);
     k=':';
     for(j=0; j<HOPE_NUM; j++) {   // отметим свои частоты
        if(hop_list[j] == i) {
          k='#';
        }
     }
     Serial.write(k); Serial.print(" ");
     print3(rMin);   
     k=rAvr/R_AVR;  print3(k);
     print3(rMax);

     if(str[0] == 'N') {         // если надо, печатаем псевдографику 
       rMin=margin(rMin); 
       rMax=margin(rMax); 
       k=margin(k); 

       for(j=0; j<=rMax; j++) {                         // нарисуем псевдографик
         if(j == k) Serial.print("*");
         else if(j == rMin) Serial.print("<");
         else if(j == rMax) Serial.print(">");
         else if(j>rMin && j <rMax) Serial.print(".");
         else Serial.print(" ");
       }
     }
     
     Serial.println();
     wdt_reset();               //  поддержка сторожевого таймера
  }
}

void doMenu()                       // работаем с меню
{
  char str[8];
  int reg,val;
  Serial.println("To Enter MENU Press ENTER");
  getStr(str);
  if(str[0] == 'q' || str[0] == 'Q') return;     // Q - то quit
  
  while(1) {
    Serial.println("Rg=Val \tComments -----------------");
    showRegs();
    Serial.println("Type Reg and press ENTER, type Value and press ENTER (q=Quit; Nx-y = Show noise)");

rep:  
    getStr(str);

    if(str[0] == 'n' || str[0] == 'N') {  // отсканировать и отобразить уровень шума 
       showNoise(str);
       goto rep;
    }
    if(str[0] == 'q' || str[0] == 'Q') return;     // Q - то quit
    reg=atoi(str);
    if(reg<0 || reg>REGS_NUM) continue; 

    getStr(str);
    if(str[0] == 'q' || str[0] == 'Q') return;     // Q - то quit
    val=atoi(str);
    if(val<0 || val>255) continue; 
    if(reg == 0 && val ==0) continue;              // избегаем потери s/n

    Serial.print(reg); Serial.print("=");   Serial.println(val);  // Отобразим полученное
    
     write_eeprom_uchar(reg,val);  // пишем регистр
     read_eeprom();                // читаем из EEPROM    
     write_eeprom();               // и тут-же пишем, что-бы сформировать КС 
  }    
}  
