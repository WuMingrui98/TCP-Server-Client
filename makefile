CFLAGS = -I./ -I./serverFile -I./clientFile
server = server
client = client
$(server):
	g++ ./serverFile/main.cpp ./serverFile/log.cpp ./serverFile/tcp_conn.cpp -lpthread -g -o $@
$(client):
	g++ ./clientFile/main.cpp ./clientFile/client.cpp -lpthread -g -o $@
clean:
	rm -rf $(server) $(client)
