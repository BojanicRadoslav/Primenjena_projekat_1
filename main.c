#include<p30fxxxx.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "driverGLCD.h"
#include "adc.h"
#define servo_min 11
#define servo_max 80

enum strana{tastatura, settings, stats};
 
enum strana aktivna_strana = tastatura;
int mq3_en = 1;
int mq3_threshold = 2000;
int mq3_value = 0;
int mq3_counter = 0;

int pir_cnt = 0;
int faza_cnt = 0;


//_FOSC(CSW_FSCM_OFF & XT_PLL4);//instruction takt je isti kao i kristal//
_FOSC(CSW_ON_FSCM_OFF & HS3_PLL4);
_FWDT(WDT_OFF);
_FGS(CODE_PROT_OFF);

unsigned int X, Y,x_vrednost, y_vrednost;
unsigned int alarm_enable=1;
unsigned int silent=0;
unsigned int mq3_count=0;
unsigned int pin_count=0;
unsigned int faza_count=0;
unsigned int pir_count=0;
unsigned int alarm = 0;
unsigned int buzzer_counter = 0;
const char pin[4]="1234";

//const unsigned int ADC_THRESHOLD = 900; 
const unsigned int AD_Xmin =314;
const unsigned int AD_Xmax =4095;
const unsigned int AD_Ymin =688;
const unsigned int AD_Ymax =4035;

unsigned int sirovi0,sirovi1, sirovi2;
unsigned int broj,broj1,broj2,temp0,temp1; 

int servo_counter = 0;
int servo_pos = servo_min;
int pomocni_counter = 0;
int servo_enable = 0;

//#define DRIVE_A PORTBbits.RB10
//#define DRIVE_B PORTCbits.RC13
#define DRIVE_A PORTCbits.RC13
#define DRIVE_B PORTCbits.RC14

#define TMR2_period 300 /*  Fosc = 3.3333MHz,
					          1/Fosc = 0.3us !!!, 0.3us * 300 = 9us  */
#define TMR1_period 100

void Init_T2(void)
{
	TMR2 = 0;
	PR2 = TMR2_period;
	
	T2CONbits.TCS = 0; // 0 = Internal clock (FOSC/4)
	//IPC1bits.T2IP = 3 // T2 interrupt pririty (0-7)
	//SRbits.IPL = 3; // CPU interrupt priority is 3(11)
	IFS0bits.T2IF = 0; // clear interrupt flag
	IEC0bits.T2IE = 1; // enable interrupt

	T2CONbits.TON = 1; // T2 on 
}

void servo(int pos){
    servo_enable = 1;
    pomocni_counter = 0;
    servo_pos = pos;
}

int c = 0;
void __attribute__ ((__interrupt__)) _T2Interrupt(void) // svakih 1ms
{

     TMR2 =0;
    
    if(silent==0){
        
        if(alarm==1 || alarm ==2 || alarm == 3){// kada je buzzer_counter manji od 500 buzzer daje zvuk kada je izmedju 500 i 1000 cuti
            buzzer_counter++;
            if(buzzer_counter < 500) LATAbits.LATA11 = ~LATAbits.LATA11;
            else LATAbits.LATA11=0;
            if(buzzer_counter == 1000) buzzer_counter = 0;
        }
        else LATAbits.LATA11=0;
    }
    
    IFS0bits.T2IF = 0; 
       
}

void Init_T1(void)
{
	TMR1 = 0;
	PR1 = TMR1_period;
	
	T1CONbits.TCS = 0; // 0 = Internal clock (FOSC/4)
	//IPC1bits.T2IP = 3 // T2 interrupt pririty (0-7)
	//SRbits.IPL = 3; // CPU interrupt priority is 3(11)
	IFS0bits.T1IF = 0; // clear interrupt flag
	IEC0bits.T1IE = 1; // enable interrupt

	T1CONbits.TON = 1; // T2 on 
}

void __attribute__ ((__interrupt__)) _T1Interrupt(void) // svakih 1ms
{
    if(servo_enable == 1){
        TMR1 = 0;
        if(servo_counter < servo_pos) LATDbits.LATD9 = 1;
        else LATDbits.LATD9 = 0;

        if(servo_counter == 200){
            servo_counter = 0;
            pomocni_counter++;
        }
        
        if(pomocni_counter == 50) servo_enable = 0;
        servo_counter++;
    }
     
    IFS0bits.T1IF = 0; 
       
}

void trigger_pir(){
    if(alarm_enable == 1){
        pir_count++;
        alarm = 1;
        servo(servo_max);
        WriteStringToUART1("ALARM: Detektovan pokret\n");
    }
}

void trigger_mq3(){
    if(alarm_enable == 1){
        mq3_count++;
        alarm = 2;
        servo(servo_min);
        WriteStringToUART1("ALARM: Detektovan dim\n");
    }
} 

void trigger_faza(){
    if(alarm_enable == 1){
        faza_count++;
        alarm = 3;
        servo(servo_min);
        WriteStringToUART1("ALARM: Detektovan nestanak faze\n");
    }
}

void pump(int alarm){
    if(alarm == 2)
    {
        LATDbits.LATD8 = 1;
    }
    else if (alarm == 0 || alarm == 1)
        {
        LATDbits.LATD8 = 0;
        }
}

void ConfigureTSPins(void)
{
	//ADPCFGbits.PCFG10=1;
	//ADPCFGbits.PCFG7=digital;

	//TRISBbits.TRISB10=0;
	TRISCbits.TRISC13=0;
    TRISCbits.TRISC14=0;
	
	//LATCbits.LATC14=0;
	//LATCbits.LATC13=0;
}

void initUART1(void)
{
    U1BRG=0x0015;//ovim odredjujemo baudrate

    U1MODEbits.ALTIO=0;//biramo koje pinove koristimo za komunikaciju osnovne ili alternativne

    IEC0bits.U1RXIE=1;//omogucavamo rx1 interupt

    U1STA&=0xfffc;

    U1MODEbits.UARTEN=1;//ukljucujemo ovaj modul

    U1STAbits.UTXEN=1;//ukljucujemo predaju
}

void writeStatsToUART1(){
    
    int mq3_percent = 0;
    int faza_percent = 0;
    int pir_percent = 0;
    int pin_percent = 0;
    if(mq3_count+faza_count+pir_count+pin_count != 0){
        mq3_percent = mq3_count*100/(mq3_count+faza_count+pir_count+pin_count);
        faza_percent = faza_count*100/(mq3_count+faza_count+pir_count+pin_count);
        pir_percent = pir_count*100/(mq3_count+faza_count+pir_count+pin_count);
        pin_percent = pin_count*(100)/(mq3_count+faza_count+pir_count+pin_count);
    }else{
        WriteStringToUART1("---STATISTIKA---\n   Gas: 0 %\n   RST: 0 %\n   Mot: 0 %\n   Pin: 0 %\n");
        WriteStringToUART1("----------------\n");
        return 0;
    }
    
    char buff[100];
    sprintf(buff, "---STATISTIKA---\n   Gas: %d %%\n   RST: %d %%\n   Mot: %d %%\n   Pin: %d %%\n", mq3_percent, faza_percent, pir_percent, pin_percent);
    WriteStringToUART1(buff);
    WriteStringToUART1("----------------\n");
}

void print_help(){
    WriteStringToUART1("Spisak dozvoljenih komandi:\n");
    WriteStringToUART1("alarm on --> pali alarm\n");
    WriteStringToUART1("alarm off --> gasi alarm nakon kucanja lozinke\n");
    WriteStringToUART1("pump on --> pali pumpu i alarmira\n");
    WriteStringToUART1("pump off --> gasi pumpu\n");
    WriteStringToUART1("door open --> pomera servo motor u krajnji desni polozaj\n");
    WriteStringToUART1("door close --> pomera servo motor u krajnju levi polozaj\n");
    WriteStringToUART1("mq3_sens xxxx --> podesava osetljivost senzora dima i alkogola\nxxxx je broj koji moze uzimati vrednost od 0 do 4095\n");
    WriteStringToUART1("stats --> ispisuje statistiku zastupljenosti svakog od alarma u procentima\n");
}
char msg[100];
int char_count = 0;
void __attribute__((__interrupt__)) _U1RXInterrupt(void) 
{
    IFS0bits.U1RXIF = 0;
        
     char temp_char = U1RXREG;
    msg[char_count]= temp_char;
    char_count++;
    if(temp_char == '\r'){
        if(!strcmp("alarm on\r", msg)){
            alarm = 1;
            servo(servo_max);
        }
        else if(!strcmp("faza\r", msg)){
            trigger_faza();
        }
        else if(!strcmp("alarm off\r", msg)) WriteStringToUART1("Za gasenje alarma unesite lozinku\n");
        else if(!strcmp("1234\r", msg)){
            alarm = 0;
            mq3_en = 1;
            servo(servo_min);
        }
        else if(!(strcmp("stats\r", msg))) writeStatsToUART1();
        else if(!(strcmp("door open\r", msg))){
            
            if(alarm == 1) servo(servo_max);
            else servo(servo_min);
        }
        else if(!(strcmp("door close\r", msg))) servo(servo_max);
        else if(!(strcmp("pokret\r", msg))) trigger_pir();
        else if(!(strcmp("pump on\r", msg))){
            alarm = 2;
           // WriteStringToUART1("Pumpa ukljucena\n");
        }
        else if(!(strcmp("pump off\r", msg))) alarm = 0;
        else if(msg[0] == 'm' && msg[1] == 'q' && msg[2] == '3' && msg[3] == '_' && msg[4] == 's' && msg[5] == 'e' && msg[6] == 'n' && msg[7] == 's'){
            int temp;
            sscanf(msg, "mq3_sens %d", &temp);
            sprintf(msg, "mq3 sensitivity set to %d\n", temp);
            mq3_en = 1;
            mq3_threshold = temp;
            WriteStringToUART1(msg);
            
        }else if(!(strcmp("help\r", msg))) print_help();
        else WriteStringToUART1("Neispravan format komande\n");
        
        //sprintf(msg, "");
        int i = 0;
        for(i=0;i<char_count;i++) msg[i] = 0;
        WriteStringToUART1(msg);
        char_count = 0;
    }
   // tempRX=U1RXREG;

} 

void WriteUART1(unsigned int data)
{
	  while(!U1STAbits.TRMT);

    if(U1MODEbits.PDSEL == 3)
        U1TXREG = data;
    else
        U1TXREG = data & 0xFF;
}

/***********************************************************************
* Ime funkcije      : WriteUART1dec2string                     		   *
* Opis              : Funkcija salje 4-cifrene brojeve (cifru po cifru)*
* Parameteri        : unsigned int data-podatak koji zelimo poslati    *
* Povratna vrednost : Nema                                             *
************************************************************************/
void WriteUART1dec2string(unsigned int data)
{
	unsigned char temp;

	temp=data/1000;
	WriteUART1(temp+'0');
	data=data-temp*1000;
	temp=data/100;
	WriteUART1(temp+'0');
	data=data-temp*100;
	temp=data/10;
	WriteUART1(temp+'0');
	data=data-temp*10;
	WriteUART1(data+'0');
}


void WriteStringToUART1(unsigned char * str){
    unsigned int k = 0;
    while(*(str+k)!= 0x00){
        WriteUART1(*(str+k));
        k++;
    }
}



void Delay(unsigned int N)
{
	unsigned int i;
	for(i=0;i<N;i++);
}

void Touch_Panel (void)
{
// vode horizontalni tranzistori
	DRIVE_A = 1;  
	DRIVE_B = 0;
    
     LATCbits.LATC13=1;
     LATCbits.LATC14=0;

	Delay(500); //cekamo jedno vreme da se odradi AD konverzija
				
	// ocitavamo x	
	x_vrednost = temp0;//temp0 je vrednost koji nam daje AD konvertor na BOTTOM pinu		
    
	// vode vertikalni tranzistori
     LATCbits.LATC13=0;
     LATCbits.LATC14=1;
	DRIVE_A = 0;  
	DRIVE_B = 1;

	Delay(500); //cekamo jedno vreme da se odradi AD konverzija
	
	// ocitavamo y	
	y_vrednost = temp1;// temp1 je vrednost koji nam daje AD konvertor na LEFT pinu	
//Ako �elimo da nam X i Y koordinate budu kao rezolucija ekrana 128x64 treba skalirati vrednosti x_vrednost i y_vrednost tako da budu u opsegu od 0-128 odnosno 0-64
//skaliranje x-koordinate

    X=(x_vrednost-314)*0.0338534;



    //X= ((x_vrednost-AD_Xmin)/(AD_Xmax-AD_Xmin))*128;	
//vrednosti AD_Xmin i AD_Xmax su minimalne i maksimalne vrednosti koje daje AD konvertor za touch panel.


//Skaliranje Y-koordinate
	Y= ((y_vrednost-688)*0.0191216);

    //Y= ((y_vrednost-AD_Ymin)/(AD_Ymax-AD_Ymin))*64;
}


	
void __attribute__((__interrupt__)) _ADCInterrupt(void) 
{
							
	
	sirovi0=ADCBUF0;//0
	sirovi1=ADCBUF1;//1
    sirovi2=ADCBUF2;

	temp0=sirovi0;
	temp1=sirovi1;
    mq3_value = sirovi2;
    //char buff[100];
    //sprintf(buff, "ADC za MQ3 je: %d\n", mq3_value);
    //WriteStringToUART1(buff);
    IFS0bits.ADIF = 0;
} 

void ChechMQ3(){
        if(mq3_en == 1 && mq3_value > mq3_threshold){
            trigger_mq3();
            mq3_en = 0;
            //WriteStringToUART1("Triggerovan mq3\n");
        }
        
        
        
        if(mq3_en == 0 && mq3_value < mq3_threshold/2) mq3_en = 1;
}

void Write_GLCD(unsigned int data)
{
unsigned char temp;

temp=data/1000;
Glcd_PutChar(temp+'0');
data=data-temp*1000;
temp=data/100;
Glcd_PutChar(temp+'0');
data=data-temp*100;
temp=data/10;
Glcd_PutChar(temp+'0');
data=data-temp*10;
Glcd_PutChar(data+'0');
}

void rectangle(int posx, int posy, int sizex, int sizey){
    GLCD_Rectangle(posx, posy, sizex+posx,sizey+posy);
}

void Print_Keyboard(){
            
        GLCD_Rectangle(3,5,67,17);
        
        GoToXY(10,3);
        Glcd_PutChar('1');
        GoToXY(10+25,3);
        Glcd_PutChar('2');
        GoToXY(10+50,3);
        Glcd_PutChar('3');
        
        GoToXY(10,5);
        Glcd_PutChar('4');
        GoToXY(10+25,5);
        Glcd_PutChar('5');
        GoToXY(10+50,5);
        Glcd_PutChar('6');
        
        GoToXY(10,7);
        Glcd_PutChar('7');
        GoToXY(10+25,7);
        Glcd_PutChar('8');
        GoToXY(10+50,7);
        Glcd_PutChar('9');
        
        GoToXY(10+75, 1);
        GLCD_Printf("STATS");
        
        GoToXY(10+75,3);
        GLCD_Printf("SET");
        
        GoToXY(10+75,5);
        GLCD_Printf("CLEAR");
        
        GoToXY(10+75,7);
        GLCD_Printf("OK");
}

void printStats(){
     GoToXY(38,1);
    GLCD_Printf("STATISTICS");
    //fillRectangle(10,10, 50,50);
    GoToXY(3,3);
    GLCD_Printf("Gas");
    GoToXY(3,4);
    GLCD_Printf("RST");
    GoToXY(3,5);
    GLCD_Printf("Mot");
    GoToXY(3,6);
    GLCD_Printf("Pin");
    if(mq3_count+faza_count+pir_count+pin_count != 0){
        int mq3_percent = mq3_count*100/(mq3_count+faza_count+pir_count+pin_count);
        int faza_percent = faza_count*100/(mq3_count+faza_count+pir_count+pin_count);
        int pir_percent = pir_count*100/(mq3_count+faza_count+pir_count+pin_count);
        int pin_percent = pin_count*(100)/(mq3_count+faza_count+pir_count+pin_count);
        fillRectangle(26,3*8+1,26+mq3_percent,4*8-1);//gas
        fillRectangle(26, 4*8+1,26+faza_percent, 5*8-1);//rst
        fillRectangle(26, 5*8+1,26+pir_percent,6*8-1);//mot
        fillRectangle(26, 6*8+1,26+pin_percent,7*8-1);//mot
    }
    

    GoToXY(90,7);
    GLCD_Printf("BACK");
}

void printSettings(){
    GoToXY(40,1);
    GLCD_Printf("SETTINGS");
    GoToXY(5,3);
    if(silent == 1){
        GLCD_Printf("Silent: ON");
    }else{
        GLCD_Printf("Silent: OFF");
    }
    GoToXY(5,5);
    if(alarm_enable==1){
        GLCD_Printf("Alarm:  ON");
    }else{
        GLCD_Printf("Alarm:  OFF");
    }
    GoToXY(90,7);
    GLCD_Printf("BACK");
}

void fillRectangle(int startx, int starty, int endx, int endy){
    int i, j;
    for(i=startx;i<endx;i++)
        for(j=starty;j<endy;j++)  LcdSetDot(i,j);
    
}

void main(void)
{

	ConfigureLCDPins();
	ConfigureTSPins();

	GLCD_LcdInit();

	GLCD_ClrScr();
    initUART1();
    //Delay(100000);
    WriteStringToUART1("Alarm init...\n");
	
	ADCinit();
	ConfigureADCPins();
	ADCON1bits.ADON=1;
    
    TRISAbits.TRISA11 = 0;
    TRISDbits.TRISD9 = 0;
    TRISDbits.TRISD8 = 0;
    
    ADPCFGbits.PCFG6=1;
    ADPCFGbits.PCFG7=1;
    TRISBbits.TRISB6 = 1;
    TRISBbits.TRISB7 = 1;
    
    //LATBbits.LATB6 = 1;
    //LATBbits.LATB7 = 1;
    Init_T2();
    Init_T1();
    
    servo(servo_min);
    
    char buffer[30];
    char uneta_lozinka[10]= "****";
    int char_pos=0;
    char button_pressed = 0;
    
    Print_Keyboard();
    
    GoToXY(10,1);
    GLCD_Printf(uneta_lozinka);
    aktivna_strana == tastatura;

	while(1)
	{

    ChechMQ3();
    if(PORTBbits.RB6 == 1){
        //pir senzor na RB6
        pir_cnt++;
        if(pir_cnt == 20) trigger_pir();
    }else pir_cnt = 0;
    
    if(PORTBbits.RB7 == 1){
        //senzor nestanka faze na RB7
        faza_cnt++;
        if(faza_cnt == 20) trigger_faza();
    }else faza_cnt = 0;

        
	Touch_Panel ();
    pump(alarm);
    if(aktivna_strana == tastatura){
        if(X>0 && X<128 && Y>0 && Y<64){
            if(char_pos>3) char_pos=3;
            if(X>3 && X<23 && Y>31 && Y<43){
                //pritisnuta jedinica
                button_pressed = '1';
                
            }else if(X>27 && X<43 && Y>31 && Y<43){
                //pritisnuta dvojka
                button_pressed = '2';
                
            }else if(X>62 && X<70 && Y>31 && Y<43){
                //pritisnuta trojka
                button_pressed = '3';
            }else if(X>3 && X<23 && Y>12 && Y<26){
                //pritisnuta cetvorka
                button_pressed = '4';
            }else if(X>27 && X<43 && Y>12 && Y<26){
                //pritisnuta petica
                button_pressed = '5';
            }else if(X>62 && X<70 && Y>12 && Y<26){
                //pritisnuta sestica
                button_pressed = '6';
            }else if(X>3 && X<23 && Y>0 && Y<10){
                //pritisnuta sedmica
                button_pressed = '7';
            }else if(X>27 && X<43 && Y>0 && Y<10){
                //pritisnuta petica
                button_pressed = '8';
            }else if(X>62 && X<70 && Y>0 && Y<10){
                //pritisnuta devetla
                button_pressed = '9';
            }else if(X>85 && X<127 && Y>47 && Y<60){
                //pritisnuto stats
                button_pressed = 's';
            }else if(X>75 && X<127 && Y>25 && Y<43){
                //pritisnuto set
                button_pressed = 'p';
            }else if(X>75 && X<127 && Y>13 && Y<24){
                //pritisnuto clear
                button_pressed = 'c';
            }else if(X>75 && X<127 && Y>0 && Y<10){
                //pritisnuto ok
                //WriteStringToUART1("OK\n");
                button_pressed = 'o';
            }
           //sprintf(buffer, "X=%d, Y=%d\n", X, Y);
           //WriteStringToUART1(buffer);
            Delay(1000);
            }else{
            if(button_pressed == 's'){
                WriteStringToUART1("Stats\n");
                aktivna_strana = stats;
                GLCD_ClrScr();
                printStats();
                button_pressed = 0;
                //stats
            }else if(button_pressed == 'p'){
                WriteStringToUART1("Settings\n");
                aktivna_strana = settings;
                GLCD_ClrScr();
                printSettings();
                button_pressed = 0;
                //settings
            }else if(button_pressed == 'o'){
                int i = 0;
                int ok = 1;
                for(i=0;i<4;i++){
                    if(uneta_lozinka[i]!=pin[i]) ok = 0;
                }
                if(ok){
                    servo(servo_min);
                    alarm = 0;
                    WriteStringToUART1("Uneta ispravna lozinka\n");
                    char_pos=0;
                    for(i=0;i<4;i++) uneta_lozinka[i]='*';
                    GoToXY(10,1);
                    GLCD_Printf("Accepted");
                    for(i=0;i<100;i++) Delay(5000);
                    GoToXY(10,1);
                    GLCD_Printf("        ");
                }else{
                    if(alarm_enable == 1){
                        WriteStringToUART1("ALARM: pogresna lozinka\n");
                        pin_count++;
                        alarm = 1;
                        servo(servo_max);
                    }
                    char_pos=0;
                    for(i=0;i<4;i++) uneta_lozinka[i]='*';
                    GoToXY(10,1);
                    GLCD_Printf("Error");
                    for(i=0;i<100;i++) Delay(5000);
                    GoToXY(10,1);
                    GLCD_Printf("        ");
                }
                GoToXY(10,1);
                GLCD_Printf(uneta_lozinka);
                button_pressed = 0;
                //ok
            }else if(button_pressed == 'c'){
                WriteStringToUART1("Clear\n");
                char_pos=0;
                int i;
                for(i=0;i<4;i++) uneta_lozinka[i]='*';
                GoToXY(10,1);
                GLCD_Printf("****    ");
                button_pressed = 0;
                //clear
            }
                if(button_pressed != 0){
                    uneta_lozinka[char_pos] = button_pressed;
                    char_pos++;
                    button_pressed = 0;
                    GoToXY(10,1);
                    GLCD_Printf(uneta_lozinka);
                }
            }
        }else if(aktivna_strana == stats){
            if(X>0 && X<128 && Y>0 && Y<64){
                if(X>90 && X<127 && Y>0 && Y<8){
                    GLCD_ClrScr();
                    aktivna_strana = tastatura;
                    Print_Keyboard();
                }
                Delay(1000);
            }
            //statistika
        }else{
            if(X>0 && X<128 && Y>0 && Y<64){
                //sprintf(buffer, "X=%d, Y=%d\n", X, Y);
                //WriteStringToUART1(buffer);
                if(X>90 && X<127 && Y>0 && Y<8){
                    GLCD_ClrScr();
                    aktivna_strana = tastatura;
                    Print_Keyboard();
                }else if(X>47 && X<83 && Y>30 && Y<40){
                    //silent_alarm
                    button_pressed = 's';

                }else if(X>47 && X<83 && Y>13 && Y<26){
                    //alarm_enable
                    button_pressed = 'a';
                   
                }
                Delay(1000);
            }else{
                if(button_pressed == 's'){
                    GoToXY(5,3);
                    if(silent == 1){
                        silent = 0;
                        WriteStringToUART1("Tihi alarm iskljucen\n");
                        GLCD_Printf("Silent: OFF");
                    }else{
                        silent = 1;
                        WriteStringToUART1("Tihi alarm ukljucen\n");
                        GLCD_Printf("Silent: ON ");
                    }
                    button_pressed = 0;
                    //silent
                }else if(button_pressed == 'a'){
                     GoToXY(5,5);
                    if(alarm_enable == 1){
                        if(alarm == 0){
                            alarm_enable = 0;
                            GLCD_Printf("Alarm:  OFF");
                            WriteStringToUART1("Alarm iskljucen\n");
                        }
                    }else{
                        alarm_enable = 1;
                        WriteStringToUART1("Alarm ukljucen\n");
                        GLCD_Printf("Alarm:  ON ");
                    }
                     button_pressed = 0;
                }
                    //alarm
            }
            //podesavanja
        }
            
     
	}//while

}//main

			
		
												
