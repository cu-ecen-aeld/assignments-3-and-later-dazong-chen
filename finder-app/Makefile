CROSS_COMPILE=aarch64-none-linux-gnu-gcc

writer : writer.o
	$(CROSS_COMPILE) -o writer writer.o 
	
writer.o: writer.c
	$(CROSS_COMPILE) -c writer.c -o writer.o
    
clean: 
	rm writer writer.o


