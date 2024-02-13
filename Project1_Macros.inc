cseg

;##COPIED FROM LCD_4bit.inc##
;---------------------------------;
; Wait 'R2' milliseconds          ;
;---------------------------------;
Wait_Milli_Seconds mac
	push AR2
	mov R2, %0
	lcall ?Wait_Milli_Seconds
	pop AR2
endmac

;---------------------------------;
; Wait 'R2' milliseconds          ;
;---------------------------------;
?Wait_Milli_Seconds:
    push AR0
    push AR1
L3: mov R1, #40
L2: mov R0, #104
L1: djnz R0, L1 ; 4 cycles->4*60.24ns*104=25.0us
    djnz R1, L2 ; 25us*40=1.0ms
    djnz R2, L3 ; number of millisecons to wait passed in R2
    pop AR1
    pop AR0
    ret

;##CUSTOM MACROS##
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
; bseg																							 ;
; PUSH BUTTON FLAGS:																			 ;
; (matches push button values)																	 ;
; incdec_flag: dbit 1 ;controller set to increase counters if 0, decrease if 1					 ;
; temprfl_flag: dbit 1 ;controller set to adjust temprfl if 0									 ;
; timerfl_flag: dbit 1 ;controller set to adjust timerfl if 0									 ;
; tempsoak_flag: dbit 1 ;controller set to adjust temprfl if 0									 ;
; timesoak_flag: dbit 1 ;controller set to adjust timesoak if 0									 ;
; setup_flag: dbit 1; controller set to adjust counters	(so users can't adjust mid reflow)		 ;
; pwm_flag: dbit 1 ; start/stop switch, 0 if start, 1 if stop                          			 ;
; ^^SET ALL TO 0 INITIALLY    																	 ;
; shiftpb: dbit 1 ;button to increase and decrease the counters                                  ;
;																								 ;
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
; EXCEPT: shiftpb						;
; parameters (flags):					;
; setup, incdec, temprfl, timerfl, 		;
; tempsoak, timesoak, pwmpb 			;
;---------------------------------------;

ADC_to_PB MAC
	anl ADCCON0, #0xF0
	orl ADCCON0, #0x00 ; Select AIN0
	
	clr ADCF
	setb ADCS   ; ADC start trigger signal
    jnb ADCF, $ ; Wait for conversion complete

ADC_to_setup_pb%M:
	clr c
	mov a, ADCRH
	subb a, #0x3F
	jc ADC_to_incdec_pb%M
	cpl %0

	;check incdecpb
ADC_to_incdecpb%M:
	clr c
	mov a, ADCRH ;ADRH: ADC result 8 high bits, since all lower bits are the same between each voltage val
	subb a, #0x5F ;if 0x48 > ADRH, carry = 1 (negative num), incDECpb not pressed
	jc ADC_to_temprfl_pb%M
	cpl %1

	; Check temprfl_pb
ADC_to_temprfl_pb%M:
	clr c
	mov a, ADCRH
	subb a, #0x7F
	jc ADC_to_timerfl_pb%M
	cpl %2

	; Check timerfl_pb
ADC_to_timerfl_pb%M:
	clr c
	mov a, ADCRH
	subb a, #0x9F
	jc ADC_to_tempsoak_pb%M
	cpl %3

	; Check tempsoak_pb
ADC_to_tempsoak_pb%M:
	clr c
	mov a, ADCRH
	subb a, #0xBF
	jc ADC_to_timesoak_pb%M
	cpl %4

	; Check timesoak_pb
ADC_to_timesoak_pb%M:
	clr c
	mov a, ADCRH
	subb a, #0xDF
	jc ADC_to_powerpb%M
	cpl %5

	; Check pwmpb
ADC_to_pwmpb%M:
	clr c
	mov a, ADCRH
	subb a, #0xFF
	jc ADC_to_pbdone%M
	cpl %6
	
ADC_to_pbdone%M:
	; No pushbutton pressed	or all buttons checked
	ENDMAC

;---------------------------------------;
; ADC_incdec                            ;
; store val of SHIFTPB connected to 	;
; AIN0									;
; parameters (flags):					;
; shiftpb								;
;---------------------------------------;
ADC_incdec MAC
	anl ADCCON0, #0xF0
	orl ADCCON0, #0x00 ; Select AIN0
	
	clr ADCF
	setb ADCS   ; ADC start trigger signal
    jnb ADCF, $ ; Wait for conversion complete

	clr %0

ADC_to_shift_pb%M:
	clr c
	mov a, ADCRH
	subb a, #0x1F
	jc ADC_to_shiftpbdone
	setb %0

ADC_to_shiftpbdone%M:
ENDMAC

;------------------------------------------------------;
; TempSoakSet                                   	   ;
; function: set TempSoak     						   ;
; parameters: tempsoak, tempsoak (FLAG), incdec (FLAG) ;
;------------------------------------------------------;
TempSoakSet MAC
		jnb %2, endTempSoakSet%M ;exit if controller not set to adjust tempsoak

		mov a, %1
        jnb %3, increaseTempS%M ;if dec pb not pushed, increase tempsoak
		subb a, #0x130
        jz endTempSoakSet%M ;if tempsoak = to 130 (minimum temp), do not decrease
        dec %0
        sjmp endTempSoakSet%M

	increaseTempS%M:
		subb a, #0x170
		jz endTempSoakSet%M ;if tempsoak = to 170, do not increase
		inc %0
endTempSoakSet%M:
ENDMAC

;------------------------------------------------------;
; TimeSoakSet                                   	   ;
; function: set TimeSoak     						   ;
; parameters: timesoak, timesoak (FLAG), incdec (FLAG) ;	
;------------------------------------------------------;
TimeSoakSet MAC
		jnb %1, endTimeSoakSet%M ;exit if controller not set to adjust timesoak

		mov a, %0

        jnb %2, increaseTimeS%M ;if dec pb not pushed, increase timesoak
        jz endTimeSoakSet%M ;if timesoak = to 0 (minimum time), do not decrease
        dec %0
        sjmp endTimeSoakSet%M

	increaseTimeS%M:
		subb a, #0x60
		jz endTimeSoakSet%M ;if timesoak = to 60, do not increase
		inc %0
endTimeSoakSet%M:
ENDMAC

;------------------------------------------------------;
; TempRflSet                                   	   	   ;
; function: set TempRfl     						   ;
; parameters: temprfl, temprfl (FLAG), incdec (FLAG)   ;
;------------------------------------------------------;
TempRflSet MAC
		jnb %1, endTempRflSet%M ;exit if controller not set to adjust temprfl

		mov a, %0
        jnb %3, increaseTempR%M ;if dec pb not pushed, increase temprfl
		subb a, #0x150
        jz endTempRflSet%M ;if temprfl = to 150 (minimum temp), do not decrease
        dec %0
        sjmp endTempRflSet%M

	increaseTempR%M:
		subb a, #0x220
		jz endTempRflSet%M ;if temprfl = to 220, do not increase
		inc %0
endTempRflSet%M:
ENDMAC

;------------------------------------------------------;
; TimeRflSet										   ;
; function: set TimeRfl		   						   ;
; parameters: timerfl, timerfl (FLAG), incdec (FLAG) ;	
;------------------------------------------------------;
TimeRflSet MAC
		jnb %1, endTimeRflSet%M ;exit if controller not set to adjust timerfl

		mov a, %0

        jnb %2, increaseTimeR%M ;if set to inc, increase timerfl
        jz endTimeRflSet%M ;if timerfl = to 0 (minimum time), do not decrease
        dec %0
        sjmp endTimeRflSet%M

	increaseTimeR%M:
		subb a, #0x45
		jz endTimeRflSet%M ;if timerfl = to 45, do not increase
		inc %0
endTimeRflSet%M:
ENDMAC

