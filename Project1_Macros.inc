cseg
;##CUSTOM MACROS##
;----------------------------------------------------------;
; ADC MUXED BUTTONS                                        ;
; when button pressed, adc reading changes to 1/7 step V   ;
; calculates which button pressed based on voltage reading ;
; all buttons connected to pin 1.7                         ;
;----------------------------------------------------------;

;---------------------------------------;
; ADC_to_PB                             ;
; stores values of pb connected to AIN0 ;
; EXCEPT: shiftpb						;
; parameters (flags):					;
; setup, incdec, temprfl, timerfl, 		;
; tempsoak, timesoak, pwmpb 			;
;---------------------------------------;

ADC_to_PB MAC
	mov ADCCON0, #0xF1
	
	clr ADCF
	setb ADCS   ; ADC start trigger signal
    jnb ADCF, $ ; Wait for conversion complete

ADC_to_setup_pb%M:
	clr c
	mov a, ADCRH
	subb a, #0x30
	jc ADC_to_incdec_pb%M
	cpl %0

	;check incdecpb
ADC_to_incdec_pb%M:
	clr c
	mov a, ADCRH 	;ADRH: ADC result 8 high bits, since all lower bits are the same between each voltage val
	subb a, #0x50 	;if 0x50 > ADRH, carry = 1 (negative num), incDECpb not pressed
	jc ADC_to_temprfl_pb%M
	cpl %1

	; Check temprfl_pb
ADC_to_temprfl_pb%M:
	clr c
	mov a, ADCRH
	subb a, #0x70
	jc ADC_to_timerfl_pb%M
	cpl %2

	; Check timerfl_pb
ADC_to_timerfl_pb%M:
	clr c
	mov a, ADCRH
	subb a, #0x90
	jc ADC_to_tempsoak_pb%M
	cpl %3

	; Check tempsoak_pb
ADC_to_tempsoak_pb%M:
	clr c
	mov a, ADCRH
	subb a, #0xB0
	jc ADC_to_timesoak_pb%M
	cpl %4

	; Check timesoak_pb
ADC_to_timesoak_pb%M:
	clr c
	mov a, ADCRH
	subb a, #0xD0
	jc ADC_to_powerpb%M  ;has no landing?
	cpl %5

	; Check pwmpb
ADC_to_powerpb%M:   ;
	clr c
	mov a, ADCRH
	subb a, #0xF0
	jc ADC_to_pbdone%M
	cpl %6
	
ADC_to_pbdone%M:
	; No pushbutton pressed	or all buttons checked
	mov ADCCON0, #0xF7
	ENDMAC

;---------------------------------------;
; ADC_incdec                            ;
; store val of SHIFTPB connected to 	;
; AIN0									;
; parameters (flags):					;
; shiftpb								;
;---------------------------------------;
ADC_incdec MAC
	mov ADCCON0, #0xF1 ; Select AIN0
	
	clr ADCF
	setb ADCS   ; ADC start trigger signal
    jnb ADCF, $ ; Wait for conversion complete

	clr %0

ADC_to_shift_pb%M:
	clr c
	mov a, ADCRH
	subb a, #0x1F
	jc ADC_to_shiftpbdone%M
	setb %0

ADC_to_shiftpbdone%M:
mov ADCCON0, #0xF7
ENDMAC

;------------------------------------------------------;
; TempSoakSet                                   	   ;
; function: set TempSoak     						   ;
; parameters: tempsoak, tempsoak (FLAG), incdec (FLAG) ;
;------------------------------------------------------;
TempSoakSet MAC
		jnb %2, endTempSoakSet%M ;exit if controller not set to adjust tempsoak

		mov a, %1
        jnb %2, increaseTempS%M ;if dec pb not pushed, increase tempsoak
		subb a, #130
        jz endTempSoakSet%M ;if tempsoak = to 130 (minimum temp), do not decrease
        dec %0
        sjmp endTempSoakSet%M

	increaseTempS%M:
		subb a, #170
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
		subb a, #60
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
        jnb %2, increaseTempR%M ;if dec pb not pushed, increase temprfl
		subb a, #150
        jz endTempRflSet%M ;if temprfl = to 150 (minimum temp), do not decrease
        dec %0
        sjmp endTempRflSet%M

	increaseTempR%M:
		subb a, #220
		jz endTempRflSet%M ;if temprfl = to 220, do not increase
		inc %0
endTempRflSet%M:
ENDMAC

;------------------------------------------------------;
; TimeRflSet										   ;
; function: set TimeRfl		   						   ;
; parameters: timerfl, timerfl (FLAG), incdec (FLAG)   ;	
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

;------------------------------------------------------;
; CMPTempTime			      		       ;
; function: make sure time != 60 and temp < 50, sets   ;
; flag if not					       ;
; parameters: state_seconds, temp, error (flag)	       ;	
;------------------------------------------------------;
CMPTempTime MAC
clr c
mov a, %0
subb a, #0x60

jnz SafetyNet%M; jump if time != 60 
SafetyNet%M:
jnc NoError%M; jump if time !> 60

CheckTemp%M:
clr c
mov a, %1 
subb a, #0x49
jc NoError%M; jump if temp > 49C
setb %2
sjmp CMPTempTime_done%M

NoError%M:
clr %2 ;sets flag to zero if no error (or no error yet)

CMPTempTime_done%M:
ENDMAC
