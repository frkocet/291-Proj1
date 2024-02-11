cseg
    
;----------------------------------------------------------;
; ADC MUXED BUTTONS                                        ;
; when button pressed, adc reading changes to 1/7 step V   ;
; calculates which button pressed based on voltage reading ;
; all buttons connected to pin 1.7                         ;
;----------------------------------------------------------;

;------------------------------------------------------------------------------------------------;
; INITIALIZATON:                                                                                 ;
; $NOLIST                                                                                        ;
; $MODN76E003                                                                                    ;
; $LIST                                                                                          ;
;                                                                                                ;
; CLK               EQU 16600000 ; Microcontroller system frequency in Hz                        ;
; BAUD              EQU 115200 ; Baud rate of UART in bps                                        ;
; TIMER1_RELOAD     EQU (0x100-(CLK/(16*BAUD)))                                                  ;
; TIMER0_RELOAD_1MS EQU (0x10000-(CLK/1000))                                                     ;
;                                                                                                ;
;                                                                                                ;
; BSEG                                                                                           ;
; **These eight bit variables store the value of the pushbuttons after calling 'ADC_to_PB' below ;
; powerpb: dbit 1                                                                                ;
; timesoak_pb: dbit 1                                                                            ;
; tempsoak_pb: dbit 1                                                                            ;
; timerfl_pb: dbit 1                                                                             ;
; temprfl_pb: dbit 1                                                                             ;
; incpb: dbit 1                                                                                  ;
; decpb: dbit 1                                                                                  ;
; PB7: dbit 1                                                                                    ;
;                                                                                                ;
; CSEG                                                                                           ;
; Init_All:                                                                                      ;
; 	; Configure all the pins for biderectional I/O                                               ;
; 	mov	P3M1, #0x00                                                                              ;
; 	mov	P3M2, #0x00                                                                              ;
; 	mov	P1M1, #0x00                                                                              ;
; 	mov	P1M2, #0x00                                                                              ;
; 	mov	P0M1, #0x00                                                                              ;
; 	mov	P0M2, #0x00                                                                              ;
; 	                                                                                             ;
; 	orl	CKCON, #0x10 ; CLK is the input for timer 1                                              ;
; 	orl	PCON, #0x80 ; Bit SMOD=1, double baud rate                                               ;
; 	mov	SCON, #0x52                                                                              ;
; 	anl	T3CON, #0b11011111                                                                       ;
; 	anl	TMOD, #0x0F ; Clear the configuration bits for timer 1                                   ;
; 	orl	TMOD, #0x20 ; Timer 1 Mode 2                                                             ;
; 	mov	TH1, #TIMER1_RELOAD ; TH1=TIMER1_RELOAD;                                                 ;
; 	setb TR1                                                                                     ;
; 	                                                                                             ;
; 	; Using timer 0 for delay functions.  Initialize here:                                       ;
; 	clr	TR0 ; Stop timer 0                                                                       ;
; 	orl	CKCON,#0x08 ; CLK is the input for timer 0                                               ;
; 	anl	TMOD,#0xF0 ; Clear the configuration bits for timer 0                                    ;
; 	orl	TMOD,#0x01 ; Timer 0 in Mode 1: 16-bit timer                                             ;
; 	                                                                                             ;
; 	; Initialize and start the ADC:                                                              ;
; 	                                                                                             ;
; 	; AIN0 is connected to P1.7.  Configure P1.7 as input.                                       ;
; 	orl	P1M1, #0b10000000                                                                        ;
; 	anl	P1M2, #0b01111111                                                                        ;
; 	                                                                                             ;
; 	; AINDIDS select if some pins are analog inputs or digital I/O:                              ;
; 	mov AINDIDS, #0x00 ; Disable all analog inputs                                               ;
; 	orl AINDIDS, #0b00000001 ; Using AIN0                                                        ;
; 	orl ADCCON1, #0x01 ; Enable ADC                                                              ;
; 	                                                                                             ;
; 	ret                                                                                          ;
;------------------------------------------------------------------------------------------------;
    
;---------------------------------------;
; ADC_to_PB                             ;
; stores values of pb connected to AIN0 ;
;---------------------------------------;

ADC_to_PB:
	anl ADCCON0, #0xF0
	orl ADCCON0, #0x00 ; Select AIN0
	
	clr ADCF
	setb ADCS   ; ADC start trigger signal
    jnb ADCF, $ ; Wait for conversion complete

	setb incdecpb ; 1 if increasing, 0 if decreasing
    ;0 if setting time/temp, 1 if not
	setb temprfl_pb 
	setb timerfl_pb
	setb tempsoak_pb
	setb timesoak_pb
	setb powerpb
	
    ;goes from buttons w highest voltage to lowest voltage

	; Check incdecpb
ADC_to_incdecpb:
	clr c
	mov a, ADCRH ;ADRH: ADC result 8 high bits, since all lower bits are the same between each voltage val
	subb a, #0xFF ;if 0xFF > ADRH, carry = 1 (negative num), PB7 not pressed
	jc ADC_to_temprfl_pb
	clr incdecpb

	; Check temprfl_pb
ADC_to_temprfl_pb:
	clr c
	mov a, ADCRH
	subb a, #0xD4
	jc ADC_to_timerfl_pb
	clr temprfl_pb

	; Check timerfl_pb
ADC_to_timerfl_pb:
	clr c
	mov a, ADCRH
	subb a, #0xAA
	jc ADC_to_tempsoak_pb
	clr timerfl_pb

	; Check tempsoak_pb
ADC_to_tempsoak_pb:
	clr c
	mov a, ADCRH
	subb a, #0x7F
	jc ADC_to_timesoak_pb
	clr tempsoak_pb

	; Check timesoak_pb
ADC_to_timesoak_pb:
	clr c
	mov a, ADCRH
	subb a, #0x55
	jc ADC_to_powerpb
	clr timesoak_pb

	; Check powerpb
ADC_to_powerpb:
	clr c
	mov a, ADCRH
	subb a, #0x2A
	jc ADC_to_pbdone
	clr powerpb
	
ADC_to_pbdone:
	; No pushbutton pressed	
	ret

;------------------------------------------------------;
; CounterSet                                           ;
; function: set counters, doesn't allow dec < 0        ;
; parameters: button to dec/inc, variable to inc/dec,  ;
; label to branch to if button to dec/inc not pushed   ;
;------------------------------------------------------;
CounterSet MAC
    CounterPush%M:
        jb %0, %2
        Wait_Milli_Seconds(#50) ; debounce
        jb %0, %2

        jb incdecpb, increase%M
        mov a, %1
        jz endCounterSet%M
        dec %1
        sjmp endCounterSet%M
increase%M:
    inc %1
endCounterSet%M:
ENDMAC

;-----------------------------------------------------------------;
; CounterCheck                                                    ;
; Checks the counters, resets them if they're out of upper range  ;
;-----------------------------------------------------------------;

CounterCheck:
    TempSoakCheck:
        mov a, #0x150
        subb a, tempsoak
        jnz TimeSoakCheck
        mov a, #0x0
        mov tempsoak, a

    TimeSoakCheck:
        mov a, #0x60
        subb a, timesoak
        jnz TempRflCheck
        mov a, #0x0
        mov timesoak, a

    TempRflCheck:
        mov a, #0x220
        subb a, temprfl
        jnz TimeRflCheck
        mov a, #0x150
        mov tempfl, a

    TimeRflCheck:
        mov a, #0x45
        subb a, timerfl
        jnz CounterCheckDone
        mov a, #0x0
        mov timerfl, a

    CounterCheckDone:
        ret




