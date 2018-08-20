default: netfileserver.c client.c
	gcc -o client client.c -g
	gcc -o server netfileserver.c -lpthread -lm -g
server: netfileserver.c
	gcc -o server netfileserver.c -lpthread -lm -g
client: client.c
	gcc -o client client.c -g 

clean:
	rm client
	rm server