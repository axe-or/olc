#include "base.hpp"

extern "C" int printf(char const*, ...);

template<Integer I>
struct Non_Zero {
private:
	I value_;

	Non_Zero(I v) : value_{v}{}

public:
	constexpr I get() const { return value_; }

	constexpr
	static Option<Non_Zero<I>> from(I v){
		if(v == 0){
			return {};
		}
		return res;
	}
};

void entrypoint(){
	auto n = Non_Zero<i32>::from(0);
}

void init(){
	// Init heap
	constexpr usize heap_size = 16ull * 1024ull * 1024ull;
	u8 static heap_data[heap_size] = {0};
	auto heap = heap_from_buffer({&heap_data[0], heap_size});

	// Init temp
	constexpr usize temp_size = 1ull * 1024ull * 1024ull;
	u8 static temp_data[temp_size] = {0};
	auto temp = arena_from_buffer({&temp_data[0], temp_size});
}

int main(){
	init();
	entrypoint();
}

#include "base.cpp"
