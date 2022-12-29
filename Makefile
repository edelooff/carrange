.DEFAULT_GOAL=composer

composer: composer.cpp
	clang++ -O3 -Wall --std=c++20 composer.cpp -o composer

test: composer
	./composer < example.in.txt | diff - example.out.txt

clean:
	rm composer
