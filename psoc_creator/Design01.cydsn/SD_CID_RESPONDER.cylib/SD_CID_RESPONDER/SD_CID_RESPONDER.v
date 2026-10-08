`include "cypress.v"

// =============================================================================
// SD_CID_RESPONDER — UDB spoofer de respuesta R2 (CID/CSD), versión mínima.
//
// Estrategia (ver memoria del proyecto [[project-spoof-strategy]]):
//   - CARD_CLK puenteado a HOST_CLK (bodge): la tarjeta recibe siempre reloj y
//     completa su respuesta real aislada, avanzando su máquina de estados.
//   - El byte 0 de toda R2 es 0x3F (idéntico real/spoof): lo conduce la tarjeta.
//     El PSoC toma el bus en el bit 8 y conduce los bytes 1..16 (bits 8..135).
//
// Reloj: toda la lógica corre con CLK (HFCLK 24 MHz, componente Clock en
// TopDesign). HOST_CLK es una ENTRADA DE DATOS: se sincroniza (Double-Sync en
// el pin), se detecta su flanco de subida y ese pulso habilita el reloj (UDB
// Clock/Enable) del resto de la lógica, que así avanza una vez por ciclo de
// HOST_CLK, unos 2-3 ciclos de CLK (~85-125 ns) después del flanco real.
//
// Muestreo de CMD: HOST_CMD pasa por 2 FF más que HOST_CLK, de modo que el
// valor que usa la lógica es el de 42-84 ns ANTES del flanco de subida del
// host (no el de después). Así vale para hosts que cambian CMD en el flanco
// de bajada y para los que lo cambian justo tras la subida (hold de sólo
// 5 ns según la especificación SD).
// REQUIERE: pines HOST_CLK y CARD_CMD con Sync mode = Double-Sync.
//
// Sin relojes ruteados
// desde pines no hay skew entre UDBs (violaciones de hold) ni cruces
// asíncronos con los registros de la CPU. Válido para HOST_CLK <= ~2 MHz.
//
// Optimización de recursos (caber en 32 macrocells / 64 P-terms del CY8C4245):
//   - `ctr` de 8 bits: llega a 45 (fin de comando / R1) y a 133 (fin de R2).
//     Las comparaciones de fin se registran un ciclo antes (end_r, at7_r) para
//     que ninguna ecuación pase de 12 entradas (evita splits).
//   - La FSM sale sola de RESPONSE: a los 48 bits (R1/R3/R6/R7) o, si hubo
//     CMD2/CMD9 (active), a los 136 bits (R2), y entonces limpia active. El
//     firmware ya no necesita det_clr (queda sólo como reset de emergencia).
//   - Un comando nuevo limpia active: si el host manda CMD2/CMD9 y no llega
//     respuesta, la detección no se queda colgada para la trama siguiente.
//   - El FIN de TX lo marca el propio UDB (end_r en el bit 135). El firmware
//     desarma tx_arm tras entregar frame[16] para no arrancar en otra R2.
//   - El registro de desplazamiento de TX vive en una DATAPATH (dp): desplaza
//     dentro de un único bloque, sin cruzar UDBs. Con flip-flops en PLD el
//     skew del reloj ruteado desde pin entre UDBs (~5 ns) provocaba una
//     violación de hold (txsh_5→txsh_6). Además ahorra 8 macrocells.
//   - Sin FFs de sincronización para tx_arm/det_clr: CtrlReg y la lógica usan
//     el mismo reloj (CLK) y tx_arm nunca cambia cerca de un punto de carga (se
//     arma ~40 bits antes y se desarma ~8 bits antes).
//   - Detección CMD2/CMD9 en dos etapas (match_r registrado, comprobado en
//     ctr==7) para que ninguna ecuación pase de 12 entradas (evita splits).
//   - Todas las salidas salen directas de un registro (sin macrocells comb.)
//     y todo usa el flanco de subida (un solo reloj → mejor empaquetado).
//
// Mapa de ctr en la fase RESPONSE:  posición_de_bit = ctr + 2
//   (bits 0,1 = start,dir los consumen las transiciones 00->01->10).
//   bit 8 (primer contenido) ↔ ctr muestrea bit 7 en ctr==5.
//   Las cargas de byte caen en ctr[2:0]==5  (ctr = 5,13,21,...,125,133).
//
// PUERTOS (Design01.cydwr / TopDesign):
//   CLK (Clock HFCLK 24 MHz) in ·
//   HOST_CLK P2[2] in · HOST_CMD P2[3] in · PSOC_CMD P0[4] out · SW_IN P1[5] out
//   CMD_DET (int, nivel) · BYTE_REQ (int, pulso)
// REGISTROS CPU:
//   StatusReg bit0=active bit1=is9 (0=CMD2, 1=CMD9) [3:2]=state bit4=tx_run
//   CtrlReg   bit0=tx_arm bit1=det_clr
//   dp.D0     byte a transmitir (el firmware lo refresca en cada BYTE_REQ;
//             registro SD_CID_RESPONDER_1_dp_u0__D0_REG)
// =============================================================================

module SD_CID_RESPONDER (
    input  CLK,
    input  HOST_CLK,
    input  HOST_CMD,
    output PSOC_CMD,
    output SW_IN,
    output CMD_DET,
    output BYTE_REQ
);

// Punto de toma de bus: en el flanco en que el host muestrea el bit 7 (ctr==5)
// se carga el byte y el bit 8 aparece en PSOC_CMD justo después. Ajustar ±1
// en el osciloscopio si hace falta.
localparam [2:0] LOAD_PHASE = 3'd5;   // ctr[2:0] en el que se carga cada byte

// -----------------------------------------------------------------------------
// Sincronización de HOST_CLK y habilitación de reloj
// -----------------------------------------------------------------------------
// HOST_CLK y HOST_CMD llegan ya sincronizados por el pin (Double-Sync, 2 FF
// en el puerto, sin coste de macrocells): valor del pin de hace ~2 ciclos.
reg hclk_d;                        // HOST_CLK retrasado 1 ciclo
reg cmd_d1, cmd_d2;                // HOST_CMD retrasado 2 ciclos más
always @(posedge CLK) begin
    hclk_d <= HOST_CLK;
    cmd_d1 <= HOST_CMD;
    cmd_d2 <= cmd_d1;
end
wire hclk_rise = HOST_CLK & ~hclk_d; // 1 ciclo de CLK por flanco de subida

// Valor de CMD que corresponde a este ciclo de HOST_CLK. Con la lógica
// habilitada actualizándose en el ciclo de CLK siguiente a hclk_rise:
//   flanco del host en (t-2, t-1]  ·  cmd_d2 = pin en t-3  →  42-84 ns antes.
wire cmd_s = cmd_d2;

wire clk_en;                       // CLK habilitado sólo en hclk_rise
cy_psoc3_udb_clock_enable_v1_0 #(.sync_mode(`TRUE))
    ClkEn (.clock_in(CLK), .enable(hclk_rise), .clock_out(clk_en));

// -----------------------------------------------------------------------------
// Control/estado CPU
// -----------------------------------------------------------------------------
wire [7:0] ctrl;
cy_psoc3_control #(.cy_force_order(`TRUE), .cy_init_value(8'b00000000))
    CtrlReg (.control(ctrl));
wire arm_s = ctrl[0];
wire clr_s = ctrl[1];


// -----------------------------------------------------------------------------
// FSM de framing + detección CMD2/CMD9
// -----------------------------------------------------------------------------
reg [1:0] state;      // 00=IDLE 01=start 10=RESPONSE 11=COMMAND
reg [7:0] ctr;
reg [5:0] cmd;
reg       at7_r;      // ctr==7 en este flanco (decodificado un ciclo antes)
reg       end_r;      // último bit de la trama en este flanco (ídem)
reg       active;     // CMD2 o CMD9 detectado (nivel → CMD_DET)
reg       is9;        // 0=CMD2 (CID), 1=CMD9 (CSD)
reg       match_r;    // cmd==CMD2/CMD9 en el ciclo anterior

always @(posedge clk_en) begin
    cmd     <= {cmd[4:0], cmd_s};
    match_r <= (cmd == 6'b000010) || (cmd == 6'b001001);
    // is9 sigue al índice mientras no hay detección y se congela al detectar.
    // En ctr==7 el bit 0 del índice ya se desplazó a cmd[1].
    if (!active) is9 <= cmd[1];

    // Decodificaciones adelantadas un ciclo (valen 1 en el flanco con ctr==N).
    // Fin: COMMAND y R1/R3/R6/R7 → ctr==45 (bit 47); R2 → ctr==133 (bit 135).
    at7_r <= (ctr == 8'd6);
    end_r <= (state[0] || !active) ? (ctr == 8'd44) : (ctr == 8'd132);
end

always @(posedge clk_en)
    if (clr_s) begin
        state <= 2'b00; active <= 1'b0;
    end else case (state)
        2'b00: if (cmd_s == 1'b0) state <= 2'b01;
        2'b01: if (cmd_s == 1'b0) state <= 2'b10;               // respuesta
               else begin state <= 2'b11; active <= 1'b0; end    // comando nuevo
        // Fin de respuesta: 48 bits, o 136 si es la R2 de CMD2/CMD9.
        2'b10: if (end_r) begin state <= 2'b00; active <= 1'b0; end
        2'b11: begin
            // match_r refleja el índice completo (muestreado en ctr==6)
            if (at7_r && match_r) active <= 1'b1;
            if (end_r) state <= 2'b00;                           // fin comando
        end
    endcase

always @(posedge clk_en)
    if (state[1] == 1'b0) ctr <= 8'b0;
    else                  ctr <= ctr + 8'd1;

assign CMD_DET = active;

// -----------------------------------------------------------------------------
// Serializador (reutiliza ctr). Datapath: A0 = registro de desplazamiento.
//   cs_addr=0 (desplazar): A0 <= A0 << 1          so = A0[7]
//   cs_addr=1 (cargar)   : A0 <= D0 (byte de CPU)  so = A0[7] (bit 0 del byte
//                          anterior, que el host muestrea en ese mismo flanco)
// En cada punto de carga tx_run se re-evalúa: sigue mientras el firmware
// mantenga tx_arm. Cae al final de la R2 (end_r, bit 135) en cualquier caso.
// -----------------------------------------------------------------------------
reg       tx_run;
reg       byte_req_r;
wire      dp_so;

wire load_now = (state == 2'b10) && (ctr[2:0] == LOAD_PHASE);
wire go       = arm_s && active;

cy_psoc3_dp8 #(.cy_dpconfig_a(
{
    `CS_ALU_OP_PASS, `CS_SRCA_A0, `CS_SRCB_D0,
    `CS_SHFT_OP___SL, `CS_A0_SRC__ALU, `CS_A1_SRC_NONE,
    `CS_FEEDBACK_DSBL, `CS_CI_SEL_CFGA, `CS_SI_SEL_CFGA,
    `CS_CMP_SEL_CFGA, /*CFGRAM0: desplazar A0 a la izquierda*/
    `CS_ALU_OP_PASS, `CS_SRCA_A0, `CS_SRCB_D0,
    `CS_SHFT_OP___SL, `CS_A0_SRC___D0, `CS_A1_SRC_NONE,
    `CS_FEEDBACK_DSBL, `CS_CI_SEL_CFGA, `CS_SI_SEL_CFGA,
    `CS_CMP_SEL_CFGA, /*CFGRAM1: cargar A0 desde D0*/
    `CS_ALU_OP_PASS, `CS_SRCA_A0, `CS_SRCB_D0,
    `CS_SHFT_OP_PASS, `CS_A0_SRC_NONE, `CS_A1_SRC_NONE,
    `CS_FEEDBACK_DSBL, `CS_CI_SEL_CFGA, `CS_SI_SEL_CFGA,
    `CS_CMP_SEL_CFGA, /*CFGRAM2: no usado*/
    `CS_ALU_OP_PASS, `CS_SRCA_A0, `CS_SRCB_D0,
    `CS_SHFT_OP_PASS, `CS_A0_SRC_NONE, `CS_A1_SRC_NONE,
    `CS_FEEDBACK_DSBL, `CS_CI_SEL_CFGA, `CS_SI_SEL_CFGA,
    `CS_CMP_SEL_CFGA, /*CFGRAM3: no usado*/
    `CS_ALU_OP_PASS, `CS_SRCA_A0, `CS_SRCB_D0,
    `CS_SHFT_OP_PASS, `CS_A0_SRC_NONE, `CS_A1_SRC_NONE,
    `CS_FEEDBACK_DSBL, `CS_CI_SEL_CFGA, `CS_SI_SEL_CFGA,
    `CS_CMP_SEL_CFGA, /*CFGRAM4: no usado*/
    `CS_ALU_OP_PASS, `CS_SRCA_A0, `CS_SRCB_D0,
    `CS_SHFT_OP_PASS, `CS_A0_SRC_NONE, `CS_A1_SRC_NONE,
    `CS_FEEDBACK_DSBL, `CS_CI_SEL_CFGA, `CS_SI_SEL_CFGA,
    `CS_CMP_SEL_CFGA, /*CFGRAM5: no usado*/
    `CS_ALU_OP_PASS, `CS_SRCA_A0, `CS_SRCB_D0,
    `CS_SHFT_OP_PASS, `CS_A0_SRC_NONE, `CS_A1_SRC_NONE,
    `CS_FEEDBACK_DSBL, `CS_CI_SEL_CFGA, `CS_SI_SEL_CFGA,
    `CS_CMP_SEL_CFGA, /*CFGRAM6: no usado*/
    `CS_ALU_OP_PASS, `CS_SRCA_A0, `CS_SRCB_D0,
    `CS_SHFT_OP_PASS, `CS_A0_SRC_NONE, `CS_A1_SRC_NONE,
    `CS_FEEDBACK_DSBL, `CS_CI_SEL_CFGA, `CS_SI_SEL_CFGA,
    `CS_CMP_SEL_CFGA, /*CFGRAM7: no usado*/
    8'hFF, 8'h00,  /*CFG9*/
    8'hFF, 8'hFF,  /*CFG11-10*/
    `SC_CMPB_A1_D1, `SC_CMPA_A1_D1, `SC_CI_B_ARITH,
    `SC_CI_A_ARITH, `SC_C1_MASK_DSBL, `SC_C0_MASK_DSBL,
    `SC_A_MASK_DSBL, `SC_DEF_SI_1, `SC_SI_B_DEFSI,
    `SC_SI_A_DEFSI, /*CFG13-12: shift-in = 1 (idle)*/
    `SC_A0_SRC_ACC, `SC_SHIFT_SL, 1'h0,
    1'h0, `SC_FIFO1_BUS, `SC_FIFO0_BUS,
    `SC_MSB_DSBL, `SC_MSB_BIT0, `SC_MSB_NOCHN,
    `SC_FB_NOCHN, `SC_CMP1_NOCHN,
    `SC_CMP0_NOCHN, /*CFG15-14: shift-out = MSB*/
    10'h00, `SC_FIFO_CLK__DP, `SC_FIFO_CAP_AX,
    `SC_FIFO_LEVEL, `SC_FIFO__SYNC, `SC_EXTCRC_DSBL,
    `SC_WRK16CAT_DSBL /*CFG17-16*/
}), .d0_init_a(8'hFF), .a0_init_a(8'hFF)) dp (
    .reset(1'b0),
    .clk(clk_en),
    .cs_addr({2'b00, load_now}),
    .route_si(1'b0),
    .route_ci(1'b0),
    .f0_load(1'b0),
    .f1_load(1'b0),
    .d0_load(1'b0),
    .d1_load(1'b0),
    .ce0(), .cl0(), .z0(), .ff0(),
    .ce1(), .cl1(), .z1(), .ff1(),
    .ov_msb(), .co_msb(), .cmsb(),
    .so(dp_so),
    .f0_bus_stat(), .f0_blk_stat(),
    .f1_bus_stat(), .f1_blk_stat()
);

always @(posedge clk_en) begin
    byte_req_r <= load_now && go;            // pedir el siguiente byte a la CPU

    // end_r: fin de trama → suelta el bus pase lo que pase (aunque el firmware
    // no haya desarmado a tiempo). En el bit 135 coincide con load_now.
    if (clr_s || end_r) tx_run <= 1'b0;
    else if (load_now)  tx_run <= go;
end

assign BYTE_REQ = byte_req_r;

// Salida = shift-out de la datapath (A0[7]), que cambia ~100 ns después del
// flanco de SUBIDA de HOST_CLK: el host muestrea el bit en la subida
// siguiente (casi un periodo completo de setup y ~150 ns de hold).
// Fuera de TX el valor es basura, pero SW_IN=0 aísla PSOC_CMD.
assign PSOC_CMD = dp_so;
assign SW_IN    = tx_run;

// -----------------------------------------------------------------------------
// Estado para la CPU
// -----------------------------------------------------------------------------
wire [7:0] status_reg_out;
assign status_reg_out[0]   = active;
assign status_reg_out[1]   = is9;
assign status_reg_out[3:2] = state;
assign status_reg_out[4]   = tx_run;
assign status_reg_out[7:5] = 3'b0;

cy_psoc3_status #(.cy_force_order(`TRUE), .cy_md_select(8'b00000000))
    StatusReg (.status(status_reg_out), .reset(1'b0), .clock(CLK));

endmodule
