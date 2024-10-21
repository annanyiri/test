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

priority_values=($(seq 0 $((cores - 1))))  

get_queue_bytes() {
    sudo tc -s qdisc show dev red0 | grep "Sent" | awk '{print $2}' 
}

for i in 4 6; do
    [ $i == 4 ] && TGT=$TGT4_NO_MASK || TGT=$TGT6_NO_MASK
    
    for p in u i r; do
        echo "Test IPV$i, prot: $p" >&2
        for value in "${priority_values[@]}"; do
            ./cmsg_sender_new -a $value -p $p $TGT $PORT
            setsockopt_priority_bytes_num=($(get_queue_bytes))

            sleep 1

            ./cmsg_sender_new -P $value -p $p $TGT $PORT
            cmsg_priority_bytes_num=($(get_queue_bytes))

            if [[ "${cmsg_priority_bytes_num[$actual_queue]}" != "${setsockopt_priority_bytes_num[$actual_queue]}" ]]; then
                echo "Queue $value: cmsg priority setting OK" >&2
            else
                echo "Queue $value: No change in bytes, test failed." >&2
            fi
        done
    done
done
