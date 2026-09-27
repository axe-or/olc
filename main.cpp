#include <stdio.h>
#include "base.hpp"

template<typename T>
concept Eq = requires(T a, T b){
	{ a == b } -> Convertible_To<bool>;
};

template<typename T>
concept Hash = Eq<T> && (requires(T obj){ { obj.hash() } -> Convertible_To<u64>; } || Convertible_To<T, u64>);

int main(){
	static_assert(Hash<String>, "");
	static_assert(Hash<float>, "");
}

#include "base.cpp"
