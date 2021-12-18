#!/bin/bash
IP=127.0.0.1 #连接的ip
Port=8081		#连接的端口
clients=50		#并发数
nsum=100000		#发送消息总数
message=hello	#发送的消息
./client $clients $nsum $message $IP $Port
