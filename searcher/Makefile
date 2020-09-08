FLAG=-std=c++11 -I /tmp/Tools/third_part/include -l jsoncpp
 

.PHONY:all
all:search_test searcherT libsearcher.so
search_test:searcher.cc search_test.cc
	g++ $^ -o $@ $(FLAG)
searcherT:searcherT.cc searcher.cc
	g++ $^ -o $@ $(FLAG)

libsearcher.so:searcher.cc searcher.h
	g++ searcher.cc -o $@ -shared -fPIC $(FLAG)

.PHONY:clean
clean:	
	rm search_test searcherT libsearcher.so
