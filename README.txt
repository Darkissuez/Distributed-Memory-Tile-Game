This project was made for the Systems Programming course. It is a distributed (server-client based) implementation of
the memory-tile game.

To compile the clients and server do:
$ gcc client.c -lpthread  UI_library.c -o client -lSDL2 -lSDL2_ttf
$ gcc server.c -lpthread  UI_library.c board_library.c lista.c -o server -lSDL2 -lSDL2_ttf
$ gcc simple_bot.c board_library.c -o simple_bot

Run the server with:
$ ./server DIMENSION
where DIMENSION is the dimension of the board (3x3, 4x4, etc.)

Run the clients with:
$ ./client ADDRESS
where ADDRESS is the address of the server (local address is 127.0.0.1)


Package requirements:
SDL2/SDL
SDL2/SDL_ttf


How do I install the SDL2 Libraries on the MAC OS X?
Although there are some specific packages for MAC OS X, these only work from inside XCode.
If the students are using the regular gcc development procedures, it is necessary to install specific libraries.
This is accomplished running the following commands on the terminal:
    > brew install sdl2
    > brew install sdl2_ttf


How do I install the SDL2 libraries in Linux?
The way to install the required libraries depends on the linux distribution. This procedure is similar to the
installation of any one software package and depend on the software manager (apt, apt-get, yum  or zypper, ...).
In Debian-based systems (including Ubuntu) the commands are:
  $ sudo apt-get install libsdl2-dev
  $ sudo apt-get install libsdl2-ttf-dev

In Red Hat-based systems (including Fedora) the commands are:
  $ sudo yum install SDL2-devel
  $ sudo yum install SDL2_ttf-devel

In Open Suse the commands are:
  $ sudo zypper install SDL2-devel
  $ sudo zypper install SDL2_ttf-devel
