cppcheck:
	./misc/travis/cppcheck.sh

cpptest:
	CC=gcc CXX=g++ CMAKE_BUILD_TYPE=Debug ./misc/travis/cpptest.sh

gotest:
	cd golibmc; go test -v

pytest:
	pytest tests/

clean:
	rm -fr build cython_debug libmc/*.so libmc/*.cpp .eggs
