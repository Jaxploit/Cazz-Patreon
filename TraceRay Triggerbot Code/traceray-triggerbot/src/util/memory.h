#pragma once
#include <cstdint>

namespace memory
{
	// given an interface pointer, calls the function at
	// the index provided
	template <typename Return, typename ... Args>
	constexpr Return Call(void* _this, const std::uintptr_t index, Args ... args) noexcept
	{
		using Function = Return(__thiscall*)(void*, decltype(args)...);
		return (*static_cast<Function**>(_this))[index](_this, args...);
	}

	// given an interface pointer, returns the function at
	// the index provided
	constexpr void* Get(void* _this, const std::uintptr_t index) noexcept
	{
		return (*static_cast<void***>(_this))[index];
	}
}
