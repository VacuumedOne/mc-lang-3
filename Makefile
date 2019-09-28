CXX = clang++
CXXFLAGS = `llvm-config --cxxflags --ldflags --system-libs --libs all`

.PHONY: mc binsearch typetest func clean FORCE

mc: src/mc.cpp
	$(CXX) $(CXXFLAGS) src/mc.cpp -o mc

binsearch: test/test_binsearch.mc binsearch.cpp
	./mc test/test_binsearch.mc
	$(CXX) binsearch.cpp output.o -o binsearch
	./binsearch

typetest: FORCE
	./mc test/test_typetest.mc

func: test/test_func.mc func.cpp
	./mc test/test_func.mc

clean:
	rm mc output.o
