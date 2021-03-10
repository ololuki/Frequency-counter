/*************************************************************************************/
//
//	Makra do ustawiania rejestrów odpowiedzialnych za timery i przerwania
//	Mikrokotroler:	Attiny2313
//	Kompilator: 	AVR-GCC
//	Autor: 			Ololuki
//
/*************************************************************************************/



#define ENABLE_INT0_INTERRUPT GIMSK |= (1 << INT0);
#define DISABLE_INT0_INTERRUPT GIMSK &= ~(1 << INT0);
#define ENABLE_INT1_INTERRUPT GIMSK |= (1 << INT1);
#define DISABLE_INT1_INTERRUPT GIMSK &= ~(1 << INT1);

#define INT0_FALLING MCUCR |= (1 << ISC01); MCUCR &= ~(1 << ISC00);
#define INT0_RISING MCUCR |= (1 << ISC01) | (1 << ISC00);
#define INT0_CHANGE MCUCR &= ~(1 << ISC01); MCUCR |= (1 << ISC00);
#define INT0_LOW_LEVEL MCUCR &= ~((1 << ISC01) | (1 << ISC00));

#define INT1_FALLING MCUCR |= (1 << ISC11); MCUCR &= ~(1 << ISC10);
#define INT1_RISING MCUCR |= (1 << ISC11) | (1 << ISC10);
#define INT1_CHANGE MCUCR &= ~(1 << ISC11); MCUCR |= (1 << ISC10);
#define INT1_LOW_LEVEL MCUCR &= ~((1 << ISC11) | (1 << ISC10));

#define CLEAR_INT0_FLAG EIFR |= (1<<INTF0); //kasowanie flagi wyst¹pienia przerwania INT0
#define CLEAR_INT1_FLAG EIFR |= (1<<INTF1); //kasowanie flagi wyst¹pienia przerwania INT1

#define Timer0_Start_8 TCCR0B |= (1 << CS01); //start timera0 z preskalerem /8
#define Timer0_Start_64 TCCR0B |= (1 << CS01) | (1 << CS00); //start timera0 z preskalerem /64
#define Timer0_Start_256 TCCR0B |= (1 << CS02); //start timera0 z preskalerem /256
#define TIMER0_STOP TCCR0B &= ~((1 << CS00) | (1 << CS01) | (1 << CS02)); //zatrzymanie timera0

#define ENABLE_TIMER0_OVF_INTERRUPT TIMSK |= (1 << TOIE0); //w³¹czenie przerwania od przepe³nienia timera0
#define DISABLE_TIMER0_OVF_INTERRUPT TIMSK &= ~(1 << TOIE0); //wy³¹czenie przerwania od przepe³nienia timera0

#define TIMER1_START_1 TCCR1B = (TCCR1B & ~((1 << CS11) | (1 << CS12))) | (1 << CS10); //start timera1 bez preskalera
#define Timer1_Start_64 TCCR1B |= (1 << CS11) | (1 << CS10); //start timera1 z preskalerem /64
#define Timer1_Start_256 TCCR1B |= (1 << CS12); //start timera1 z preskalerem /256
#define Timer1_Start_Falling TCCR1B |= (1 << CS12) | (1 << CS11); //start timera1 licznik opadaj¹ce zbocze
#define Timer1_Start_Rising TCCR1B |= (1 << CS12) | (1 << CS11) | (1 << CS10); //start timera1 licznik narastaj¹ce zbocze
#define TIMER1_STOP TCCR1B &= ~((1 << CS12) | (1 << CS11) | (1 << CS10));

#define ENABLE_TIMER1_OVF_INTERRUPT TIMSK |= (1 << TOIE1); //w³¹czenie przerwania od przepe³nienia timera1
#define DISABLE_TIMER1_OVF_INTERRUPT TIMSK &= ~(1 << TOIE1); //wy³¹czenie przerwania od przepe³nienia timera1

#define TIMER1_MODE_CTC_OCR1A TCCR1B |= (1 << WGM12); //w³¹czenie timera1 w trybie CTC - reset po osi¹gniêciu wartoœci OCR1A