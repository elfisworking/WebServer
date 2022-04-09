all:
	cd src && g++ -o main.out main.cpp
clean:
	cd src && rm -rf *.out