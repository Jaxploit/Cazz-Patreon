#pragma once
#include "../util/memory.h"

#include "cvector.h"

class CEntity
{
public:
	constexpr std::int32_t GetTeam() noexcept
	{
		return memory::Call<std::int32_t>(this, 88);
	}

	constexpr bool IsAlive() noexcept
	{
		return memory::Call<bool>(this, 156);
	}

	constexpr bool IsPlayer() noexcept
	{
		return memory::Call<bool>(this, 158);
	}

	constexpr void GetEyePosition(CVector& eyePositionOut) noexcept
	{
		memory::Call<void, CVector&>(this, 285, eyePositionOut);
	}

	constexpr void GetAimPunch(CVector& aimPunchOut) noexcept
	{
		memory::Call<void, CVector&>(this, 346, aimPunchOut);
	}
};
