#!/bin/bash

ip netns add red
ip netns add green
ip link add br0 type bridge
ip link set br0 up
ip addr add 2001:db8::1/64 dev br0
ip link add vethcab0 type veth peer name red0
ip link set vethcab0 master br0
ip link set red0 netns red 
ip netns exec red bash -c "ip link set lo up; ip link set red0 up; ip addr add 2001:db8::2/64 dev red0; exit"
ip link set vethcab0 up

ip link add vethcab1 type veth peer name green0
ip link set vethcab1 master br0
ip link set green0 netns green
ip netns exec green bash -c "ip link set lo up; ip link set green0 up; ip addr add 2001:db8::3/64 dev green0; exit" 
ip link set vethcab1 up
