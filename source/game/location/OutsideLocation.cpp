// zewn�trzna lokacja
#include "Pch.h"
#include "Base.h"
#include "OutsideLocation.h"
#include "SaveState.h"

namespace OLD
{
	enum TERRAIN_TILE : byte
	{
		TT_GRASS,
		TT_GRASS2,
		TT_GRASS3,
		TT_SAND,
		TT_ROAD,
		TT_BUILD_GRASS,
		TT_BUILD_SAND,
		TT_BUILD_ROAD
	};
}

//=================================================================================================
OutsideLocation::~OutsideLocation()
{
	delete[] tiles;
	delete[] h;
	DeleteElements(units);
	DeleteElements(chests);
	DeleteElements(useables);
	DeleteElements(items);
}

//=================================================================================================
void OutsideLocation::ApplyContext(LevelContext& ctx)
{
	ctx.units = &units;
	ctx.objects = &objects;
	ctx.chests = &chests;
	ctx.traps = NULL;
	ctx.doors = NULL;
	ctx.items = &items;
	ctx.useables = &useables;
	ctx.bloods = &bloods;
	ctx.lights = NULL;
	ctx.have_terrain = true;
	ctx.require_tmp_ctx = true;
	ctx.type = LevelContext::Outside;
	ctx.building_id = -1;
	ctx.mine = INT2(0,0);
	ctx.maxe = INT2(size, size);
	ctx.tmp_ctx = NULL;
	ctx.masks = NULL;
}

//=================================================================================================
void OutsideLocation::Save(HANDLE file, bool local)
{
	Location::Save(file, local);

	if(last_visit != -1)
	{
		// jednostki
		uint ile = units.size();
		WriteFile(file, &ile, sizeof(ile), &tmp, NULL);
		for(vector<Unit*>::iterator it = units.begin(), end = units.end(); it != end; ++it)
			(*it)->Save(file, local);

		// obiekty
		ile = objects.size();
		WriteFile(file, &ile, sizeof(ile), &tmp, NULL);
		for(vector<Object>::iterator it = objects.begin(), end = objects.end(); it != end; ++it)
			it->Save(file);

		// skrzynie
		ile = chests.size();
		WriteFile(file, &ile, sizeof(ile), &tmp, NULL);
		for(vector<Chest*>::iterator it = chests.begin(), end = chests.end(); it != end; ++it)
			(*it)->Save(file, local);

		// przedmioty
		ile = items.size();
		WriteFile(file, &ile, sizeof(ile), &tmp, NULL);
		for(vector<GroundItem*>::iterator it = items.begin(), end = items.end(); it != end; ++it)
			(*it)->Save(file);

		// u�ywalne
		ile = useables.size();
		WriteFile(file, &ile, sizeof(ile), &tmp, NULL);
		for(vector<Useable*>::iterator it = useables.begin(), end = useables.end(); it != end; ++it)
			(*it)->Save(file, local);

		// krew
		File f(file);
		ile = bloods.size();
		WriteFile(file, &ile, sizeof(ile), &tmp, NULL);
		for(vector<Blood>::iterator it = bloods.begin(), end = bloods.end(); it != end; ++it)
			it->Save(f);

		// teren
		WriteFile(file, tiles, sizeof(TerrainTile)*size*size, &tmp, NULL);
		int size2 = size+1;
		size2 *= size2;
		WriteFile(file, h, sizeof(float)*size2, &tmp, NULL);
	}
}

//=================================================================================================
void OutsideLocation::Load(HANDLE file, bool local)
{
	Location::Load(file, local);

	if(last_visit != -1)
	{
		// jednostki
		uint ile;
		ReadFile(file, &ile, sizeof(ile), &tmp, NULL);
		units.resize(ile);
		for(vector<Unit*>::iterator it = units.begin(), end = units.end(); it != end; ++it)
		{
			*it = new Unit;
			Unit::AddRefid(*it);
			(*it)->Load(file, local);
		}

		// obiekty
		ReadFile(file, &ile, sizeof(ile), &tmp, NULL);
		objects.resize(ile);
		for(vector<Object>::iterator it = objects.begin(), end = objects.end(); it != end; ++it)
			it->Load(file);

		// skrzynie
		ReadFile(file, &ile, sizeof(ile), &tmp, NULL);
		chests.resize(ile);
		for(vector<Chest*>::iterator it = chests.begin(), end = chests.end(); it != end; ++it)
		{
			*it = new Chest;
			(*it)->Load(file, local);
		}

		// przedmioty
		ReadFile(file, &ile, sizeof(ile), &tmp, NULL);
		items.resize(ile);
		for(vector<GroundItem*>::iterator it = items.begin(), end = items.end(); it != end; ++it)
		{
			*it = new GroundItem;
			(*it)->Load(file);
		}

		// u�ywalne
		ReadFile(file, &ile, sizeof(ile), &tmp, NULL);
		useables.resize(ile);
		for(vector<Useable*>::iterator it = useables.begin(), end = useables.end(); it != end; ++it)
		{
			*it = new Useable;
			Useable::AddRefid(*it);
			(*it)->Load(file, local);
		}

		// krew
		File f(file);
		ReadFile(file, &ile, sizeof(ile), &tmp, NULL);
		bloods.resize(ile);
		for(vector<Blood>::iterator it = bloods.begin(), end = bloods.end(); it != end; ++it)
			it->Load(f);

		// teren
		int size2 = size+1;
		size2 *= size2;
		h = new float[size2];
		tiles = new TerrainTile[size*size];
		if(LOAD_VERSION >= V_0_3)
			ReadFile(file, tiles, sizeof(TerrainTile)*size*size, &tmp, NULL);
		else
		{
			OLD::TERRAIN_TILE* old_tiles = new OLD::TERRAIN_TILE[size*size];
			ReadFile(file, old_tiles, sizeof(OLD::TERRAIN_TILE)*size*size, &tmp, NULL);
			for(int i=0; i<size*size; ++i)
			{
				TerrainTile& tt = tiles[i];
				tt.t2 = TT_ROAD;
				tt.alpha = 0;
				switch(old_tiles[i])
				{
				case OLD::TT_GRASS:
					tt.t = TT_GRASS;
					tt.mode = TM_NORMAL;
					break;
				case OLD::TT_GRASS2:
					tt.t = TT_GRASS2;
					tt.mode = TM_NORMAL;
					break;
				case OLD::TT_GRASS3:
					tt.t = TT_GRASS3;
					tt.mode = TM_NORMAL;
					break;
				case OLD::TT_SAND:
					tt.t = TT_SAND;
					tt.mode = TM_PATH;
					break;
				case OLD::TT_ROAD:
					tt.t = TT_ROAD;
					tt.mode = TM_ROAD;
					break;
				case OLD::TT_BUILD_GRASS:
					tt.t = TT_GRASS;
					tt.mode = TM_BUILDING_BLOCK;
					break;
				case OLD::TT_BUILD_SAND:
					tt.t = TT_SAND;
					tt.mode = TM_BUILDING_BLOCK;
					break;
				case OLD::TT_BUILD_ROAD:
					tt.t = TT_ROAD;
					tt.mode = TM_BUILDING_BLOCK;
					break;
				}
			}
			delete[] old_tiles;
		}
		ReadFile(file, h, sizeof(float)*size2, &tmp, NULL);

		// konwersja �awy w obr�con� �aw� i ustawienie wariantu
		if(LOAD_VERSION < V_0_2_20)
		{
			for(vector<Useable*>::iterator it = useables.begin(), end = useables.end(); it != end; ++it)
			{
				Useable& u = **it;
				if(u.type == U_LAWA)
				{
					if(type == L_CITY || type == L_VILLAGE)
					{
						u.type = U_LAWA_DIR;
						u.variant = 0;
					}
					else
						u.variant = rand2()%2+2;
				}
			}
		}
	}
}

//=================================================================================================
void OutsideLocation::BuildRefidTable()
{
	for(vector<Unit*>::iterator it = units.begin(), end = units.end(); it != end; ++it)
	{
		(*it)->refid = (int)Unit::refid_table.size();
		Unit::refid_table.push_back(*it);
	}

	for(vector<Useable*>::iterator it = useables.begin(), end = useables.end(); it != end; ++it)
	{
		(*it)->refid = (int)Useable::refid_table.size();
		Useable::refid_table.push_back(*it);
	}
}