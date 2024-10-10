#!/bin/bash

sudo ip link add type veth
sudo ethtool -L veth0 tx 12 rx 12
sudo tc qdisc add dev veth0 root mqprio num_tc 12 queues 1@0 1@1 1@2 1@3 1@4 1@5 1@6 1@7 1@8 1@9 1@10 1@11 map 0 1 2 3 4 5 6 7 8 9 10 11 hw 0
sudo ip link set veth1 up
sudo ip link set veth0 up
sudo ip addr add 1.1.1.1/24 dev veth0
sudo ip addr add 1.1.1.2/24 dev veth1
#sudo ip nei add lladdr 0a:0a:0a:0a:0a:0a dev veth1 1.1.1.2
