;  N76E003 pinout:
;                               -------
;       PWM2/IC6/T0/AIN4/P0.5 -|1    20|- P0.4/AIN5/STADC/PWM3/IC3
;               TXD/AIN3/P0.6 -|2    19|- P0.3/PWM5/IC5/AIN6/           PB0
;               RXD/AIN2/P0.7 -|3    18|- P0.2/ICPCK/OCDCK/RXD_1/[SCL]  PB1
;                    RST/P2.0 -|4    17|- P0.1/PWM4/IC4/MISO/           PB2
;        INT0/OSCIN/AIN1/P3.0 -|5    16|- P0.0/PWM3/IC3/MOSI/T1/        PB3
;              INT1/AIN0/P1.7 -|6    15|- P1.0/PWM2/IC2/SPCLK
;                         GND -|7    14|- P1.1/PWM1/IC1/AIN7/CLO
;[SDA]/TXD_1/ICPDA/OCDDA/P1.6 -|8    13|- P1.2/PWM0/IC0
;                         VDD -|9    12|- P1.3/SCL/[STADC]/             PB4
;            PWM5/IC7/SS/P1.5 -|10   11|- P1.4/SDA/FB/PWM1
;                               -------
;

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
    
    ADC_to_PB:
	anl ADCCON0, #0xF0
	orl ADCCON0, #0x00 ; Select AIN0
	
	clr ADCF
	setb ADCS   ; ADC start trigger signal
    jnb ADCF, $ ; Wait for conversion complete

	setb decpb
	setb incpb
	setb temprfl_pb
	setb timerfl_pb
	setb tempsoak_pb
	setb timesoak_pb
	setb powerpb
	
	;Check decpb
    ;goes from button resulting in highest voltage to lowest voltage

ADC_to_decpb:
	clr c
	mov a, ADCRH ; ADRH: ADC result 8 high bits, since all lower bits are the same between each voltage val
	subb a, #0xFF ;if 0xFF > ADRH, carry = 1 (negative num), PB7 not pressed
	jc ADC_to_incpb
	clr decpb
	ret

	; Check incpb
ADC_to_incpb:
	clr c
	mov a, ADCRH
	subb a, #0xDA
	jc ADC_to_temprfl_pb
	clr incpb
	ret

	; Check temprfl_pb
ADC_to_temprfl_pb:
	clr c
	mov a, ADCRH
	subb a, #0xB6
	jc ADC_to_timerfl_pb
	clr temprfl_pb
	ret

	; Check timerfl_pb
ADC_to_timerfl_pb:
	clr c
	mov a, ADCRH
	subb a, #0x91
	jc ADC_to_tempsoak_pb
	clr timerfl_pb
	ret

	; Check tempsoak_pb
ADC_to_tempsoak_pb:
	clr c
	mov a, ADCRH
	subb a, #0x6D
	jc ADC_to_timesoak_pb
	clr tempsoak_pb
	ret

	; Check timesoak_pb
ADC_to_timesoak_pb:
	clr c
	mov a, ADCRH
	subb a, #0x48
	jc ADC_to_powerpb
	clr timesoak_pb
	ret

	; Check powerpb
ADC_to_powerpb:
	clr c
	mov a, ADCRH
	subb a, #0x24
	jc ADC_to_pbdone
	clr powerpb
	ret
	
ADC_to_pbdone:
	; No pushbutton pressed	
	ret


