cseg
;##CUSTOM MACROS##
;----------------------------------------------------------;
; ADC MUXED BUTTONS                                        ;
; when button pressed, adc reading changes to 1/7 step V   ;
; calculates which button pressed based on voltage reading ;
; all buttons connected to pin 1.7                         ;
;----------------------------------------------------------;

;---------------------------------------;
; ADC_to_varpb                          ;
; stores variable pbs connected to AIN0 ;
; parameters (flags):					;
; temprefl, timerefl, tempsoak,  		;
; timesoak								;
;---------------------------------------;
;ADC VALUES: 249, 492, 6DB, 924, B6D, DB6, FFF

ADC_to_varpb MAC
	push acc
	push psw
		mov a, #0xF1
		mov ADCCON0, a
		
		clr ADCF
		setb ADCS   ; ADC start trigger signal
		jnb ADCF, $ ; Wait for conversion complete

		; Check temprfl_pb
	ADC_to_temprfl_pb%M:
		clr c
		mov a, ADCRH
		subb a, #0x98
		jc ADC_to_timerfl_pb%M
		cpl %0

		; Check timerfl_pb
	ADC_to_timerfl_pb%M:
		clr c
		mov a, ADCRH
		subb a, #0x65
		jc ADC_to_tempsoak_pb%M
		cpl %1

		; Check tempsoak_pb
	ADC_to_tempsoak_pb%M:
		clr c
		mov a, ADCRH
		subb a, #0xFF
		jc ADC_to_timesoak_pb%M
		cpl %2

		; Check timesoak_pb
	ADC_to_timesoak_pb%M:
		clr c
		mov a, ADCRH
		subb a, #0xCB
		jc ADC_to_pbdone%M
		cpl %3
		
	ADC_to_pbdone%M:
		; No pushbutton pressed	or all buttons checked
		mov a, #0xF7
		mov ADCCON0, a
	pop psw
	pop acc
	ENDMAC

;---------------------------------------;
; ADC_idpb                        		;
; store val of idpb connected to 		;
; AIN1									;
; parameters (flags):					;
; idpb									;
;---------------------------------------;
ADC_idpb MAC
	push acc
	push psw
		mov a, #0xF1
		mov ADCCON0, a ; Select AIN1
		
		clr ADCF
		setb ADCS   ; ADC start trigger signal
		jnb ADCF, $ ; Wait for conversion complete
		

	ADC_idpbskip%M:
		clr c
		mov a, ADCRH
		subb a, #0x32
		jc ADC_to_idpbdone%M
		cpl %0

	ADC_to_idpbdone%M:
	mov a, #0xF7
	mov ADCCON0, a
pop psw
pop acc
ENDMAC

;------------------------------------------------------;
; TempSoakSet                                   	   ;
; function: set TempSoak     						   ;
; parameters: tempsoak, tempsoak (FLAG), 			   ;
; incdec (FLAG) 									   ;
;------------------------------------------------------;
TempSoakSet MAC
	push acc
	push psw
		clr c
		jnb %1, endTempSoakSet%M ;exit if tempsoak pb not pressed
        
		jnb %2, increaseTempS%M ;if dec pb not pushed, increase tempsoak
		
		mov a, %0
		subb a, #129
		jnc endTempSoakSet%M ;if tempsoak = 130, do not decrease
		dec %0
        sjmp endTempSoakSet%M

	increaseTempS%M:
		clr c
		mov a, %0
		subb a, #169
		jnc endTempSoakSet%M
		inc %0
endTempSoakSet%M:
	pop psw
	pop acc
ENDMAC

;------------------------------------------------------;
; TimeSoakSet                                   	   ;
; function: set TimeSoak     						   ;
; parameters: timesoak, timesoak (FLAG), incdec (FLAG) ;	
;------------------------------------------------------;
TimeSoakSet MAC
	push acc
	push psw
		jnb %1, endTimeSoakSet%M ;exit if timesoak not pressed

		mov a, %0

        jnb %2, increaseTimeS%M ;if incdec = 0, increase timesoak

		jz endTimeSoakSet%M ;if timesoak = to 0 (minimum time), do not decrease
        
		dec %0
        sjmp endTimeSoakSet%M

	increaseTimeS%M:
		subb a, #59
		jnc endTimeSoakSet%M
		inc %0
endTimeSoakSet%M:
	pop psw
	pop acc
ENDMAC

;------------------------------------------------------;
; TempReflSet                                   	   ;
; function: set TempRefl     						   ;
; parameters: temprefl, temprefl (FLAG),  				;
; incdec (FLAG)						
;------------------------------------------------------;
TempReflSet MAC
	push acc
	push psw
	
		jnb %1, endTempReflSet%M ;exit if temprefl not pressed

        jnb %2, increaseTempR%M ;if incdec = 0, increase temprefl
		
		clr c
		mov a, %0 
		subb a, #149
        jnc endTempReflSet%M ;if temprefl != 150, decrease

		decTempR%M:
			dec %0
        	sjmp endTempReflSet%M

	increaseTempR%M:
		mov a, %0 
		subb a, #219
		jnc endTempReflSet%M ;if temprefl != 220, increase
		
		inc %0
endTempReflSet%M:
	pop psw
	pop acc
ENDMAC

;------------------------------------------------------;
; TimeReflSet										   ;
; function: set TimeRefl		   					   ;
; parameters: timerefl, timerefl (FLAG), incdec (FLAG) ;	
;------------------------------------------------------;
TimeReflSet MAC
	push acc
	push psw
		jnb %1, endTimeReflSet%M ;exit if timerefl not pressed

        jnb %2, increaseTimeR%M ;if set to inc, increase timerefl

		mov a, %0
		jz endTimeReflSet%M ;if timerefl = 0 (minimum time), do not decrease
        
		dec %0
        sjmp endTimeReflSet%M

	increaseTimeR%M:
		clr c
		mov a, %0
		subb a, #44
		jnc endTimeReflSet%M ;if timerefl = 45, do not increase
		
		inc %0
endTimeReflSet%M:
	pop psw
	pop acc
ENDMAC

;------------------------------------------------------;
; CMPTempTime										   ;
; function: make sure time != 60 and temp < 50, sets   ;
; flag if not										   ;
; parameters: state_seconds, temp, error (flag)	   	   ;	
;------------------------------------------------------;
CMPTempTime MAC
	push acc
	push psw
	clr c
	mov a, %0
	subb a, #60
	jc CMPTempTime_done%M
	
    clr c
	mov a, %1 
	subb a, #49
	jnc NoError%M ;jump if temp > 49C
	setb %2
	sjmp CMPTempTime_done%M

	NoError%M:
		clr %2 ;sets flag to zero if no error (or no error yet)

CMPTempTime_done%M:
	pop psw
	pop acc
ENDMAC
