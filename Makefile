CXX = clang++
CXXFLAGS = `llvm-config --cxxflags --ldflags --system-libs --libs all`

.PHONY: mc

mc: src/mc.cpp
	$(CXX) $(CXXFLAGS) src/mc.cpp -o mc

binsearch: mc test/test_binsearch.mc binsearch.cpp
	./mc test/test_binsearch.mc
	$(CXX) binsearch.cpp output.o -o binsearch
	./binsearch

typetest: mc test/test_typetest.mc typetest.cpp
	./mc test/test_typetest.mc
	$(CXX) typetest.cpp output.o -o typetest
	./typetest

func: mc test/test_func.mc func.cpp
	./mc test/test_func.mc
	$(CXX) func.cpp output.o -o func
	./func

clean:
	rm mc output.o
