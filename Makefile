ALL: http-scws++ 

http-scws++: http-scws.cpp
	g++ -o $@ $^ -ljsoncpp -lscws -L/data/home/yaaf_proj/workspace/build-yaaf-extlib/extlib/lib -levent -lm -lrt

clean:
	@rm http-scws++
