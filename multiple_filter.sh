#!/bin/bash

IP4=192.0.2.1/24
TGT4=192.0.2.2/24 
TGT4_NO_MASK=192.0.2.2
IP6=2001:db8::1/64
TGT6=2001:db8::2/64
TGT6_NO_MASK=2001:db8::2
PORT=1234
DELAY=4000

cleanup() {
    ip link del dummy1 2>/dev/null
    ip -n ns1 link del dummy1.10 2>/dev/null
    ip netns del ns1 2>/dev/null
}

trap cleanup EXIT



ip netns add ns1

ip -n ns1 link set dev lo up
ip -n ns1 link add name dummy1 up type dummy

ip -n ns1 link add link dummy1 name dummy1.10 up type vlan id 10 \
        egress-qos-map 0:0 1:1 2:2 3:3 4:4 5:5 6:6 7:7

ip -n ns1 address add $IP4 dev dummy1.10
ip -n ns1 address add $IP6 dev dummy1.10

ip netns exec ns1 bash -c "
sysctl -w net.ipv4.ping_group_range='0 2147483647'
exit"


ip -n ns1 neigh add $TGT4_NO_MASK lladdr 00:11:22:33:44:55 nud permanent dev \
        dummy1.10
ip -n ns1 neigh add $TGT6_NO_MASK lladdr 00:11:22:33:44:55 nud permanent dev dummy1.10


tc -n ns1 qdisc add dev dummy1 clsact

for i in {0..7}; do
    #IPv4 UDP
    tc -n ns1 filter add dev dummy1 egress pref 1 handle 10${i} \
            proto 802.1q flower vlan_prio $i vlan_ethtype ipv4 \
            ip_proto udp dst_ip $TGT4_NO_MASK action pass
    
    #IPv4 ICMP
    tc -n ns1 filter add dev dummy1 egress pref 1 handle 20${i} \
            proto 802.1q flower vlan_prio $i vlan_ethtype ipv4 \
            ip_proto icmp dst_ip $TGT4_NO_MASK action pass

    #IPv4 RAW
    tc -n ns1 filter add dev dummy1 egress pref 1 handle 30${i} \
            proto 802.1q flower vlan_prio $i vlan_ethtype ipv4 \
            dst_ip $TGT4_NO_MASK action pass
    
    #IPv6 UDP
    tc -n ns1 filter add dev dummy1 egress pref 1 handle 40${i} \
            proto 802.1q flower vlan_prio $i vlan_ethtype ipv6 \
            ip_proto udp dst_ip $TGT6_NO_MASK action pass
   
    #IPv6 ICMP
    tc -n ns1 filter add dev dummy1 egress pref 1 handle 50${i} \
            proto 802.1q flower vlan_prio $i vlan_ethtype ipv6 \
            ip_proto icmpv6 dst_ip $TGT6_NO_MASK action pass

    #IPv6
    tc -n ns1 filter add dev dummy1 egress pref 1 handle 60${i} \
            proto 802.1q flower vlan_prio $i vlan_ethtype ipv6 \
            dst_ip $TGT6_NO_MASK action pass
done

FILTER_COUNTER=10

for i in 4 6; do
    [ $i == 4 ] && TGT=$TGT4_NO_MASK || TGT=$TGT6_NO_MASK
    for proto in u i r; do
        echo "Test IPV$i, prot: $proto"
        for priority in {0..7}; do
            handle="${FILTER_COUNTER}${priority}"
            pkts=$(tc -n ns1 -j -s filter show dev dummy1 egress \
                | jq ".[] | select(.options.handle == ${handle}) | \
                .options.actions[0].stats.packets")
            [[ $pkts == 0 ]] || echo "prio $priority: expected 0, got $pkts"

            ip netns exec ns1 ./cmsg_sender -$i -Q $priority -d "${DELAY}" -p $proto $TGT $PORT

            pkts=$(tc -n ns1 -j -s filter show dev dummy1 egress \
                | jq ".[] | select(.options.handle == ${handle}) | \
                .options.actions[0].stats.packets")
            [[ $pkts == 1 ]] || echo "prio $priority: expected 1, got $pkts"
        done
        FILTER_COUNTER=$((FILTER_COUNTER + 10))
    done
done
