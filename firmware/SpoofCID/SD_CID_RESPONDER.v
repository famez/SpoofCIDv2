// SD_CID_RESPONDER — UDB Verilog component for SpoofCIDv2
//
// Fuente original: github.com/spoofcid/spoofcid (c) 2017 Rick Sanchez - modimo
// Adaptado para SpoofCIDv2: HOST_CLK=P2[2], HOST_CMD=P2[3]
//
// Este módulo se ejecuta íntegramente en el fabric UDB del PSoC 4.
// La CPU no muestrea ningún bit: solo lee registros de estado y escribe
// nibbles cuando el UDB los solicita.
//
// Interfaz CPU <-> UDB:
//   enable    (CONTROL_REG, CTL_00, 0x400f0070) — escrito por CPU:
//               bit0 = OVERRIDE_EN  : arranca la FSM de detección
//               bit1 = OVERRIDE_CID : habilita inyección en respuesta a CMD2
//               bit2 = OVERRIDE_CSD : habilita inyección en respuesta a CMD9
//               bit7 = kill-switch  : fuerza CMD_OVERRIDE=0 (modo pasivo)
//   dataIn    (CONTROL_REG, CTL_03, 0x400f0073) — escrito por CPU:
//               8 bits, se toma el nibble alto [7:4] cada 4 ciclos HOST_CLK
//               durante la fase de respuesta (es como la CPU pasa el CID byte a byte)
//   StatusReg (STATUS_REG,  ST_02,  0x400f0062) — leído por CPU:
//               bit0 = cmd2 (CMD2 / ALL_SEND_CID detectado y en curso)
//               bit1 = cmd9 (CMD9 / SEND_CSD detectado y en curso)
//               bits[3:2] = state FSM (00=IDLE, 01=start, 10=RESP, 11=CMD)
//   fifo_cnt_rd (STATUS_REG, ST_01, 0x400f0061) — leído por CPU:
//               contador de bits dentro de la trama actual (ctr[7:0])
//               la CPU lo usa para temporizarse al escribir nibbles a dataIn
//
// Secuencia de uso (modo spoof CID):
//   1. CPU escribe enable = OVERRIDE_EN | OVERRIDE_CID  (0x03)
//   2. CPU hace polling de StatusReg esperando bit0=1 (CMD2 detectado)
//   3. CPU espera a que state deje de ser 11 (fin trama CMD → entra en RESP)
//   4. CPU espera a que fifo_cnt_rd sea bajo (ctr recién reseteado)
//   5. CPU escribe 16 bytes del CID en dataIn, 2 escrituras por byte
//      (byte completo: nibble alto en [7:4], luego byte con nibble bajo en [7:4])
//      sincronizando cada escritura con fifo_cnt_rd (incrementa de 4 en 4)
//   6. CPU espera a que cmd2 vuelva a 0 (FSM vuelve a IDLE)
//
`include "cypress.v"

module SD_CID_RESPONDER (
    output CMD_OVERRIDE,    // controla tri-estado del pin HOST_CMD del PSoC (activo = PSoC conduce)
    output CMD_OVERRIDE_N,  // complementario
    input  HOST_CLK,        // reloj del bus SD (P2[2]) — asíncrono a CPU
    input  SD_CMD,          // línea CMD de la tarjeta real (P2[3])
    output HOST_CMD_DRIVE,  // bit que sale hacia el host cuando CMD_OVERRIDE=1
    output DBG0,            // debug: state[0]
    output DBG1             // debug: copia de SD_CMD
);

// ---- registros internos ----
reg [1:0] state;    // FSM: 00=IDLE, 01=start-bit, 10=RESPONSE, 11=COMMAND
reg [7:0] ctr;      // contador de bits dentro de la trama
reg [5:0] cmd;      // ventana deslizante de 6 bits para detectar índice de comando
reg [7:4] dout;     // shift register de 4 bits que genera HOST_CMD_DRIVE
reg cmd2;           // flag: CMD2 detectado en trama actual
reg cmd9;           // flag: CMD9 detectado en trama actual
reg preload;        // pulso: "carga siguiente nibble de data_from_cpu en dout"
reg takebus;        // 1 mientras el PSoC conduce HOST_CMD_DRIVE

wire clk_n = ~HOST_CLK;
wire [7:0] data_from_cpu;   // registro dataIn escrito por CPU
wire [7:0] status_reg_out;
wire [7:0] enable;          // registro enable escrito por CPU

assign CMD_OVERRIDE   = takebus & (~enable[7]);
assign CMD_OVERRIDE_N = ~takebus & (~enable[7]);
assign DBG0 = state[0];
assign DBG1 = SD_CMD;

assign status_reg_out[7:4] = 4'b0;
assign status_reg_out[3:2] = state;
assign status_reg_out[0]   = cmd2;
assign status_reg_out[1]   = cmd9;
assign HOST_CMD_DRIVE      = dout[7];

// captura continua de los últimos 6 bits de SD_CMD
always @(posedge HOST_CLK) begin
    cmd <= cmd << 1;
    cmd[0] <= SD_CMD;
end

// control del bus (flanco de bajada — datos SD estables en flanco de subida)
always @(negedge HOST_CLK) begin
    if (ctr[1:0] == 2'b01 &&
        ((cmd2 && enable[1]) || (cmd9 && enable[2])) &&
        state == 2'b10)
    begin
        takebus <= 1'b1;
        preload <= 1'b1;
    end else begin
        preload <= 1'b0;
        takebus <= takebus && (state[1] == 1'b1);
    end

    if (preload)
        dout <= data_from_cpu[7:4];
    else begin
        dout <= dout << 1;
        dout[4] <= 1'b1;
    end
end

// FSM de framing SD
always @(posedge HOST_CLK)
    case (state)
    2'b00: // IDLE — espera bit de start (0) y enable[0]
        if (SD_CMD == 1'b0 && enable[0] == 1'b1)
            state <= 2'b01;
    2'b01: // start-bit: bit de transmisión determina dirección
        if (SD_CMD == 1'b0) state <= 2'b10; // respuesta tarjeta→host
        else                state <= 2'b11; // comando host→tarjeta
    2'b10: // RESPONSE
        if (cmd2 || cmd9) begin
            if (ctr == 8'b10000101) begin // fin de respuesta larga R2 (136 bits)
                state <= 2'b00;
                cmd2  <= 1'b0;
                cmd9  <= 1'b0;
            end
        end else
            if (ctr == 8'b101101) state <= 2'b00; // fin respuesta corta R1 (48 bits)
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

// registros de control/estado accesibles desde la CPU
cy_psoc3_status #(.cy_force_order(`TRUE), .cy_md_select(8'b00000000))
    fifo_cnt_rd (.status(ctr), .reset(1'b0), .clock(clk_n));

cy_psoc3_status #(.cy_force_order(`TRUE), .cy_md_select(8'b00000000))
    StatusReg   (.status(status_reg_out), .reset(1'b0), .clock(clk_n));

cy_psoc3_control #(.cy_init_value(8'b11111111), .cy_force_order(`TRUE))
    dataIn  (.control(data_from_cpu));

cy_psoc3_control #(.cy_init_value(8'b11111111), .cy_force_order(`TRUE))
    enable  (.control(enable));

endmodule
