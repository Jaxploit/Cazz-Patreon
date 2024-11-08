#pragma once
#include "interfaces.h"

namespace hooks
{
	// call once to emplace all hooks
	void Setup() noexcept;

	// call once to restore all hooks
	void Destroy() noexcept;

	// bypass return address checks (thx osiris)
	using AllocKeyValuesMemoryFn = void* (__thiscall*)(void*, const std::int32_t) noexcept;
	inline AllocKeyValuesMemoryFn AllocKeyValuesMemoryOriginal;
	void* __stdcall AllocKeyValuesMemory(const std::int32_t size) noexcept;

	// example CreateMove hook
	using CreateMoveFn = bool(__thiscall*)(IClientModeShared*, float, CUserCmd*) noexcept;
	inline CreateMoveFn CreateMoveOriginal = nullptr;
	bool __stdcall CreateMove(float frameTime, CUserCmd* cmd) noexcept;

	// ListLeavesInBox hook
	using ListLeavesInBoxFn = int(__thiscall*)(ISpacialQuery*, const CVector&, const CVector&, std::uint16_t*, int) noexcept;
	inline ListLeavesInBoxFn ListLeavesInBoxOriginal = nullptr;
	int __stdcall ListLeavesInBox(const CVector& mins, const CVector& maxs, std::uint16_t* list, int listMax) noexcept;

	// DrawModel hook
	using DrawModelFn = void(__thiscall*)(
		IStudioRender*,
		void*,
		const CDrawModelInfo&,
		CMatrix3x4*,
		float*,
		float*,
		const CVector&,
		const std::int32_t
		) noexcept;
	inline DrawModelFn DrawModelOriginal = nullptr;
	void __stdcall DrawModel(
		void* results,
		const CDrawModelInfo& info,
		CMatrix3x4* bones,
		float* flexWeights,
		float* flexDelayedWeights,
		const CVector& modelOrigin,
		const std::int32_t flags
	) noexcept;
}
