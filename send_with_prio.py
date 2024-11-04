from socket import *

if __name__ == '__main__':
    sk = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)
    sk.setsockopt(SOL_SOCKET, SO_PRIORITY, 5)
    sk.sendto(b"00000", ("192.168.0.3", 8080))
    sk.sendto(b"00000", ("192.168.0.3", 8080))
    sk.sendto(b"00000", ("192.168.0.3", 8080))
    sk.sendto(b"00000", ("192.168.0.3", 8080))
    sk.setsockopt(SOL_SOCKET, SO_PRIORITY, 2)
    sk.sendto(b"00000", ("192.168.0.3", 8080))
    sk.setsockopt(SOL_SOCKET, SO_PRIORITY, 1)
    sk.sendto(b"00000", ("192.168.0.3", 8080))
