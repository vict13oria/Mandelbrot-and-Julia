# APD - Tema 1
# Octombrie 2020

build:
	gcc tema1_par.c -o tema1_par -lpthread -lm -Wall

clean:
	rm -rf tema1_par
