`include "cypress.v"

// SD_CID_RESPONDER — UDB Verilog — modo sniffer (solo lectura)
//
// Detecta CMD2 (ALL_SEND_CID) y CMD9 (SEND_CSD) en el bus SD.
// El PSoC NUNCA conduce el bus. Todos los outputs de bus están a 0/1 fijos.
//
// Flujo de lectura del CID/CSD:
//   1. CPU hace polling de StatusReg hasta cmd2=1 o cmd9=1.
//   2. CPU espera state=10 (inicio de la respuesta R2 de la tarjeta).
//   3. CPU muestrea el pin SD_CMD (P2[3]) en cada cambio de fifo_cnt_rd.
//      La respuesta R2 tiene 136 bits (ctr va de 0 a 135).
//      Los bytes de CID/CSD están en los bits 8..135 de la trama R2.
//   4. Cuando cmd2/cmd9 vuelve a 0, la FSM regresó a IDLE.
//
// Registros CPU (direcciones asignadas por PSoC Creator al compilar):
//   StatusReg   (STATUS_REG):
//               bit0 = cmd2  — CMD2 detectado, ventana de detección activa
//               bit1 = cmd9  — CMD9 detectado
//               bit3:2 = state  (00=IDLE, 01=start, 10=RESP, 11=CMD)
//   fifo_cnt_rd (STATUS_REG):
//               posición de bit dentro de la trama actual (0..135 en R2)
//

module SD_CID_RESPONDER (
    output CMD_OVERRIDE,    // fijo 0 — PSoC nunca conduce
    output CMD_OVERRIDE_N,  // fijo 1
    input  HOST_CLK,        // reloj del bus SD (P2[2])
    input  SD_CMD,          // línea CMD (P2[3])
    output HOST_CMD_DRIVE,  // fijo 1 (idle)
    output DBG0,            // state[0]
    output DBG1             // copia de SD_CMD
);

reg [1:0] state;    // 00=IDLE, 01=start-bit, 10=RESPONSE, 11=COMMAND
reg [7:0] ctr;      // posición de bit dentro de la trama
reg [5:0] cmd;      // ventana deslizante 6 bits para detectar índice de comando
reg cmd2;           // flag CMD2 (ALL_SEND_CID) activo
reg cmd9;           // flag CMD9 (SEND_CSD) activo

wire clk_n = ~HOST_CLK;
wire [7:0] status_reg_out;

assign CMD_OVERRIDE   = 1'b0;
assign CMD_OVERRIDE_N = 1'b1;
assign HOST_CMD_DRIVE = 1'b1;
assign DBG0 = state[0];
assign DBG1 = SD_CMD;

assign status_reg_out[0]   = cmd2;
assign status_reg_out[1]   = cmd9;
assign status_reg_out[3:2] = state;
assign status_reg_out[7:4] = 4'b0;

// ventana deslizante para detectar índice de comando
always @(posedge HOST_CLK) begin
    cmd <= {cmd[4:0], SD_CMD};
end

// FSM de framing SD
always @(posedge HOST_CLK)
    case (state)
    2'b00: // IDLE — espera bit de start
        if (SD_CMD == 1'b0)
            state <= 2'b01;
    2'b01: // start-bit: bit de transmisión determina dirección
        if (SD_CMD == 1'b0) state <= 2'b10; // respuesta tarjeta→host
        else                state <= 2'b11; // comando host→tarjeta
    2'b10: // RESPONSE
        if (cmd2 || cmd9) begin
            if (ctr == 8'b10000101) begin   // fin R2 (136 bits)
                state <= 2'b00;
                cmd2  <= 1'b0;
                cmd9  <= 1'b0;
            end
        end else begin
            if (ctr == 8'b101101) state <= 2'b00; // fin R1 (48 bits)
        end
    2'b11: // COMMAND
        begin
            if (ctr == 8'h06 && cmd == 6'b000010) cmd2 <= 1'b1; // CMD2
            if (ctr == 8'h06 && cmd == 6'b001001) cmd9 <= 1'b1; // CMD9
            if (ctr == 8'b101101) state <= 2'b00;
        end
    endcase

// contador de posición dentro de la trama
always @(posedge HOST_CLK) begin
    if (state[1] == 1'b0) ctr <= 8'b0;
    else                  ctr <= ctr + 1;
end

cy_psoc3_status #(.cy_force_order(`TRUE), .cy_md_select(8'b00000000))
    fifo_cnt_rd (.status(ctr), .reset(1'b0), .clock(clk_n));

cy_psoc3_status #(.cy_force_order(`TRUE), .cy_md_select(8'b00000000))
    StatusReg   (.status(status_reg_out), .reset(1'b0), .clock(clk_n));

endmodule
