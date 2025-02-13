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
    , _stream(capacity)
    , _timeout(retx_timeout) {}

uint64_t TCPSender::bytes_in_flight() const { return _flight_bytes; }

void TCPSender::fill_window() {
    size_t free_window = _recv_win_size == 0 ? 1 : _recv_win_size;
    if (free_window <= _flight_bytes)
        return;
    free_window -= _flight_bytes;
    // close
    if (_close)
        return;


    // free_window -= data.size();

    for (;;) {

        TCPSegment tcp_segment;
        TCPHeader &tcp_header = tcp_segment.header();
        // syn
        if (_next_seqno == 0) {
            tcp_header.syn = true;
            free_window -= 1;
        }
        tcp_header.seqno = wrap(_next_seqno, _isn);
        size_t len = std::min(free_window,TCPConfig::MAX_PAYLOAD_SIZE);
        string data = _stream.read(len);

        Buffer &buffer = tcp_segment.payload();
        free_window -= data.size();
        Buffer buffer1(static_cast<std::string &&>(data));
        buffer = buffer1;

            // eof
        if (_stream.eof() && free_window > 0 && !_close) {
            _close = true;
            tcp_header.fin = true;
            free_window -= 1;
        }

        // send
        if (buffer.size() == 0 && !(tcp_header.syn | tcp_header.fin))
            break;

        // push out and insert into unack_segments
        _segments_out.push(tcp_segment);
        _segments_unack.push(UnackSegment(_next_seqno, tcp_segment));
        _flight_bytes += tcp_segment.length_in_sequence_space();
        _next_seqno = _next_seqno + tcp_segment.length_in_sequence_space();

        if(_stream.remaining_capacity()==0||free_window==0) break;
    }

    // start timer
    if (!_rt_timer.working()) {
        _rt_timer.start(_timeout);
    }
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    // DUMMY_CODE(ackno, window_size);
    _recv_win_size = window_size;
    uint64_t ab_ackno = unwrap(ackno, _isn, _next_seqno);
    UnackSegment segment_ack(ab_ackno, {});

    // unack empty() or ackno have recived or ack seg didn't send
    if (_segments_unack.empty() || ab_ackno <= _max_ackno || ab_ackno > _next_seqno) {
    } else {
        _max_ackno = ab_ackno;
        while(!_segments_unack.empty()) {
            UnackSegment u_segment =  _segments_unack.front();
            if(u_segment._tcp_segment_unack.length_in_sequence_space() + u_segment._seqno<=ab_ackno) {
                _flight_bytes -= u_segment._tcp_segment_unack.length_in_sequence_space();
                _segments_unack.pop();
            } else {
                break;
            }
        }

        _timeout = _initial_retransmission_timeout;
        if (_flight_bytes == 0) {
            _rt_timer.stop();
        } else {
            _rt_timer.reset(_timeout);
        }
        _retry_count = 0;
    }

    // re fill
    fill_window();
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
    // DUMMY_CODE(ms_since_last_tick);
    if (!_rt_timer.working())
        return;
    _rt_timer.add_time(ms_since_last_tick);
    if (_rt_timer.timeout()) {
        //assert(!_segments_unack.empty());
        if(_segments_unack.size()==0) {
            _rt_timer.stop();
            _retry_count = 0;
            _timeout = _initial_retransmission_timeout;
            return;
        }
        if (_recv_win_size != 0) {
            if (++_retry_count > TCPConfig::MAX_RETX_ATTEMPTS) {
                return;
            }
            _timeout *= 2;
        }
        auto &it = _segments_unack.front();
        _segments_out.push(it._tcp_segment_unack);
        _rt_timer.reset(_timeout);
    }
}

unsigned int TCPSender::consecutive_retransmissions() const { return _retry_count; }

void TCPSender::send_empty_segment() {
    TCPSegment tcp_segment;
    // not occupy isn
    tcp_segment.header().seqno = wrap(_next_seqno,_isn);
    _segments_out.push(tcp_segment);
}



