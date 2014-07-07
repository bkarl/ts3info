all: 
	gcc main.c -lcurl -o ts3info

clean:
	rm -rf *o ts3info