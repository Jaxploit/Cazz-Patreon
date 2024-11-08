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

	// PaintTraverse hook
	MH_CreateHook(
		memory::Get(interfaces::panel, 41),
		&PaintTraverse,
		reinterpret_cast<void**>(&PaintTraverseOriginal)
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

static bool WorldToScreen(const CVector& point, CVector& screen) noexcept
{
	// get the w2s matrix
	const CMatrix4x4& matrix = interfaces::engine->WorldToScreenMatrix();

	// calc width first to test whether on screen or not
	float w = matrix.data[3][0] * point.x + matrix.data[3][1] * point.y + matrix.data[3][2] * point.z + matrix.data[3][3];

	// not on screen
	if (w < 0.001f)
		return false;

	float inverse = 1.f / w;

	screen.x = (matrix.data[0][0] * point.x + matrix.data[0][1] * point.y + matrix.data[0][2] * point.z + matrix.data[0][3]) * inverse;
	screen.y = (matrix.data[1][0] * point.x + matrix.data[1][1] * point.y + matrix.data[1][2] * point.z + matrix.data[1][3]) * inverse;

	int x, y;
	interfaces::engine->GetScreenSize(x, y);

	screen.x = (x * 0.5f) + (screen.x * x) * 0.5f;
	screen.y = (y * 0.5f) - (screen.y * y) * 0.5f;

	// on screen
	return true;
}

void __stdcall hooks::PaintTraverse(std::uintptr_t vguiPanel, bool forceRepaint, bool allowForce) noexcept 
{
	// make sure we have the right panel
	if (vguiPanel == interfaces::engineVGui->GetPanel(PANEL_TOOLS))
	{
		// make sure we are in-game
		if (interfaces::engine->IsInGame() && globals::localPlayer)
		{
			// loop through players
			for (int i = 1; i <= interfaces::globals->maxClients; ++i)
			{
				// get the player
				CEntity* player = interfaces::entityList->GetEntityFromIndex(i);

				// make sure player is valid
				if (!player)
					continue;

				// make sure they aren't dormant && are alive
				if (player->IsDormant() || !player->IsAlive())
					continue;

				// no esp on teammates
				if (player->GetTeam() == globals::localPlayer->GetTeam())
					continue;

				// dont do esp on who we are spectating
				if (!globals::localPlayer->IsAlive())
					if (globals::localPlayer->GetObserverTarget() == player)
						continue;

				// player's bone matrix
				CMatrix3x4 bones[128];
				if (!player->SetupBones(bones, 128, 0x7FF00, interfaces::globals->currentTime))
					continue;

				// screen position of head
				// we add 11.f here because we want the box ABOVE their head
				CVector top;
				if (!WorldToScreen(bones[8].Origin() + CVector{ 0.f, 0.f, 11.f }, top))
					continue;

				// screen position of feet
				// we subtract 9.f here because we want the box BELOW their feet
				CVector bottom;
				if (!WorldToScreen(player->GetAbsOrigin() - CVector{ 0.f, 0.f, 9.f }, bottom))
					continue;

				// the height of the box is the difference between
				// the bottom (larger number) and the top
				const float h = bottom.y - top.y;

				// we can use the height to determine a width
				const float w = h * 0.3f;

				const auto left = static_cast<int>(top.x - w);
				const auto right = static_cast<int>(top.x + w);

				// set the color to white
				interfaces::surface->DrawSetColor(255, 255, 255, 255);

				// draw the normal box
				interfaces::surface->DrawOutlinedRect(left, top.y, right, bottom.y);

				// set the color to black for outlines
				interfaces::surface->DrawSetColor(0, 0, 0, 255);

				// normal box outline
				interfaces::surface->DrawOutlinedRect(left - 1, top.y - 1, right + 1, bottom.y + 1);
				interfaces::surface->DrawOutlinedRect(left + 1, top.y + 1, right - 1, bottom.y - 1);

				// health bar outline (use the black color here)
				interfaces::surface->DrawOutlinedRect(left - 6, top.y - 1, left - 3, bottom.y + 1);

				// health is an integer from 0 to 100
				// we can make it a percentage by multipying
				// it by 0.01
				const float healthFrac = player->GetHealth() * 0.01f;

				// set the color of the health bar to a split between red / green
				interfaces::surface->DrawSetColor((1.f - healthFrac) * 255, 255 * healthFrac, 0, 255);

				// draw it
				interfaces::surface->DrawFilledRect(left - 5, bottom.y - (h * healthFrac), left - 4, bottom.y);
			}
		}
	}

	// call original function
	PaintTraverseOriginal(interfaces::panel, vguiPanel, forceRepaint, allowForce);
}
