#include "interfaces.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

void interfaces::Setup() noexcept
{
	entityList = Capture<IClientEntityList>("VClientEntityList003", "client.dll");
	engine = Capture<IEngineClient>("VEngineClient014", "engine.dll");
	trace = Capture<IEngineTraceClient>("EngineTraceClient004", "engine.dll");

	client = Capture<void>("VClient018", "client.dll");
	clientMode = **reinterpret_cast<void***>((*reinterpret_cast<unsigned int**>(client))[10] + 5);
}

template <typename T>
T* interfaces::Capture(const char* name, const char* lib) noexcept
{
	const HINSTANCE handle = GetModuleHandle(lib);

	if (!handle)
		return nullptr;

	using Function = T * (*)(const char*, int*);
	Function CreateInterface = reinterpret_cast<Function>(GetProcAddress(handle, "CreateInterface"));

	return CreateInterface(name, nullptr);
}
