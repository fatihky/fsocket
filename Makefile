INSTALL_MSG = "\nlibfsocket successfully installed! Thank you for using it."

all:
	cd src && make && cd ..

examples:
	cd src && make examples &&  cd ..

bench:
	cd src && make bench &&  cd ..

install:
	cd src && make install &&  cd .. && echo $(INSTALL_MSG)

uninstall:
	cd src && make uninstall &&  cd ..

clean:
	cd src && make clean &&  cd ..