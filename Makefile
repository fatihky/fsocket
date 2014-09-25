all:
	cd src && make && cd ..

test:
	cd src && make test &&  cd ..

install:
	cd src && make install &&  cd ..

clean:
	cd src && make clean &&  cd ..