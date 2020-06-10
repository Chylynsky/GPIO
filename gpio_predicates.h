#pragma once

struct input
{
};

struct output
{
};

template<typename _Dir>
bool constexpr Is_input()
{
	return false;
}

template<>
bool constexpr Is_input<input>()
{
	return true;
}

template<typename _Dir>
bool constexpr Is_output()
{
	return false;
}

template<>
bool constexpr Is_output<output>()
{
	return true;
}