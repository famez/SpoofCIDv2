`include "cypress.v"

// SD_CID_RESPONDER — UDB sniffer con captura CID/CSD e interrupciones
//
// Puertos de interrupción — conectar a componentes isr en TopDesign:
//   DBG0  nivel alto mientras CMD2/CMD9 activo  → isr SD_CMD_ISR (Rising Edge)
//   DBG1  pulso 1 ciclo cuando byte listo       → isr SD_BYTE_ISR (Rising Edge)
//
// Registros CPU (cyfitter.h tras Build):
//   StatusReg   bit0=cmd2, bit1=cmd9, bits[3:2]=state (00=IDLE,01=start,10=RESP,11=CMD)
//   fifo_cnt_rd posición de bit en trama actual (0..135 en R2)
//   CaptureReg  último byte completo capturado de la respuesta R2
//
// Estructura respuesta R2 (136 bits = 17 bytes):
//   byte 0  : 0b00111111  (start + dir + cmd_index)
//   bytes 1-16: CID o CSD (128 bits de datos)
//

module SD_CID_RESPONDER (
    output CMD_OVERRIDE,    // fijo 0 — PSoC nunca conduce el bus
    output CMD_OVERRIDE_N,  // fijo 1
    input  HOST_CLK,        // reloj bus SD (P2[2])
    input  SD_CMD,          // línea CMD (P2[3])
    output HOST_CMD_DRIVE,  // fijo 1 (idle)
    output DBG0,            // INT: nivel alto cuando CMD2/CMD9 activo
    output DBG1             // INT: pulso 1 ciclo por byte capturado listo
);

reg [1:0] state;    // 00=IDLE, 01=start, 10=RESPONSE, 11=COMMAND
reg [7:0] ctr;      // posición de bit en trama
reg [5:0] cmd;      // ventana deslizante 6 bits para detectar comando
reg       cmd2;
reg       cmd9;
reg [7:0] shift;  // shift register captura bits de R2
reg       dbg1_r; // DBG1 retrasado 1 ciclo: garantiza que CaptureReg (clk_n) ya latcheó

wire clk_n      = ~HOST_CLK;
wire active     = cmd2 | cmd9;
wire byte_ready = (state == 2'b10) && active && (ctr[2:0] == 3'b111);

wire [7:0] status_reg_out;

assign CMD_OVERRIDE   = 1'b0;
assign CMD_OVERRIDE_N = 1'b1;
assign HOST_CMD_DRIVE = 1'b1;

assign DBG0 = active;  // nivel: alto mientras CMD2/CMD9 activo
assign DBG1 = dbg1_r; // byte_ready retrasado 1 ciclo → ISR lee CaptureReg ya actualizado

assign status_reg_out[0]   = cmd2;
assign status_reg_out[1]   = cmd9;
assign status_reg_out[3:2] = state;
assign status_reg_out[7:4] = 4'b0;

// Ventana deslizante para detectar índice de comando
always @(posedge HOST_CLK) begin
    cmd <= {cmd[4:0], SD_CMD};
end

// FSM de framing SD
always @(posedge HOST_CLK)
    case (state)
    2'b00:
        if (SD_CMD == 1'b0) state <= 2'b01;
    2'b01:
        if (SD_CMD == 1'b0) state <= 2'b10;   // respuesta tarjeta→host
        else                state <= 2'b11;   // comando host→tarjeta
    2'b10:
        if (cmd2 || cmd9) begin
            if (ctr == 8'b10000101) begin      // fin R2 (136 bits)
                state <= 2'b00;
                cmd2  <= 1'b0;
                cmd9  <= 1'b0;
            end
        end else begin
            if (ctr == 8'b101101) state <= 2'b00; // fin R1 (48 bits)
        end
    2'b11:
        begin
            if (ctr == 8'h06 && cmd == 6'b000010) cmd2 <= 1'b1; // CMD2
            if (ctr == 8'h06 && cmd == 6'b001001) cmd9 <= 1'b1; // CMD9
            if (ctr == 8'b101101) state <= 2'b00;
        end
    endcase

// Contador de posición en trama
always @(posedge HOST_CLK) begin
    if (state[1] == 1'b0) ctr <= 8'b0;
    else                  ctr <= ctr + 1;
end

// Captura continua — shift siempre corre, CaptureReg solo se lee cuando byte_ready
always @(posedge HOST_CLK)
    shift <= {shift[6:0], SD_CMD};

// dbg1_r: byte_ready del ciclo anterior.
// En posedge ctr=8: CaptureReg ya latcheó el byte completo en el negedge de ctr=7.
always @(posedge HOST_CLK) dbg1_r <= byte_ready;

cy_psoc3_status #(.cy_force_order(`TRUE), .cy_md_select(8'b00000000))
    StatusReg  (.status(status_reg_out), .reset(1'b0), .clock(clk_n));

// CaptureReg latchea shift en clk_n (negedge entre ctr=7 y ctr=8).
// El ISR dispara en ctr=8 (via dbg1_r) → shift ya es estable.
cy_psoc3_status #(.cy_force_order(`TRUE), .cy_md_select(8'b00000000))
    CaptureReg (.status(shift),          .reset(1'b0), .clock(clk_n));

endmodule
