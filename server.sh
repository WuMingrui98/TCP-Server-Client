#!/bin/bash
Port=8081		#绑定的连接端口
nThread=10		#线程池大小（默认大小为8，可以不填写）
./server $Port $nThread
