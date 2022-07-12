#include "byte_stream.hh"

#include <algorithm>
#include <vector>

// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

ByteStream::ByteStream(const size_t capacity) : cap(capacity), nRead(0), nWrite(0), end_in(false), buf() {
    // DUMMY_CODE(capacity);
}

size_t ByteStream::write(const string &data) {
    // DUMMY_CODE(data);
    unsigned int fr = cap - buf.size();
    size_t n = 0;
    for (auto ch : data) {
        if (fr--) {
            n += 1;
            buf.push_back(ch);
        } else {
            break;
        }
    }
    nWrite += n;
    return n;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    // DUMMY_CODE(len);
    int m_len = std::min(len, buf.size());
    string out;
    for (int i = 0; i < m_len; i++) {
        out.append(1, buf.at(i));
    }
    return out;
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) {
    // DUMMY_CODE(len);
    int m_len = std::min(len, buf.size());
    while (m_len--) {
        buf.pop_front();
        nRead += 1;
    }
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    // DUMMY_CODE(len);
    string res = peek_output(len);
    pop_output(len);
    return res;
}

void ByteStream::end_input() { end_in = true; }

bool ByteStream::input_ended() const { return end_in; }

size_t ByteStream::buffer_size() const { return buf.size(); }

bool ByteStream::buffer_empty() const { return buf.size() == 0; }

bool ByteStream::eof() const { return end_in && buf.size() == 0; }

size_t ByteStream::bytes_written() const { return nWrite; }

size_t ByteStream::bytes_read() const { return nRead; }

size_t ByteStream::remaining_capacity() const { return cap - buf.size(); }
