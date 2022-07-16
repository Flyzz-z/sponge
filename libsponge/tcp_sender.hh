#ifndef SPONGE_LIBSPONGE_TCP_SENDER_HH
#define SPONGE_LIBSPONGE_TCP_SENDER_HH

#include "byte_stream.hh"
#include "tcp_config.hh"
#include "tcp_segment.hh"
#include "wrapping_integers.hh"

#include <queue>
#include <set>
#include<cassert>

class RetransmissionTimer
{
private:
    std::size_t _total_ms{};
    unsigned int _rto{};
    bool _start{false};
public:
    bool timeout() {
        return _total_ms >= _rto;
    }
    void add_time(const size_t ms_since_last_tick) {
        _total_ms += ms_since_last_tick;
    }
    void start(unsigned int rto) {
        _rto = rto;
        _start = true;
        _total_ms = 0UL;
    }
    void stop() {
        _rto = 0U;
        _total_ms = 0UL;
        _start = false;
    }
    void reset(unsigned int rto) {
      if(!_start) return;
      _rto = rto;
      _total_ms = 0UL;
    }
    bool working() {
        return _start;
    }
};

//! \brief The "sender" part of a TCP implementation.

//! Accepts a ByteStream, divides it up into segments and sends the
//! segments, keeps track of which segments are still in-flight,
//! maintains the Retransmission Timer, and retransmits in-flight
//! segments if the retransmission timer expires.
class TCPSender {
  private:
    //! our initial sequence number, the number for our SYN.
    WrappingInt32 _isn;

    //! outbound queue of segments that the TCPSender wants sent
    std::queue<TCPSegment> _segments_out{};

    //! retransmission timer for the connection
    unsigned int _initial_retransmission_timeout;

    //! outgoing stream of bytes that have not yet been sent
    ByteStream _stream;

    //! the (absolute) sequence number for the next byte to be sent
    uint64_t _next_seqno{0};

    size_t _recv_win_size{1};

    bool _close{false};

    RetransmissionTimer _rt_timer{};

    unsigned int _timeout;

    unsigned _retry_count{0};

    // recive max ackno
    uint64_t _max_ackno{0};

    // ackno to send
    std::optional<WrappingInt32> _send_ackno{0};

    // window_size to send (my win)
    size_t _send_win_size{0};

    class UnackSegment {
      public:
        uint64_t _seqno;
        TCPSegment _tcp_segment_unack;
        UnackSegment(uint64_t seqno,TCPSegment tcp_segment):
        _seqno(seqno),_tcp_segment_unack(tcp_segment){}
        bool operator < (const UnackSegment& rhs) const{
          return _seqno < rhs._seqno;
        }

    };

    std::queue<UnackSegment> _segments_unack{};

    uint64_t _flight_bytes{0};


  public:
    //! Initialize a TCPSender
    TCPSender(const size_t capacity = TCPConfig::DEFAULT_CAPACITY,
              const uint16_t retx_timeout = TCPConfig::TIMEOUT_DFLT,
              const std::optional<WrappingInt32> fixed_isn = {});

    //! \name "Input" interface for the writer
    //!@{
    ByteStream &stream_in() { return _stream; }
    const ByteStream &stream_in() const { return _stream; }
    //!@}

    //! \name Methods that can cause the TCPSender to send a segment
    //!@{

    //! \brief A new acknowledgment was received
    void ack_received(const WrappingInt32 ackno, const uint16_t window_size);

    //! \brief Generate an empty-payload segment (useful for creating empty ACK segments)
    void send_empty_segment();

    //! \brief create and send segments to fill as much of the window as possible
    void fill_window();

    //! \brief Notifies the TCPSender of the passage of time
    void tick(const size_t ms_since_last_tick);
    //!@}

    //! \name Accessors
    //!@{

    //! \brief How many sequence numbers are occupied by segments sent but not yet acknowledged?
    //! \note count is in "sequence space," i.e. SYN and FIN each count for one byte
    //! (see TCPSegment::length_in_sequence_space())
    size_t bytes_in_flight() const;

    //! \brief Number of consecutive retransmissions that have occurred in a row
    unsigned int consecutive_retransmissions() const;

    //! \brief TCPSegments that the TCPSender has enqueued for transmission.
    //! \note These must be dequeued and sent by the TCPConnection,
    //! which will need to fill in the fields that are set by the TCPReceiver
    //! (ackno and window size) before sending.
    std::queue<TCPSegment> &segments_out() { return _segments_out; }
    //!@}

    //! \name What is the next sequence number? (used for testing)
    //!@{

    //! \brief absolute seqno for the next byte to be sent
    uint64_t next_seqno_absolute() const { return _next_seqno; }

    //! \brief relative seqno for the next byte to be sent
    WrappingInt32 next_seqno() const { return wrap(_next_seqno, _isn); }
    //!@}

    void set_ack(WrappingInt32 to_ackno) {
      _send_ackno = to_ackno;
    }

    void set_recv_win_size(size_t win_size) {
      _recv_win_size = win_size;
    }

};




#endif  // SPONGE_LIBSPONGE_TCP_SENDER_HH
