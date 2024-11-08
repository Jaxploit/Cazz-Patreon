#include "hooks.h"

// include minhook for epic hookage
#include "../../ext/minhook/minhook.h"

#include <intrin.h>

void hooks::Setup() noexcept
{
	MH_Initialize();

	// AllocKeyValuesMemory hook
	MH_CreateHook(
		memory::Get(interfaces::keyValuesSystem, 1),
		&AllocKeyValuesMemory,
		reinterpret_cast<void**>(&AllocKeyValuesMemoryOriginal)
	);

	// CreateMove hook
	MH_CreateHook(
		memory::Get(interfaces::clientMode, 24),
		&CreateMove,
		reinterpret_cast<void**>(&CreateMoveOriginal)
	);

	// ListLeavesInBox hook
	MH_CreateHook(
		memory::Get(interfaces::engine->GetBSPTreeQuery(), 6),
		&ListLeavesInBox,
		reinterpret_cast<void**>(&ListLeavesInBoxOriginal)
	);

	// DrawModel hook
	MH_CreateHook(
		memory::Get(interfaces::studioRender, 29),
		&DrawModel,
		reinterpret_cast<void**>(&DrawModelOriginal)
	);

	MH_EnableHook(MH_ALL_HOOKS);
}

void hooks::Destroy() noexcept
{
	// restore hooks
	MH_DisableHook(MH_ALL_HOOKS);
	MH_RemoveHook(MH_ALL_HOOKS);

	// uninit minhook
	MH_Uninitialize();
}

void* __stdcall hooks::AllocKeyValuesMemory(const std::int32_t size) noexcept
{
	// if function is returning to speficied addresses, return nullptr to "bypass"
	if (const std::uint32_t address = reinterpret_cast<std::uint32_t>(_ReturnAddress());
		address == reinterpret_cast<std::uint32_t>(memory::allocKeyValuesEngine) ||
		address == reinterpret_cast<std::uint32_t>(memory::allocKeyValuesClient)) 
		return nullptr;

	// return original
	return AllocKeyValuesMemoryOriginal(interfaces::keyValuesSystem, size);
}

bool __stdcall hooks::CreateMove(float frameTime, CUserCmd* cmd) noexcept
{
	// make sure this function is being called from CInput::CreateMove
	if (!cmd->commandNumber)
		return CreateMoveOriginal(interfaces::clientMode, frameTime, cmd);

	// this would be done anyway by returning true
	if (CreateMoveOriginal(interfaces::clientMode, frameTime, cmd))
		interfaces::engine->SetViewAngles(cmd->viewAngles);

	// get our local player here
	globals::UpdateLocalPlayer();

	if (globals::localPlayer && globals::localPlayer->IsAlive())
	{
		// example bhop
		if (!(globals::localPlayer->GetFlags() & CEntity::FL_ONGROUND))
			cmd->buttons &= ~CUserCmd::IN_JUMP;
	}

	return false;
}

int __stdcall hooks::ListLeavesInBox(const CVector& mins, const CVector& maxs, std::uint16_t* list, int listMax) noexcept
{
	static const auto insertIntoTree = reinterpret_cast<std::uintptr_t>(memory::insertIntoTree);

	if (reinterpret_cast<std::uintptr_t>(_ReturnAddress()) != insertIntoTree)
		return ListLeavesInBoxOriginal(interfaces::engine->GetBSPTreeQuery(), mins, maxs, list, listMax);

	// get the RenderableInfo pointer from the stack
	const auto info = *reinterpret_cast<CRenderableInfo**>(reinterpret_cast<std::uintptr_t>(_AddressOfReturnAddress()) + 0x14);

	// make sure it is valid
	if (!info || !info->renderable)
		return ListLeavesInBoxOriginal(interfaces::engine->GetBSPTreeQuery(), mins, maxs, list, listMax);

	// get the base entity from the IClientRenderable pointer
	CEntity* entity = info->renderable->GetIClientUnknown()->GetBaseEntity();

	// make sure it is valid
	if (!entity || !entity->IsPlayer())
		return ListLeavesInBoxOriginal(interfaces::engine->GetBSPTreeQuery(), mins, maxs, list, listMax);

	// no need to reorder for teammates
	if (entity->GetTeam() == globals::localPlayer->GetTeam())
		return ListLeavesInBoxOriginal(interfaces::engine->GetBSPTreeQuery(), mins, maxs, list, listMax);

	// set the flags
	info->flags &= ~RENDER_FLAGS_FORCE_OPAQUE_PASS;
	info->flags2 |= RENDER_FLAGS_BOUNDS_ALWAYS_RECOMPUTE;

	constexpr float maxCoord = 16384.0f;
	constexpr float minCoord = -maxCoord;

	constexpr CVector min{ minCoord, minCoord, minCoord };
	constexpr CVector max{ maxCoord, maxCoord, maxCoord };

	// return with maximum bounds
	return ListLeavesInBoxOriginal(interfaces::engine->GetBSPTreeQuery(), min, max, list, listMax);
}

void __stdcall hooks::DrawModel(
	void* results,
	const CDrawModelInfo& info,
	CMatrix3x4* bones,
	float* flexWeights,
	float* flexDelayedWeights,
	const CVector& modelOrigin,
	const std::int32_t flags
) noexcept
{
	// make sure we are in-game
	if (globals::localPlayer && info.renderable)
	{
		// get the base entity
		CEntity* entity = info.renderable->GetIClientUnknown()->GetBaseEntity();

		// make sure they are a valid enemy player
		if (entity && entity->IsPlayer() && entity->GetTeam() != globals::localPlayer->GetTeam())
		{
			// create our custom material
			static IMaterial* material = interfaces::materialSystem->CreateMaterial(
				"pearlescent",
				CKeyValues::FromString(
					"VertexLitGeneric",
					"$phong 1 "
					"$basemapalphaphongmask 1 "
					"$pearlescent 2"
				)
			);

			// float arrays to hold our chams colors
			// put these in globals:: to modify with a menu
			constexpr float hidden[3] = { 0.f, 1.f, 0.f };
			constexpr float visible[3] = { 1.f, 0.f, 1.f };

			// alpha modulate (once in my case)
			interfaces::studioRender->SetAlphaModulation(1.f);

			// show through walls
			material->SetMaterialVarFlag(IMaterial::IGNOREZ, true);
			interfaces::studioRender->SetColorModulation(hidden);
			interfaces::studioRender->ForcedMaterialOverride(material);
			DrawModelOriginal(interfaces::studioRender, results, info, bones, flexWeights, flexDelayedWeights, modelOrigin, flags);

			// do not show through walls
			material->SetMaterialVarFlag(IMaterial::IGNOREZ, false);
			interfaces::studioRender->SetColorModulation(visible);
			interfaces::studioRender->ForcedMaterialOverride(material);
			DrawModelOriginal(interfaces::studioRender, results, info, bones, flexWeights, flexDelayedWeights, modelOrigin, flags);

			// reset the material override + return from hook
			return interfaces::studioRender->ForcedMaterialOverride(nullptr);
		}
	}

	DrawModelOriginal(interfaces::studioRender, results, info, bones, flexWeights, flexDelayedWeights, modelOrigin, flags);
}
