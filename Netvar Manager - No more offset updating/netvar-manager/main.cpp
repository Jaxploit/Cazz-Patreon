#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "interfaces.h"

template <typename T>
T* GetInterface(const char* interfaceName, const char* moduleName)
{
	const auto handle = GetModuleHandle(moduleName);

	if (!handle)
	{
		return nullptr;
	}

	using Fn = T * (*)(const char*, int*);
	const auto CreateInterface = reinterpret_cast<Fn>(GetProcAddress(handle, "CreateInterface"));

	if (!CreateInterface)
	{
		return nullptr;
	}

	return CreateInterface(interfaceName, nullptr);
}

void HackThread(HMODULE instance)
{
	client = GetInterface<IClient>("VClient018", "client.dll");
	entityList = GetInterface<IEntityList>("VClientEntityList003", "client.dll");

	SetupNetvars();

	while (!GetAsyncKeyState(VK_END))
	{
		for (auto i = 1; i <= 64; ++i)
		{
			const auto entity = entityList->GetClientEntity(i);

			if (!entity)
				continue;

			entity->Spotted() = true;
		}
	}

	FreeLibraryAndExitThread(instance, 0);
}

BOOL WINAPI DllMain(HMODULE instance, DWORD reason, LPVOID reserved)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		DisableThreadLibraryCalls(instance);

		const auto thread = CreateThread(
			nullptr,
			0,
			reinterpret_cast<LPTHREAD_START_ROUTINE>(HackThread),
			instance,
			0,
			nullptr
		);

		if (thread)
			CloseHandle(thread);
	}

	return TRUE;
}
