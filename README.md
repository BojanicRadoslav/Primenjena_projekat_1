# Primenjena_projekat_1
GLCD lepo radi, touch screen manje vise.
Opcija za iskljucivanje i ukljucivanje alarma, kada je alarm aktiviran ne moze da se iskljuci dok se ne unese ispravna sifra.
Sifra je "1234", ima promenljiva pa moze da se menja u kodu.
Opcija za tihi alarm, buzzer se ne oglasava nego samo salje na uart koji je alarm aktiviran.
Stats prozor iscrtava grafik zastupljenosti alarma.
Set prozor omogucava podesavanje da li je alarm aktiviran ili nije i da li je tihi alarm ili nije.
Uart se menja sam od sebe iz nekog razloga, BRG = 0x0015, nekad je 2400 a nekad 4800, moras ga traziti.
Odradjene funkcije trigger_pir, trigger_faza, trigger_mq3, kada dodje logicka jedinica sa senzora samo ih pozvati.
Dodati funkciju void setServo(int angle) gde angle ide od 0 do 180, za podseavanje polozaja servo motora.
Buzzerom upravljam sa Tajmerom 2 tako da je za PWM slobodan tajmer 1.
Zauzeti pinovi su: 
                  za GLCD: RB4, RB5, RF0, RF1, RF4, RB0, RB1, RB2, RB3, RD0, RD1, RD2, RD3, RF5
                  za Touch: RC13, RC14
                  za Uart: RF2, RF3
                  za Buzzer: RA11

update 29.12.2020.
	radi servo ali brljavi, hoce da se cima levo desno
	mq3 radi lepo
	idalje menja frekvenciju kako njemu dodje
	uart varira izmedju 2400 i 4800 ne moze da se predvidi

