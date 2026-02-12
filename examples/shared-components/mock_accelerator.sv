module mock_accelerator #(
    parameter DATA_WIDTH = 64,
    parameter COMPUTE_LATENCY = 5
)(
    input  logic                     aclk,
    input  logic                     aresetn,

    // ---------------------------------
    // AXI-Stream Slave (requests)
    // ---------------------------------
    input  logic [DATA_WIDTH-1:0]     s_axis_tdata,
    input  logic                      s_axis_tvalid,
    output logic                      s_axis_tready,
    input  logic                      s_axis_tlast,
    input  logic[1:0]                 s_axis_tdest,

    // ---------------------------------
    // AXI-Stream Master (responses)
    // ---------------------------------
    output logic [DATA_WIDTH-1:0]     m_axis_tdata,
    output logic                      m_axis_tvalid,
    input  logic                      m_axis_tready,
    output logic                      m_axis_tlast,
    output logic[1:0]                 m_axis_tdest
);

    typedef enum logic [1:0] {
        RX_IDLE,
        RX_OP_A,
        RX_OP_B,
        RX_OPCODE
    } rx_state_t;

    typedef enum logic [1:0] {
        TX_IDLE,
        TX_WAIT,
        TX_SEND
    } tx_state_t;

    rx_state_t rx_state;
    tx_state_t tx_state;

    logic [DATA_WIDTH-1:0] op_a;
    logic [DATA_WIDTH-1:0] op_b;
    logic [DATA_WIDTH-1:0] result;

    logic [$clog2(COMPUTE_LATENCY+1)-1:0] compute_cnt;

        assign s_axis_tready = (rx_state != RX_OPCODE);

    always_ff @(posedge aclk) begin
        if (!aresetn) begin
            rx_state <= RX_IDLE;
        end else if (s_axis_tvalid && s_axis_tready) begin
            case (rx_state)
                RX_IDLE: begin
                    op_a     <= s_axis_tdata;
                    rx_state <= RX_OP_A;
                end
                RX_OP_A: begin
                    op_b     <= s_axis_tdata;
                    rx_state <= RX_OP_B;
                end
                RX_OP_B: begin
                    // opcode ignored
                    if (s_axis_tlast)
                        rx_state <= RX_IDLE;
                    else
                        rx_state <= RX_OPCODE;
                end
                RX_OPCODE: begin
                    if (s_axis_tlast)
                        rx_state <= RX_IDLE;
                end
            endcase
        end
    end

    always_ff @(posedge aclk) begin
        if (!aresetn) begin
            compute_cnt <= '0;
            tx_state    <= TX_IDLE;
        end else begin
            case (tx_state)
                TX_IDLE: begin
                    if (rx_state == RX_IDLE && compute_cnt == 0) begin
                        compute_cnt <= COMPUTE_LATENCY;
                        tx_state    <= TX_WAIT;
                    end
                end

                TX_WAIT: begin
                    if (compute_cnt != 0)
                        compute_cnt <= compute_cnt - 1;
                    else begin
                        result   <= op_a + op_b;
                        tx_state <= TX_SEND;
                    end
                end

                TX_SEND: begin
                    if (m_axis_tvalid && m_axis_tready)
                        tx_state <= TX_IDLE;
                end
            endcase
        end
    end

    assign m_axis_tvalid = 0;//(tx_state == TX_SEND);
    assign m_axis_tdata  = result;
    assign m_axis_tlast  = 1'b1;


endmodule

