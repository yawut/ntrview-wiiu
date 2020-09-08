; $MODE = "UniformRegister"
; $ATTRIB_VARS[0].name = "aPosition"
; $ATTRIB_VARS[0].type = "Float2"
; $ATTRIB_VARS[0].location = 0
; $ATTRIB_VARS[1].name = "aTexCoord"
; $ATTRIB_VARS[1].type = "Float2"
; $ATTRIB_VARS[1].location = 1

; Semantic 0: aTexCoord
; $NUM_SPI_VS_OUT_ID = 1
; $SPI_VS_OUT_ID[0].SEMANTIC_0 = 0

00 CALL_FS NO_BARRIER
01 EXP_DONE: POS0, R1.xy01
02 EXP_DONE: PARAM0, R2.xy00 NO_BARRIER
END_OF_PROGRAM
