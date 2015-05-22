// character class
#pragma once

//-----------------------------------------------------------------------------
enum class Class
{
	BARBARIAN,
	BARD,
	CLERIC,
	DRUID,
	HUNTER,
	MAGE,
	MONK,
	PALADIN,
	ROGUE,
	WARRIOR,

	MAX,
	INVALID,
	RANDOM
};

//-----------------------------------------------------------------------------
struct ClassInfo
{
	Class class_id;
	cstring id, unit_data, icon_file;
	string name, desc;
	TEX icon;
	bool pickable;

	inline ClassInfo(Class class_id, cstring id, cstring unit_data, cstring icon_file, bool pickable) : class_id(class_id), id(id), unit_data(unit_data), icon_file(icon_file), icon(NULL),
		pickable(pickable)
	{

	}

	static ClassInfo* Find(const string& id);
	static inline bool IsPickable(Class c);
	static Class GetRandom();
	static Class GetRandomPlayer();
	static Class GetRandomEvil();
	static void Validate(int &err);
	static Class OldToNew(Class c);
};

//-----------------------------------------------------------------------------
extern ClassInfo g_classes[(int)Class::MAX];

//=================================================================================================
inline bool ClassInfo::IsPickable(Class c)
{
	if(c < (Class)0 || c >= Class::MAX)
		return false;
	return g_classes[(int)c].pickable;
}
