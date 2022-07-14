#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <random>

// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _stream(capacity) {}

uint64_t TCPSender::bytes_in_flight() const { return _flight_bytes; }

void TCPSender::fill_window() {
    // close
    if (_close)
        return;
    TCPSegment tcp_segment;
    TCPHeader &tcp_header = tcp_segment.header();
    // syn
    if (_next_seqno == 0) {
        tcp_header.syn = true;
    }
    tcp_header.seqno = wrap(_next_seqno, _isn);

    // eof
    if (_stream.eof()) {
        _close = true;
        tcp_header.fin = true;
    }

    size_t read_size;
    if(_window_size==0) read_size==1;
    string data = _stream.read(_window_size == 0 ? 1 : _window_size);

    Buffer &buffer = tcp_segment.payload();
    Buffer buffer1(static_cast<std::string &&>(data));
    buffer = buffer1;

    // send
    if (buffer.size() == 0 && !(tcp_header.syn | tcp_header.fin))
        return;
    _segments_out.push(tcp_segment);
    _segments_unack.insert(UnackSegment(_next_seqno, tcp_segment));
    _flight_bytes += tcp_segment.length_in_sequence_space();
    _next_seqno = _next_seqno + tcp_segment.length_in_sequence_space();
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    // DUMMY_CODE(ackno, window_size);
    _window_size = window_size;
    uint64_t ab_ackno = unwrap(ackno, _isn, _next_seqno);
    UnackSegment segment_ack(ab_ackno, {});

    if (_segments_unack.empty())
        return;
    auto it = _segments_unack.lower_bound(segment_ack);

    for (auto it1=_segments_unack.begin();it1!=it;) {
       if(it1->_seqno+it1->_tcp_segment_unack.length_in_sequence_space()<=ab_ackno) {
        _flight_bytes -= it1->_tcp_segment_unack.length_in_sequence_space();
        _segments_unack.erase(it1++);
       } else it1++;
    }

    //todo 再充满
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) { DUMMY_CODE(ms_since_last_tick); }

unsigned int TCPSender::consecutive_retransmissions() const { return {}; }

void TCPSender::send_empty_segment() {}
