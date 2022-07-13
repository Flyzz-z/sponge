#include "wrapping_integers.hh"

// Dummy implementation of a 32-bit wrapping integer

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

//! Transform an "absolute" 64-bit sequence number (zero-indexed) into a WrappingInt32
//! \param n The input absolute 64-bit sequence number
//! \param isn The initial sequence number
WrappingInt32 wrap(uint64_t n, WrappingInt32 isn) {
    // DUMMY_CODE(n, isn);
    return WrappingInt32{isn + static_cast<uint32_t>(n)};
}

//! Transform a WrappingInt32 into an "absolute" 64-bit sequence number (zero-indexed)
//! \param n The relative sequence number
//! \param isn The initial sequence number
//! \param checkpoint A recent absolute 64-bit sequence number
//! \returns the 64-bit sequence number that wraps to `n` and is closest to `checkpoint`
//!
//! \note Each of the two streams of the TCP connection has its own ISN. One stream
//! runs from the local TCPSender to the remote TCPReceiver and has one ISN,
//! and the other stream runs from the remote TCPSender to the local TCPReceiver and
//! has a different ISN.
uint64_t unwrap(WrappingInt32 n, WrappingInt32 isn, uint64_t checkpoint) {
    // DUMMY_CODE(n, isn, checkpoint);
    uint32_t ex_v = n - isn;
    uint64_t ex = static_cast<uint64_t>(ex_v);
    uint64_t re = static_cast<uint32_t>(checkpoint);
    uint64_t m = checkpoint - re + ex;
    uint64_t ab_seq_no = m,dis = max(checkpoint,m) - min(checkpoint,m);
    if(m>=(1UL<<32)&&(1UL<<32)+re-ex<dis) {
        ab_seq_no = m - (1UL<<32);
        dis = (1UL<<32)+re-ex;
    }
    if(m<=INT64_MAX-(1UL<<32)&&(1UL<<32)-re+ex<dis) {
        ab_seq_no = m + (1UL<<32);
        dis = (1UL<<32)-re+ex;
    }
    return ab_seq_no;
}
