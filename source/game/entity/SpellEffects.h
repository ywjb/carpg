// efekty czar�w, eksplozje, electro, drain
#pragma once

//-----------------------------------------------------------------------------
#include "Resource.h"

//-----------------------------------------------------------------------------
struct Unit;
struct Spell;
struct ParticleEmitter;

//-----------------------------------------------------------------------------
struct Explo
{
	VEC3 pos;
	float size, sizemax, dmg;
	Texture tex;
	vector<Unit*> hitted;
	Unit* owner;

	void Save(HANDLE file);
	void Load(HANDLE file);
};

//-----------------------------------------------------------------------------
struct ElectroLine
{
	vector<VEC3> pts;
	float t;
};

//-----------------------------------------------------------------------------
struct Electro
{
	vector<ElectroLine> lines;
	vector<Unit*> hitted;
	float dmg;
	Unit* owner;
	Spell* spell;
	bool valid, hitsome;
	VEC3 start_pos;
	int netid;

	void AddLine(const VEC3& from, const VEC3& to);
	void Save(HANDLE file);
	void Load(HANDLE file);
};

//-----------------------------------------------------------------------------
struct Drain
{
	Unit* from, *to;
	ParticleEmitter* pe;
	float t;

	void Save(HANDLE file);
	void Load(HANDLE file);
};