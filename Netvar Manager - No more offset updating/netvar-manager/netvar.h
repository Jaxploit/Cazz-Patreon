#pragma once
#include "fnv.h"
#include "datatable.h"

#include <unordered_map>

// call once to dump netvars
void SetupNetvars();

// recursive dump
void Dump(const char* baseClass, RecvTable* table, std::uint32_t offset = 0);

// hold our offset
inline std::unordered_map<std::uint32_t, std::uint32_t> netvars;

#define NETVAR(func_name, netvar, type) type& func_name() \
{ \
	static auto offset = netvars[fnv::HashConst(netvar)]; \
	return *reinterpret_cast<type*>(std::uint32_t(this) + offset); \
}
