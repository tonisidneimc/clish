shell: main.c
	$(CC) main.c -o shell -lreadline
	
install: shell
	sudo cp shell /usr/local/bin
	sudo chmod +x /usr/local/bin
	
clean:
	@rm -f *.o shell *~ *.txt
