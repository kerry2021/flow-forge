module test_master #(
    parameter DATA_WIDTH = 64
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
    // Conceptually:
    //   s_axis_* signals are directly forwarded to m_axis_*
    //   Optionally, you could add buffering, decoding, or handshaking here later
    always @(posedge aclk) begin
        m_axis_tdata <= s_axis_tdata;
        m_axis_tvalid <= s_axis_tvalid;
        s_axis_tready <= m_axis_tready;
        m_axis_tlast <= s_axis_tlast;
        m_axis_tdest <= s_axis_tdest;
    end
endmodule