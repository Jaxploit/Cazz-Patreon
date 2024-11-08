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

	// DoPostScreenSpaceEffects hook
	MH_CreateHook(
		memory::Get(interfaces::clientMode, 44),
		&DoPostScreenSpaceEffects,
		reinterpret_cast<void**>(&DoPostScreenSpaceEffectsOriginal)
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

void __stdcall hooks::DoPostScreenSpaceEffects(const void* viewSetup) noexcept 
{
	// make sure local player is valid & we are in-game
	if (globals::localPlayer && interfaces::engine->IsInGame())
	{
		// loop through glow objects
		for (int i = 0; i < interfaces::glow->glowObjects.size; ++i)
		{
			// get the object
			IGlowManager::CGlowObject& object = interfaces::glow->glowObjects[i];

			// make sure it is used
			if (object.IsUnused())
				continue;

			// make sure we have a valid entity
			if (!object.entity)
				continue;

			// check the entity's class index
			switch (object.entity->GetClientClass()->classID)
			{
			// entity is a player
			case CClientClass::CCSPlayer:
				// make sure they are alive
				if (!object.entity->IsAlive())
					break;

				// enemy
				if (object.entity->GetTeam() != globals::localPlayer->GetTeam())
					// make them red
					object.SetColor(1.f, 0.f, 0.f);

				// teammate
				else
					// make them green
					object.SetColor(0.f, 1.f, 0.f);
				break;

			// is a bomb
			case CClientClass::CC4:
			case CClientClass::CPlantedC4:
				object.SetColor(1.f, 1.f, 0.f);
				break;

			default:
				if (object.entity->IsWeapon())
					object.SetColor(1.f, 1.f, 1.f);
				break;
			}
		}
	}

	// call original function
	return DoPostScreenSpaceEffectsOriginal(interfaces::clientMode, viewSetup);
}
