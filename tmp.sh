

ip netns exec red bash -c "
for value in "${priority_values[@]}"; do
    actual_queue=$((cores - value))
    
    priority_before=$(sudo tc -s qdisc show dev red0 | grep "Sent" | awk -v n="$actual_queue" 'NR==n {print $2}')
    ./cmsg_sender_new -a $PRIORITY $TGT4_NO_MASK $PORT
    priority_after=$(sudo tc -s qdisc show dev red0 | grep "Sent" | awk -v n="$actual_queue" 'NR==n {print $2}')
    
    echo "Queue: $actual_queue, Priority Before: $priority_before, Priority After: $priority_after"
done;
exit"

for PRIORITY in "${priority_values[@]}"; do
    ip netns exec red bash -c "
        check_queues() {
            local priority=\$1
            local queue_num=\$((cores - priority))
            local actual_queue=\$(sudo tc -s qdisc show dev red0 | sed -n \"\${queue_num}p\")
            echo \"\$actual_queue\"
        }

        queue_before=\$(check_queues $PRIORITY);
        echo \"before: \$queue_before\";
        ./cmsg_sender_new -a $PRIORITY $TGT4 $PORT;
        queue_after=\$(check_queues $PRIORITY);
        echo \"after: \$queue_after\";
        exit"
done


:'for PRIORITY in "${priority_values[@]}"; do
    	for p in u i r; do
	    [ $p == "u" ] && prot=UDP
	    [ $p == "i" ] && prot=ICMP
	    [ $p == "r" ] && prot=RAW

	    [ $ovr == "setsock" ] && m="-P"
	    [ $ovr == "cmsg" ]    && m="-a"
	    [ $ovr == "both" ]    && m="-P $PRIORITY -a"

	    ip netns exec red bash -c "sudo"

	    [ $ovr == "diff" ] && m="-M $((MARK + 1)) -m"

	    ip netns exec $NS ./cmsg_sender -$i -p $p $m $MARK -s $TGT 1234
	    check_result $? 1 "$prot $ovr - rejection"
done'
         


