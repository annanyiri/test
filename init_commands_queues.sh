#!/bin/bash

ethtool -L red0 tx 12 rx 12
sudo tc qdisc add dev red0 root mqprio num_tc 12 queues 1@0 1@1 1@2 1@3 1@4 1@5 1@6 1@7 1@8 1@9 1@10 1@11 map 0 1 2 3 4 5 6 7 8 9 10 11 hw 0

