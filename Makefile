all: build
	./bin/game
	clear


build: bin
	gcc -O3 game.c -DOLIVEC_IMPLEMENTATION -Iext -I/opt/homebrew/include -L/opt/homebrew/lib -lX11 -lm -o bin/game

run:
	./bin/game
	clear

clean:
	rm -rf bin
	mkdir bin

git:
	git add . 
	git commit -a -m "EDITOR" 
	git push
