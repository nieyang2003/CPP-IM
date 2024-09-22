[gate]
    logic_service_address: 127.0.0.1:10001
    service_address: 127.0.0.1:10000
    ws_host: 127.0.0.1
    ws_port: 10010
[logic]
    kafka_brokers: 127.0.0.1:9092
    kafka_topic: ychat
    mysql_conn: host=127.0.0.1;user=root;password=123456;db=ychat
    service_address: 127.0.0.1:10001
[push]
    service_address: 127.0.0.1:10002
[route]
    service_address: 0.0.0.0:10003
    msg_gate_servers: 127.0.0.1:10000
[consumer]
    kafka_brokers: 127.0.0.1:9092
[cassandra]
    cluster_host: 127.0.0.1
    cluster_port: 9042
    groupid: cassandra
    topic: topic
[mysql]
    groupid: mysql
    topic: topic
[varify]
    service_address: 0.0.0.0:10004