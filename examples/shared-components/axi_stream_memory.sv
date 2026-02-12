module axi_stream_memory #(
    parameter DATA_WIDTH = 64,
    parameter ADDR_WIDTH = 8,   // depth = 256 entries
    parameter READ_LATENCY = 3
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

    // -------------------------------------------
    // Command definitions
    // -------------------------------------------
    localparam logic [DATA_WIDTH-1:0] CMD_READ  = 64'h0;
    localparam logic [DATA_WIDTH-1:0] CMD_WRITE = 64'h1;

    logic [DATA_WIDTH-1:0] mem [0:(1<<ADDR_WIDTH)-1];

    typedef enum logic [1:0] {
        RX_CMD,
        RX_ADDR,
        RX_DATA
    } rx_state_t;

    typedef enum logic [1:0] {
        TX_IDLE,
        TX_WAIT,
        TX_SEND
    } tx_state_t;

    rx_state_t rx_state;
    tx_state_t tx_state;

    logic [DATA_WIDTH-1:0] cmd_reg;
    logic [ADDR_WIDTH-1:0] addr_reg;
    logic [DATA_WIDTH-1:0] data_reg;

    logic [$clog2(READ_LATENCY+1)-1:0] rd_cnt;
    
    assign s_axis_tready = (rx_state != RX_DATA);

    always_ff @(posedge aclk) begin
        if (!aresetn) begin
            rx_state <= RX_CMD;
        end else if (s_axis_tvalid && s_axis_tready) begin
            case (rx_state)
                RX_CMD: begin
                    cmd_reg  <= s_axis_tdata;
                    rx_state <= RX_ADDR;
                end

                RX_ADDR: begin
                    addr_reg <= s_axis_tdata[ADDR_WIDTH-1:0];
                    if (cmd_reg == CMD_WRITE)
                        rx_state <= RX_DATA;
                    else
                        rx_state <= RX_CMD;
                end

                RX_DATA: begin
                    data_reg <= s_axis_tdata;
                    rx_state <= RX_CMD;
                end
            endcase
        end
    end
    
    always_ff @(posedge aclk) begin
        if (!aresetn) begin
            tx_state <= TX_IDLE;
            rd_cnt   <= '0;
        end else begin
            case (tx_state)
                TX_IDLE: begin
                    if (cmd_reg == CMD_WRITE && rx_state == RX_CMD) begin
                        mem[addr_reg] <= data_reg;
                        tx_state <= TX_SEND;
                        data_reg <= 64'h1; // write ACK
                    end
                    else if (cmd_reg == CMD_READ && rx_state == RX_CMD) begin
                        rd_cnt   <= READ_LATENCY;
                        tx_state <= TX_WAIT;
                    end
                end

                TX_WAIT: begin
                    if (rd_cnt != 0)
                        rd_cnt <= rd_cnt - 1;
                    else begin
                        data_reg <= mem[addr_reg];
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

    assign m_axis_tvalid = (tx_state == TX_SEND);
    assign m_axis_tdata  = data_reg;
    assign m_axis_tlast  = 1'b1;

endmodule

