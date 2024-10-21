#!/bin/bash


IP4=192.168.0.2/16
TGT4=192.168.0.3/16
TGT4_NO_MASK=192.168.0.3
IP6=2001:db8::2/64
TGT6=2001:db8::3/64
TGT6_NO_MASK=2001:db8::3
IP4BR=192.168.0.1/16
IP6BR=2001:db8::1/64
PORT=8080

cores=$(nproc)
tx_queues=$cores
rx_queues=$cores

priority_values=($(seq 0 $((cores - 1))))  

queue_config=""
for ((i=0; i<$cores; i++)); do
    queue_config+=" 1@$i"
done

map_config=$(seq 0 $((cores - 1)) | tr '\n' ' ')

ip netns add red
ip netns add green
ip link add br0 type bridge
ip link set br0 up
ip addr add $IP4BR dev br0
ip addr add $IP6BR dev br0
ip link add vethcab0 type veth peer name red0
ip link set vethcab0 master br0
ip link set red0 netns red 
ip netns exec red bash -c "
ip link set lo up; ip link set red0 up; 
ip addr add $IP4 dev red0; 
ip addr add $IP6 dev red0;
sysctl -w net.ipv4.ping_group_range="0 2147483647";
exit"
ip link set vethcab0 up

ip link add vethcab1 type veth peer name green0
ip link set vethcab1 master br0
ip link set green0 netns green
ip netns exec green bash -c "ip link set lo up; ip link set green0 up; ip addr add $TGT4 dev green0; ip addr add $TGT6 dev green0; exit" 
ip link set vethcab1 up

ip netns exec red bash -c "sudo ethtool -L red0 tx $tx_queues rx $rx_queues; sudo tc qdisc add dev red0 root mqprio num_tc $cores queues $queue_config map $map_config hw 0; exit"

ip netns exec red ./priority_sender.sh