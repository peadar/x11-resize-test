all: test

test: test.o
	$(CXX) -o $@ $^ -lX11 -lXmu

clean:
	rm test.o test

.PHONY: clean all
