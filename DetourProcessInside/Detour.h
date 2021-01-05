#pragma once
class Detour
{
	void* targetPtr;
	void* detourFunc;

public:
	Detour(void* src, void* dest);
	~Detour();
};

#define DETOURS(src, dest) Detour instance{##src, ##dest}

