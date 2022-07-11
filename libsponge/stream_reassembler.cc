#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) : _output(capacity), _capacity(capacity)
,next_index(0),unassem_n(0),seq_set() {}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    //DUMMY_CODE(data, index, eof);
    if(next_index>=index+data.size()) return;
    size_t n_index = max(next_index,index);
    push_data(data.substr(n_index-index,data.size()),n_index);
    write_data();
    if(eof) _output.end_input();
}

size_t StreamReassembler::unassembled_bytes() const { return unassem_n; }

bool StreamReassembler::empty() const { return seq_set.size()==0; }

void StreamReassembler::push_data(const std::string &data, const uint64_t index) {
    SeqData seq_data(data, index);
    std::set<SeqData>::iterator it = seq_set.lower_bound(seq_data);
    if (it != seq_set.begin()) {
        it--;
    }
    uint64_t st = index, ed = index + data.size();

    uint64_t in_index = st,last_index=st;
    string in_data;
    size_t unused = _capacity - unassem_n - _output.buffer_size();
    for (; it != seq_set.end() && it->index < ed; it++) {
        if(!unused) break;
        uint64_t it_st = it->index, it_ed = it->index + it->data.size();

        if (it_st <= index && it_ed >= ed) {
            break;
        } else if (it_st < st && it_ed > st) {
            in_data.append(it->data);
            in_index = it->index;
            last_index = it_ed;
            seq_set.erase(it);
        } else if (it_st >= st && it_ed <= ed) {
            if(unused<it_st - last_index) {
                in_data.append(data.substr(last_index-st,last_index-st+unused));
                unused = 0;
                last_index = last_index-st+unused;
            } else {
                in_data.append(data.substr(last_index-st,it_ed-st));
                unused -= it_st - last_index;
                seq_set.erase(it);
                last_index = it_ed;
            }

        } else if (it_st > st && it_ed > ed) {

            if(unused<it_st - last_index) {
                in_data.append(data.substr(last_index-st,last_index-st+unused));
                unused = 0;
                last_index = last_index-st+unused;
            } else {
                in_data.append(data.substr(last_index-st,it_st-st));
                in_data.append(it->data);
                unused -= it_st - last_index;
                seq_set.erase(it);
                last_index = ed;
            }
        }
    }
    uint64_t tail = std::min(unused,ed);
    if(tail-last_index>0) {
        in_data.append(data.substr(last_index-st,last_index-st+tail));
    }
    if(in_data.size()!=0) {
        unassem_n += in_data.size();
        SeqData in_seq_data(in_data,in_index);
        seq_set.insert(in_seq_data);
    }
}

void StreamReassembler::write_data() {
    std::set<SeqData>::iterator it = seq_set.begin();
    while (it!=seq_set.end()&&it->index==next_index)
    {   
        _output.write(it->data);
        next_index = it->index + it->data.size();
        unassem_n -= it->data.size();
        seq_set.erase(it);
    }
}