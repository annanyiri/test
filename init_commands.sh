sudo ip link add type veth
sudo ip addr add 1.1.1.1/24 dev veth0
sudo ip addr add 1.1.1.2/24 dev veth1
sudo ip link set veth0 up
sudo ip link set veth1 up

