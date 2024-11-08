#include "hooks.h"

// include minhook to perform epic hookage
#include "../../ext/minhook/MinHook.h"

void hooks::Setup() noexcept
{
	MH_Initialize();

	// place createmove hook
	MH_CreateHook(
		memory::Get(interfaces::clientMode, 24), // CreateMove is index 24
		&CreateMove,
		reinterpret_cast<void**>(&CreateMoveOriginal)
	);

	MH_EnableHook(MH_ALL_HOOKS);
}

void hooks::Destroy() noexcept
{
	MH_DisableHook(MH_ALL_HOOKS);
	MH_RemoveHook(MH_ALL_HOOKS);
	MH_Uninitialize();
}

bool __stdcall hooks::CreateMove(float frameTime, CUserCmd* cmd) noexcept
{
	// store original return value
	const bool result = CreateMoveOriginal(interfaces::clientMode, frameTime, cmd);

	// dont run hack logic if not called by CInput::CreateMove
	if (!cmd->commandNumber)
		return result;

	// set view angles if return value is true to avoid stuttering
	if (result)
		interfaces::engine->SetViewAngles(cmd->viewAngles);

	// get local player
	globals::localPlayer = interfaces::entityList->GetEntityFromIndex(interfaces::engine->GetLocalPlayerIndex());

	// make sure we are pressing our triggerbot key
	if (!GetAsyncKeyState(VK_XBUTTON2)) // mouse button 5 in this case :)
		return false;

	// make sure local player is alive
	if (!globals::localPlayer || !globals::localPlayer->IsAlive())
		return false;

	CVector eyePosition;
	globals::localPlayer->GetEyePosition(eyePosition);

	CVector aimPunch;
	globals::localPlayer->GetAimPunch(aimPunch);

	// calculate the destination of the ray
	const CVector dst = eyePosition + CVector{ cmd->viewAngles + aimPunch }.ToVector() * 10000.f;

	// trace the ray from eyes -> dest
	CTrace trace;
	interfaces::trace->TraceRay({ eyePosition, dst }, 0x46004009, globals::localPlayer, trace);

	// make sure we hit a player
	if (!trace.entity || !trace.entity->IsPlayer())
		return false;

	// make sure player is alive & is an enemy
	if (!trace.entity->IsAlive() || trace.entity->GetTeam() == globals::localPlayer->GetTeam())
		return false;

	// make our local player shoot
	cmd->buttons |= IN_ATTACK;

	return false;
}
