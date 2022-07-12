#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity)
    : _output(capacity), _capacity(capacity), next_index(0), unassem_n(0), eof_index(0), eof_f(false), seq_set() {}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    // DUMMY_CODE(data, index, eof);
    if (eof) {
        eof_f = true;
        eof_index = index + data.size();
    }
    if (next_index < index + data.size()) {
        size_t n_index = max(next_index, index);
        push_data(data.substr(n_index - index, data.size() - (n_index - index)), n_index);
    }

    write_data();
}

size_t StreamReassembler::unassembled_bytes() const { return unassem_n; }

bool StreamReassembler::empty() const { return seq_set.size() == 0; }

void StreamReassembler::push_data(const std::string &data, const uint64_t index) {
    SeqData seq_data(data, index);
    std::set<SeqData>::iterator it = seq_set.lower_bound(seq_data);
    if (it != seq_set.begin()) {
        it--;
    }
    uint64_t st = index, ed = index + data.size();

    uint64_t in_index = st, last_index = st;
    string in_data;
    for (; it != seq_set.end() && it->index <= ed;) {
        uint64_t it_st = it->index, it_ed = it->index + it->data.size();

        if (it_st <= index && it_ed >= ed) {
            last_index = ed;
            break;
        } else if (it_st < st && it_ed > st) {
            in_data.append(it->data);
            in_index = it->index;
            last_index = it_ed;
            unassem_n -= it->data.size();
            seq_set.erase(it++);
        } else if (it_st >= st && it_ed <= ed) {
            in_data.append(data.substr(last_index - st, it_ed - last_index));
            unassem_n -= it->data.size();
            seq_set.erase(it++);
            last_index = it_ed;
        } else if (it_st > st && it_ed > ed) {
            if (it_st > last_index)
                in_data.append(data.substr(last_index - st, it_st - last_index));
            in_data.append(it->data);
            unassem_n -= it->data.size();
            seq_set.erase(it++);
            last_index = ed;
        } else {
            it++;
        }
    }

    if (ed > last_index) {
        in_data.append(data.substr(last_index - st, ed - last_index));
    }
    if (in_data.size() != 0) {
        SeqData in_seq_data(in_data, in_index);
        seq_set.insert(in_seq_data);
        unassem_n += in_data.size();

        for (auto it1 = seq_set.rbegin(); it1 != seq_set.rend() && unassem_n + _output.buffer_size() > _capacity;) {
            size_t more = unassem_n + _output.buffer_size() - _capacity;
            if (it1->data.size() <= more) {
                unassem_n -= it1->data.size();
                seq_set.erase((++it1).base());
            } else {
                size_t n_len = it1->data.size() - more;
                SeqData n_seq_data(it1->data.substr(0, n_len), it1->index);
                unassem_n -= more;
                seq_set.erase((++it1).base());
                seq_set.insert(n_seq_data);
            }
        }
    }
}

void StreamReassembler::write_data() {
    std::set<SeqData>::iterator it = seq_set.begin();
    while (it != seq_set.end() && it->index == next_index) {
        _output.write(it->data);
        next_index = it->index + it->data.size();
        unassem_n -= it->data.size();
        seq_set.erase(it++);
    }
    if (next_index == eof_index && eof_f) {
        _output.end_input();
    }
}