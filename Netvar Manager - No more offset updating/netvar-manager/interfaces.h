#pragma once
#include "entity.h"
#include "datatable.h"

class IEntityList
{
private:
	virtual void Unused_0() = 0;
	virtual void Unused_1() = 0;
	virtual void Unused_2() = 0;
public:
	virtual CEntity* GetClientEntity(int index) = 0;
};

class IClient
{
private:
	virtual void Unused_0() = 0;
	virtual void Unused_1() = 0;
	virtual void Unused_2() = 0;
	virtual void Unused_3() = 0;
	virtual void Unused_4() = 0;
	virtual void Unused_5() = 0;
	virtual void Unused_6() = 0;
	virtual void Unused_7() = 0;

public:
	virtual ClientClass* GetAllClasses(void) = 0;
};

inline IEntityList* entityList = nullptr;
inline IClient* client = nullptr;
