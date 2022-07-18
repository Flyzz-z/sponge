#include "network_interface.hh"

#include "arp_message.hh"
#include "ethernet_frame.hh"

#include <iostream>

// Dummy implementation of a network interface
// Translates from {IP datagram, next hop address} to link-layer frame, and from link-layer frame to IP datagram

// For Lab 5, please replace with a real implementation that passes the
// automated checks run by `make check_lab5`.

// You will need to add private members to the class declaration in `network_interface.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

//! \param[in] ethernet_address Ethernet (what ARP calls "hardware") address of the interface
//! \param[in] ip_address IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface(const EthernetAddress &ethernet_address, const Address &ip_address)
    : _ethernet_address(ethernet_address), _ip_address(ip_address) {
    cerr << "DEBUG: Network interface has Ethernet address " << to_string(_ethernet_address) << " and IP address "
         << ip_address.ip() << "\n";
}

//! \param[in] dgram the IPv4 datagram to be sent
//! \param[in] next_hop the IP address of the interface to send it to (typically a router or default gateway, but may also be another host if directly connected to the same network as the destination)
//! (Note: the Address type can be converted to a uint32_t (raw 32-bit IP address) with the Address::ipv4_numeric() method.)
void NetworkInterface::send_datagram(const InternetDatagram &dgram, const Address &next_hop) {
    // convert IP address of next hop to raw 32-bit representation (used in ARP header)
    const uint32_t next_hop_ip = next_hop.ipv4_numeric();
    // DUMMY_CODE(dgram, next_hop, next_hop_ip);
    EthernetFrame eth_frame;
    // serialize: cal checksum
    eth_frame.payload() = dgram.serialize();

    if (arp_table.find(next_hop_ip) != arp_table.end()) {
        eth_frame.header().dst = arp_table[next_hop_ip]._eth_addr;
    } else {
        // send arp
        if (last_arp_req.find(next_hop_ip) == last_arp_req.end() || last_arp_req[next_hop_ip] > 5 * 1000) {
            last_arp_req[next_hop_ip] = 0;
            send_arp(ARPMessage::OPCODE_REQUEST, next_hop_ip);
        }
        data_to_send.push_back(IPData(dgram, next_hop));
    }

    eth_frame.header().src = _ethernet_address;
    eth_frame.header().type = EthernetHeader::TYPE_IPv4;
    _frames_out.push(eth_frame);
}

//! \param[in] frame the incoming Ethernet frame
optional<InternetDatagram> NetworkInterface::recv_frame(const EthernetFrame &frame) {
    if (frame.header().dst != _ethernet_address && frame.header().dst != ETHERNET_BROADCAST)
        return;

    if (frame.header().type == EthernetHeader::TYPE_IPv4) {
        IPv4Datagram ipv4_datagram;
        ParseResult res = ipv4_datagram.parse(Buffer(frame.payload()));
        if (res == ParseResult::NoError) {
            return ipv4_datagram;
        }
    } else if (frame.header().type == EthernetHeader::TYPE_ARP) {
        ARPMessage arp_mes;
        ParseResult res = arp_mes.parse(Buffer(frame.payload()));
        if (res == ParseResult::NoError) {
            // learn
            arp_table[arp_mes.sender_ip_address] = ArpTableEntry(arp_mes.sender_ethernet_address, 0);
            if (arp_mes.opcode == ARPMessage::OPCODE_REQUEST &&
                arp_mes.target_ip_address == _ip_address.ipv4_numeric()) {
                send_arp(ARPMessage::OPCODE_REPLY, arp_mes.sender_ip_address);
            }

            // resend ipdata
            for (auto it = data_to_send.begin(); it != data_to_send.end();) {
                if(it->_next_hop.ipv4_numeric()==arp_mes.sender_ip_address) {
                    send_datagram(it->_ip_data,it->_next_hop);
                    data_to_send.erase(it++);
                } else {
                    it++;
                }
            }
        }
    }
    return {};
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick(const size_t ms_since_last_tick) {
    // DUMMY_CODE(ms_since_last_tick);
    // expired arp
    for (auto it = arp_table.begin(); it != arp_table.end();) {
        size_t now_time = arp_table[it->first]._time_since_created + ms_since_last_tick;
        if (now_time > 30 * 1000) {
            arp_table.erase(it++);
        } else {
            arp_table[it->first]._time_since_created = now_time;
            it++;
        }
    }

    // expired req
    for (auto it = last_arp_req.begin(); it != last_arp_req.end();) {
        size_t now_time = last_arp_req[it->first] + ms_since_last_tick;
        if (now_time > 5 * 1000) {
            last_arp_req.erase(it++);
        } else {
            last_arp_req[it->first] = now_time;
            it++;
        }
    }
}

void NetworkInterface::send_arp(const uint16_t opcode, const uint32_t target_ip) {
    ARPMessage arp_message;
    arp_message.opcode = opcode;
    arp_message.sender_ethernet_address = _ethernet_address;
    arp_message.sender_ip_address = _ip_address.ipv4_numeric();
    arp_message.target_ip_address = target_ip;

    EthernetFrame eth_frame;
    EthernetHeader eth_header = eth_frame.header();
    eth_header.src = _ethernet_address;
    eth_header.dst = ETHERNET_BROADCAST;
    eth_header.type = EthernetHeader::TYPE_ARP;
    eth_frame.payload() = arp_message.serialize();

    _frames_out.push(eth_frame);
}