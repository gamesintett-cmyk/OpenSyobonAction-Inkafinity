CXX ?= g++
CXXFLAGS ?= -O2
SDL_CFLAGS := $(shell sdl-config --cflags)
SDL_LIBS := $(shell sdl-config --libs)

OBJS=main.o loadg.o DxLib.o interactive.o

SyobonAction: $(OBJS)
	$(CXX) $(OBJS) -o SyobonAction $(SDL_LIBS) -lSDL_gfx -lSDL_image -lSDL_mixer -lSDL_ttf -pthread

main.o: main.cpp main.h interactive.h
	$(CXX) $(CXXFLAGS) $(SDL_CFLAGS) -c main.cpp

loadg.o: loadg.cpp main.h
	$(CXX) $(CXXFLAGS) $(SDL_CFLAGS) -c loadg.cpp

DxLib.o: DxLib.cpp DxLib.h
	$(CXX) $(CXXFLAGS) $(SDL_CFLAGS) -c DxLib.cpp

interactive.o: interactive.cpp interactive.h main.h
	$(CXX) $(CXXFLAGS) $(SDL_CFLAGS) -c interactive.cpp

clean:
	rm -f $(OBJS) SyobonAction
