PKG_LIBS=glib-2.0 libcurl

PC_CFLAGS=`pkg-config --cflags ${PKG_LIBS}`
PC_LIBS=`pkg-config --libs ${PKG_LIBS}`

#uncomment this line to get some debug data:
#DEBUG=-Wall -Wextra -g -DDEBUG

nosebus:
	gcc $(DEBUG) $(PC_CFLAGS) -c nosebus.c
	gcc $(DEBUG) $(PC_CFLAGS) -c web.c
	gcc $(DEBUG) $(PC_CFLAGS) -c predict.c
	gcc $(DEBUG) $(PC_LIBS) -o nosebus nosebus.o web.o predict.o

clean:
	rm *.o nosebus
