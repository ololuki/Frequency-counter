/********************************************************/
/* Miernik częstotliwości Attiny2313 wyswietlacz LCD 	*/
/*														*/



/*	wersja 09-10-2012 -poprawiono błedne jednoski(ns,ms)*/

/********************************************************/



#include <stdlib.h>
#include <avr/io.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "defines.h"
#include "LcdHd44780.h"

#define DLUGOSC_IMPULSU_W_NS (F_CPU/1000000)
#define DLUGOSC_IMPULSU_W_MS (F_CPU/1000)
#define DLUGOSC_IMPULSU_W_S (F_CPU/1)

#define LICZBA_PRZERWAN_TIM_0 2000 //4000 dla 1 sekundy

#define WYBOR_TRYBU_POMIAR_CZESTOTLIWOSCI (!(PIND & (1<<PD0)) && (PIND & (1<<PD1))) //PD0=0 && PD1=1
#define WYBOR_TRYBU_POMIAR_DLUGOSCI_IMPULSU ((PIND & (1<<PD0)) && !(PIND & (1<<PD1))) //PD0=1 && PD1=0
#define WYBOR_TRYBU_POMIAR_LICZBY_IMPULSOW ((PIND & (1<<PD0)) && (PIND & (1<<PD1))) //PD0=1 && PD1=1
//#define WYBOR_TRYBU_POMIAR_CZ_DZIELNIK_16 (!(PIND & (1<<PD0)) && !(PIND & (1<<PD1))) //PD0=0 && PD1=0

#define WYBOR_TRYBU_INICJALIZACJA PORTD |= (1 << PD0) | (1 << PD1) | (1 << PD3) //DDRD &= ~((1 << PD0) | (1 << PD1)) //ustawienie podciągania pinów, ustawienie jako wejścia-domyślne

#define PRESKALER (!(PIND & (1<<PD6))) //PD6=0

#define ZERUJ_POMIAR (!(PIND & (1<<PD3))) //PD3=0

#define MUX_DZIELNIK_DIR DDRD
#define MUX_DZIELNIK_PORT PORTD
#define MUX_DZIELNIK 0

volatile uint16_t Liczba_przerwan_tim0;
volatile uint8_t Lczba_przep_tim1;
uint8_t poprzednia_liczba_przep_tim1;
uint32_t wynik_pomiaru;
uint32_t poprzedni_wynik;

volatile uint8_t pomiar_czasu; //flaga
#define GOTOWY 0 //oczekiwanie na zbocze rozpoczynające pomiar
#define TRWA 1 //oczekiwanie na zbocze kończące pomiar
#define WYSWIETL 2 //obliczenia i wyświetlanie na wyswietlaczu


char Bufor_LCD[8];
uint8_t kursor_LCD; //zmienna potrzebna przy LCD 1*16 o organizacji pamięci 2*8, przechowuje aktualne położenie kursora na LCD

//napisy EEprom
//unsigned char nazwa_zmiennej[] EEMEM = "Napis do LCD";

unsigned char oczekiwanie[] EEMEM = "Oczekiwanie";
unsigned char Zbyt_dlugi_czas[] EEMEM = "Zbyt długi czas impulsu";
unsigned char milisekundy[] EEMEM = " ms";
unsigned char impulsow[] EEMEM = " impulsow";
unsigned char Przepelnienie[] EEMEM = "Przepelnienie";
unsigned char zmien_zakres[] EEMEM = "Zmien zakres";



void LCD_WriteText_eeprom(const unsigned char *data)
{
	unsigned char c;
    while(1) {
		c = eeprom_read_byte(data);
		if(c==0) return;
        LcdHd44780_writeData(c);
        data++;
		++kursor_LCD;
		if (kursor_LCD == 8) {
			LcdHd44780_cursor(1, 0); //po wyświetleniu ósmego znaku przechodzi do kolejnej linii
		}
    }
}

ISR (INT0_vect) { //przerwanie zewnetrzne INT0 - pomiar długości impulsu międzi kolejnymi zboczami
	if (pomiar_czasu == GOTOWY) { //jeśli pomiar długości impulsu nie trwa
		TIMER1_START_1; //włącz odmierzanie czasu do kolejnego zbocza konczacego pomiar
		pomiar_czasu = TRWA;
	} else { //pomiar_czasu == TRWA
		TIMER1_STOP; //wyłącz odmierzanie czasu
		DISABLE_TIMER1_OVF_INTERRUPT; //wyłącz przerwanie zwiększające zmienną Lczba_przep_tim1 na wypadek przepełnienia timera1 podczas bieżącej obsługi przerwania
		DISABLE_INT0_INTERRUPT; //wyłącz to przerwanie na czas obliczeń i wysłania wyniku na wyswietlacz(w pętli głownej wyświetlanie i ponowne włączenie przerwania)
		pomiar_czasu = WYSWIETL; //(wyświetl=1;)
	}
}

ISR (TIMER0_OVF_vect) {
	//TCNT0 += 6;
	++Liczba_przerwan_tim0;
	if(Liczba_przerwan_tim0 == LICZBA_PRZERWAN_TIM_0){
		TIMER1_STOP
		TIMER0_STOP
		pomiar_czasu = WYSWIETL;
		//dalsza część w pętli głownej

	}
}


ISR (TIMER1_OVF_vect) {
	++Lczba_przep_tim1;
}

//funkcja wyświetlania tekstu na wyswietlaczu 1*16 o organizacji pamięci 2*8
void LCD_WriteText_dzielonyLCD(char * text)
{
	while(*text) {
		LcdHd44780_writeData(*text++);
		++kursor_LCD;
		if (kursor_LCD == 8) {
			LcdHd44780_cursor(1, 0); //po wyświetleniu ósmego znaku przechodzi do kolejnej linii
		}
	}
}

//funkcja wpisująca liczbe uint32_t w systemie dziesiętnym na LCD
void LCD_WriteDecimalLong(uint32_t liczba)
{
	ltoa(liczba,Bufor_LCD,10);
	LCD_WriteText_dzielonyLCD(Bufor_LCD);
}

int main(void)
{
	/*****************************Globalne wstępne ustawienia****************************/
	
		//DDRB = 0xFF; //portb jako wyjście - ustawiane w LCD_initialize
		//WYBOR_TRYBU_INICJALIZACJA; //domyslnie jako wejścia, włączenie pullupów - zewnętrzne rezystory
		LcdHd44780_init();
		//LCD_WriteText("Hello");
		sei();
		//ENABLE_TIMER1_OVF_INTERRUPT; //włączenie przerwania od przepełnienia TIMERA1
		
	/***********************************Główna pętla**************************************/
	
	while(1) {
		
		/*************************Tryb pomiaru długości impulsu***********************/
			
			if (WYBOR_TRYBU_POMIAR_DLUGOSCI_IMPULSU) {
				
				//////////////////wstępne ustawienia///////////////////////////
					
					LcdHd44780_clear();
					//LCD_Home();
					kursor_LCD = 0;
					//LCD_WriteText_dzielonyLCD("Oczekiwanie");
					LCD_WriteText_eeprom(oczekiwanie);
					
					///////goto
					pomiar_dlugosci_impulsu_poczatek: //goto
					DISABLE_INT0_INTERRUPT;
					if (poprzednia_liczba_przep_tim1) {
						poprzednia_liczba_przep_tim1 = 0;
						LcdHd44780_clear();
						kursor_LCD = 0;
						//LCD_WriteText_dzielonyLCD("Zbyt długi czas impulsu");
						LCD_WriteText_eeprom(Zbyt_dlugi_czas);
					}
					
					
					
					pomiar_czasu = GOTOWY;
					///////
					
					TIMER0_STOP;
					DISABLE_TIMER0_OVF_INTERRUPT;
					INT0_FALLING;
					TIMER1_STOP;
					ENABLE_TIMER1_OVF_INTERRUPT
					TCNT1 = 0;
					Lczba_przep_tim1 = 0;
					CLEAR_INT0_FLAG;
					ENABLE_INT0_INTERRUPT;
					


				
				////////////////pętla pomiaru długości impulsu////////////////////
				
					while (WYBOR_TRYBU_POMIAR_DLUGOSCI_IMPULSU) {
			
						///////goto
						if (Lczba_przep_tim1 < poprzednia_liczba_przep_tim1) { // && TCNT1 < poprzedni_TCNT1
							goto pomiar_dlugosci_impulsu_poczatek;
						}
						poprzednia_liczba_przep_tim1 = Lczba_przep_tim1;
						_delay_ms(10);
						///////
						
						/*
						if(pomiar_czasu == TRWA) {
							LcdHd44780_clear();
							//LCD_Home();
							kursor_LCD = 0;
							LCD_WriteText_dzielonyLCD("Pomiar");
						}
						*/
						if (pomiar_czasu == WYSWIETL) {
							
							wynik_pomiaru = ((65536 * Lczba_przep_tim1) + TCNT1)/ DLUGOSC_IMPULSU_W_NS; //wynik w ns
							
							if (PRESKALER) {
								wynik_pomiaru /= 16; //podział preskalera
							}
							//wynik_pomiaru /= DLUGOSC_IMPULSU_W_NS; //wynik w ns
							
							LcdHd44780_clear();
							//LCD_Home();
							kursor_LCD = 0;
							if (!ZERUJ_POMIAR) { //normalne wyswietlanie
							
								if (wynik_pomiaru >= 100000) {
									wynik_pomiaru /= 1000;
									//ltoa(wynik_pomiaru,Bufor_LCD,10);
									//LCD_WriteText_dzielonyLCD(Bufor_LCD);
									LCD_WriteDecimalLong(wynik_pomiaru);///////////////
									//LCD_WriteText_dzielonyLCD(" ms");
									LCD_WriteText_eeprom(milisekundy);
								}else if (wynik_pomiaru > 100){
									//wynik_pomiaru = ((65536 * Lczba_przep_tim1) + TCNT1)/DLUGOSC_IMPULSU_W_NS;
									//ltoa(wynik_pomiaru,Bufor_LCD,10);
									//LCD_WriteText_dzielonyLCD(Bufor_LCD);
									LCD_WriteDecimalLong(wynik_pomiaru);
									LCD_WriteText_dzielonyLCD(" us");
								}else{
									//LCD_WriteText_dzielonyLCD("Zmien zakres");
									LCD_WriteText_eeprom(zmien_zakres);
								}
								
							}else{	
								wynik_pomiaru = 1000000/wynik_pomiaru; //przekształcenie z ns w Hz
								//ltoa(wynik_pomiaru,Bufor_LCD,10);
								//LCD_WriteText_dzielonyLCD(Bufor_LCD);
								LCD_WriteDecimalLong(wynik_pomiaru);
								LCD_WriteText_dzielonyLCD(" Hz");
							}
							TCNT1 = 0; //wyzerowanie timera1 mierzącego czas przed kolejnym pomiarem
							Lczba_przep_tim1 = 0;
							
							///////goto
							poprzednia_liczba_przep_tim1 = 0;
							///////
							
							//kasowanie flagi przrwania od przepełnienia timera1
							ENABLE_TIMER1_OVF_INTERRUPT;
							_delay_ms(200);
							pomiar_czasu = GOTOWY; //ustawienie flagi gotowości do kolejnego pomiaru
							CLEAR_INT0_FLAG; //skasować flage INTF0 w EIFR, żeby nie zaliczyło zbocza sprzed włączenia przerwania
							ENABLE_INT0_INTERRUPT;
							
						}
					}
			}
			
		/****************************Tryb pomiaru częstotliwości*****************************/
		
			if (WYBOR_TRYBU_POMIAR_CZESTOTLIWOSCI) {
				
				//////////////////wstępne ustawienia///////////////////////////
				
					DISABLE_INT0_INTERRUPT; //wyłączenie przerwania, potrzebnego tylko przy pamiarze długości impulsu
					
					///////goto
					poprzednia_liczba_przep_tim1 = 0; //trzeba wyzerować; w trybie pomiaru dł. impulsu uzywane do wykrywania przekroczenia czasu oczekawiania (zbyt długi okres impulsu)
					///////
					
					ENABLE_TIMER0_OVF_INTERRUPT
					ENABLE_TIMER1_OVF_INTERRUPT
					
					Timer1_Start_Falling
					Timer0_Start_8
					

				
				////////////////pętla pomiaru częstotliwości////////////////////

					while(WYBOR_TRYBU_POMIAR_CZESTOTLIWOSCI){
						if (pomiar_czasu == WYSWIETL) {
							pomiar_czasu = GOTOWY; //ustawienie flagi gotowości do kolejnego pomiaru
							
							//wycięte z obsługi przerwania i przeniesione tutaj:
							Liczba_przerwan_tim0 = 0;
							wynik_pomiaru = ((65536 * Lczba_przep_tim1) + TCNT1)*2;/////////
							
							if (PRESKALER) {
								wynik_pomiaru *= 16; //podział preskalera
							}
							
							Lczba_przep_tim1 = 0;
							TCNT1 = 0;
							
							LcdHd44780_clear();
							kursor_LCD = 0;
								
							if (!ZERUJ_POMIAR) { //normalne wyswietlanie
								//ltoa(wynik_pomiaru,Bufor_LCD,10);
								//LCD_WriteText_dzielonyLCD(Bufor_LCD);
								LCD_WriteDecimalLong(wynik_pomiaru);
								LCD_WriteText_dzielonyLCD(" Hz");
							}else{
								if (wynik_pomiaru < 10) {
									//LCD_WriteText_dzielonyLCD("Zmien zakres");
									LCD_WriteText_eeprom(zmien_zakres);
								}else if (wynik_pomiaru < 100) {
									wynik_pomiaru = 1000/wynik_pomiaru;
									//////////////
									//ltoa(wynik_pomiaru,Bufor_LCD,10);
									//LCD_WriteText_dzielonyLCD(Bufor_LCD);
									LCD_WriteDecimalLong(wynik_pomiaru);
									//LCD_WriteText_dzielonyLCD(" ms");
									LCD_WriteText_eeprom(milisekundy);
									//////////////
								}else if (wynik_pomiaru < 100000){
									wynik_pomiaru = 1000000/wynik_pomiaru;
									//////////////
									//ltoa(wynik_pomiaru,Bufor_LCD,10);
									//LCD_WriteText_dzielonyLCD(Bufor_LCD);
									LCD_WriteDecimalLong(wynik_pomiaru);
									LCD_WriteText_dzielonyLCD(" us");
									/////////////
								}else{
									wynik_pomiaru = 1000000000/wynik_pomiaru;
									//ltoa(wynik_pomiaru,Bufor_LCD,10);
									//LCD_WriteText_dzielonyLCD(Bufor_LCD);
									LCD_WriteDecimalLong(wynik_pomiaru);
									LCD_WriteText_dzielonyLCD(" ns");
								}
								
								/*
								ltoa(wynik_pomiaru,Bufor_LCD,10);
								//LCD_WriteText(Bufor_LCD);
								LCD_WriteText_dzielonyLCD(Bufor_LCD);
								//LCD_WriteText(" Hz");
								
								if (wynik_pomiaru < 100) {
									LCD_WriteText_dzielonyLCD(" ms");
								}else{
									LCD_WriteText_dzielonyLCD(" ns");
								}
								*/
							}
							//_delay_ms(300);
							Timer1_Start_Falling;
							Timer0_Start_8;
						}
					}
			}
			
		/***********************Tryb pomiaru częstotliwości z dzielnikiem /16*********************/
			/*
			if (WYBOR_TRYBU_POMIAR_CZ_DZIELNIK_16) {
				
				//////////////////wstępne ustawienia//////////////////////////////////////
				
					
				
				//////////////pętla pomiaru częstotliwości z dzielnikiem  /16////////////
				
					while (WYBOR_TRYBU_POMIAR_CZ_DZIELNIK_16) {
						
						
						
					}
			}
			*/
		/******************************Tryb pomiaru liczby impulsów********************************/
		
			if (WYBOR_TRYBU_POMIAR_LICZBY_IMPULSOW) {
				
				////////////////////////////wstępne ustawienia/////////////////////////////
				
					DISABLE_INT0_INTERRUPT; //wyłączenie przerwania, potrzebnego tylko przy pamiarze długości impulsu
					//TIMER0_STOP;
					DISABLE_TIMER0_OVF_INTERRUPT;
					
					///////goto
					poprzednia_liczba_przep_tim1 = 0; //trzeba wyzerować; w trybie pomiaru dł. impulsu uzywane do wykrywania przekroczenia czasu oczekawiania (zbyt długi okres impulsu)
					///////
					
					ENABLE_TIMER1_OVF_INTERRUPT
					Timer1_Start_Falling;
					Lczba_przep_tim1 = 0;
					TCNT1 = 0;
					poprzedni_wynik = 0;
					
				
				///////////////////pętla pomiaru pomiaru liczby impulsów//////////////////////
				
					while (WYBOR_TRYBU_POMIAR_LICZBY_IMPULSOW) {
						
						wynik_pomiaru = ((65536 * Lczba_przep_tim1) + TCNT1);
						
						if (PRESKALER) {
							wynik_pomiaru *= 16; //podział preskalera
						}
						
						LcdHd44780_clear();
						kursor_LCD = 0;
						if(wynik_pomiaru >= poprzedni_wynik) {
							//ltoa(wynik_pomiaru,Bufor_LCD,10);
							//LCD_WriteText_dzielonyLCD(Bufor_LCD);
							LCD_WriteDecimalLong(wynik_pomiaru);
							//LCD_WriteText_dzielonyLCD(" impulsow");
							LCD_WriteText_eeprom(impulsow);
						}else{
							//LCD_WriteText_dzielonyLCD("Przepelnienie");
							LCD_WriteText_eeprom(Przepelnienie);
							while(ZERUJ_POMIAR == 0 && WYBOR_TRYBU_POMIAR_LICZBY_IMPULSOW == 1);
						}
						poprzedni_wynik = wynik_pomiaru;
						
						for(uint8_t i=0;i<=20;++i){
							if (ZERUJ_POMIAR) {
								Lczba_przep_tim1 = 0;
								TCNT1 = 0;
								poprzedni_wynik = 0;
							}
							_delay_ms(10);
						}
						
						
					}
			}	
	}
}
