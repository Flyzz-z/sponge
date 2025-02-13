#include "tcp_receiver.hh"
#include "tcp_header.hh"
#include "tcp_segment.hh"
// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    //DUMMY_CODE(seg);
    TCPHeader tcp_header = seg.header();
    if(tcp_header.syn) {
        _isn_set = true;
        _isn = tcp_header.seqno;
    }
    if(!_isn_set) return;

    if(tcp_header.fin) _fin = true;
    WrappingInt32 seqno = tcp_header.seqno;
    uint64_t ab_seqno = unwrap(seqno,_isn,tcp_header.syn?_reassembler.next_index():0);
    // ab_seqno==0 without syn, wrong
    if(ab_seqno==0&&!tcp_header.syn) return;
    // out of window range
    if(ab_seqno >=_reassembler.next_index() + 1 + (_fin&&_reassembler.empty()) + window_size()) return;
    uint64_t str_index = ab_seqno==0?unwrap(seqno+1,_isn,0)-1:ab_seqno-1;
    auto data = seg.payload().str();
    string full_data(data.begin(),data.end());
    _reassembler.push_substring(full_data,str_index,tcp_header.fin);
    
}

optional<WrappingInt32> TCPReceiver::ackno() const { 
    if(!_isn_set) {
        return {}; 
    } else {
        uint64_t ab_seqno = _reassembler.next_index() + 1 + (_fin&&_reassembler.empty());
        return optional(wrap(ab_seqno,_isn));
    }
    
}

size_t TCPReceiver::window_size() const { return _capacity - _reassembler.stream_out().buffer_size(); }


