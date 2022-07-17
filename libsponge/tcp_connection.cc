#include "tcp_connection.hh"

#include <iostream>

// Dummy implementation of a TCP connection

// For Lab 4, please replace with a real implementation that passes the
// automated checks run by `make check`.

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

size_t TCPConnection::remaining_outbound_capacity() const { return _sender.stream_in().remaining_capacity(); }

size_t TCPConnection::bytes_in_flight() const { return _sender.bytes_in_flight(); }

size_t TCPConnection::unassembled_bytes() const { return _receiver.unassembled_bytes(); }

size_t TCPConnection::time_since_last_segment_received() const { return {}; }

void TCPConnection::segment_received(const TCPSegment &seg) {
    // DUMMY_CODE(seg);
    const TCPHeader &tcp_header = seg.header();
    if (tcp_header.rst) {
        _sender.stream_in().error();
        _receiver.stream_out().error();
        // todo close connection
    }

    _receiver.segment_received(seg);
    if (tcp_header.ack) {
        _sender.ack_received(tcp_header.ackno, tcp_header.win);
    }

    set_sender_ack_win();
    if (seg.length_in_sequence_space() != 0) {
        _sender.send_empty_segment();
    }

    // keep-alive
    if (_receiver.ackno().has_value() and (seg.length_in_sequence_space() == 0) and
        seg.header().seqno == _receiver.ackno().value() - 1) {
        _sender.send_empty_segment();
    }
}

bool TCPConnection::active() const { return {}; }

size_t TCPConnection::write(const string &data) {
    // DUMMY_CODE(data);
    set_sender_ack_win();
    size_t n = _sender.stream_in().write(data);
    _sender.fill_window();
    return n;
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) { DUMMY_CODE(ms_since_last_tick); }

void TCPConnection::end_input_stream() { _sender.stream_in().end_input(); }

void TCPConnection::connect() { _sender.fill_window(); }

TCPConnection::~TCPConnection() {
    try {
        if (active()) {
            cerr << "Warning: Unclean shutdown of TCPConnection\n";

            // Your code here: need to send a RST segment to the peer
        }
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}

void TCPConnection::set_sender_ack_win() {
    size_t win_size = _receiver.window_size();
    optional<WrappingInt32> op_ackno = _receiver.ackno();
    if (op_ackno.has_value()) {
        _sender.set_ack(op_ackno.value());
    }
    _sender.set_recv_win_size(win_size);
}