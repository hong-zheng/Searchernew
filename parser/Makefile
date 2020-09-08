FLAG=-std=c++11 -l boost_filesystem -l boost_system

parser:parser.cc
	g++ $^ -o $@ $(FLAG)

.PHONY:clean
clean:	
	rm parser
