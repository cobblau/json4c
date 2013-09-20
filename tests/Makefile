CFLAGS=-g
INCLUDE=-I../

test_parse : json.o test_parse.o
	gcc $(CFLAGS) $(INCLUDE) -o $@ $^
json.o : 
	cc $(INCLUDE) -c ../json.c json.h 
test_parse.o : 
	cc $(INCLUDE) -c test_parse.c

test_api : json.o test_api.o
	gcc $(CFLAGS) $(INCLUDE) -o $@ $^
test_api.o : 
	cc $(INCLUDE) -c test_api.c

test_dump : json.o test_dump.o
	gcc $(CFLAGS) $(INCLUDE) -o $@ $^
test_dump.o : 
	cc $(INCLUDE) -c test_dump.c


.PHONY : clean
clean:
	rm *.o
	rm *.out