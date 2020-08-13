obj = main.o Util/linux_file.o Util/Util.o DataFile/DataFile.o VirtualFileSys/VFS.o LOB/LOBimpl.o LOB/LOB_Interface.o DB/DB.o DB/Table.o
run : $(obj)
	g++ -g -o run $(obj) -std=c++11
main.o : main.cpp
	g++ -g -c main.cpp -o main.o -std=c++11
Util/linux_file.o : Util/linux_file.cpp
	g++ -g -c Util/linux_file.cpp -o Util/linux_file.o -std=c++11
Util/Util.o : Util/Util.cpp
	g++ -g -c Util/Util.cpp -o Util/Util.o -std=c++11
DataFile/DataFile.o : DataFile/DataFile.cpp
	g++ -g -c DataFile/DataFile.cpp -o DataFile/DataFile.o -std=c++11
VirtualFileSys/VFS.o : VirtualFileSys/VFS.cpp
	g++ -g -c VirtualFileSys/VFS.cpp -o VirtualFileSys/VFS.o -std=c++11
LOB/LOBimpl.o : LOB/LOBimpl.cpp
	g++ -g -c LOB/LOBimpl.cpp -o LOB/LOBimpl.o -std=c++11
LOB/LOB_Interface.o : LOB/LOB_Interface.cpp
	g++ -g -c LOB/LOB_Interface.cpp -o LOB/LOB_Interface.o -std=c++11
DB/DB.o : DB/DB.cpp
	g++ -g -c DB/DB.cpp -o DB/DB.o -std=c++11
DB/Table.o : DB/Table.cpp
	g++ -g -c DB/Table.cpp -o DB/Table.o -std=c++11


clean :
	rm *.o
	rm Util/*.o
	rm DataFile/*.o
	rm VirtualFileSys/*.o
	rm LOB/*.o
	rm DB/*.o
	rm run
	rm /data/test/myFile