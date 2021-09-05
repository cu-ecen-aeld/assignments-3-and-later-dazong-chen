CROSS_COMPILE=

writer : writer.o
	$(CROSS_COMPILE)gcc -o writer writer.o 
	
writer.o: writer.c
	$(CROSS_COMPILE)gcc -c writer.c -o writer.o
    
clean: 
	rm writer writer.o


