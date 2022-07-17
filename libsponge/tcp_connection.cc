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

size_t TCPConnection::time_since_last_segment_received() const { return _time_since_last_seg_recv; }

void TCPConnection::segment_received(const TCPSegment &seg) {
    // DUMMY_CODE(seg);
    _time_since_last_seg_recv = 0;
    const TCPHeader &tcp_header = seg.header();
    if (tcp_header.rst) {
        _sender.stream_in().error();
        _receiver.stream_out().error();
        // todo close connection
        _active = false;
    }

    _receiver.segment_received(seg);

    if (tcp_header.ack) {
        _sender.ack_received(tcp_header.ackno, tcp_header.win);
    }

    // in_bound has over but fin not send(so fin can ack peer's fin)
    if (check_inbound_end() && not (_sender.next_seqno_absolute() == _sender.stream_in().bytes_written() + 2)) {
        _linger_after_streams_finish = false;
    }


    bool is_send = real_send();
    if (seg.header().fin)
        _recv_fin = true;
    if (seg.length_in_sequence_space() != 0 && !is_send) {
        _sender.send_empty_segment();
    }

    // keep-alive
    if (_receiver.ackno().has_value() and (seg.length_in_sequence_space() == 0) and
        seg.header().seqno == _receiver.ackno().value() - 1) {
        _sender.send_empty_segment();
    }
    real_send();
}

bool TCPConnection::active() const { return _active; }

size_t TCPConnection::write(const string &data) {
    // DUMMY_CODE(data);
    size_t n = _sender.stream_in().write(data);
    _sender.fill_window();
    real_send();
    return n;
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) {
    // DUMMY_CODE(ms_since_last_tick);
    _time_since_last_seg_recv += ms_since_last_tick;
    _sender.tick(ms_since_last_tick);

    if (_sender.consecutive_retransmissions() > TCPConfig::MAX_RETX_ATTEMPTS) {
        // abort connection
        _sender.send_rst_seg();
        real_send();
    }

    if (check_inbound_end() && check_outbound_end()) {
        if (_linger_after_streams_finish && _time_since_last_seg_recv >= 10 * _cfg.rt_timeout) {
            _active = false;
        } else if (not _linger_after_streams_finish) {
            _active = false;
        }
    }
    real_send();
}

void TCPConnection::end_input_stream() {
    _sender.stream_in().end_input();
    _sender.fill_window();
    real_send();
}

void TCPConnection::connect() {
    // link out queue
    // _sender.segments_out() = _segments_out;
    _sender.fill_window();
    real_send();
}

TCPConnection::~TCPConnection() {
    try {
        if (active()) {
            cerr << "Warning: Unclean shutdown of TCPConnection\n";

            // Your code here: need to send a RST segment to the peer
            _active = false;
            _sender.send_rst_seg();
        }
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}

void TCPConnection::set_ack_win(TCPHeader &tcp_header) {
    size_t win_size = _receiver.window_size();
    std::numeric_limits<uint16_t> lim;
    win_size = std::min(win_size, static_cast<size_t>(lim.max()));
    optional<WrappingInt32> op_ackno = _receiver.ackno();
    if (op_ackno.has_value()) {
        tcp_header.ack = true;
        tcp_header.ackno = op_ackno.value();
        tcp_header.win = win_size;
    }
}

bool TCPConnection::real_send() {
    bool is_send = false;
    while (!_sender.segments_out().empty()) {
        is_send = true;
        TCPSegment tcp_segment = _sender.segments_out().front();
        set_ack_win(tcp_segment.header());
        _segments_out.push(tcp_segment);
        _sender.segments_out().pop();
    }
    return is_send;
}

bool TCPConnection::check_inbound_end() { return _receiver.stream_out().input_ended()&&_receiver.unassembled_bytes()==0; }

bool TCPConnection::check_outbound_end() {
    return _sender.stream_in().eof() && _sender.bytes_in_flight() == 0 &&
           _sender.next_seqno_absolute() == _sender.stream_in().bytes_written() + 2;
}