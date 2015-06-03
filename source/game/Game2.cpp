#include "Pch.h"
#include "Base.h"
#include "Game.h"
#include "ParticleSystem.h"
#include "Terrain.h"
#include "ItemScript.h"
#include "RoomType.h"
#include "EnemyGroup.h"
#include "SaveState.h"

const int SAVE_VERSION = V_CURRENT;
int LOAD_VERSION;
const INT2 SUPPORT_LOAD_VERSION(0, V_CURRENT);

const VEC2 alert_range(20.f,30.f);
const float pickup_range = 2.f;
const float ARROW_SPEED = 45.f;
const float ARROW_TIMER = 5.f;
const float MIN_H = 1.5f;

const float SS = 0.25f;//0.25f/8;
const int NN = 64;

TEX icon[7];
extern cstring g_ctime;
MATRIX m1, m2, m3, m4;
UINT passes;

//VEC3 t_v1, t_v2;

// sta�e do trenowania
const float C_TRAIN_GIVE_DMG = 1000.f;
const float C_TRAIN_NO_DMG = 20.f;
const float C_TRAIN_KILL_RATIO = 0.1f;
const float C_TRAIN_ARMOR_HURT = 2000.f;
const float C_TRAIN_BLOCK = 500.f;
const float C_TRAIN_SHOT_BLOCK = 100.f;
const float C_TRAIN_SHOT = 1000.f;
const float C_TRAIN_SHOT_MISSED = 10.f;

//=================================================================================================
// Przerywa akcj� postaci
//=================================================================================================
void Game::BreakAction(Unit& u, bool fall)
{
	switch(u.action)
	{
	case A_POSITION:
		return;
	case A_SHOOT:
		if(u.bow_instance)
		{
			bow_instances.push_back(u.bow_instance);
			u.bow_instance = NULL;
		}
		u.action = A_NONE;
		break;
	case A_DRINK:
		if(u.etap_animacji == 0)
		{
			AddItem(u, u.used_item, 1, u.used_item_is_team);
			if(!fall)
				u.used_item = NULL;
		}
		else
			u.used_item = NULL;
		u.ani->Deactivate(1);
		u.action = A_NONE;
		break;
	case A_EAT:
		if(u.etap_animacji < 2)
		{
			AddItem(u, u.used_item, 1, u.used_item_is_team);
			if(!fall)
				u.used_item = NULL;
		}
		else
			u.used_item = NULL;
		u.ani->Deactivate(1);
		u.action = A_NONE;
		break;
	case A_TAKE_WEAPON:
		if(u.stan_broni == BRON_CHOWA)
		{
			if(u.etap_animacji == 0)
			{
				u.stan_broni = BRON_WYJETA;
				u.wyjeta = u.chowana;
				u.chowana = B_BRAK;
			}
			else
			{
				u.stan_broni = BRON_SCHOWANA;
				u.wyjeta = u.chowana = B_BRAK;
			}
		}
		else
		{
			if(u.etap_animacji == 0)
			{
				u.stan_broni = BRON_SCHOWANA;
				u.wyjeta = B_BRAK;
			}
			else
				u.stan_broni = BRON_WYJETA;
		}
		u.action = A_NONE;
		break;
	case A_BLOCK:
		u.ani->Deactivate(1);
		u.action = A_NONE;
		break;
	}

	u.ani->frame_end_info = false;
	u.ani->frame_end_info2 = false;
	u.atak_w_biegu = false;

	if(u.IsPlayer())
	{
		u.player->po_akcja = PO_BRAK;
		if(u.player == pc)
			Inventory::lock_id = LOCK_NO;
	}
	else
		u.ai->potion = -1;

	if(&u == pc->unit && inventory_mode > I_INVENTORY)
		CloseInventory();
}

void Game::BreakAction2(Unit& u, bool fall)
{
	if(u.action == A_POSITION)
		return;

	switch(u.action)
	{
	case A_POSITION:
		return;
	case A_SHOOT:
		if(u.bow_instance)
		{
			bow_instances.push_back(u.bow_instance);
			u.bow_instance = NULL;
		}
		u.action = A_NONE;
		break;
	case A_DRINK:
		if(!fall && u.etap_animacji == 0)
			u.used_item = NULL;
		u.ani->Deactivate(1);
		u.action = A_NONE;
		break;
	case A_EAT:
		if(!fall && u.etap_animacji < 2)
			u.used_item = NULL;
		u.ani->Deactivate(1);
		u.action = A_NONE;
		break;
	case A_TAKE_WEAPON:
		if(u.stan_broni == BRON_CHOWA)
		{
			if(u.etap_animacji == 0)
			{
				u.stan_broni = BRON_WYJETA;
				u.wyjeta = u.chowana;
				u.chowana = B_BRAK;
			}
			else
			{
				u.stan_broni = BRON_SCHOWANA;
				u.wyjeta = u.chowana = B_BRAK;
			}
		}
		else
		{
			if(u.etap_animacji == 0)
			{
				u.stan_broni = BRON_SCHOWANA;
				u.wyjeta = B_BRAK;
			}
			else
				u.stan_broni = BRON_WYJETA;
		}
		u.action = A_NONE;
		break;
	case A_BLOCK:
		u.ani->Deactivate(1);
		u.action = A_NONE;
		break;
	}

	if(u.useable)
	{
		if(fall)
			u.useable->user = NULL;
		u.target_pos2 = u.target_pos = u.pos;
		const Item* prev_used_item = u.used_item;
		Unit_StopUsingUseable(GetContext(u), u, !fall);
		if(prev_used_item && u.slots[SLOT_WEAPON] == prev_used_item && !u.HaveShield())
		{
			u.stan_broni = BRON_WYJETA;
			u.wyjeta = B_JEDNORECZNA;
			u.chowana = B_BRAK;
		}
		else if(fall)
			u.used_item = prev_used_item;
		if(&u == pc->unit)
		{
			u.action = A_POSITION;
			u.etap_animacji = 0;
		}
	}
	else
		u.action = A_NONE;

	u.ani->frame_end_info = false;
	u.ani->frame_end_info2 = false;
	u.atak_w_biegu = false;

	if(&u == pc->unit && inventory_mode > I_INVENTORY)
		CloseInventory();
}

//=================================================================================================
// Rysowanie
//=================================================================================================
void Game::Draw()
{
	PROFILER_BLOCK("Draw");

	LevelContext& ctx = GetContext(*pc->unit);
	bool outside;
	if(ctx.type == LevelContext::Outside)
		outside = true;
	else if(ctx.type == LevelContext::Inside)
		outside = false;
	else if(city_ctx->inside_buildings[ctx.building_id]->top > 0.f)
		outside = false;
	else
		outside = true;

	ListDrawObjects(ctx, camera_frustum, outside);
	DrawScene(outside);

	// rysowanie local pathfind map
#ifdef DRAW_LOCAL_PATH
	V( eMesh->SetTechnique(techMeshSimple2) );
	V( eMesh->Begin(&passes, 0) );
	V( eMesh->BeginPass(0) );

	V( eMesh->SetMatrix(hMeshCombined, &tmp_matViewProj) );
	V( eMesh->CommitChanges() );

	SetAlphaBlend(true);
	SetAlphaTest(false);
	SetNoZWrite(false);

	for(vector<std::pair<VEC2, int> >::iterator it = test_pf.begin(), end = test_pf.end(); it != end; ++it)
	{
		VEC3 v[4] = {
			VEC3(it->first.x, 0.1f, it->first.y+SS),
			VEC3(it->first.x+SS, 0.1f, it->first.y+SS),
			VEC3(it->first.x, 0.1f, it->first.y),
			VEC3(it->first.x+SS, 0.1f, it->first.y)
		};

		if(test_pf_outside)
		{
			float h = terrain->GetH(v[0].x, v[0].z) + 0.1f;
			for(int i=0; i<4; ++i)
				v[i].y = h;
		}

		VEC4 color;
		switch(it->second)
		{
		case 0:
			color = VEC4(0,1,0,0.5f);
			break;
		case 1:
			color = VEC4(1,0,0,0.5f);
			break;
		case 2:
			color = VEC4(0,0,0,0.5f);
			break;
		}

		V( eMesh->SetVector(hMeshTint, &color) );
		V( eMesh->CommitChanges() );

		device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, v, sizeof(VEC3));
	}

	V( eMesh->EndPass()	);
	V( eMesh->End() );
#endif
}

//=================================================================================================
// Generuje obrazek przedmiotu
//=================================================================================================
void Game::GenerateImage(Item* item)
{
	assert(item && item->ani);

	SetAlphaBlend(false);
	SetAlphaTest(false);
	SetNoCulling(false);
	SetNoZWrite(false);

	// ustaw render target
	SURFACE surf = NULL;
	if(sItemRegion)
		V( device->SetRenderTarget(0, sItemRegion) );
	else
	{
		V( tItemRegion->GetSurfaceLevel(0, &surf) );
		V( device->SetRenderTarget(0, surf) );
	}

	// pocz�tek renderowania
	V( device->Clear(0, NULL, D3DCLEAR_ZBUFFER | D3DCLEAR_TARGET, 0, 1.f, 0) );
	V( device->BeginScene() );

	const Animesh& a = *item->ani;

	MATRIX matWorld, matView, matProj;
	D3DXMatrixIdentity(&matWorld);
	D3DXMatrixLookAtLH(&matView, &a.cam_pos, &a.cam_target, &a.cam_up);
	D3DXMatrixPerspectiveFovLH(&matProj, PI/4, 1.f, 0.1f, 25.f);

	LightData ld;
	ld.pos = a.cam_pos;
	ld.color = VEC3(1,1,1);
	ld.range = 10.f;

	V( eMesh->SetTechnique(techMesh) );
	V( eMesh->SetMatrix(hMeshCombined, &(matView*matProj)) );
	V( eMesh->SetMatrix(hMeshWorld, &matWorld) );
	V( eMesh->SetVector(hMeshFogColor, &VEC4(1,1,1,1)) );
	V( eMesh->SetVector(hMeshFogParam, &VEC4(25.f,50.f,25.f,0)) );
	V( eMesh->SetVector(hMeshAmbientColor, &VEC4(0.5f,0.5f,0.5f,1)));
	V( eMesh->SetRawValue(hMeshLights, &ld, 0, sizeof(LightData)) );
	V( eMesh->SetVector(hMeshTint, &VEC4(1,1,1,1)));

	V( device->SetVertexDeclaration(vertex_decl[a.vertex_decl]) );
	V( device->SetStreamSource(0, a.vb, 0, a.vertex_size) );
	V( device->SetIndices(a.ib) );

	UINT passes;
	V( eMesh->Begin(&passes, 0) );
	V( eMesh->BeginPass(0) );

	for(int i=0; i<a.head.n_subs; ++i)
	{
		const Animesh::Submesh& sub = a.subs[i];
		V( eMesh->SetTexture(hMeshTex, a.GetTexture(i)) );
		V( eMesh->CommitChanges() );
		V( device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, sub.min_ind, sub.n_ind, sub.first*3, sub.tris) );
	}

	V( eMesh->EndPass() );
	V( eMesh->End() );

	// koniec renderowania
	V( device->EndScene() );

	// kopiuj do tekstury
	if(sItemRegion)
	{
		V( tItemRegion->GetSurfaceLevel(0, &surf) );
		V( device->StretchRect(sItemRegion, NULL, surf, NULL, D3DTEXF_NONE) );
	}

	// stw�rz now� tekstur� i skopuj obrazek do niej
	TEX t;
	SURFACE out_surface;
	V( device->CreateTexture(64, 64, 0, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &t, NULL) );
	V( t->GetSurfaceLevel(0, &out_surface) );
	V( D3DXLoadSurfaceFromSurface(out_surface, NULL, NULL, surf, NULL, NULL, D3DX_DEFAULT, 0) );
	surf->Release();
	out_surface->Release();

	// przywr�� stary render target
	V( device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &surf) );
	V( device->SetRenderTarget(0, surf) );
	surf->Release();

	item->tex = t;
}

//=================================================================================================
// Zwraca jednostk� za kt�r� pod��a kamera
//=================================================================================================
Unit* Game::GetFollowTarget()
{
	return pc->unit;
}

//=================================================================================================
// Ustawia kamer�
//=================================================================================================
void Game::SetupCamera(float dt)
{
	Unit* target = GetFollowTarget();
	LevelContext& ctx = GetContext(*target);

	MATRIX mat, matProj, matView;
	const VEC3 cam_h(0, target->GetUnitHeight()+0.2f, 0);
	VEC3 dist(0,-cam_dist,0);
// 	target->ani->need_update = true;
// 	target->ani->SetupBones(&target->human_data->mat_scale[0]);
// 	const VEC3 cam_h = target->GetEyePos() - target->pos;
// 	VEC3 dist(0,-0.05f/*cam_dist*/,0);

	D3DXMatrixRotationYawPitchRoll(&mat, target->rot, rotY, 0);
	D3DXVec3TransformCoord(&dist, &dist, &mat);

	// !!! to => from !!!
	// kamera idzie od g�owy do ty�u
	VEC3 to = target->pos + cam_h;
	float tout, min_tout=2.f;

	int tx = int(target->pos.x/2),
		tz = int(target->pos.z/2);

	/*t_v1 = to;
	tmp_pos = to + VEC3(0,10,0);
	//VEC3 dirr = VEC3(sin(target->rot+PI)*10,-5,cos(target->rot+PI)*10);
	VEC3 dirr(0,-20,0);
	VEC3 tmp_pos2 = tmp_pos + dirr;
	t_v2 = tmp_pos2;
	float te = terrain->Raytest(tmp_pos, tmp_pos2);
	if(te < 0.f || Key.Down('X'))
		te = 1.f;
	tmp_pos = tmp_pos + dirr * te;*/

	if(ctx.type == LevelContext::Outside)
	{
		OutsideLocation* outside = (OutsideLocation*)location;

		// teren
		tout = terrain->Raytest(to, to+dist);
		if(tout < min_tout && tout > 0.f)
			min_tout = tout;

		// budynki
		const uint _s = 16 * 8;
		int minx = max(0, tx-3),
			minz = max(0, tz-3),
			maxx = min((int)_s-1, tx+3),
			maxz = min((int)_s-1, tz+3);

		for(int z=minz; z<=maxz; ++z)
		{
			for(int x=minx; x<=maxx; ++x)
			{
				if(outside->tiles[x+z*_s].mode >= TM_BUILDING_BLOCK)
				{
					const BOX box(float(x)*2, 0, float(z)*2, float(x+1)*2, 8.f, float(z+1)*2);
					if(RayToBox(to, dist, box, &tout) && tout < min_tout && tout > 0.f)
						min_tout = tout;
				}
			}
		}

		// kolizje z obiektami
		for(vector<CollisionObject>::iterator it = ctx.colliders->begin(), end = ctx.colliders->end(); it != end; ++it)
		{
			if(it->ptr != CAM_COLLIDER)
				continue;

			if(it->type == CollisionObject::SPHERE)
			{
				if(RayToCylinder(to, to+dist, VEC3(it->pt.x,0,it->pt.y), VEC3(it->pt.x,32.f,it->pt.y), it->radius, tout) && tout < min_tout && tout > 0.f)
					min_tout = tout;
			}
			else if(it->type == CollisionObject::RECTANGLE)
			{
				BOX box(it->pt.x-it->w, 0.f, it->pt.y-it->h, it->pt.x+it->w, 32.f, it->pt.y+it->h);
				if(RayToBox(to, dist, box, &tout) && tout < min_tout && tout > 0.f)
					min_tout = tout;
			}
			else
			{
				float w, h;
				if(equal(it->rot, PI/2) || equal(it->rot, PI*3/2))
				{
					w = it->h;
					h = it->w;
				}
				else
				{
					w = it->w;
					h = it->h;
				}

				BOX box(it->pt.x-w, 0.f, it->pt.y-h, it->pt.x+w, 32.f, it->pt.y+h);
				if(RayToBox(to, dist, box, &tout) && tout < min_tout && tout > 0.f)
					min_tout = tout;
			}
		}

		for(vector<CameraCollider>::iterator it = cam_colliders.begin(), end = cam_colliders.end(); it != end; ++it)
		{
			if(RayToBox(to, dist, it->box, &tout) && tout < min_tout && tout > 0.f)
				min_tout = tout;
		}
	}
	else if(ctx.type == LevelContext::Inside)
	{
		InsideLocation* inside = (InsideLocation*)location;
		InsideLocationLevel& lvl = inside->GetLevelData();

		int minx = max(0, tx-3),
			minz = max(0, tz-3),
			maxx = min(lvl.w-1, tx+3),
			maxz = min(lvl.h-1, tz+3);

		// sufit
		const D3DXPLANE sufit(0,-1,0,4);
		if(RayToPlane(to, dist, sufit, &tout) && tout < min_tout && tout > 0.f)
		{
			//tmpvar2 = 1;
			min_tout = tout;
		}

		// pod�oga
		const D3DXPLANE podloga(0,1,0,0);
		if(RayToPlane(to, dist, podloga, &tout) && tout < min_tout && tout > 0.f)
			min_tout = tout;

		// podziemia
		for(int z=minz; z<=maxz; ++z)
		{
			for(int x=minx; x<=maxx; ++x)
			{
				Pole& p = lvl.mapa[x+z*lvl.w];
				if(czy_blokuje2(p.co))
				{
					const BOX box(float(x)*2, 0, float(z)*2, float(x+1)*2, 4.f, float(z+1)*2);
					if(RayToBox(to, dist, box, &tout) && tout < min_tout && tout > 0.f)
						min_tout = tout;
				}
				else if(IS_SET(p.flagi, Pole::F_NISKI_SUFIT))
				{
					const BOX box(float(x)*2, 3.f, float(z)*2, float(x+1)*2, 4.f, float(z+1)*2);
					if(RayToBox(to, dist, box, &tout) && tout < min_tout && tout > 0.f)
						min_tout = tout;
				}
				if(p.co == SCHODY_GORA)
				{
					if(RayToMesh(to, dist, pt_to_pos(lvl.schody_gora), dir_to_rot(lvl.schody_gora_dir), vdSchodyGora, tout) && tout < min_tout)
						min_tout = tout;
				}
				else if(p.co == SCHODY_DOL)
				{
					if(!lvl.schody_dol_w_scianie && RayToMesh(to, dist, pt_to_pos(lvl.schody_dol), dir_to_rot(lvl.schody_dol_dir), vdSchodyDol, tout) && tout < min_tout)
						min_tout = tout;
				}
				else if(p.co == DRZWI || p.co == OTWOR_NA_DRZWI)
				{
					VEC3 pos(float(x*2)+1,0,float(z*2)+1);
					float rot;

					if(czy_blokuje2(lvl.mapa[x-1+z*lvl.w].co))
					{
						rot = 0;
						int mov = 0;
						if(lvl.pokoje[lvl.mapa[x+(z-1)*lvl.w].pokoj].korytarz)
							++mov;
						if(lvl.pokoje[lvl.mapa[x+(z+1)*lvl.w].pokoj].korytarz)
							--mov;
						if(mov == 1)
							pos.z += 0.8229f;
						else if(mov == -1)
							pos.z -= 0.8229f;
					}
					else
					{
						rot = PI/2;
						int mov = 0;
						if(lvl.pokoje[lvl.mapa[x-1+z*lvl.w].pokoj].korytarz)
							++mov;
						if(lvl.pokoje[lvl.mapa[x+1+z*lvl.w].pokoj].korytarz)
							--mov;
						if(mov == 1)
							pos.x += 0.8229f;
						else if(mov == -1)
							pos.x -= 0.8229f;
					}

					if(RayToMesh(to, dist, pos, rot, vdNaDrzwi, tout) && tout < min_tout)
						min_tout = tout;

					Door* door = FindDoor(ctx, INT2(x, z));
					if(door && door->IsBlocking())
					{
						// 0.842f, 1.319f, 0.181f
						BOX box(pos.x, 0.f, pos.z);
						box.v2.y = 1.319f*2;
						if(rot == 0.f)
						{
							box.v1.x -= 0.842f;
							box.v2.x += 0.842f;
							box.v1.z -= 0.181f;
							box.v2.z += 0.181f;
						}
						else
						{
							box.v1.z -= 0.842f;
							box.v2.z += 0.842f;
							box.v1.x -= 0.181f;
							box.v2.x += 0.181f;
						}
						
						if(RayToBox(to, dist, box, &tout) && tout < min_tout && tout > 0.f)
							min_tout = tout;
					}
				}
			}
		}
	}
	else
	{
		InsideBuilding& building = *city_ctx->inside_buildings[ctx.building_id];

		// budynek

		// pod�oga
		const D3DXPLANE podloga(0,1,0,0);
		if(RayToPlane(to, dist, podloga, &tout) && tout < min_tout && tout > 0.f)
			min_tout = tout;

		// sufit
		if(building.top > 0.f)
		{
			const D3DXPLANE sufit(0,-1,0,4);
			if(RayToPlane(to, dist, sufit, &tout) && tout < min_tout && tout > 0.f)
				min_tout = tout;
		}

		// xsphere
		if(building.xsphere_radius > 0.f)
		{
			VEC3 from = to + dist;
			if(RayToSphere(from, -dist, building.xsphere_pos, building.xsphere_radius, tout) && tout > 0.f)
			{
				tout = 1.f - tout;
				if(tout < min_tout)
					min_tout = tout;
			}
		}

		// kolizje z obiektami
		for(vector<CollisionObject>::iterator it = ctx.colliders->begin(), end = ctx.colliders->end(); it != end; ++it)
		{
			if(it->ptr != CAM_COLLIDER || it->type != CollisionObject::RECTANGLE)
				continue;

			BOX box(it->pt.x-it->w, 0.f, it->pt.y-it->h, it->pt.x+it->w, 10.f, it->pt.y+it->h);
			if(RayToBox(to, dist, box, &tout) && tout < min_tout && tout > 0.f)
				min_tout = tout;
		}

		// kolizje z drzwiami
		for(vector<Door*>::iterator it = ctx.doors->begin(), end = ctx.doors->end(); it != end; ++it)
		{
			Door& door = **it;
			if(door.IsBlocking())
			{
				BOX box(door.pos);
				box.v2.y = 1.319f*2;
				if(door.rot == 0.f)
				{
					box.v1.x -= 0.842f;
					box.v2.x += 0.842f;
					box.v1.z -= 0.181f;
					box.v2.z += 0.181f;
				}
				else
				{
					box.v1.z -= 0.842f;
					box.v2.z += 0.842f;
					box.v1.x -= 0.181f;
					box.v2.x += 0.181f;
				}

				if(RayToBox(to, dist, box, &tout) && tout < min_tout && tout > 0.f)
					min_tout = tout;
			}

			if(RayToMesh(to, dist, door.pos, door.rot, vdNaDrzwi, tout) && tout < min_tout)
				min_tout = tout;
		}
	}
	
	// uwzgl�dnienie znear
	if(min_tout > 1.f || pc->noclip)
		min_tout = 1.f;
	else
	{
		min_tout -= 0.1f;
		if(min_tout < 0.1f)
			min_tout = 0.1f;
	}

	VEC3 from = to + dist*min_tout;
	VEC3 ffrom, fto;

	if(distance(from, prev_from) > 10.f || first_frame)
	{
		ffrom = prev_from = from;
		fto = prev_to = to;
	}
	else
	{
		ffrom = prev_from += (from - prev_from)*dt*10;
		fto = prev_to += (to - prev_to)*dt*10;
	}

	float drunk = pc->unit->alcohol/pc->unit->hpmax;
	float drunk_mod = (drunk > 0.1f ? (drunk-0.1f)/0.9f : 0.f);

	D3DXMatrixLookAtLH(&matView, &ffrom, &fto, &VEC3(0,1,0));
	D3DXMatrixPerspectiveFovLH(&matProj, PI/4+sin(drunk_anim)*(PI/16)*drunk_mod, float(wnd_size.x)/wnd_size.y*(1.f+sin(drunk_anim)/10*drunk_mod), 0.1f, draw_range);
	tmp_matViewProj = matView * matProj;
	D3DXMatrixInverse(&tmp_matViewInv, NULL, &matView);

	MATRIX matProj2;
	D3DXMatrixPerspectiveFovLH(&matProj2, PI/4+sin(drunk_anim)*(PI/16)*drunk_mod, float(wnd_size.x)/wnd_size.y*(1.f+sin(drunk_anim)/10*drunk_mod), 0.1f, grass_range);

	camera_center = ffrom;

	camera_frustum.Set(tmp_matViewProj);
	camera_frustum2.Set(matView * matProj2);

	// centrum d�wi�ku 3d
	VEC3 listener_pos = target->GetHeadSoundPos();
	fmod_system->set3DListenerAttributes(0, (const FMOD_VECTOR*)&listener_pos, NULL, (const FMOD_VECTOR*)&VEC3(sin(target->rot+PI),0,cos(target->rot+PI)), (const FMOD_VECTOR*)&VEC3(0,1,0));
}

//=================================================================================================
// Ustawia parametry shader�w
//=================================================================================================
void Game::SetupShaders()
{
	techMesh = eMesh->GetTechniqueByName("mesh");
	techMeshDir = eMesh->GetTechniqueByName("mesh_dir");
	techMeshSimple = eMesh->GetTechniqueByName("mesh_simple");
	techMeshSimple2 = eMesh->GetTechniqueByName("mesh_simple2");
	techMeshExplo = eMesh->GetTechniqueByName("mesh_explo");
	techParticle = eParticle->GetTechniqueByName("particle");
	techTrail = eParticle->GetTechniqueByName("trail");
	techSkybox = eSkybox->GetTechniqueByName("skybox");
	techTerrain = eTerrain->GetTechniqueByName("terrain");
	techArea = eArea->GetTechniqueByName("area");
	techGui = eGui->GetTechniqueByName("gui");
	techGlowMesh = eGlow->GetTechniqueByName("mesh");
	techGlowAni = eGlow->GetTechniqueByName("ani");
	techGrass = eGrass->GetTechniqueByName("grass");
	assert(techAnim && techHair && techAnimDir && techHairDir && techMesh && techMeshDir && techMeshSimple && techMeshSimple2 && techMeshExplo && techParticle && techTrail && techSkybox &&
		techTerrain && techArea && techGui && techGlowMesh && techGlowAni && techGrass);

	hMeshCombined = eMesh->GetParameterByName(NULL, "matCombined");
	hMeshWorld = eMesh->GetParameterByName(NULL, "matWorld");
	hMeshTex = eMesh->GetParameterByName(NULL, "texDiffuse");
	hMeshFogColor = eMesh->GetParameterByName(NULL, "fogColor");
	hMeshFogParam = eMesh->GetParameterByName(NULL, "fogParam");
	hMeshTint = eMesh->GetParameterByName(NULL, "tint");
	hMeshAmbientColor = eMesh->GetParameterByName(NULL, "ambientColor");
	hMeshLightDir = eMesh->GetParameterByName(NULL, "lightDir");
	hMeshLightColor = eMesh->GetParameterByName(NULL, "lightColor");
	hMeshLights = eMesh->GetParameterByName(NULL, "lights");
	assert(hMeshCombined && hMeshWorld && hMeshTex && hMeshFogColor && hMeshFogParam && hMeshTint && hMeshAmbientColor && hMeshLightDir && hMeshLightColor && hMeshLights);

	hParticleCombined = eParticle->GetParameterByName(NULL, "matCombined");
	hParticleTex = eParticle->GetParameterByName(NULL, "tex0");
	assert(hParticleCombined && hParticleTex);
	
	hSkyboxCombined = eSkybox->GetParameterByName(NULL, "matCombined");
	hSkyboxTex = eSkybox->GetParameterByName(NULL, "tex0");
	assert(hSkyboxCombined && hSkyboxTex);

	hTerrainCombined = eTerrain->GetParameterByName(NULL, "matCombined");
	hTerrainWorld = eTerrain->GetParameterByName(NULL, "matWorld");
	hTerrainTexBlend = eTerrain->GetParameterByName(NULL, "texBlend");
	hTerrainTex[0] = eTerrain->GetParameterByName(NULL, "tex0");
	hTerrainTex[1] = eTerrain->GetParameterByName(NULL, "tex1");
	hTerrainTex[2] = eTerrain->GetParameterByName(NULL, "tex2");
	hTerrainTex[3] = eTerrain->GetParameterByName(NULL, "tex3");
	hTerrainTex[4] = eTerrain->GetParameterByName(NULL, "tex4");
	hTerrainColorAmbient = eTerrain->GetParameterByName(NULL, "colorAmbient");
	hTerrainColorDiffuse = eTerrain->GetParameterByName(NULL, "colorDiffuse");
	hTerrainLightDir = eTerrain->GetParameterByName(NULL, "lightDir");
	hTerrainFogColor = eTerrain->GetParameterByName(NULL, "fogColor");
	hTerrainFogParam = eTerrain->GetParameterByName(NULL, "fogParam");
	assert(hTerrainCombined && hTerrainWorld && hTerrainTexBlend && hTerrainTex[0] && hTerrainTex[1] && hTerrainTex[2] && hTerrainTex[3] && hTerrainTex[4] &&
		hTerrainColorAmbient && hTerrainColorDiffuse && hTerrainLightDir && hTerrainFogColor && hTerrainFogParam);

	hAreaCombined = eArea->GetParameterByName(NULL, "matCombined");
	hAreaColor = eArea->GetParameterByName(NULL, "color");
	hAreaPlayerPos = eArea->GetParameterByName(NULL, "playerPos");
	hAreaRange = eArea->GetParameterByName(NULL, "range");
	assert(hAreaCombined && hAreaColor && hAreaPlayerPos && hAreaRange);

	hPostTex = ePostFx->GetParameterByName(NULL, "t0");
	hPostPower = ePostFx->GetParameterByName(NULL, "power");
	hPostSkill = ePostFx->GetParameterByName(NULL, "skill");
	assert(hPostTex && hPostPower && hPostSkill);

	hGlowCombined = eGlow->GetParameterByName(NULL, "matCombined");
	hGlowBones = eGlow->GetParameterByName(NULL, "matBones");
	hGlowColor = eGlow->GetParameterByName(NULL, "color");
	assert(hGlowCombined && hGlowBones && hGlowColor);

	hGrassViewProj = eGrass->GetParameterByName(NULL, "matViewProj");
	hGrassTex = eGrass->GetParameterByName(NULL, "texDiffuse");
	hGrassFogColor = eGrass->GetParameterByName(NULL, "fogColor");
	hGrassFogParams = eGrass->GetParameterByName(NULL, "fogParam");
	hGrassAmbientColor = eGrass->GetParameterByName(NULL, "ambientColor");
}

const INT2 g_kierunek2[4] = {
	INT2(0,-1),
	INT2(-1,0),
	INT2(0,1),
	INT2(1,0)
};

//=================================================================================================
// Aktualizuje gr�
//=================================================================================================
void Game::UpdateGame(float dt)
{
	dt *= speed;
	if(dt == 0)
		return;

	PROFILER_BLOCK("UpdateGame");

	// sanity checks
#ifdef IS_DEV
	if(sv_server && !sv_online)
	{
		AddGameMsg("sv_server was true!", 5.f);
		sv_server = false;
	}
#endif

	bool getting_out = false;

	/*Object* o = NULL;
	float dist = 999.f;
	for(vector<Object>::iterator it = local_ctx.objects->begin(), end = local_ctx.objects->end(); it != end; ++it)
	{
		float d = distance(it->pos, pc->unit->pos);
		if(d < dist)
		{
			dist = d;
			o = &*it;
		}
	}
	if(o)
	{
		if(Key.Down('9'))
			o->rot.y = clip(o->rot.y+dt);
		if(Key.Down('0'))
			o->rot.y = 0;
	}*/

	minimap_opened_doors = false;

	if(in_tutorial && !IsOnline())
		UpdateTutorial();

	drunk_anim = clip(drunk_anim+dt);
	UpdatePostEffects(dt);

	portal_anim += dt;
	if(portal_anim >= 1.f)
		portal_anim -= 1.f;
	light_angle = clip(light_angle+dt/100);

	LevelContext& player_ctx = (pc->unit->in_building == -1 ? local_ctx : city_ctx->inside_buildings[pc->unit->in_building]->ctx);

	// fallback
	if(fallback_co != -1)
	{
		if(fallback_t <= 0.f)
		{
			fallback_t += dt*2;

			if(fallback_t > 0.f)
			{
				switch(fallback_co)
				{
				case FALLBACK_TRAIN:
					if(IsLocal())
					{
						if(fallback_1 == 2)
							TournamentTrain(*pc->unit);
						else
							Train(*pc->unit, fallback_1 == 1, fallback_2);
						pc->Rest(10, false);
						if(IsOnline())
							UseDays(pc, 10);
						else
							WorldProgress(10, WPM_NORMAL);
					}
					else
					{
						fallback_co = FALLBACK_CLIENT;
						fallback_t = 0.f;
						NetChange& c = Add1(net_changes);
						c.type = NetChange::TRAIN;
						c.id = fallback_1;
						c.ile = fallback_2;
					}
					break;
				case FALLBACK_REST:
					if(IsLocal())
					{
						pc->Rest(fallback_1, true);
						if(IsOnline())
							UseDays(pc, fallback_1);
						else
							WorldProgress(fallback_1, WPM_NORMAL);
					}
					else
					{
						fallback_co = FALLBACK_CLIENT;
						fallback_t = 0.f;
						NetChange& c = Add1(net_changes);
						c.type = NetChange::REST;
						c.id = fallback_1;
					}
					break;
				case FALLBACK_ENTER:
					// wej�cie/wyj�cie z budynku
					{
						UnitWarpData& uwd = Add1(unit_warp_data);
						uwd.unit = pc->unit;
						uwd.where = fallback_1;
					}
					break;
				case FALLBACK_EXIT:
					ExitToMap();
					getting_out = true;
					break;
				case FALLBACK_CHANGE_LEVEL:
					ChangeLevel(fallback_1);
					getting_out = true;
					break;
				case FALLBACK_USE_PORTAL:
					{
						Portal* portal = location->GetPortal(fallback_1);
						Location* target_loc = locations[portal->target_loc];
						int at_level = 0;
						// aktualnie mo�na si� tepn�� z X poziomu na 1 zawsze ale �eby z X na X to musi by� odwiedzony
						// np w sekrecie z 3 na 1 i spowrotem do
						if(target_loc->portal)
							at_level = target_loc->portal->at_level;
						LeaveLocation();
						current_location = portal->target_loc;
						EnterLocation(at_level, portal->target);
					}
					break;
				case FALLBACK_NONE:
				case FALLBACK_ARENA2:
				case FALLBACK_CLIENT2:
					break;
				case FALLBACK_ARENA:
				case FALLBACK_ARENA_EXIT:
				case FALLBACK_WAIT_FOR_WARP:
				case FALLBACK_CLIENT:
					fallback_t = 0.f;
					break;
				default:
					assert(0);
					break;
				}
			}
		}
		else
		{
			fallback_t += dt*2;

			if(fallback_t >= 1.f)
			{
				if(IsLocal())
				{
					if(fallback_co != FALLBACK_ARENA2)
					{
						if(fallback_co == FALLBACK_CHANGE_LEVEL || fallback_co == FALLBACK_USE_PORTAL || fallback_co == FALLBACK_EXIT)
						{
							for(vector<Unit*>::iterator it = team.begin(), end = team.end(); it != end; ++it)
								(*it)->frozen = 0;
						}
						pc->unit->frozen = 0;
					}
				}
				else if(fallback_co == FALLBACK_CLIENT2)
					pc->unit->frozen = 0;
				fallback_co = -1;
			}
		}
	}

	if(IsLocal())
	{
		// aktualizuj arene/wymiane sprz�tu/zawody w piciu/questy
		UpdateGame2(dt);
	}

	// info o uczo�czeniu wszystkich unikalnych quest�w
	if(CanShowEndScreen())
	{
		if(IsLocal())
			unique_completed_show = true;
		else
			unique_completed_show = false;

		cstring text;

		if(IsOnline())
		{
			text = txWinMp;
			if(IsServer())
			{
				PushNetChange(NetChange::GAME_STATS);
				PushNetChange(NetChange::ALL_QUESTS_COMPLETED);
			}
		}
		else
			text = txWin;

		GUI.SimpleDialog(Format(text, pc->kills, total_kills-pc->kills), NULL);
	}

	// wyzeruj pobliskie jednostki
	for(vector<UnitView>::iterator it = unit_views.begin(), end = unit_views.end(); it != end; ++it)
		it->valid = false;

	// licznik otrzymanych obra�e�
	pc->last_dmg = 0.f;
	if(!IsOnline() || !IsClient())
		pc->last_dmg_poison = 0.f;

#ifdef _DEBUG
	if(AllowKeyboard())
	{
		if(!location->outside)
		{
			InsideLocation* inside = (InsideLocation*)location;
			InsideLocationLevel& lvl = inside->GetLevelData();

			if(Key.Pressed(VK_OEM_COMMA) && Key.Down(VK_SHIFT) && inside->HaveUpStairs())
			{
				if(!Key.Down(VK_CONTROL))
				{
					// teleportuj gracza do schod�w w g�r�
					if(IsLocal())
					{
						INT2 tile = lvl.GetUpStairsFrontTile();
						pc->unit->rot = dir_to_rot(lvl.schody_gora_dir);
						WarpUnit(*pc->unit, VEC3(2.f*tile.x+1.f, 0.f, 2.f*tile.y+1.f));
					}
					else
					{
						NetChange& c = Add1(net_changes);
						c.type = NetChange::CHEAT_WARP_TO_STAIRS;
						c.id = 0;
					}
				}
				else
				{
					// poziom w g�r�
					if(IsLocal())
					{
						ChangeLevel(-1);
						return;
					}
					else
					{
						NetChange& c = Add1(net_changes);
						c.type = NetChange::CHEAT_CHANGE_LEVEL;
						c.id = 0;
					}
				}
			}
			if(Key.Pressed(VK_OEM_PERIOD) && Key.Down(VK_SHIFT) && inside->HaveDownStairs())
			{
				if(!Key.Down(VK_CONTROL))
				{
					// teleportuj gracza do schod�w w d�
					if(IsLocal())
					{
						INT2 tile = lvl.GetDownStairsFrontTile();
						pc->unit->rot = dir_to_rot(lvl.schody_dol_dir);
						WarpUnit(*pc->unit, VEC3(2.f*tile.x+1.f, 0.f, 2.f*tile.y+1.f));
					}
					else
					{
						NetChange& c = Add1(net_changes);
						c.type = NetChange::CHEAT_WARP_TO_STAIRS;
						c.id = 1;
					}
				}
				else
				{
					// poziom w d�
					if(IsLocal())
					{
						ChangeLevel(+1);
						return;
					}
					else
					{
						NetChange& c = Add1(net_changes);
						c.type = NetChange::CHEAT_CHANGE_LEVEL;
						c.id = 1;
					}
				}
			}
		}
		else if(Key.Pressed(VK_OEM_COMMA) && Key.Down(VK_SHIFT) && Key.Down(VK_CONTROL))
		{
			if(IsLocal())
			{
				ExitToMap();
				return;
			}
			else
				PushNetChange(NetChange::CHEAT_GOTO_MAP);
		}
	}
#endif

	// obracanie kamery g�ra/d�
	if(!IsLocal() || IsAnyoneAlive())
	{
		if(dialog_context.dialog_mode && inventory_mode <= I_INVENTORY)
		{
			if(rotY > 4.2875104f)
			{
				rotY -= dt;
				if(rotY < 4.2875104f)
					rotY = 4.2875104f;
			}
			else if(rotY < 4.2875104f)
			{
				rotY += dt;
				if(rotY > 4.2875104f)
					rotY = 4.2875104f;
			}
		}
		else
		{
			if(allow_input == ALLOW_INPUT || allow_input == ALLOW_MOUSE)
			{
				const float c_cam_angle_min = PI+0.1f;
				const float c_cam_angle_max = PI*1.8f-0.1f;

				rotY += -float(mouse_dif.y)/400;
				if(rotY > c_cam_angle_max)
					rotY = c_cam_angle_max;
				if(rotY < c_cam_angle_min)
					rotY = c_cam_angle_min;
			}
		}
	}
	else
	{
		if(rotY > PI+0.1f)
		{
			rotY -= dt;
			if(rotY < PI+0.1f)
				rotY = PI+0.1f;
		}
		else if(rotY < PI+0.1f)
		{
			rotY += dt;
			if(rotY > PI+0.1f)
				rotY = PI+0.1f;
		}
	}

	// przybli�anie/oddalanie kamery
	if(AllowMouse())
	{
		if(!dialog_context.dialog_mode || !dialog_context.show_choices || !PointInRect(GUI.cursor_pos, game_gui->dialog_pos, game_gui->dialog_size))
		{
			cam_dist -= float(mouse_wheel) / WHEEL_DELTA;
			cam_dist = clamp(cam_dist, 0.5f, 5.f);
		}

		if(Key.PressedRelease(VK_MBUTTON))
			cam_dist = 3.5f;
	}

	// umieranie
	if((IsLocal() && !IsAnyoneAlive()) || death_screen != 0)
	{
		if(death_screen == 0)
		{
			LOG("Game over: all players died.");
			SetMusic(MUSIC_CRYPT);
			CloseAllPanels();
			++death_screen;
			death_fade = 0;
			death_solo = (team.size() == 1u);
			if(IsOnline())
			{
				PushNetChange(NetChange::GAME_STATS);
				PushNetChange(NetChange::GAME_OVER);
			}
		}
		else if(death_screen == 1 || death_screen == 2)
		{
			death_fade += dt/2;
			if(death_fade >= 1.f)
			{
				death_fade = 0;
				++death_screen;
			}
		}
		if(death_screen >= 2 && AllowKeyboard() && Key.Pressed2Release(VK_ESCAPE, VK_RETURN))
		{
			ExitToMenu();
			return;
		}
	}

	// aktualizuj gracza
	if(dialog_context.dialog_mode || pc->unit->look_target || inventory_mode > I_INVENTORY)
	{
		VEC3 pos;
		if(pc->unit->look_target)
		{
			pos = pc->unit->look_target->pos;
			pc->unit->animacja = ANI_STOI;
		}
		else if(inventory_mode == I_LOOT_CHEST)
		{
			assert(pc->action == PlayerController::Action_LootChest);
			pos = pc->action_chest->pos;
			pc->unit->animacja = ANI_KLEKA;
		}
		else if(inventory_mode == I_LOOT_BODY)
		{
			assert(pc->action == PlayerController::Action_LootUnit);
			pos = pc->action_unit->GetLootCenter();
			pc->unit->animacja = ANI_KLEKA;
		}
		else if(dialog_context.dialog_mode)
		{
			pos = dialog_context.talker->pos;
			pc->unit->animacja = ANI_STOI;
		}
		else
		{
			assert(pc->action == InventoryModeToActionRequired(inventory_mode));
			pos = pc->action_unit->pos;
			pc->unit->animacja = ANI_STOI;
		}

		float dir = lookat_angle(pc->unit->pos, pos);
		
		if(!equal(pc->unit->rot, dir))
		{
			const float rot_speed = 3.f*dt;
			const float rot_diff = angle_dif(pc->unit->rot, dir);
			if(shortestArc(pc->unit->rot, dir) > 0.f)
				pc->unit->animacja = ANI_W_PRAWO;
			else
				pc->unit->animacja = ANI_W_LEWO;
			if(rot_diff < rot_speed)
				pc->unit->rot = dir;
			else
			{
				const float d = sign(shortestArc(pc->unit->rot, dir)) * rot_speed;
				pc->unit->rot = clip(pc->unit->rot + d);
			}
		}

		if(inventory_mode > I_INVENTORY)
		{
			if(inventory_mode == I_LOOT_BODY)
			{
				if(pc->action_unit->IsAlive())
				{
					// grabiony cel o�y�
					CloseInventory();
				}
			}
			else if(inventory_mode == I_TRADE || inventory_mode == I_SHARE || inventory_mode == I_GIVE)
			{
				if(!pc->action_unit->IsStanding() || !IsUnitIdle(*pc->action_unit))
				{
					// handlarz umar� / zaatakowany
					CloseInventory();
				}
			}
		}
		else if(dialog_context.dialog_mode && IsLocal())
		{
			if(!dialog_context.talker->IsStanding() || !IsUnitIdle(*dialog_context.talker) || dialog_context.talker->to_remove || dialog_context.talker->frozen != 0)
			{
				// rozm�wca umar� / jest usuwany / zaatakowa� kogo�
				EndDialog(dialog_context);
			}
		}

		UpdatePlayerView();
		before_player = BP_NONE;
		rot_buf = 0.f;
	}
	else if(!IsBlocking(pc->unit->action))
		UpdatePlayer(player_ctx, dt);
	else
	{
		UpdatePlayerView();
		before_player = BP_NONE;
		rot_buf = 0.f;
	}

	// aktualizuj ai
	if(!noai && IsLocal())
		UpdateAi(dt);

	// aktualizuj konteksty poziom�w
	UpdateContext(local_ctx, dt);
	if(city_ctx)
	{
		for(vector<InsideBuilding*>::iterator it = city_ctx->inside_buildings.begin(), end = city_ctx->inside_buildings.end(); it != end; ++it)
			UpdateContext((*it)->ctx, dt);
	}

	// aktualizacja minimapy
	if(minimap_opened_doors)
	{
		for(vector<Unit*>::iterator it = active_team.begin(), end = active_team.end(); it != end; ++it)
		{
			Unit& u = **it;
			if(u.IsPlayer())
				DungeonReveal(INT2(int(u.pos.x/2), int(u.pos.z/2)));
		}
	}
	UpdateDungeonMinimap(true);

	// aktualizuj pobliskie postacie
	// 0.0 -> 0.1 niewidoczne
	// 0.1 -> 0.2 alpha 0->255
	// -0.2 -> -0.1 widoczne
	// -0.1 -> 0.0 alpha 255->0
	for(vector<UnitView>::iterator it = unit_views.begin(), end = unit_views.end(); it != end;)
	{
		bool removed = false;

		if(it->valid)
		{
			if(it->time >= 0.f)
				it->time += dt;
			else if(it->time < -UNIT_VIEW_A)
				it->time = UNIT_VIEW_B;
			else
				it->time = -it->time;
		}
		else
		{
			if(it->time >= 0.f)
			{
				if(it->time < UNIT_VIEW_A)
				{
					// usu�
					if(it+1 == end)
					{
						unit_views.pop_back();
						break;
					}
					else
					{
						std::iter_swap(it, end-1);
						unit_views.pop_back();
						end = unit_views.end();
						removed = true;
					}
				}
				else if(it->time < UNIT_VIEW_B)
					it->time = -it->time;
				else
					it->time = -UNIT_VIEW_B;
			}
			else
			{
				it->time += dt;
				if(it->time >= 0.f)
				{
					// usu�
					if(it+1 == end)
					{
						unit_views.pop_back();
						break;
					}
					else
					{
						std::iter_swap(it, end-1);
						unit_views.pop_back();
						end = unit_views.end();
						removed = true;
					}
				}
			}
		}

		if(!removed)
			++it;
	}

	// aktualizuj dialogi
	if(!IsOnline())
	{
		if(dialog_context.dialog_mode)
			UpdateGameDialog(dialog_context, dt);
	}
	else if(IsServer())
	{
		for(vector<PlayerInfo>::iterator it = game_players.begin(), end = game_players.end(); it != end; ++it)
		{
			if(it->left)
				continue;
			DialogContext& ctx = *it->u->player->dialog_ctx;
			if(ctx.dialog_mode)
			{
				if(!ctx.talker->IsStanding() || !IsUnitIdle(*ctx.talker) || ctx.talker->to_remove || ctx.talker->frozen != 0)
					EndDialog(ctx);
				else
					UpdateGameDialog(ctx, dt);
			}
		}
	}
	else
	{
		if(dialog_context.dialog_mode)
			UpdateGameDialogClient();
	}

	UpdateAttachedSounds(dt);
	if(IsLocal())
	{
		if(IsOnline())
			UpdateWarpData(dt);
		ProcessUnitWarps();
	}

	// usu� jednostki
	ProcessRemoveUnits();

	if(IsOnline())
	{
		UpdateGameNet(dt);
		if(!IsOnline() || game_state != GS_LEVEL)
			return;
	}

	// aktualizacja obrazka obra�en
	pc->Update(dt);

	// aktualizuj kamer�
	SetupCamera(dt);

	first_frame = false;

#ifdef IS_DEV
	if(IsLocal() && arena_free)
	{
		int err_count = 0;
		for(vector<Unit*>::iterator it = team.begin(), end = team.end(); it != end; ++it)
		{
			for(vector<Unit*>::iterator it2 = team.begin(), end2 = team.end(); it2 != end2; ++it2)
			{
				if(it != it2)
				{
					if(!IsFriend(**it, **it2))
					{
						WARN(Format("%s (%d,%d) i %s (%d,%d) are not friends!", (*it)->data->id, (*it)->in_arena, (*it)->IsTeamMember() ? 1 : 0, (*it2)->data->id, (*it2)->in_arena,
							(*it2)->IsTeamMember() ? 1 : 0));
						++err_count;
					}
				}
			}
		}
		if(err_count)
			AddGameMsg(Format("%d arena friends errors!", err_count), 10.f);
	}
#endif
}

//=================================================================================================
// Aktualizuje gracza
//=================================================================================================
void Game::UpdatePlayer(LevelContext& ctx, float dt)
{
	Unit& u = *pc->unit;

	/* sprawdzanie kt�re kafelki wok� gracza blokuj�	
	{
		const float s = SS;
		const int n = NN;

		test_pf.clear();
		global_col.clear();
		BOX2D bo(u.pos.x-s*n/2-s/2,u.pos.z-s*n/2-s/2,u.pos.x+s*n/2+s/2,u.pos.z+s*n/2+s/2);
		IgnoreObjects ignore = {0};
		const Unit* ignore_units[2] = {&u, NULL};
		ignore.ignored_units = ignore_units;
		GatherCollisionObjects(ctx, global_col, bo, &ignore);

		for(int y=-n/2; y<=n/2; ++y)
		{
			for(int x=-n/2; x<=n/2; ++x)
			{
				BOX2D bo2(u.pos.x-s/2+s*x,u.pos.z-s/2+s*y,u.pos.x+s/2+s*x,u.pos.z+s/2+s*y);
				if(!Collide(global_col, bo2))
					test_pf.push_back(std::pair<VEC2,int>(VEC2(u.pos.x-s/2+s*x, u.pos.z-s/2+s*y), 0));
				else
					test_pf.push_back(std::pair<VEC2,int>(VEC2(u.pos.x-s/2+s*x, u.pos.z-s/2+s*y), 1));
			}
		}
	}*/

	if(!u.IsStanding())
	{
		rot_buf = 0.f;
		UnitTryStandup(u, dt);
		return;
	}

	if(u.frozen == 2)
	{
		rot_buf = 0.f;
		u.animacja = ANI_STOI;
		return;
	}

	if(u.useable)
	{
		if(u.action == A_ANIMATION2 && OR2_EQ(u.etap_animacji, 1, 2))
		{
			if(KeyPressedReleaseAllowed(GK_ATTACK_USE) || KeyPressedReleaseAllowed(GK_USE))
				Unit_StopUsingUseable(ctx, u);
		}
		UpdatePlayerView();
		rot_buf = 0.f;
	}

	u.prev_pos = u.pos;
	u.changed = true;

	bool idle = true, this_frame_run = false;

	if(!u.useable)
	{
		if(u.wyjeta == B_BRAK)
		{
			if(u.animacja != ANI_IDLE)
				u.animacja = ANI_STOI;
		}
		else if(u.wyjeta == B_JEDNORECZNA)
			u.animacja = ANI_BOJOWA;
		else if(u.wyjeta == B_LUK)
			u.animacja = ANI_BOJOWA_LUK;

		int rotate=0, move=0;
		if(KeyDownAllowed(GK_ROTATE_LEFT))
			--rotate;
		if(KeyDownAllowed(GK_ROTATE_RIGHT))
			++rotate;
		if(u.frozen == 0)
		{
			if(u.atak_w_biegu)
			{
				move = 10;
				if(KeyDownAllowed(GK_MOVE_RIGHT))
					++move;
				if(KeyDownAllowed(GK_MOVE_LEFT))
					--move;
			}
			else
			{
				if(KeyDownAllowed(GK_MOVE_RIGHT))
					++move;
				if(KeyDownAllowed(GK_MOVE_LEFT))
					--move;
				if(KeyDownAllowed(GK_MOVE_FORWARD))
					move += 10;
				if(KeyDownAllowed(GK_MOVE_BACK))
					move -= 10;
			}
		}

		if(u.action == A_NONE && !u.talking && KeyPressedReleaseAllowed(GK_YELL))
		{
			if(IsLocal())
				PlayerYell(u);
			else
				PushNetChange(NetChange::YELL);
		}

		if(allow_input == ALLOW_INPUT || allow_input == ALLOW_MOUSE)
		{
			rot_buf *= (1.f-dt*2);
			rot_buf += float(mouse_dif.x)/200;
			if(rot_buf > 0.1f)
				rot_buf = 0.1f;
			else if(rot_buf < -0.1f)
				rot_buf = -0.1f;
		}
		else
			rot_buf = 0.f;

		const bool rotating = (rotate || rot_buf != 0.f);

		if(rotating || move)
		{
			// rotating by mouse don't affect idle timer
			if(move || rotate)
				idle = false;

			if(rotating)
			{
				const float rot_speed_dt = u.GetRotationSpeed() * dt;
				const float mouse_rot = clamp(rot_buf, -rot_speed_dt, rot_speed_dt);
				const float val = rot_speed_dt*rotate + mouse_rot;

				rot_buf -= mouse_rot;
				u.rot = clip(u.rot + clamp(val, -rot_speed_dt, rot_speed_dt));

				if(val > 0)
					u.animacja = ANI_W_PRAWO;
				else if(val < 0)
					u.animacja = ANI_W_LEWO;
			}

			if(move)
			{
				// ustal k�t i szybko�� ruchu
				float angle = u.rot;
				bool run = true;
				if(!u.atak_w_biegu && (KeyDownAllowed(GK_WALK) || !u.CanRun()))
					run = false;

				switch(move)
				{
				case 10: // prz�d
					angle += PI;
					break;
				case -10: // ty�
					run = false;
					break;
				case -1: // lewa
					angle += PI/2;
					break;
				case 1: // prawa
					angle += PI*3/2;
					break;
				case 9: // lewa g�ra
					angle += PI*3/4;
					break;
				case 11: // prawa g�ra
					angle += PI*5/4;
					break;
				case -11: // lewy ty�
					run = false;
					angle += PI/4;
					break;
				case -9: // prawy ty�
					run = false;
					angle += PI*7/4;
					break;
				}

				if(u.action == A_SHOOT)
					run = false;

				if(run)
					u.animacja = ANI_BIEGNIE;
				else if(move < -9)
					u.animacja = ANI_IDZIE_TYL;
				else if(move == -1)
					u.animacja = ANI_W_LEWO;
				else if(move == 1)
					u.animacja = ANI_W_PRAWO;
				else
					u.animacja = ANI_IDZIE;

				u.speed = run ? u.GetRunSpeed() : u.GetWalkSpeed();
				u.prev_speed = (u.prev_speed + (u.speed - u.prev_speed)*dt*3);
				float speed = u.prev_speed * dt;

				u.prev_pos = u.pos;

				const VEC3 dir(sin(angle)*speed,0,cos(angle)*speed);
				INT2 prev_tile(int(u.pos.x/2), int(u.pos.z/2));
				bool reveal_minimap = false;

				if(pc->noclip)
				{
					u.pos += dir;
					MoveUnit(u);
					if(IsLocal())
						u.player->TrainMove(dt);
					else
					{
						train_move += dt;
						if(train_move >= 1.f)
						{
							--train_move;
							PushNetChange(NetChange::TRAIN_MOVE);
						}
					}
					reveal_minimap = true;
				}
				else if(CheckMove(u.pos, dir, u.GetUnitRadius(), &u))
				{
					MoveUnit(u);
					if(IsLocal())
						u.player->TrainMove(dt);
					else
					{
						train_move += dt;
						if(train_move >= 1.f)
						{
							--train_move;
							PushNetChange(NetChange::TRAIN_MOVE);
						}
					}
					reveal_minimap = true;
				}

				// odkrywanie minimapy
				if(reveal_minimap && !location->outside)
				{
					INT2 new_tile(int(u.pos.x/2), int(u.pos.z/2));
					if(new_tile != prev_tile)
						DungeonReveal(new_tile);
				}

				if(run && abs(u.speed - u.prev_speed) < 0.25f)
					this_frame_run = true;
			}
		}

		if(move == 0)
		{
			u.prev_speed -= dt*3;
			if(u.prev_speed < 0)
				u.prev_speed = 0;
		}
	}

	if(u.action == A_NONE || u.action == A_TAKE_WEAPON || u.CanDoWhileUsing())
	{
		if(KeyPressedReleaseAllowed(GK_TAKE_WEAPON))
		{
			idle = false;
			if(u.stan_broni == BRON_WYJETA || u.stan_broni == BRON_WYJMUJE)
				u.HideWeapon();
			else
			{
				BRON bron = pc->ostatnia;

				// ustal kt�r� bro� wyj���
				if(bron == B_BRAK)
				{
					if(u.HaveWeapon())
						bron = B_JEDNORECZNA;
					else if(u.HaveBow())
						bron = B_LUK;
				}
				else if(bron == B_JEDNORECZNA)
				{
					if(!u.HaveWeapon())
					{
						if(u.HaveBow())
							bron = B_LUK;
						else
							bron = B_BRAK;
					}
				}
				else
				{
					if(!u.HaveBow())
					{
						if(u.HaveWeapon())
							bron = B_JEDNORECZNA;
						else
							bron = B_BRAK;
					}
				}

				if(bron != B_BRAK)
				{
					pc->ostatnia = bron;
					pc->po_akcja = PO_BRAK;
					Inventory::lock_id = LOCK_NO;

					switch(u.stan_broni)
					{
					case BRON_SCHOWANA:
						// bro� jest schowana, zacznij wyjmowa�
						u.ani->Play(u.GetTakeWeaponAnimation(bron == B_JEDNORECZNA), PLAY_ONCE|PLAY_PRIO1, 1);
						u.wyjeta = bron;
						u.etap_animacji = 0;
						u.stan_broni = BRON_WYJMUJE;
						u.action = A_TAKE_WEAPON;
						break;
					case BRON_CHOWA:
						// bro� jest chowana, anuluj chowanie
						if(u.etap_animacji == 0)
						{
							// jeszcze nie schowa� broni za pas, wy��cz grup�
							u.action = A_NONE;
							u.wyjeta = u.chowana;
							u.chowana = B_BRAK;
							pc->ostatnia = u.wyjeta;
							u.stan_broni = BRON_WYJETA;
							u.ani->Deactivate(1);
						}
						else
						{
							// schowa� bro� za pas, zacznij wyci�ga�
							u.wyjeta = u.chowana;
							u.chowana = B_BRAK;
							pc->ostatnia = u.wyjeta;
							u.stan_broni = BRON_WYJMUJE;
							u.etap_animacji = 0;
							CLEAR_BIT(u.ani->groups[1].state, AnimeshInstance::FLAG_BACK);
						}
						break;
					case BRON_WYJMUJE:
						// wyjmuje bro�, anuluj wyjmowanie
						if(u.etap_animacji == 0)
						{
							// jeszcze nie wyj�� broni z pasa, po prostu wy��cz t� grupe
							u.action = A_NONE;
							u.wyjeta = B_BRAK;
							u.stan_broni = BRON_SCHOWANA;
							u.ani->Deactivate(1);
						}
						else
						{
							// wyj�� bro� z pasa, zacznij chowa�
							u.chowana = u.wyjeta;
							u.wyjeta = B_BRAK;
							u.stan_broni = BRON_CHOWA;
							u.etap_animacji = 0;
							SET_BIT(u.ani->groups[1].state, AnimeshInstance::FLAG_BACK);
						}
						break;
					case BRON_WYJETA:
						// bro� jest wyj�ta, zacznij chowa�
						u.ani->Play(u.GetTakeWeaponAnimation(bron == B_JEDNORECZNA), PLAY_ONCE|PLAY_BACK|PLAY_PRIO1, 1);
						u.chowana = bron;
						u.wyjeta = B_BRAK;
						u.stan_broni = BRON_CHOWA;
						u.action = A_TAKE_WEAPON;
						u.etap_animacji = 0;
						break;
					}

					if(IsOnline())
					{
						NetChange& c = Add1(net_changes);
						c.unit = pc->unit;
						c.id = ((u.stan_broni == BRON_SCHOWANA || u.stan_broni == BRON_CHOWA) ? 1 : 0);
						c.type = NetChange::TAKE_WEAPON;
					}
				}
				else
				{
					// komunikat o braku broni
					AddGameMsg2(txINeedWeapon, 2.f, GMS_NEED_WEAPON);
				}
			}
		}
		else if(u.HaveWeapon() && KeyPressedReleaseAllowed(GK_MELEE_WEAPON))
		{
			idle = false;
			if(u.stan_broni == BRON_SCHOWANA)
			{
				// bro� schowana, zacznij wyjmowa�
				u.ani->Play(u.GetTakeWeaponAnimation(true), PLAY_ONCE|PLAY_PRIO1, 1);
				u.wyjeta = pc->ostatnia = B_JEDNORECZNA;
				u.stan_broni = BRON_WYJMUJE;
				u.action = A_TAKE_WEAPON;
				u.etap_animacji = 0;
				pc->po_akcja = PO_BRAK;
				Inventory::lock_id = LOCK_NO;

				if(IsOnline())
				{
					NetChange& c = Add1(net_changes);
					c.unit = pc->unit;
					c.id = 0;
					c.type = NetChange::TAKE_WEAPON;
				}
			}
			else if(u.stan_broni == BRON_CHOWA)
			{
				// chowa bro�
				if(u.chowana == B_JEDNORECZNA)
				{
					if(u.etap_animacji == 0)
					{
						// jeszcze nie schowa� broni za pas, wy��cz grup�
						u.action = A_NONE;
						u.wyjeta = u.chowana;
						u.chowana = B_BRAK;
						pc->ostatnia = u.wyjeta;
						u.stan_broni = BRON_WYJETA;
						u.ani->Deactivate(1);
					}
					else
					{
						// schowa� bro� za pas, zacznij wyci�ga�
						u.wyjeta = u.chowana;
						u.chowana = B_BRAK;
						pc->ostatnia = u.wyjeta;
						u.stan_broni = BRON_WYJMUJE;
						u.etap_animacji = 0;
						CLEAR_BIT(u.ani->groups[1].state, AnimeshInstance::FLAG_BACK);
					}

					if(IsOnline())
					{
						NetChange& c = Add1(net_changes);
						c.unit = pc->unit;
						c.id = 0;
						c.type = NetChange::TAKE_WEAPON;
					}
				}
				else
				{
					// chowa �uk, dodaj info �eby wyj�� bro�
					u.wyjeta = B_JEDNORECZNA;
				}
				pc->po_akcja = PO_BRAK;
				Inventory::lock_id = LOCK_NO;
			}
			else if(u.stan_broni == BRON_WYJMUJE)
			{
				// wyjmuje bro�
				if(u.wyjeta == B_LUK)
				{
					// wyjmuje �uk
					if(u.etap_animacji == 0)
					{
						// tak na prawd� to jeszcze nic nie zrobi� wi�c mo�na anuluowa�
						u.ani->Play(u.GetTakeWeaponAnimation(true), PLAY_ONCE|PLAY_PRIO1, 1);
						pc->ostatnia = u.wyjeta = B_JEDNORECZNA;
					}
					else
					{
						// ju� wyj�� wi�c trzeba schowa� i doda� info
						pc->ostatnia = u.wyjeta = B_JEDNORECZNA;
						u.chowana = B_LUK;
						u.stan_broni = BRON_CHOWA;
						u.etap_animacji = 0;
						SET_BIT(u.ani->groups[1].state, AnimeshInstance::FLAG_BACK);
					}
				}
				pc->po_akcja = PO_BRAK;
				Inventory::lock_id = LOCK_NO;

				if(IsOnline())
				{
					NetChange& c = Add1(net_changes);
					c.unit = pc->unit;
					c.id = (u.stan_broni == BRON_CHOWA ? 1 : 0);
					c.type = NetChange::TAKE_WEAPON;
				}
			}
			else
			{
				// bro� wyj�ta
				if(u.wyjeta == B_LUK)
				{
					pc->ostatnia = u.wyjeta = B_JEDNORECZNA;
					u.chowana = B_LUK;
					u.stan_broni = BRON_CHOWA;
					u.etap_animacji = 0;
					u.action = A_TAKE_WEAPON;
					u.ani->Play(NAMES::ani_take_bow, PLAY_BACK|PLAY_ONCE|PLAY_PRIO1, 1);
					pc->po_akcja = PO_BRAK;
					Inventory::lock_id = LOCK_NO;

					if(IsOnline())
					{
						NetChange& c = Add1(net_changes);
						c.unit = pc->unit;
						c.id = 1;
						c.type = NetChange::TAKE_WEAPON;
					}
				}
			}
		}
		else if(u.HaveBow() && KeyPressedReleaseAllowed(GK_RANGED_WEAPON))
		{
			idle = false;
			if(u.stan_broni == BRON_SCHOWANA)
			{
				// bro� schowana, zacznij wyjmowa�
				u.wyjeta = pc->ostatnia = B_LUK;
				u.stan_broni = BRON_WYJMUJE;
				u.action = A_TAKE_WEAPON;
				u.etap_animacji = 0;
				u.ani->Play(NAMES::ani_take_bow, PLAY_ONCE|PLAY_PRIO1, 1);
				pc->po_akcja = PO_BRAK;
				Inventory::lock_id = LOCK_NO;

				if(IsOnline())
				{
					NetChange& c = Add1(net_changes);
					c.unit = pc->unit;
					c.id = 0;
					c.type = NetChange::TAKE_WEAPON;
				}
			}
			else if(u.stan_broni == BRON_CHOWA)
			{
				// chowa
				if(u.chowana == B_LUK)
				{
					if(u.etap_animacji == 0)
					{
						// jeszcze nie schowa� �uku, wy��cz grup�
						u.action = A_NONE;
						u.wyjeta = u.chowana;
						u.chowana = B_BRAK;
						pc->ostatnia = u.wyjeta;
						u.stan_broni = BRON_WYJETA;
						u.ani->Deactivate(1);
					}
					else
					{
						// schowa� �uk, zacznij wyci�ga�
						u.wyjeta = u.chowana;
						u.chowana = B_BRAK;
						pc->ostatnia = u.wyjeta;
						u.stan_broni = BRON_WYJMUJE;
						u.etap_animacji = 0;
						CLEAR_BIT(u.ani->groups[1].state, AnimeshInstance::FLAG_BACK);
					}
				}
				else
				{
					// chowa bro�, dodaj info �eby wyj�� bro�
					u.wyjeta = B_LUK;
				}
				pc->po_akcja = PO_BRAK;
				Inventory::lock_id = LOCK_NO;

				if(IsOnline())
				{
					NetChange& c = Add1(net_changes);
					c.unit = pc->unit;
					c.id = 0;
					c.type = NetChange::TAKE_WEAPON;
				}
			}
			else if(u.stan_broni == BRON_WYJMUJE)
			{
				// wyjmuje bro�
				if(u.wyjeta == B_JEDNORECZNA)
				{
					// wyjmuje bro�
					if(u.etap_animacji == 0)
					{
						// tak na prawd� to jeszcze nic nie zrobi� wi�c mo�na anuluowa�
						pc->ostatnia = u.wyjeta = B_LUK;
						u.ani->Play(NAMES::ani_take_bow, PLAY_ONCE|PLAY_PRIO1, 1);
					}
					else
					{
						// ju� wyj�� wi�c trzeba schowa� i doda� info
						pc->ostatnia = u.wyjeta = B_LUK;
						u.chowana = B_JEDNORECZNA;
						u.stan_broni = BRON_CHOWA;
						u.etap_animacji = 0;
						SET_BIT(u.ani->groups[1].state, AnimeshInstance::FLAG_BACK);
					}
				}
				pc->po_akcja = PO_BRAK;
				Inventory::lock_id = LOCK_NO;

				if(IsOnline())
				{
					NetChange& c = Add1(net_changes);
					c.unit = pc->unit;
					c.id = (u.stan_broni == BRON_CHOWA ? 1 : 0);
					c.type = NetChange::TAKE_WEAPON;
				}
			}
			else
			{
				// bro� wyj�ta
				if(u.wyjeta == B_JEDNORECZNA)
				{
					u.ani->Play(u.GetTakeWeaponAnimation(true), PLAY_BACK|PLAY_ONCE|PLAY_PRIO1, 1);
					pc->ostatnia = u.wyjeta = B_LUK;
					u.chowana = B_JEDNORECZNA;
					u.stan_broni = BRON_CHOWA;
					u.etap_animacji = 0;
					u.action = A_TAKE_WEAPON;
					pc->po_akcja = PO_BRAK;
					Inventory::lock_id = LOCK_NO;

					if(IsOnline())
					{
						NetChange& c = Add1(net_changes);
						c.unit = pc->unit;
						c.id = 1;
						c.type = NetChange::TAKE_WEAPON;
					}
				}
			}
		}

		if(KeyPressedReleaseAllowed(GK_POTION) && !equal(u.hp, u.hpmax))
		{
			idle = false;
			// wypij miksturk� lecznicz�
			float brakuje = u.hpmax - u.hp;
			int wypij = -1, index = 0;
			float wyleczy;

			for(vector<ItemSlot>::iterator it = u.items.begin(), end = u.items.end(); it != end; ++it, ++index)
			{
				if(!it->item || it->item->type != IT_CONSUMEABLE)
					continue;
				const Consumeable& pot = it->item->ToConsumeable();
				if(pot.effect == E_HEAL)
				{
					if(wypij == -1)
					{
						wypij = index;
						wyleczy = pot.power;
					}
					else
					{
						if(pot.power > brakuje)
						{
							if(pot.power < wyleczy)
							{
								wypij = index;
								wyleczy = pot.power;
							}
						}
						else if(pot.power > wyleczy)
						{
							wypij = index;
							wyleczy = pot.power;
						}
					}
				}
			}

			if(wypij != -1)
				u.ConsumeItem(wypij);
			else
				AddGameMsg2(txNoHpp, 2.f, GMS_NO_POTION);
		}
	} // allow_input == ALLOW_INPUT || allow_input == ALLOW_KEYBOARD

	if(u.useable)
		return;

	// sprawd� co jest przed graczem oraz stw�rz list� pobliskich wrog�w
	before_player = BP_NONE;
	float dist, best_dist = 3.0f, best_dist_target = 3.0f;
	selected_target = NULL;

	for(vector<Unit*>::iterator it = ctx.units->begin(), end = ctx.units->end(); it != end; ++it)
	{
		Unit& u2 = **it;
		if(&u == &u2 || u2.to_remove)
			continue;

		bool mark = false;
		if(IsEnemy(u, u2))
		{
			if(u2.IsAlive())
				mark = true;
		}
		else if(IsFriend(u, u2))
			mark = true;

		dist = distance2d(u.visual_pos, u2.visual_pos);

		// wybieranie postaci
		if(u2.IsStanding())
			PlayerCheckObjectDistance(u, u2.visual_pos, &u2, best_dist, BP_UNIT);
		else if(u2.live_state == Unit::FALL || u2.live_state == Unit::DEAD)
			PlayerCheckObjectDistance(u, u2.GetLootCenter(), &u2, best_dist, BP_UNIT);

		// oznaczanie pobliskich postaci
		if(mark)
		{
			if(dist < alert_range.x && camera_frustum.SphereToFrustum(u2.visual_pos, u2.GetSphereRadius()) && CanSee(u, u2))
			{
				// dodaj do pobliskich jednostek
				bool jest = false;
				for(vector<UnitView>::iterator it2 = unit_views.begin(), end2 = unit_views.end(); it2 != end2; ++it2)
				{
					if(it2->unit == *it)
					{
						jest = true;
						it2->valid = true;
						it2->last_pos = u2.GetUnitTextPos();
						break;
					}
				}
				if(!jest)
				{
					UnitView& uv = Add1(unit_views);
					uv.valid = true;
					uv.unit = *it;
					uv.time = 0.f;
					uv.last_pos = u2.GetUnitTextPos();
				}
			}
		}
	}

	// skrzynie przed graczem
	if(ctx.chests && !ctx.chests->empty())
	{
		for(vector<Chest*>::iterator it = ctx.chests->begin(), end = ctx.chests->end(); it != end; ++it)
			PlayerCheckObjectDistance(u, (*it)->pos, *it, best_dist, BP_CHEST);
	}

	// drzwi przed graczem
	if(ctx.doors && !ctx.doors->empty())
	{
		for(vector<Door*>::iterator it = ctx.doors->begin(), end = ctx.doors->end(); it != end; ++it)
		{
			if(OR2_EQ((*it)->state, Door::Open, Door::Closed))
				PlayerCheckObjectDistance(u, (*it)->pos, *it, best_dist, BP_DOOR);
		}
	}

	// przedmioty przed graczem
	for(vector<GroundItem*>::iterator it = ctx.items->begin(), end = ctx.items->end(); it != end; ++it)
		PlayerCheckObjectDistance(u, (*it)->pos, *it, best_dist, BP_ITEM);

	// u�ywalne przed graczem
	for(vector<Useable*>::iterator it = ctx.useables->begin(), end = ctx.useables->end(); it != end; ++it)
	{
		if(!(*it)->user)
			PlayerCheckObjectDistance(u, (*it)->pos, *it, best_dist, BP_USEABLE);
	}

	if(best_dist < best_dist_target && before_player == BP_UNIT && before_player_ptr.unit->IsStanding())
	{
		selected_target = before_player_ptr.unit;
#ifdef DRAW_LOCAL_PATH
		if(Key.Down('K'))
			marked = selected_target;
#endif
	}

	// u�yj czego� przed graczem
	if(u.frozen == 0 && before_player != BP_NONE && (KeyPressedReleaseAllowed(GK_USE) || (u.IsNotFighting() && KeyPressedReleaseAllowed(GK_ATTACK_USE))))
	{
		idle = false;
		if(before_player == BP_UNIT)
		{
			Unit* u2 = before_player_ptr.unit;

			// przed graczem jest jaka� posta�
			if(u2->live_state == Unit::DEAD || u2->live_state == Unit::FALL)
			{
				// grabienie zw�ok
				if(u.action != A_NONE)
				{

				}
				else if(u2->live_state == Unit::FALL)
				{
					// nie mo�na okrada� osoby kt�ra zaraz wstanie
					AddGameMsg2(txCantDo, 3.f, GMS_CANT_DO);
				}
				else if(u2->IsFollower() || u2->IsPlayer())
				{
					// nie mo�na okrada� sojusznik�w
					AddGameMsg2(txDontLootFollower, 3.f, GMS_DONT_LOOT_FOLLOWER);
				}
				else if(u2->in_arena != -1)
				{
					AddGameMsg2(txDontLootArena, 3.f, GMS_DONT_LOOT_ARENA);
				}
				else if(IsLocal())
				{
					if(IsOnline() && u2->busy == Unit::Busy_Looted)
					{
						// kto� ju� ograbia zw�oki
						AddGameMsg3(GMS_IS_LOOTED);
					}
					else
					{
						// rozpoczynij wymian� przedmiot�w
						pc->action = PlayerController::Action_LootUnit;
						pc->action_unit = u2;
						u2->busy = Unit::Busy_Looted;
						pc->chest_trade = &u2->items;
						CloseGamePanels();
						inventory_mode = I_LOOT_BODY;
						BuildTmpInventory(0);
						inv_trade_mine->mode = Inventory::LOOT_MY;
						BuildTmpInventory(1);
						inv_trade_other->unit = pc->action_unit;
						inv_trade_other->items = &pc->action_unit->items;
						inv_trade_other->slots = pc->action_unit->slots;
						inv_trade_other->title = Format("%s - %s", inv_trade_other->txLooting, pc->action_unit->GetName());
						inv_trade_other->mode = Inventory::LOOT_OTHER;
						gp_trade->Show();
					}
				}
				else
				{
					// wiadomo�� o wymianie do serwera
					NetChange& c = Add1(net_changes);
					c.type = NetChange::LOOT_UNIT;
					c.id = u2->netid;
					pc->action = PlayerController::Action_LootUnit;
					pc->action_unit = u2;
					pc->chest_trade = &u2->items;
				}
			}
			else if(u2->IsAI() && IsUnitIdle(*u2) && u2->in_arena == -1 && u2->data->dialog && !IsEnemy(u, *u2))
			{
				if(IsLocal())
				{
					if(u2->busy != Unit::Busy_No || !u2->CanTalk())
					{
						// osoba jest czym� zaj�ta
						AddGameMsg3(GMS_UNIT_BUSY);
					}
					else
					{
						// rozpocznij rozmow�
						u2->auto_talk = 0;
						StartDialog(dialog_context, u2);
						before_player = BP_NONE;
					}
				}
				else
				{
					// wiadomo�� o rozmowie do serwera
					NetChange& c = Add1(net_changes);
					c.type = NetChange::TALK;
					c.id = u2->netid;
					pc->action = PlayerController::Action_Talk;
					pc->action_unit = u2;
					predialog.clear();
				}
			}
		}
		else if(before_player == BP_CHEST)
		{
			// pl�drowanie skrzyni
			if(pc->unit->action != A_NONE)
			{

			}
			else if(IsLocal())
			{
				if(before_player_ptr.chest->looted)
				{
					// kto� ju� zajmuje si� t� skrzyni�
					AddGameMsg3(GMS_IS_LOOTED);
				}
				else
				{
					// rozpocznij ograbianie skrzyni
					pc->action = PlayerController::Action_LootChest;
					pc->action_chest = before_player_ptr.chest;
					pc->action_chest->looted = true;
					pc->chest_trade = &pc->action_chest->items;
					CloseGamePanels();
					inventory_mode = I_LOOT_CHEST;
					BuildTmpInventory(0);
					inv_trade_mine->mode = Inventory::LOOT_MY;
					BuildTmpInventory(1);
					inv_trade_other->unit = NULL;
					inv_trade_other->items = &pc->action_chest->items;
					inv_trade_other->slots = NULL;
					inv_trade_other->title = Inventory::txLootingChest;
					inv_trade_other->mode = Inventory::LOOT_OTHER;
					gp_trade->Show();

					// animacja / d�wi�k
					pc->action_chest->ani->Play(&pc->action_chest->ani->ani->anims[0], PLAY_PRIO1|PLAY_ONCE|PLAY_STOP_AT_END, 0);
					if(sound_volume)
					{
						VEC3 pos = pc->action_chest->pos;
						pos.y += 0.5f;
						PlaySound3d(sChestOpen, pos, 2.f, 5.f);
					}

					// event handler
					if(before_player_ptr.chest->handler)
						before_player_ptr.chest->handler->HandleChestEvent(ChestEventHandler::Opened);

					if(IsOnline())
					{
						NetChange& c = Add1(net_changes);
						c.type = NetChange::CHEST_OPEN;
						c.id = pc->action_chest->netid;
					}
				}
			}
			else
			{
				// wy�lij wiadomo�� o pl�drowaniu skrzyni
				NetChange& c = Add1(net_changes);
				c.type = NetChange::LOOT_CHEST;
				c.id = before_player_ptr.chest->netid;
				pc->action = PlayerController::Action_LootChest;
				pc->action_chest = before_player_ptr.chest;
				pc->chest_trade = &pc->action_chest->items;
			}
		}
		else if(before_player == BP_DOOR)
		{
			// otwieranie/zamykanie drzwi
			Door* door = before_player_ptr.door;
			if(door->state == Door::Closed)
			{
				// otwieranie drzwi
				if(door->locked == LOCK_NONE)
				{
					if(!location->outside)
						minimap_opened_doors = true;
					door->state = Door::Opening;
					door->ani->Play(&door->ani->ani->anims[0], PLAY_ONCE|PLAY_STOP_AT_END|PLAY_NO_BLEND, 0);
					door->ani->frame_end_info = false;
					if(sound_volume && rand2()%2 == 0)
					{
						// skrzypienie
						VEC3 pos = door->pos;
						pos.y += 1.5f;
						PlaySound3d(sDoor[rand2()%3], pos, 2.f, 5.f);
					}
					if(IsOnline())
					{
						NetChange& c = Add1(net_changes);
						c.type = NetChange::USE_DOOR;
						c.id = door->netid;
						c.ile = 0;
					}
				}
				else
				{
					cstring key;
					switch(door->locked)
					{
					case LOCK_KOPALNIA:
						key = "key_kopalnia";
						break;
					case LOCK_ORKOWIE:
						key = "q_orkowie_klucz";
						break;
					case LOCK_UNLOCKABLE:
					default:
						key = NULL;
						break;
					}

					if(key && pc->unit->HaveItem(FindItem(key)))
					{
						if(sound_volume)
						{
							VEC3 pos = door->pos;
							pos.y += 1.5f;
							PlaySound3d(sUnlock, pos, 2.f, 5.f);
						}
						AddGameMsg2(txUnlockedDoor, 3.f, GMS_UNLOCK_DOOR);
						if(!location->outside)
							minimap_opened_doors = true;
						door->locked = LOCK_NONE;
						door->state = Door::Opening;
						door->ani->Play(&door->ani->ani->anims[0], PLAY_ONCE|PLAY_STOP_AT_END|PLAY_NO_BLEND, 0);
						door->ani->frame_end_info = false;
						if(sound_volume && rand2()%2 == 0)
						{
							// skrzypienie
							VEC3 pos = door->pos;
							pos.y += 1.5f;
							PlaySound3d(sDoor[rand2()%3], pos, 2.f, 5.f);
						}
						if(IsOnline())
						{
							NetChange& c = Add1(net_changes);
							c.type = NetChange::USE_DOOR;
							c.id = door->netid;
							c.ile = 0;
						}
					}
					else
					{
						AddGameMsg2(txNeedKey, 3.f, GMS_NEED_KEY);
						if(sound_volume)
						{
							VEC3 pos = door->pos;
							pos.y += 1.5f;
							PlaySound3d(sDoorClosed, pos, 2.f, 5.f);
						}
					}
				}
			}
			else
			{
				// zamykanie drzwi
				door->state = Door::Closing;
				door->ani->Play(&door->ani->ani->anims[0], PLAY_ONCE|PLAY_STOP_AT_END|PLAY_NO_BLEND|PLAY_BACK, 0);
				door->ani->frame_end_info = false;
				if(sound_volume && rand2()%2 == 0)
				{
					SOUND snd;
					if(rand2()%2 == 0)
						snd = sDoorClose;
					else
						snd = sDoor[rand2()%3];
					VEC3 pos = door->pos;
					pos.y += 1.5f;
					PlaySound3d(snd, pos, 2.f, 5.f);
				}
				if(IsOnline())
				{
					NetChange& c = Add1(net_changes);
					c.type = NetChange::USE_DOOR;
					c.id = door->netid;
					c.ile = 1;
				}
			}
		}
		else if(before_player == BP_ITEM)
		{
			// podnie� przedmiot
			GroundItem& item = *before_player_ptr.item;
			if(u.action == A_NONE)
			{
				bool u_gory = (item.pos.y > u.pos.y+0.5f);

				u.action = A_PICKUP;
				u.animacja = ANI_ODTWORZ;
				u.ani->Play(u_gory ? "podnosi_gora" : "podnosi", PLAY_ONCE|PLAY_PRIO2, 0);
				u.ani->frame_end_info = false;

				if(IsLocal())
				{
					AddItem(u, item);

					if(item.item->type == IT_GOLD && sound_volume)
						PlaySound2d(sMoneta);

					if(IsOnline())
					{
						NetChange& c = Add1(net_changes);
						c.type = NetChange::PICKUP_ITEM;
						c.unit = pc->unit;
						c.ile = (u_gory ? 1 : 0);

						NetChange& c2 = Add1(net_changes);
						c2.type = NetChange::REMOVE_ITEM;
						c2.id = before_player_ptr.item->netid;
					}

					DeleteElement(ctx.items, before_player_ptr.item);
					before_player = BP_NONE;
				}
				else
				{
					NetChange& c = Add1(net_changes);
					c.type = NetChange::PICKUP_ITEM;
					c.id = item.netid;

					picking_item = &item;
					picking_item_state = 1;
				}
			}
		}
		else if(u.action == A_NONE)
			PlayerUseUseable(before_player_ptr.useable, false);
	}

	if(before_player == BP_UNIT)
		selected_unit = before_player_ptr.unit;
	else
		selected_unit = NULL;
		
	// atak
	if(u.stan_broni == BRON_WYJETA)
	{
		idle = false;
		if(u.wyjeta == B_JEDNORECZNA)
		{
			if(u.action == A_ATTACK)
			{
				if(u.etap_animacji == 0)
				{
					if(KeyUpAllowed(pc->klawisz))
					{
						u.attack_power = u.ani->groups[1].time / u.GetAttackFrame(0);
						u.etap_animacji = 1;
						u.ani->groups[1].speed = u.attack_power + u.GetAttackSpeed();
						u.attack_power += 1.f;

						if(IsOnline())
						{
							NetChange& c = Add1(net_changes);
							c.type = NetChange::ATTACK;
							c.unit = pc->unit;
							c.id = AID_Attack;
							c.f[1] = u.ani->groups[1].speed;
						}
					}
				}
				else if(u.etap_animacji == 2)
				{
					byte k = KeyDoReturn(GK_ATTACK_USE, &KeyStates::Down);
					if(k != VK_NONE)
					{
						u.action = A_ATTACK;
						u.attack_id = u.GetRandomAttack();
						u.ani->Play(NAMES::ani_attacks[u.attack_id], PLAY_PRIO1|PLAY_ONCE|PLAY_RESTORE, 1);
						u.ani->groups[1].speed = u.GetPowerAttackSpeed();
						pc->klawisz = k;
						u.etap_animacji = 0;
						u.atak_w_biegu = false;
						u.trafil = false;

						if(IsOnline())
						{
							NetChange& c = Add1(net_changes);
							c.type = NetChange::ATTACK;
							c.unit = pc->unit;
							c.id = AID_PowerAttack;
							c.f[1] = u.ani->groups[1].speed;
						}
					}
				}
			}
			else if(u.action == A_BLOCK)
			{
				if(KeyUpAllowed(pc->klawisz))
				{
					// sko�cz blokowa�
					u.action = A_NONE;
					u.ani->frame_end_info2 = false;
					u.ani->Deactivate(1);

					if(IsOnline())
					{
						NetChange& c = Add1(net_changes);
						c.type = NetChange::ATTACK;
						c.unit = pc->unit;
						c.id = AID_StopBlock;
						c.f[1] = 1.f;
					}
				}
				else if(!u.ani->groups[1].IsBlending() && u.HaveShield())
				{
					if(KeyDownAllowed(GK_ATTACK_USE))
					{
						// uderz tarcz�
						u.action = A_BASH;
						u.etap_animacji = 0;
						u.ani->Play(NAMES::ani_bash, PLAY_ONCE|PLAY_PRIO1|PLAY_RESTORE, 1);
						u.ani->groups[1].speed = 2.f;
						u.ani->frame_end_info2 = false;
						u.trafil = false;

						if(IsOnline())
						{
							NetChange& c = Add1(net_changes);
							c.type = NetChange::ATTACK;
							c.unit = pc->unit;
							c.id = AID_Bash;
							c.f[1] = 2.f;
						}
					}
				}
			}
			else if(u.action == A_NONE && u.frozen == 0)
			{
				byte k = KeyDoReturn(GK_ATTACK_USE, &KeyStates::Down);
				if(k != VK_NONE)
				{
					u.action = A_ATTACK;
					u.attack_id = u.GetRandomAttack();
					u.ani->Play(NAMES::ani_attacks[u.attack_id], PLAY_PRIO1|PLAY_ONCE|PLAY_RESTORE, 1);
					if(this_frame_run)
					{
						// atak w biegu
						u.ani->groups[1].speed = u.GetAttackSpeed();
						u.etap_animacji = 1;
						u.atak_w_biegu = true;
						u.attack_power = 1.5f;

						if(IsOnline())
						{
							NetChange& c = Add1(net_changes);
							c.type = NetChange::ATTACK;
							c.unit = pc->unit;
							c.id = AID_RunningAttack;
							c.f[1] = u.ani->groups[1].speed;
						}
					}
					else
					{
						// normalny/pot�ny atak
						u.ani->groups[1].speed = u.GetPowerAttackSpeed();
						pc->klawisz = k;
						u.etap_animacji = 0;
						u.atak_w_biegu = false;

						if(IsOnline())
						{
							NetChange& c = Add1(net_changes);
							c.type = NetChange::ATTACK;
							c.unit = pc->unit;
							c.id = AID_PowerAttack;
							c.f[1] = u.ani->groups[1].speed;
						}
					}
					u.trafil = false;
				}
// 				else if(Key.Pressed('C'))
// 				{
// 					u.action = A_PAROWANIE;
// 					u.ani->Play("parry", PLAY_ONCE|PLAY_PRIO1, 1);
// 					u.ani->frame_end_info2 = false;
// 				}
			}
			if(u.frozen == 0 && /*(*/u.HaveShield() /*|| u.HaveWeapon())*/ && !u.atak_w_biegu && (u.action == A_NONE || u.action == A_ATTACK))
			{
				int oks = 0;
				if(u.action == A_ATTACK)
				{
					if(u.atak_w_biegu || (u.attack_power > 1.5f && u.etap_animacji == 1))
						oks = 1;
					else
						oks = 2;
				}

				if(oks != 1)
				{
					byte k = KeyDoReturn(GK_BLOCK, &KeyStates::Down);
					if(k != VK_NONE)
					{
						u.action = A_BLOCK;
						u.ani->Play(/*u.HaveShield() ?*/ NAMES::ani_block /*: "blok_bron"*/, PLAY_PRIO1|PLAY_STOP_AT_END|PLAY_RESTORE, 1);
						u.ani->groups[1].blend_max = (oks == 2 ? 0.33f : u.GetBlockSpeed());
						pc->klawisz = k;
						u.etap_animacji = 0;

						if(IsOnline())
						{
							NetChange& c = Add1(net_changes);
							c.type = NetChange::ATTACK;
							c.unit = pc->unit;
							c.id = AID_Block;
							c.f[1] = u.ani->groups[1].blend_max;
						}
					}
				}
			}
		}
		else
		{
			// atak z �uku
			if(u.action == A_SHOOT)
			{
				if(u.etap_animacji == 0 && KeyUpAllowed(pc->klawisz))
				{
					u.etap_animacji = 1;

					if(IsOnline())
					{
						NetChange& c = Add1(net_changes);
						c.type = NetChange::ATTACK;
						c.unit = pc->unit;
						c.id = AID_Shoot;
						c.f[1] = 1.f;
					}
				}
			}
			else if(u.frozen == 0)
			{
				byte k = KeyDoReturn(GK_ATTACK_USE, &KeyStates::Down);
				if(k != VK_NONE)
				{
					float speed = u.GetBowAttackSpeed();
					u.ani->Play(NAMES::ani_shoot, PLAY_PRIO1|PLAY_ONCE|PLAY_RESTORE, 1);
					u.ani->groups[1].speed = speed;
					u.action = A_SHOOT;
					u.etap_animacji = 0;
					u.trafil = false;
					pc->klawisz = k;
					if(bow_instances.empty())
						u.bow_instance = new AnimeshInstance(u.GetBow().ani);
					else
					{
						u.bow_instance = bow_instances.back();
						bow_instances.pop_back();
						u.bow_instance->ani = u.GetBow().ani;
					}
					u.bow_instance->Play(&u.bow_instance->ani->anims[0], PLAY_ONCE|PLAY_PRIO1|PLAY_NO_BLEND|PLAY_RESTORE, 0);
					u.bow_instance->groups[0].speed = speed;

					if(IsOnline())
					{
						NetChange& c = Add1(net_changes);
						c.type = NetChange::ATTACK;
						c.unit = pc->unit;
						c.id = AID_StartShoot;
						c.f[1] = speed;
					}
				}
			}
		}
	}

	// animacja idle
	if(idle && u.action == A_NONE)
	{
		pc->idle_timer += dt;
		if(pc->idle_timer >= 4.f)
		{
			if(u.animacja == ANI_W_LEWO || u.animacja == ANI_W_PRAWO)
				pc->idle_timer = random(2.f,3.f);
			else
			{
				int id = rand2()%u.data->idles_count;
				pc->idle_timer = random(0.f,0.5f);
				u.ani->Play(u.data->idles[id], PLAY_ONCE, 0);
				u.ani->frame_end_info = false;
				u.animacja = ANI_IDLE;

				if(IsOnline())
				{
					NetChange& c = Add1(net_changes);
					c.type = NetChange::IDLE;
					c.unit = pc->unit;
					c.id = id;
				}
			}
		}
	}
	else
		pc->idle_timer = random(0.f,0.5f);
}

void Game::PlayerCheckObjectDistance(Unit& u, const VEC3& pos, void* ptr, float& best_dist, BeforePlayer type)
{
	assert(ptr);

	if(pos.y < u.pos.y-0.5f || pos.y > u.pos.y+2.f)
		return;

	float dist = distance2d(u.pos, pos);
	if(dist < pickup_range && dist < best_dist)
	{
		float angle = angle_dif(clip(u.rot+PI/2), clip(-angle2d(u.pos, pos)));
		assert(angle >= 0.f);
		if(angle < PI/4)
		{
			if(type == BP_CHEST)
			{
				Chest* chest = (Chest*)ptr;
				if(angle_dif(clip(chest->rot-PI/2), clip(-angle2d(u.pos, pos))) > PI/2)
					return;
			}
			dist += angle;
			if(dist < best_dist)
			{
				best_dist = dist;
				before_player_ptr.any = ptr;
				before_player = type;
			}
		}
	}
}

const float SMALL_DISTANCE = 0.001f;

struct Clbk : public btCollisionWorld::ContactResultCallback
{
	btCollisionObject* ignore;
	bool hit;

	Clbk(btCollisionObject* _ignore) : ignore(_ignore), hit(false)
	{

	}

	btScalar addSingleResult(btManifoldPoint& cp,	const btCollisionObjectWrapper* colObj0Wrap,int partId0,int index0,const btCollisionObjectWrapper* colObj1Wrap,int partId1,int index1)
	{
		hit = true;
		return 0.f;
	}
};

#ifdef _DEBUG
#define TEST_MOVE { Clbk clbk(_me->cobj); phy_world->contactTest(_me->cobj, clbk); assert(!clbk.hit); }
#else
#define TEST_MOVE
#endif

int Game::CheckMove(VEC3& _pos, const VEC3& _dir, float _radius, Unit* _me, bool* is_small)
{
	assert(_radius > 0.f && _me);

	VEC3 new_pos = _pos + _dir;
	VEC3 gather_pos = _pos + _dir/2;
	float gather_radius = D3DXVec3Length(&_dir) + _radius;

	/*global_bcol.clear();

	Unit* ignored[] = {_me, NULL};
	GatherBulletCollisionObjects(global_bcol, gather_pos, gather_radius+_me->cobj->getCollisionShape()->getMargin(), (const Unit**)ignored);

	if(global_bcol.empty())
	{
		if(is_small)
			*is_small = (distance(_pos, new_pos) < SMALL_DISTANCE);
		_pos = new_pos;
		//TEST_MOVE;
		return 3;
	}

	//OutputDebugString("-------------------------\n");

	btTransform& tr = _me->cobj->getWorldTransform();
	btVector3 origin = tr.getOrigin();

	//tmpvar2 = 0;

	// id� prosto po x i z
	tr.setOrigin(btVector3(new_pos.x, origin.y(), new_pos.z));
	phy_world->updateSingleAabb(_me->cobj);
	if(!Collide(_me->cobj, global_bcol))
	{
		//OutputDebugString(format("XZ (%g,%g)->(%g,%g)\n", _pos.x, _pos.z, new_pos.x, new_pos.z));
		if(is_small)
			*is_small = (distance(_pos, new_pos) < SMALL_DISTANCE);
		_pos = new_pos;
		//tmpvar2 = 1;
		//TEST_MOVE;
		return 3;
	}
	//else
	//{
	//	OutputDebugString(format("--XZ-- (%g,%g)->(%g,%g)\n", _pos.x, _pos.z, new_pos.x, new_pos.z));
	//}

	// id� po x
	tr.setOrigin(btVector3(new_pos.x, origin.y(), _pos.z+sign(-_dir.z)*0.1f));
	phy_world->updateSingleAabb(_me->cobj);
	if(!Collide(_me->cobj, global_bcol))
	{
		//OutputDebugString(format("X (%g,%g)->(%g,%g)\n", _pos.x, _pos.z, new_pos.x, _pos.z));
		if(is_small)
			*is_small = (distance(_pos, new_pos) < SMALL_DISTANCE);
		//_pos.z += sign(-_dir.z)*0.01f;
		_pos.x = new_pos.x;
		//tmpvar2 = 2;
		//TEST_MOVE;
		return 1;
	}
	//else
	//{
	//	OutputDebugString(format("--X-- (%g,%g)->(%g,%g)\n", _pos.x, _pos.z, new_pos.x, _pos.z));
	//}

	// id� po z
	tr.setOrigin(btVector3(_pos.x+sign(-_dir.x)*0.1f, origin.y(), new_pos.z));
	phy_world->updateSingleAabb(_me->cobj);
	if(!Collide(_me->cobj, global_bcol))
	{
		//OutputDebugString(format("Z (%g,%g)->(%g,%g)\n", _pos.x, _pos.z, _pos.x, new_pos.z));
		if(is_small)
			*is_small = (distance(_pos, new_pos) < SMALL_DISTANCE);
		_pos.z = new_pos.z;
		//_pos.x += sign(-_dir.x)*0.01f;
		//tmpvar2 = 3;
		//TEST_MOVE;
		return 2;
	}
	//else
	//{
	//	OutputDebugString(format("--Z-- (%g,%g)->(%g,%g)\n", _pos.x, _pos.z, _pos.x, new_pos.z));
	//}

	// nie ma drogi
	tr.setOrigin(origin);
	phy_world->updateSingleAabb(_me->cobj);
	return 0;*/

	global_col.clear();

	IgnoreObjects ignore = {0};
	Unit* ignored[] = {_me, NULL};
	ignore.ignored_units = (const Unit**)ignored;
	GatherCollisionObjects(GetContext(*_me), global_col, gather_pos, gather_radius, &ignore);

	if(global_col.empty())
	{
		if(is_small)
			*is_small = (distance(_pos, new_pos) < SMALL_DISTANCE);
		_pos = new_pos;
		return 3;
	}

	// id� prosto po x i z
	if(!Collide(global_col, new_pos, _radius))
	{
		if(is_small)
			*is_small = (distance(_pos, new_pos) < SMALL_DISTANCE);
		_pos = new_pos;
		return 3;
	}

	// id� po x
	VEC3 new_pos2 = _me->pos;
	new_pos2.x = new_pos.x;
	if(!Collide(global_col, new_pos2, _radius))
	{
		if(is_small)
			*is_small = (distance(_pos, new_pos2) < SMALL_DISTANCE);
		_pos = new_pos2;
		return 1;
	}

	// id� po z
	new_pos2.x = _me->pos.x;
	new_pos2.z = new_pos.z;
	if(!Collide(global_col, new_pos2, _radius))
	{
		if(is_small)
			*is_small = (distance(_pos, new_pos2) < SMALL_DISTANCE);
		_pos = new_pos2;
		return 2;
	}

	// nie ma drogi
	return 0;
}

int Game::CheckMovePhase(VEC3& _pos, const VEC3& _dir, float _radius, Unit* _me, bool* is_small)
{
	assert(_radius > 0.f && _me);

	VEC3 new_pos = _pos + _dir;
	VEC3 gather_pos = _pos + _dir/2;
	float gather_radius = D3DXVec3Length(&_dir) + _radius;

	global_col.clear();

	IgnoreObjects ignore = {0};
	Unit* ignored[] = {_me, NULL};
	ignore.ignored_units = (const Unit**)ignored;
	ignore.ignore_objects = true;
	GatherCollisionObjects(GetContext(*_me), global_col, gather_pos, gather_radius, &ignore);

	if(global_col.empty())
	{
		if(is_small)
			*is_small = (distance(_pos, new_pos) < SMALL_DISTANCE);
		_pos = new_pos;
		return 3;
	}

	// id� prosto po x i z
	if(!Collide(global_col, new_pos, _radius))
	{
		if(is_small)
			*is_small = (distance(_pos, new_pos) < SMALL_DISTANCE);
		_pos = new_pos;
		return 3;
	}

	// id� po x
	VEC3 new_pos2 = _me->pos;
	new_pos2.x = new_pos.x;
	if(!Collide(global_col, new_pos2, _radius))
	{
		if(is_small)
			*is_small = (distance(_pos, new_pos2) < SMALL_DISTANCE);
		_pos = new_pos2;
		return 1;
	}

	// id� po z
	new_pos2.x = _me->pos.x;
	new_pos2.z = new_pos.z;
	if(!Collide(global_col, new_pos2, _radius))
	{
		if(is_small)
			*is_small = (distance(_pos, new_pos2) < SMALL_DISTANCE);
		_pos = new_pos2;
		return 2;
	}

	// je�li zablokowa� si� w innej jednostce to wyjd� z niej
	if(Collide(global_col, _pos, _radius))
	{
		if(is_small)
			*is_small = (distance(_pos, new_pos) < SMALL_DISTANCE);
		_pos = new_pos;
		return 4;
	}

	// nie ma drogi
	return 0;
}

void Game::GatherCollisionObjects(LevelContext& ctx, vector<CollisionObject>& _objects, const VEC3& _pos, float _radius, const IgnoreObjects* ignore)
{
	assert(_radius > 0.f);

	// tiles
	int minx = max(0, int((_pos.x-_radius)/2)),
		maxx = int((_pos.x+_radius)/2),
		minz = max(0, int((_pos.z-_radius)/2)),
		maxz = int((_pos.z+_radius)/2);

	if((!ignore || !ignore->ignore_blocks) && ctx.type != LevelContext::Building)
	{
		if(location->outside)
		{
			City* city = (City*)location;
			TerrainTile* tiles = city->tiles;
			const uint _s = 16 * 8;
			maxx = min(maxx, (int)_s);
			maxz = min(maxz, (int)_s);

			for(int z=minz; z<=maxz; ++z)
			{
				for(int x=minx; x<=maxx; ++x)
				{
					if(tiles[x+z*_s].mode >= TM_BUILDING_BLOCK)
					{
						CollisionObject& co = Add1(_objects);
						co.pt = VEC2(2.f*x+1.f, 2.f*z+1.f);
						co.w = 1.f;
						co.h = 1.f;
						co.type = CollisionObject::RECTANGLE;
					}
				}
			}
		}
		else
		{
			InsideLocation* inside = (InsideLocation*)location;
			InsideLocationLevel& lvl = inside->GetLevelData();
			maxx = min(maxx, lvl.w);
			maxz = min(maxz, lvl.h);

			for(int z=minz; z<=maxz; ++z)
			{
				for(int x=minx; x<=maxx; ++x)
				{
					POLE co = lvl.mapa[x+z*lvl.w].co;
					if(czy_blokuje2(co))
					{
						CollisionObject& co = Add1(_objects);
						co.pt = VEC2(2.f*x+1.f, 2.f*z+1.f);
						co.w = 1.f;
						co.h = 1.f;
						co.type = CollisionObject::RECTANGLE;
					}
					else if(co == SCHODY_DOL)
					{
						if(!lvl.schody_dol_w_scianie)
						{
							CollisionObject& co = Add1(_objects);
							co.pt = VEC2(2.f*x+1.f, 2.f*z+1.f);
							co.check = &Game::CollideWithStairs;
							co.check_rect = &Game::CollideWithStairsRect;
							co.extra = 0;
							co.type = CollisionObject::CUSTOM;
						}
					}
					else if(co == SCHODY_GORA)
					{
						CollisionObject& co = Add1(_objects);
						co.pt = VEC2(2.f*x+1.f, 2.f*z+1.f);
						co.check = &Game::CollideWithStairs;
						co.check_rect = &Game::CollideWithStairsRect;
						co.extra = 1;
						co.type = CollisionObject::CUSTOM;
					}
				}
			}
		}
	}

	// units
	float radius;
	VEC3 pos;
	if(ignore && ignore->ignored_units)
	{
		for(vector<Unit*>::iterator it = ctx.units->begin(), end = ctx.units->end(); it != end; ++it)
		{
			if(!*it || !(*it)->IsStanding())
				continue;

			const Unit** u = ignore->ignored_units;
			do 
			{
				if(!*u)
					break;
				if(*u == *it)
					goto jest;
				++u;
			} while (1);

			radius = (*it)->GetUnitRadius();
			pos = (*it)->GetColliderPos();
			if(distance(pos.x, pos.z, _pos.x, _pos.z) <= radius+_radius)
			{
				CollisionObject& co = Add1(_objects);
				co.pt = VEC2(pos.x, pos.z);
				co.radius = radius;
				co.type = CollisionObject::SPHERE;
			}

jest:
			;
		}
	}
	else
	{
		for(vector<Unit*>::iterator it = ctx.units->begin(), end = ctx.units->end(); it != end; ++it)
		{
			if(!*it || !(*it)->IsStanding())
				continue;

			radius = (*it)->GetUnitRadius();
			VEC3 pos = (*it)->GetColliderPos();
			if(distance(pos.x, pos.z, _pos.x, _pos.z) <= radius+_radius)
			{
				CollisionObject& co = Add1(_objects);
				co.pt = VEC2(pos.x, pos.z);
				co.radius = radius;
				co.type = CollisionObject::SPHERE;
			}
		}
	}

	// obiekty kolizji
	if(ignore && ignore->ignore_objects)
	{
		// ignoruj obiekty
	}
	else if(!ignore || !ignore->ignored_objects)
	{
		for(vector<CollisionObject>::iterator it = ctx.colliders->begin(), end = ctx.colliders->end(); it != end; ++it)
		{
			if(it->type == CollisionObject::RECTANGLE)
			{
				if(CircleToRectangle(_pos.x, _pos.z, _radius, it->pt.x, it->pt.y, it->w, it->h))
					_objects.push_back(*it);
			}
			else
			{
				if(CircleToCircle(_pos.x, _pos.z, _radius, it->pt.x, it->pt.y, it->radius))
					_objects.push_back(*it);
			}
		}
	}
	else
	{
		for(vector<CollisionObject>::iterator it = ctx.colliders->begin(), end = ctx.colliders->end(); it != end; ++it)
		{
			if(it->ptr)
			{
				const void** objs = ignore->ignored_objects;
				do
				{
					if(it->ptr == *objs)
						goto ignoruj;
					else if(!*objs)
						break;
					else
						++objs;
				}
				while(1);
			}

			if(it->type == CollisionObject::RECTANGLE)
			{
				if(CircleToRectangle(_pos.x, _pos.z, _radius, it->pt.x, it->pt.y, it->w, it->h))
					_objects.push_back(*it);
			}
			else
			{
				if(CircleToCircle(_pos.x, _pos.z, _radius, it->pt.x, it->pt.y, it->radius))
					_objects.push_back(*it);
			}

ignoruj:
			;
		}
	}

	// drzwi
	if(ctx.doors && !ctx.doors->empty())
	{
		for(vector<Door*>::iterator it = ctx.doors->begin(), end = ctx.doors->end(); it != end; ++it)
		{
			if((*it)->IsBlocking() && CircleToRotatedRectangle(_pos.x, _pos.z, _radius, (*it)->pos.x, (*it)->pos.z, 0.842f, 0.181f, (*it)->rot))
			{
				CollisionObject& co = Add1(_objects);
				co.pt = VEC2((*it)->pos.x, (*it)->pos.z);
				co.type = CollisionObject::RECTANGLE_ROT;
				co.w = 0.842f;
				co.h = 0.181f;
				co.rot = (*it)->rot;
			}
		}
	}
}

void Game::GatherCollisionObjects(LevelContext& ctx, vector<CollisionObject>& _objects, const BOX2D& _box, const IgnoreObjects* ignore)
{
	// tiles
	int minx = max(0, int(_box.v1.x/2)),
		maxx = int(_box.v2.x/2),
		minz = max(0, int(_box.v1.y/2)),
		maxz = int(_box.v2.y/2);

	if((!ignore || !ignore->ignore_blocks) && ctx.type != LevelContext::Building)
	{
		if(location->outside)
		{
			City* city = (City*)location;
			TerrainTile* tiles = city->tiles;
			const uint _s = 16 * 8;
			maxx = min(maxx, (int)_s);
			maxz = min(maxz, (int)_s);

			for(int z=minz; z<=maxz; ++z)
			{
				for(int x=minx; x<=maxx; ++x)
				{
					if(tiles[x+z*_s].mode >= TM_BUILDING_BLOCK)
					{
						CollisionObject& co = Add1(_objects);
						co.pt = VEC2(2.f*x+1.f, 2.f*z+1.f);
						co.w = 1.f;
						co.h = 1.f;
						co.type = CollisionObject::RECTANGLE;
					}
				}
			}
		}
		else
		{
			InsideLocation* inside = (InsideLocation*)location;
			InsideLocationLevel& lvl = inside->GetLevelData();
			maxx = min(maxx, lvl.w);
			maxz = min(maxz, lvl.h);

			for(int z=minz; z<=maxz; ++z)
			{
				for(int x=minx; x<=maxx; ++x)
				{
					POLE co = lvl.mapa[x+z*lvl.w].co;
					if(czy_blokuje2(co))
					{
						CollisionObject& co = Add1(_objects);
						co.pt = VEC2(2.f*x+1.f, 2.f*z+1.f);
						co.w = 1.f;
						co.h = 1.f;
						co.type = CollisionObject::RECTANGLE;
					}
					else if(co == SCHODY_DOL)
					{
						if(!lvl.schody_dol_w_scianie)
						{
							CollisionObject& co = Add1(_objects);
							co.pt = VEC2(2.f*x+1.f, 2.f*z+1.f);
							co.check = &Game::CollideWithStairs;
							co.check_rect = &Game::CollideWithStairsRect;
							co.extra = 0;
							co.type = CollisionObject::CUSTOM;
						}
					}
					else if(co == SCHODY_GORA)
					{
						CollisionObject& co = Add1(_objects);
						co.pt = VEC2(2.f*x+1.f, 2.f*z+1.f);
						co.check = &Game::CollideWithStairs;
						co.check_rect = &Game::CollideWithStairsRect;
						co.extra = 1;
						co.type = CollisionObject::CUSTOM;
					}
				}
			}
		}
	}

	VEC2 rectpos = _box.Midpoint(),
		 rectsize = _box.Size();

	// units
	float radius;
	if(ignore && ignore->ignored_units)
	{
		for(vector<Unit*>::iterator it = ctx.units->begin(), end = ctx.units->end(); it != end; ++it)
		{
			if(!(*it)->IsStanding())
				continue;

			const Unit** u = ignore->ignored_units;
			do 
			{
				if(!*u)
					break;
				if(*u == *it)
					goto jest;
				++u;
			} while (1);

			radius = (*it)->GetUnitRadius();
			if(CircleToRectangle((*it)->pos.x, (*it)->pos.z, radius, rectpos.x, rectpos.y, rectsize.x, rectsize.y))
			{
				CollisionObject& co = Add1(_objects);
				co.pt = VEC2((*it)->pos.x, (*it)->pos.z);
				co.radius = radius;
				co.type = CollisionObject::SPHERE;
			}

jest:
			;
		}
	}
	else
	{
		for(vector<Unit*>::iterator it = ctx.units->begin(), end = ctx.units->end(); it != end; ++it)
		{
			if(!(*it)->IsStanding())
				continue;

			radius = (*it)->GetUnitRadius();
			if(CircleToRectangle((*it)->pos.x, (*it)->pos.z, radius, rectpos.x, rectpos.y, rectsize.x, rectsize.y))
			{
				CollisionObject& co = Add1(_objects);
				co.pt = VEC2((*it)->pos.x, (*it)->pos.z);
				co.radius = radius;
				co.type = CollisionObject::SPHERE;
			}
		}
	}

	// obiekty kolizji
	if(ignore && ignore->ignore_objects)
	{
		// ignoruj obiekty
	}
	else if(!ignore || !ignore->ignored_objects)
	{
		for(vector<CollisionObject>::iterator it = ctx.colliders->begin(), end = ctx.colliders->end(); it != end; ++it)
		{
			if(it->type == CollisionObject::RECTANGLE)
			{
				if(RectangleToRectangle(it->pt.x-it->w, it->pt.y-it->h, it->pt.x+it->w, it->pt.y+it->h, _box.v1.x, _box.v1.y, _box.v2.x, _box.v2.y))
					_objects.push_back(*it);
			}
			else
			{
				if(CircleToRectangle(it->pt.x, it->pt.y, it->radius, rectpos.x, rectpos.y, rectsize.x, rectsize.y))
					_objects.push_back(*it);
			}
		}
	}
	else
	{
		for(vector<CollisionObject>::iterator it = ctx.colliders->begin(), end = ctx.colliders->end(); it != end; ++it)
		{
			if(it->ptr)
			{
				const void** objs = ignore->ignored_objects;
				do
				{
					if(it->ptr == *objs)
						goto ignoruj;
					else if(!*objs)
						break;
					else
						++objs;
				}
				while(1);
			}

			if(it->type == CollisionObject::RECTANGLE)
			{
				if(RectangleToRectangle(it->pt.x-it->w, it->pt.y-it->h, it->pt.x+it->w, it->pt.y+it->h, _box.v1.x, _box.v1.y, _box.v2.x, _box.v2.y))
					_objects.push_back(*it);
			}
			else
			{
				if(CircleToRectangle(it->pt.x, it->pt.y, it->radius, rectpos.x, rectpos.y, rectsize.x, rectsize.y))
					_objects.push_back(*it);
			}

ignoruj:
			;
		}
	}
}

bool Game::Collide(const vector<CollisionObject>& _objects, const VEC3& _pos, float _radius)
{
	assert(_radius > 0.f);

	for(vector<CollisionObject>::const_iterator it = _objects.begin(), end = _objects.end(); it != end; ++it)
	{
		switch(it->type)
		{
		case CollisionObject::SPHERE:
			if(distance(_pos.x, _pos.z, it->pt.x, it->pt.y) <= it->radius+_radius)
				return true;
			break;
		case CollisionObject::RECTANGLE:
			if(CircleToRectangle(_pos.x, _pos.z, _radius, it->pt.x, it->pt.y, it->w, it->h))
				return true;
			break;
		case CollisionObject::RECTANGLE_ROT:
			if(CircleToRotatedRectangle(_pos.x, _pos.z, _radius, it->pt.x, it->pt.y, it->w, it->h, it->rot))
				return true;
			break;
		case CollisionObject::CUSTOM:
			if((this->*(it->check))(*it, _pos, _radius))
				return true;
			break;
		}
	}

	return false;
}

struct AnyContactCallback : public btCollisionWorld::ContactResultCallback
{
	bool hit;

	AnyContactCallback() : hit(false)
	{

	}

	btScalar addSingleResult(btManifoldPoint& cp, const btCollisionObjectWrapper* colObj0Wrap, int partId0, int index0, const btCollisionObjectWrapper* colObj1Wrap, int partId1, int index1)
	{
		hit = true;
		return 0.f;
	}
};

bool Game::Collide(const vector<CollisionObject>& _objects, const BOX2D& _box, float _margin)
{
	BOX2D box = _box;
	box.v1.x -= _margin;
	box.v1.y -= _margin;
	box.v2.x += _margin;
	box.v2.y += _margin;

	VEC2 rectpos = _box.Midpoint(),
		 rectsize = _box.Size()/2;

	for(vector<CollisionObject>::const_iterator it = _objects.begin(), end = _objects.end(); it != end; ++it)
	{
		switch(it->type)
		{
		case CollisionObject::SPHERE:
			if(CircleToRectangle(it->pt.x, it->pt.y, it->radius+_margin, rectpos.x, rectpos.y, rectsize.x, rectsize.y))
				return true;
			break;
		case CollisionObject::RECTANGLE:
			if(RectangleToRectangle(box.v1.x, box.v1.y, box.v2.x, box.v2.y, it->pt.x-it->w, it->pt.y-it->h, it->pt.x+it->w, it->pt.y+it->h))
				return true;
			break;
		case CollisionObject::RECTANGLE_ROT:
			{
				RotRect r1, r2;
				r1.center = it->pt;
				r1.size.x = it->w+_margin;
				r1.size.y = it->h+_margin;
				r1.rot = -it->rot;
				r2.center = rectpos;
				r2.size = rectsize;
				r2.rot = 0.f;

				if(RotatedRectanglesCollision(r1, r2))
					return true;
			}
			break;
		case CollisionObject::CUSTOM:
			if((this->*(it->check_rect))(*it, box))
				return true;
			break;
		}
	}

	return false;
}

bool Game::Collide(const vector<CollisionObject>& _objects, const BOX2D& _box, float margin, float _rot)
{
	if(!not_zero(_rot))
		return Collide(_objects, _box, margin);

	BOX2D box = _box;
	box.v1.x -= margin;
	box.v1.y -= margin;
	box.v2.x += margin;
	box.v2.y += margin;

	VEC2 rectpos = box.Midpoint(),
		 rectsize = box.Size()/2;

	for(vector<CollisionObject>::const_iterator it = _objects.begin(), end = _objects.end(); it != end; ++it)
	{
		switch(it->type)
		{
		case CollisionObject::SPHERE:
			if(CircleToRotatedRectangle(it->pt.x, it->pt.y, it->radius, rectpos.x, rectpos.y, rectsize.x, rectsize.y, _rot))
				return true;
			break;
		case CollisionObject::RECTANGLE:
			{
				RotRect r1, r2;
				r1.center = it->pt;
				r1.size.x = it->w;
				r1.size.y = it->h;
				r1.rot = 0.f;
				r2.center = rectpos;
				r2.size = rectsize;
				r2.rot = _rot;

				if(RotatedRectanglesCollision(r1, r2))
					return true;
			}
			break;
		case CollisionObject::RECTANGLE_ROT:
			{
				RotRect r1, r2;
				r1.center = it->pt;
				r1.size.x = it->w;
				r1.size.y = it->h;
				r1.rot = it->rot;
				r2.center = rectpos;
				r2.size = rectsize;
				r2.rot = _rot;

				if(RotatedRectanglesCollision(r1, r2))
					return true;
			}
			break;
		case CollisionObject::CUSTOM:
			if((this->*(it->check_rect))(*it, box))
				return true;
			break;
		}
	}

	return false;
}

void Game::AddConsoleMsg(cstring _msg)
{
	console->AddText(_msg);
}

void Game::StartDialog(DialogContext& ctx, Unit* talker, DialogEntry* dialog)
{
	assert(talker);

	ctx.dialog_mode = true;
	ctx.dialog_wait = -1;
	ctx.dialog_pos = 0;
	ctx.show_choices = false;
	ctx.dialog_text = NULL;
	ctx.dialog_level = 0;
	ctx.dialog_once = true;
	ctx.dialog_quest = NULL;
	ctx.dialog_skip = -1;
	ctx.dialog_esc = -1;
	ctx.prev_dialog = NULL;
	ctx.talker = talker;
	ctx.talker->busy = Unit::Busy_Talking;
	ctx.talker->look_target = ctx.pc->unit;
	ctx.update_news = true;
	ctx.pc->action = PlayerController::Action_Talk;
	ctx.pc->action_unit = talker;

	if(dialog)
		ctx.dialog = dialog;
	else
		ctx.dialog = talker->data->dialog;

	if(IsLocal())
	{
		// d�wi�k powitania
		SOUND snd = GetTalkSound(*ctx.talker);
		if(snd)
		{
			if(sound_volume)
				PlayAttachedSound(*ctx.talker, snd, 2.f, 5.f);
			if(IsOnline() && IsServer())
			{
				NetChange& c = Add1(net_changes);
				c.type = NetChange::HELLO;
				c.unit = ctx.talker;
			}
		}
	}

	if(ctx.is_local)
	{
		// zamknij gui
		CloseAllPanels();
	}
}

void Game::EndDialog(DialogContext& ctx)
{
	ctx.choices.clear();
	ctx.dialog_mode = false;

	if(ctx.talker->busy == Unit::Busy_Trading)
	{
		if(!ctx.is_local)
		{
			NetChangePlayer& c = Add1(net_changes_player);
			c.type = NetChangePlayer::END_DIALOG;
			c.pc = ctx.pc;
			GetPlayerInfo(c.pc->id).NeedUpdate();
		}

		return;
	}

	ctx.talker->busy = Unit::Busy_No;
	ctx.talker->look_target = NULL;
	ctx.pc->action = PlayerController::Action_None;

	if(ctx.is_local)
	{
		if(ctx.next_talker)
		{
			ctx.dialog_mode = true;
			Unit* t = ctx.next_talker;
			ctx.next_talker = NULL;
			t->auto_talk = 0;
			StartDialog(ctx, t, ctx.next_dialog);
		}
	}
	else
	{
		NetChangePlayer& c = Add1(net_changes_player);
		c.type = NetChangePlayer::END_DIALOG;
		c.pc = ctx.pc;
		GetPlayerInfo(c.pc->id).NeedUpdate();

		if(ctx.next_talker)
		{
			Unit* t = ctx.next_talker;
			ctx.next_talker = NULL;
			t->auto_talk = 0;
			StartDialog2(c.pc, t, ctx.next_dialog);
		}
	}
}

//							WEAPON	BOW		SHIELD	ARMOR	LETTER	POTION	GOLD	OTHER
bool merchant_buy[]	  =	{	true,	true,	true,	true,	true,	true,	false,	true	};
bool blacksmith_buy[] =	{	true,	true,	true,	true,	false,	false,	false,	false	};
bool alchemist_buy[]  = {	false,	false,	false,	false,	false,	true,	false,	false	};
bool innkeeper_buy[]  = {	false,	false,	false,	false,	false,	true,	false,	false	};
bool foodseller_buy[] = {	false,	false,	false,	false,	false,	true,	false,	false	};

void Game::UpdateGameDialog(DialogContext& ctx, float dt)
{
	current_dialog = &ctx;

	// wy�wietlono opcje dialogowe, wybierz jedn� z nich (w mp czekaj na wyb�r)
	if(ctx.show_choices)
	{
		bool ok = false;
		if(!ctx.is_local)
		{
			if(ctx.choice_selected != -1)
				ok = true;
		}
		else
			ok = game_gui->UpdateChoice(ctx, ctx.choices.size());

		if(ok)
		{
			cstring msg = ctx.choices[ctx.choice_selected].msg;
			game_gui->AddSpeechBubble(ctx.pc->unit, msg);

			if(IsOnline())
			{
				NetChange& c = Add1(net_changes);
				c.type = NetChange::TALK;
				c.unit = ctx.pc->unit;
				c.str = StringPool.Get();
				*c.str = msg;
				c.id = 0;
				c.ile = 0;
				net_talk.push_back(c.str);
			}

			ctx.show_choices = false;
			ctx.dialog_pos = ctx.choices[ctx.choice_selected].pos;
			ctx.dialog_level = ctx.choices[ctx.choice_selected].lvl;
			ctx.choices.clear();
			ctx.choice_selected = -1;
			ctx.dialog_esc = -1;
		}
		else
			return;
	}

	if(ctx.dialog_wait > 0.f)
	{
		if(ctx.is_local)
		{
			if(KeyPressedReleaseAllowed(GK_SELECT_DIALOG) || KeyPressedReleaseAllowed(GK_SKIP_DIALOG) || KeyPressedReleaseAllowed(GK_ATTACK_USE) ||
				(AllowKeyboard() && Key.PressedRelease(VK_ESCAPE)))
				ctx.dialog_wait = -1.f;
			else
				ctx.dialog_wait -= dt;
		}
		else
		{
			if(ctx.choice_selected == 1)
			{
				ctx.dialog_wait = -1.f;
				ctx.choice_selected = -1;
			}
			else
				ctx.dialog_wait -= dt;
		}

		if(ctx.dialog_wait > 0.f)
			return;
	}
	
	if(ctx.dialog_skip != -1)
	{
		ctx.dialog_pos = ctx.dialog_skip;
		ctx.dialog_skip = -1;
	}

	int if_level = ctx.dialog_level;

	while(1)
	{
		DialogEntry& de = *(ctx.dialog+ctx.dialog_pos);

		switch(de.type)
		{
		case DT_CHOICE:
			if(if_level == ctx.dialog_level)
			{
				cstring text = txDialog[(int)de.msg];
				if(text[0] == '$')
				{
					if(strncmp(text, "$player", 7) == 0)
					{
						int id = int(text[7]-'1');
						ctx.choices.push_back(DialogChoice(ctx.dialog_pos+1, near_players_str[id].c_str(), ctx.dialog_level+1));
					}
					else
						ctx.choices.push_back(DialogChoice(ctx.dialog_pos+1, "!Broken choice!", ctx.dialog_level+1));
				}
				else
					ctx.choices.push_back(DialogChoice(ctx.dialog_pos+1, text, ctx.dialog_level+1));
			}
			++if_level;
			break;
		case DT_END_CHOICE:
		case DT_END_IF:
			if(if_level == ctx.dialog_level)
				--ctx.dialog_level;
			--if_level;
			break;
		case DT_END:
			if(if_level == ctx.dialog_level)
			{
				if(!ctx.prev_dialog)
				{
					EndDialog(ctx);
					return;
				}
				else
				{
					ctx.dialog = ctx.prev_dialog;
					ctx.dialog_pos = ctx.prev_dialog_pos;
					ctx.dialog_level = ctx.prev_dialog_level;
					if_level = ctx.dialog_level;
					ctx.prev_dialog = NULL;
					ctx.dialog_quest = NULL;
				}
			}
			break;
		case DT_END2:
			if(if_level == ctx.dialog_level)
			{
				EndDialog(ctx);
				return;
			}
			break;
		case DT_SHOW_CHOICES:
			if(if_level == ctx.dialog_level)
			{
				ctx.show_choices = true;
				if(ctx.is_local)
				{
					ctx.choice_selected = 0;
					dialog_cursor_pos = INT2(-1,-1);
					game_gui->UpdateScrollbar(ctx.choices.size());
				}
				else
				{
					ctx.choice_selected = -1;
					NetChangePlayer& c = Add1(net_changes_player);
					c.type = NetChangePlayer::SHOW_CHOICES;
					c.pc = ctx.pc;
					GetPlayerInfo(c.pc->id).NeedUpdate();
				}
				return;
			}
			break;
		case DT_RESTART:
			if(if_level == ctx.dialog_level)
				ctx.dialog_pos = -1;
			break;
		case DT_TRADE:
			if(if_level == ctx.dialog_level)
			{
				Unit* t = ctx.talker;
				t->busy = Unit::Busy_Trading;
				EndDialog(ctx);
				ctx.pc->action = PlayerController::Action_Trade;
				ctx.pc->action_unit = t;
				bool* old_trader_buy = trader_buy;

				if(strcmp(t->data->id, "blacksmith") == 0)
				{
					ctx.pc->chest_trade = &chest_blacksmith;
					trader_buy = blacksmith_buy;
				}
				else if(strcmp(t->data->id, "merchant") == 0 || strcmp(t->data->id, "tut_czlowiek") == 0)
				{
					ctx.pc->chest_trade = &chest_merchant;
					trader_buy = merchant_buy;
				}
				else if(strcmp(t->data->id, "alchemist") == 0)
				{
					ctx.pc->chest_trade = &chest_alchemist;
					trader_buy = alchemist_buy;
				}
				else if(strcmp(t->data->id, "innkeeper") == 0)
				{
					ctx.pc->chest_trade = &chest_innkeeper;
					trader_buy = innkeeper_buy;
				}
				else if(strcmp(t->data->id, "q_orkowie_kowal") == 0)
				{
					ctx.pc->chest_trade = &orkowie_towar;
					trader_buy = blacksmith_buy;
				}
				else if(strcmp(t->data->id, "food_seller") == 0)
				{
					ctx.pc->chest_trade = &chest_food_seller;
					trader_buy = foodseller_buy;
				}
				else
				{
					assert(0);
					return;
				}

				if(ctx.is_local)
				{
					CloseGamePanels();
					inventory_mode = I_TRADE;
					BuildTmpInventory(0);
					inv_trade_mine->mode = Inventory::TRADE_MY;
					BuildTmpInventory(1);
					inv_trade_other->unit = t;
					inv_trade_other->items = ctx.pc->chest_trade;
					inv_trade_other->slots = NULL;
					inv_trade_other->title = Format("%s - %s", inv_trade_other->txTrading, t->GetName());
					inv_trade_other->mode = Inventory::TRADE_OTHER;
					gp_trade->Show();
				}
				else
				{
					NetChangePlayer& c = Add1(net_changes_player);
					c.type = NetChangePlayer::START_TRADE;
					c.pc = ctx.pc;
					c.id = t->netid;
					GetPlayerInfo(c.pc->id).NeedUpdate();

					trader_buy = old_trader_buy;
				}
				
				return;
			}
			break;
		case DT_TALK:
			if(ctx.dialog_level == if_level)
			{
				DialogTalk(ctx, txDialog[(int)de.msg]);
				++ctx.dialog_pos;
				return;
			}
			break;
		case DT_TALK2:
			if(ctx.dialog_level == if_level)
			{
				static string str_part;
				ctx.dialog_s_text.clear();
				cstring text = txDialog[(int)de.msg];
				for(uint i=0, len = strlen(text); i < len; ++i)
				{
					if(text[i] == '$')
					{
						str_part.clear();
						++i;
						while(text[i] != '$')
						{
							str_part.push_back(text[i]);
							++i;
						}
						if(ctx.dialog_quest)
							ctx.dialog_s_text += ctx.dialog_quest->FormatString(str_part);
						else
							ctx.dialog_s_text += FormatString(ctx, str_part);
					}
					else
						ctx.dialog_s_text.push_back(text[i]);
				}

				DialogTalk(ctx, ctx.dialog_s_text.c_str());
				++ctx.dialog_pos;
				return;
			}
			break;
		case DT_SPECIAL:
			if(ctx.dialog_level == if_level)
			{
				if(strcmp(de.msg, "burmistrz_quest") == 0)
				{
					if(city_ctx->quest_burmistrz == 2)
					{
						DialogTalk(ctx, random_string(txMayorQFailed));
						++ctx.dialog_pos;
						return;
					}
					else if(worldtime - city_ctx->quest_burmistrz_czas > 30 || city_ctx->quest_burmistrz_czas == -1)
					{
						if(city_ctx->quest_burmistrz == 1)
						{
							Quest* quest = FindUnacceptedQuest(current_location, 0);
							DeleteElement(unaccepted_quests, quest);
						}

						// jest nowe zadanie (mo�e), czas starego min��
						int co = rand2()%12;
						city_ctx->quest_burmistrz_czas = worldtime;
						city_ctx->quest_burmistrz = 1;

						Quest* quest;

						switch(co)
						{
						case 0:
						case 1:
						case 2:
							// dostarcz list
							quest = new Quest_DostarczList;
							break;
						case 3:
						case 4:
						case 5:
							// dostarcz przesy�k�
							quest = new Quest_DostarczPaczke;
							break;
						case 6:
						case 7:
							// roznie� wie�ci
							quest = new Quest_RozniesWiesci;
							break;
						case 8:
						case 9:
							// odzyskaj paczk�
							quest = new Quest_OdzyskajPaczke;
							break;
						case 10:
						case 11:
						default:
							// nic
							goto brak_questa;
						}

						quest->refid = quest_counter;
						++quest_counter;
						quest->Start();
						unaccepted_quests.push_back(quest);
						ctx.prev_dialog = ctx.dialog;
						ctx.prev_dialog_pos = ctx.dialog_pos;
						ctx.dialog = quest->GetDialog(QUEST_DIALOG_START);
						ctx.dialog_pos = -1;
						ctx.dialog_quest = quest;
					}
					else if(city_ctx->quest_burmistrz == 1)
					{
						// ju� ma przydzielone zadanie ?
						Quest* quest = FindUnacceptedQuest(current_location, 0);
						if(quest)
						{
							// quest nie zosta� zaakceptowany
							ctx.prev_dialog = ctx.dialog;
							ctx.prev_dialog_pos = ctx.dialog_pos;
							ctx.dialog = quest->GetDialog(QUEST_DIALOG_START);
							ctx.dialog_pos = -1;
							ctx.dialog_quest = quest;
						}
						else
						{
							quest = FindQuest(current_location, 0);
							if(!quest)
								goto brak_questa;
							DialogTalk(ctx, random_string(txQuestAlreadyGiven));
							++ctx.dialog_pos;
							return;
						}
					}
					else
					{
brak_questa:
						DialogTalk(ctx, random_string(txMayorNoQ));
						++ctx.dialog_pos;
						return;
					}
				}
				else if(strcmp(de.msg, "dowodca_quest") == 0)
				{
					if(city_ctx->quest_dowodca == 2)
					{
						DialogTalk(ctx, random_string(txCaptainQFailed));
						++ctx.dialog_pos;
						return;
					}
					else if(worldtime - city_ctx->quest_dowodca_czas > 30 || city_ctx->quest_dowodca_czas == -1)
					{
						if(city_ctx->quest_dowodca == 1)
						{
							Quest* quest = FindUnacceptedQuest(current_location, 1);
							DeleteElement(unaccepted_quests, quest);
						}

						// jest nowe zadanie (mo�e), czas starego min��
						int co = rand2()%11;
						city_ctx->quest_dowodca_czas = worldtime;
						city_ctx->quest_dowodca = 1;

						Quest* quest;

						switch(co)
						{
						case 0:
						case 1:
							quest = new Quest_UratujPorwanaOsobe;
							break;
						case 2:
						case 3:
							quest = new Quest_BandyciPobierajaOplate;
							break;
						case 4:
						case 5:
							quest = new Quest_ObozKoloMiasta;
							break;
						case 6:
						case 7:
							quest = new Quest_ZabijZwierzeta;
							break;
						case 8:
						case 9:
							quest = new Quest_ListGonczy;
							break;
						case 10:
						default:
							// nic
							goto brak_questa2;
						}

						quest->refid = quest_counter;
						++quest_counter;
						quest->Start();
						unaccepted_quests.push_back(quest);
						ctx.prev_dialog = ctx.dialog;
						ctx.prev_dialog_pos = ctx.dialog_pos;
						ctx.dialog = quest->GetDialog(QUEST_DIALOG_START);
						ctx.dialog_pos = -1;
						ctx.dialog_quest = quest;
					}
					else if(city_ctx->quest_dowodca == 1)
					{
						// ju� ma przydzielone zadanie
						Quest* quest = FindUnacceptedQuest(current_location, 1);
						if(quest)
						{
							// quest nie zosta� zaakceptowany
							ctx.prev_dialog = ctx.dialog;
							ctx.prev_dialog_pos = ctx.dialog_pos;
							ctx.dialog = quest->GetDialog(QUEST_DIALOG_START);
							ctx.dialog_pos = -1;
							ctx.dialog_quest = quest;
						}
						else
						{
							quest = FindQuest(current_location, 1);
							if(!quest)
								goto brak_questa2;
							DialogTalk(ctx, random_string(txQuestAlreadyGiven));
							++ctx.dialog_pos;
							return;
						}
					}
					else
					{
brak_questa2:
						DialogTalk(ctx, random_string(txCaptainNoQ));
						++ctx.dialog_pos;
						return;
					}
				}
				else if(strcmp(de.msg, "przedmiot_quest") == 0)
				{
					if(ctx.talker->quest_refid == -1)
					{
						Quest* quest;

						switch(rand2()%3)
						{
						case 0:
						default:
							quest = new Quest_ZnajdzArtefakt;
							break;
						case 1:
							quest = new Quest_ZgubionyPrzedmiot;
							break;
						case 2:
							quest = new Quest_UkradzionyPrzedmiot;
							break;
						}

						quest->refid = quest_counter;
						ctx.talker->quest_refid = quest_counter;
						++quest_counter;
						quest->Start();
						unaccepted_quests.push_back(quest);
						ctx.prev_dialog = ctx.dialog;
						ctx.prev_dialog_pos = ctx.dialog_pos;
						ctx.dialog = quest->GetDialog(QUEST_DIALOG_START);
						ctx.dialog_pos = -1;
						ctx.dialog_quest = quest;
					}
					else
					{
						Quest* quest = FindUnacceptedQuest(ctx.talker->quest_refid);
						ctx.prev_dialog = ctx.dialog;
						ctx.prev_dialog_pos = ctx.dialog_pos;
						ctx.dialog = quest->GetDialog(QUEST_DIALOG_START);
						ctx.dialog_pos = -1;
						ctx.dialog_quest = quest;
					}
				}
				else if(strcmp(de.msg, "arena_combat1") == 0 || strcmp(de.msg, "arena_combat2") == 0 || strcmp(de.msg, "arena_combat3") == 0)
					StartArenaCombat(de.msg[12]-'0');
				else if(strcmp(de.msg, "rest1") == 0 || strcmp(de.msg, "rest5") == 0 || strcmp(de.msg, "rest10") == 0 || strcmp(de.msg, "rest30") == 0)
				{
					// odpoczynek w karczmie
					// ustal ile to dni i ile kosztuje
					int ile, koszt;
					if(strcmp(de.msg, "rest1") == 0)
					{
						ile = 1;
						koszt = 5;
					}
					else if(strcmp(de.msg, "rest5") == 0)
					{
						ile = 5;
						koszt = 20;
					}
					else if(strcmp(de.msg, "rest10") == 0)
					{
						ile = 10;
						koszt = 35;
					}
					else
					{
						ile = 30;
						koszt = 100;
					}

					// czy gracz ma z�oto?
					if(ctx.pc->unit->gold < koszt)
					{
						ctx.dialog_s_text = Format(txNeedMoreGold, koszt-ctx.pc->unit->gold);
						DialogTalk(ctx, ctx.dialog_s_text.c_str());
						// resetuj dialog
						ctx.dialog_pos = 0;
						ctx.dialog_level = 0;
						return;
					}

					// zabierz z�oto i zamr�
					ctx.pc->unit->gold -= koszt;
					ctx.pc->unit->frozen = 2;
					if(ctx.is_local)
					{
						// lokalny fallback
						fallback_co = FALLBACK_REST;
						fallback_t = -1.f;
						fallback_1 = ile;
					}
					else
					{
						// wy�lij info o odpoczynku
						NetChangePlayer& c = Add1(net_changes_player);
						c.type = NetChangePlayer::REST;
						c.pc = ctx.pc;
						c.id = ile;
						GetPlayerInfo(ctx.pc).NeedUpdateAndGold();
					}
				}
				else if(strcmp(de.msg, "gossip") == 0 || strcmp(de.msg, "gossip_drunk") == 0)
				{
					bool dbg = false;
#ifdef _DEBUG
					if(ctx.is_local)
						dbg = true;
#endif
					bool pijak = (strcmp(de.msg, "gossip_drunk") == 0);
					if(!pijak && (rand2()%3 == 0 || (Key.Down(VK_SHIFT) && dbg)))
					{
						int co = rand2()%3;
						if(ile_plotek_questowych && rand2()%2 == 0)
							co = 2;
						if(dbg)
						{
							if(Key.Down('1'))
								co = 0;
							else if(Key.Down('2'))
								co = 1;
							else if(Key.Down('3'))
								co = 2;
						}
						switch(co)
						{
						case 0:
							// info o nieznanej lokacji
							{
								int id = rand2()%locations.size();
								int id2 = id;
								bool ok = false;
								do
								{
									if(locations[id2] && locations[id2]->state == LS_UNKNOWN)
									{
										ok = true;
										break;
									}
									else
									{
										++id2;
										if(id2 == locations.size())
											id2 = 0;
									}
								}
								while(id != id2);
								if(ok)
								{
									Location& loc = *locations[id2];
									loc.state = LS_KNOWN;
									Location& cloc = *locations[current_location];
									cstring s_daleko;
									float dist = distance(loc.pos, cloc.pos);
									if(dist <= 300)
										s_daleko = txNear;
									else if(dist <= 500)
										s_daleko = "";
									else if(dist <= 700)
										s_daleko = txFar;
									else
										s_daleko = txVeryFar;

									ctx.dialog_s_text = Format(random_string(txLocationDiscovered), s_daleko, GetLocationDirName(cloc.pos, loc.pos), loc.name.c_str());
									DialogTalk(ctx, ctx.dialog_s_text.c_str());
									++ctx.dialog_pos;

									if(IsOnline())
										Net_ChangeLocationState(id2, false);
									return;
								}
								else
								{
									DialogTalk(ctx, random_string(txAllDiscovered));
									++ctx.dialog_pos;
									return;
								}
							}
							break;
						case 1:
							// info o obozie
							{
								Location* new_camp = NULL;
								static vector<Location*> camps;
								for(vector<Location*>::iterator it = locations.begin(), end = locations.end(); it != end; ++it)
								{
									if(*it && (*it)->type == L_CAMP)
									{
										camps.push_back(*it);
										if((*it)->state == LS_UNKNOWN && !new_camp)
											new_camp = *it;
									}
								}

								if(!new_camp && !camps.empty())
									new_camp = camps[rand2()%camps.size()];

								camps.clear();

								if(new_camp)
								{
									Location& loc = *new_camp;
									Location& cloc = *locations[current_location];
									cstring s_daleko;
									float dist = distance(loc.pos, cloc.pos);
									if(dist <= 300)
										s_daleko = txNear;
									else if(dist <= 500)
										s_daleko = "";
									else if(dist <= 700)
										s_daleko = txFar;
									else
										s_daleko = txVeryFar;

									cstring co;
									switch(loc.spawn)
									{
									case SG_ORKOWIE:
										co = txSGOOrcs;
										break;
									case SG_BANDYCI:
										co = txSGOBandits;
										break;
									case SG_GOBLINY:
										co = txSGOGoblins;
										break;
									default:
										assert(0);
										co = txSGOEnemies;
										break;
									}

									if(loc.state == LS_UNKNOWN)
									{
										loc.state = LS_KNOWN;
										if(IsOnline())
											Net_ChangeLocationState(FindLocationId(new_camp), false);
									}

									ctx.dialog_s_text = Format(random_string(txCampDiscovered), s_daleko, GetLocationDirName(cloc.pos, loc.pos), co);
									DialogTalk(ctx, ctx.dialog_s_text.c_str());
									++ctx.dialog_pos;
									return;
								}
								else
								{
									DialogTalk(ctx, random_string(txAllCampDiscovered));
									++ctx.dialog_pos;
									return;
								}
							}
							break;
						case 2:
							// plotka o quescie
							if(ile_plotek_questowych == 0)
							{
								DialogTalk(ctx, random_string(txNoQRumors));
								++ctx.dialog_pos;
								return;
							}
							else
							{
								co = rand2()%P_MAX;
								while(plotka_questowa[co])
									co = (co+1)%P_MAX;
								--ile_plotek_questowych;
								plotka_questowa[co] = true;

								switch(co)
								{
								case P_TARTAK:
									ctx.dialog_s_text = Format(txRumorQ[0],	locations[tartak_miasto]->name.c_str());
									break;
								case P_KOPALNIA:
									ctx.dialog_s_text = Format(txRumorQ[1], locations[kopalnia_miasto]->name.c_str());
									break;
								case P_ZAWODY_W_PICIU:
									ctx.dialog_s_text = txRumorQ[2];
									break;
								case P_BANDYCI:
									ctx.dialog_s_text = Format(txRumorQ[3], locations[bandyci_miasto]->name.c_str());
									break;
								case P_MAGOWIE:
									ctx.dialog_s_text = Format(txRumorQ[4], locations[magowie_miasto]->name.c_str());
									break;
								case P_MAGOWIE2:
									ctx.dialog_s_text = txRumorQ[5];
									break;
								case P_ORKOWIE:
									ctx.dialog_s_text = Format(txRumorQ[6], locations[orkowie_miasto]->name.c_str());
									break;
								case P_GOBLINY:
									ctx.dialog_s_text = Format(txRumorQ[7], locations[gobliny_miasto]->name.c_str());
									break;
								case P_ZLO:
									ctx.dialog_s_text = Format(txRumorQ[8], locations[zlo_miasto]->name.c_str());
									break;
								default:
									assert(0);
									return;
								}

								if(IsOnline())
								{
									NetChange& c = Add1(net_changes);
									c.type = NetChange::ADD_RUMOR;
									c.id = plotki.size();
								}

								plotki.push_back(Format(journal->txAddTime, day+1, month+1, year, ctx.dialog_s_text.c_str()));
								journal->NeedUpdate(Journal::Rumors);
								AddGameMsg3(GMS_ADDED_RUMOR);
								DialogTalk(ctx, ctx.dialog_s_text.c_str());
								++ctx.dialog_pos;
								return;
							}
							break;
						default:
							assert(0);
							break;
						}
					}
					else
					{
						int ile = countof(txRumor);
						if(pijak)
							ile += countof(txRumorD);
						cstring plotka;
						do
						{
							int co = rand2()%ile;
							if(co < countof(txRumor))
								plotka = txRumor[co];
							else
								plotka = txRumorD[co-countof(txRumor)];
						}
						while(ctx.ostatnia_plotka == plotka);
						ctx.ostatnia_plotka = plotka;

						static string str, str_part;
						str.clear();
						for(uint i=0, len = strlen(plotka); i < len; ++i)
						{
							if(plotka[i] == '$')
							{
								str_part.clear();
								++i;
								while(plotka[i] != '$')
								{
									str_part.push_back(plotka[i]);
									++i;
								}
								str += FormatString(ctx, str_part);
							}
							else
								str.push_back(plotka[i]);
						}

						DialogTalk(ctx, str.c_str());
						++ctx.dialog_pos;
						return;
					}
				}
				else if(strncmp(de.msg, "train_", 6) == 0)
				{
					bool skill;
					int co;

					if(strcmp(de.msg+6, "str") == 0)
					{
						skill = false;
						co = (int)Attribute::STR;
					}
					else if(strcmp(de.msg+6, "end") == 0)
					{
						skill = false;
						co = (int)Attribute::CON;
					}
					else if(strcmp(de.msg+6, "dex") == 0)
					{
						skill = false;
						co = (int)Attribute::DEX;
					}
					else if(strcmp(de.msg+6, "wep") == 0)
					{
						skill = true;
						co = (int)Skill::ONE_HANDED_WEAPON;
					}
					else if(strcmp(de.msg+6, "bow") == 0)
					{
						skill = true;
						co = (int)Skill::BOW;
					}
					else if(strcmp(de.msg+6, "shi") == 0)
					{
						skill = true;
						co = (int)Skill::SHIELD;
					}
					else if(strcmp(de.msg+6, "hea") == 0)
					{
						skill = true;
						co = (int)Skill::HEAVY_ARMOR;
					}
					else if(strcmp(de.msg+6, "lia") == 0)
					{
						skill = true;
						co = (int)Skill::LIGHT_ARMOR;
					}
					else
					{
						assert(0);
						skill = false;
						co = (int)Attribute::STR;
					}

					// czy gracz ma z�oto?
					if(ctx.pc->unit->gold < 200)
					{
						ctx.dialog_s_text = Format(txNeedMoreGold, 200-ctx.pc->unit->gold);
						DialogTalk(ctx, ctx.dialog_s_text.c_str());
						// resetuj dialog
						ctx.dialog_pos = 0;
						ctx.dialog_level = 0;
						return;
					}

					// zabierz z�oto i zamr�
					ctx.pc->unit->gold -= 200;
					ctx.pc->unit->frozen = 2;
					if(ctx.is_local)
					{
						// lokalny fallback
						fallback_co = FALLBACK_TRAIN;
						fallback_t = -1.f;
						fallback_1 = (skill ? 1 : 0);
						fallback_2 = co;
					}
					else
					{
						// wy�lij info o trenowaniu
						NetChangePlayer& c = Add1(net_changes_player);
						c.type = NetChangePlayer::TRAIN;
						c.pc = ctx.pc;
						c.id = (skill ? 1 : 0);
						c.ile = co;
						GetPlayerInfo(ctx.pc).NeedUpdateAndGold();
					}
				}
				else if(strcmp(de.msg, "near_loc") == 0)
				{
					static vector<int> close;
					int index = cities, force = -1;
					for(vector<Location*>::iterator it = locations.begin()+cities, end = locations.end(); it != end; ++it, ++index)
					{
						if(*it && distance((*it)->pos, world_pos) <= 150.f)
						{
							close.push_back(index);
							if((*it)->state == LS_UNKNOWN)
								force = index;
						}
					}

					int id;
					if(force != -1)
						id = force;
					else if(!close.empty())
						id = close[rand2()%close.size()];
					else
					{
						DialogTalk(ctx, txNoNearLoc);
						++ctx.dialog_pos;
						return;
					}

					close.clear();
					Location& loc = *locations[id];
					if(loc.state == LS_UNKNOWN)
					{
						loc.state = LS_KNOWN;
						if(IsOnline())
							Net_ChangeLocationState(id, false);
					}
					ctx.dialog_s_text = Format(txNearLoc, GetLocationDirName(world_pos, loc.pos), loc.name.c_str());
					if(loc.spawn == SG_BRAK)
					{
						if(loc.type != L_CAVE && loc.type != L_FOREST && loc.type != L_MOONWELL)
							ctx.dialog_s_text += random_string(txNearLocEmpty);
					}
					else if(loc.state == LS_CLEARED)
						ctx.dialog_s_text += Format(txNearLocCleared, g_spawn_groups[loc.spawn].co);
					else
					{
						SpawnGroup& sg = g_spawn_groups[loc.spawn];
						cstring jacy;
						if(loc.st < 5)
						{
							if(sg.k == K_I)
								jacy = txELvlVeryWeak[0];
							else
								jacy = txELvlVeryWeak[1];
						}
						else if(loc.st < 8)
						{
							if(sg.k == K_I)
								jacy = txELvlWeak[0];
							else
								jacy = txELvlWeak[1];
						}
						else if(loc.st < 11)
						{
							if(sg.k == K_I)
								jacy = txELvlAvarage[0];
							else
								jacy = txELvlAvarage[1];
						}
						else if(loc.st < 14)
						{
							if(sg.k == K_I)
								jacy = txELvlQuiteStrong[0];
							else
								jacy = txELvlQuiteStrong[1];
						}
						else
						{
							if(sg.k == K_I)
								jacy = txELvlStrong[0];
							else
								jacy = txELvlStrong[1];
						}
						ctx.dialog_s_text += Format(random_string(txNearLocEnemy), jacy, g_spawn_groups[loc.spawn].co);
					}

					DialogTalk(ctx, ctx.dialog_s_text.c_str());
					++ctx.dialog_pos;
					return;
				}
				else if(strcmp(de.msg, "tell_name") == 0)
				{
					assert(ctx.talker->IsHero());
					ctx.talker->hero->know_name = true;
					if(IsOnline())
					{
						NetChange& c = Add1(net_changes);
						c.type = NetChange::TELL_NAME;
						c.unit = ctx.talker;
					}
				}
				else if(strcmp(de.msg, "hero_about") == 0)
				{
					assert(ctx.talker->IsHero());
					DialogTalk(ctx, g_classes[(int)ctx.talker->hero->clas].about.c_str());
					++ctx.dialog_pos;
					return;
				}
				else if(strcmp(de.msg, "recruit") == 0)
				{
					int koszt = ctx.talker->hero->JoinCost();
					ctx.pc->unit->gold -= koszt;
					ctx.talker->gold += koszt;
					ctx.talker->hero->team_member = true;
					ctx.talker->hero->mode = HeroData::Follow;
					ctx.talker->hero->free = false;
					AddTeamMember(ctx.talker, true);
					ctx.talker->temporary = false;
					free_recruit = false;
					if(IS_SET(ctx.talker->data->flagi2, F2_WALKA_WRECZ))
						ctx.talker->hero->melee = true;
					else if(IS_SET(ctx.talker->data->flagi2, F2_WALKA_WRECZ_50) && rand2()%2 == 0)
						ctx.talker->hero->melee = true;
					if(IsOnline())
					{
						if(!ctx.is_local)
							GetPlayerInfo(ctx.pc).UpdateGold();
						Net_RecruitNpc(ctx.talker);
					}
				}
				else if(strcmp(de.msg, "recruit_free") == 0)
				{
					ctx.talker->hero->team_member = true;
					ctx.talker->hero->mode = HeroData::Follow;
					ctx.talker->hero->free = false;
					AddTeamMember(ctx.talker, true);
					free_recruit = false;
					ctx.talker->temporary = false;
					if(IS_SET(ctx.talker->data->flagi2, F2_WALKA_WRECZ))
						ctx.talker->hero->melee = true;
					else if(IS_SET(ctx.talker->data->flagi2, F2_WALKA_WRECZ_50) && rand2()%2 == 0)
						ctx.talker->hero->melee = true;
					if(IsOnline())
						Net_RemoveUnit(ctx.talker);
				}
				else if(strcmp(de.msg, "follow_me") == 0)
				{
					assert(ctx.talker->IsFollower());
					ctx.talker->hero->mode = HeroData::Follow;
					ctx.talker->ai->city_wander = false;
				}
				else if(strcmp(de.msg, "go_free") == 0)
				{
					assert(ctx.talker->IsFollower());
					ctx.talker->hero->mode = HeroData::Wander;
					ctx.talker->ai->city_wander = false;
					ctx.talker->ai->loc_timer = random(5.f,10.f);
				}
				else if(strcmp(de.msg, "wait") == 0)
				{
					assert(ctx.talker->IsFollower());
					ctx.talker->hero->mode = HeroData::Wait;
					ctx.talker->ai->city_wander = false;
				}
				else if(strcmp(de.msg, "give_item") == 0)
				{
					Unit* t = ctx.talker;
					t->busy = Unit::Busy_Trading;
					EndDialog(ctx);
					ctx.pc->action = PlayerController::Action_GiveItems;
					ctx.pc->action_unit = t;
					ctx.pc->chest_trade = &t->items;
					if(ctx.is_local)
					{
						CloseGamePanels();
						inventory_mode = I_GIVE;
						BuildTmpInventory(0);
						inv_trade_mine->mode = Inventory::GIVE_MY;
						BuildTmpInventory(1);
						inv_trade_other->unit = t;
						inv_trade_other->items = &t->items;
						inv_trade_other->slots = t->slots;
						inv_trade_other->title = Format("%s - %s", Inventory::txGiveItems, t->GetName());
						inv_trade_other->mode = Inventory::GIVE_OTHER;
						gp_trade->Show();
					}
					else
					{
						NetChangePlayer& c = Add1(net_changes_player);
						c.type = NetChangePlayer::START_GIVE;
						c.pc = ctx.pc;
						GetPlayerInfo(ctx.pc->id).NeedUpdate();
					}
					return;
				}
				else if(strcmp(de.msg, "share_items") == 0)
				{
					Unit* t = ctx.talker;
					t->busy = Unit::Busy_Trading;
					EndDialog(ctx);
					ctx.pc->action = PlayerController::Action_ShareItems;
					ctx.pc->action_unit = t;
					ctx.pc->chest_trade = &t->items;
					if(ctx.is_local)
					{
						CloseGamePanels();
						inventory_mode = I_SHARE;
						BuildTmpInventory(0);
						inv_trade_mine->mode = Inventory::SHARE_MY;
						BuildTmpInventory(1);
						inv_trade_other->unit = t;
						inv_trade_other->items = &t->items;
						inv_trade_other->slots = t->slots;
						inv_trade_other->title = Format("%s - %s", Inventory::txShareItems, t->GetName());
						inv_trade_other->mode = Inventory::SHARE_OTHER;
						gp_trade->Show();
					}
					else
					{
						NetChangePlayer& c = Add1(net_changes_player);
						c.type = NetChangePlayer::START_SHARE;
						c.pc = ctx.pc;
						GetPlayerInfo(ctx.pc->id).NeedUpdate();
					}
					return;
				}
				else if(strcmp(de.msg, "kick_npc") == 0)
				{
					if(city_ctx)
						ctx.talker->hero->mode = HeroData::Wander;
					else
						ctx.talker->hero->mode = HeroData::Leave;
					ctx.talker->hero->team_member = false;
					ctx.talker->hero->kredyt = 0;
					ctx.talker->ai->city_wander = true;
					ctx.talker->ai->loc_timer = random(5.f,10.f);
					RemoveTeamMember(ctx.talker);
					CheckCredit(false);
					ctx.talker->temporary = true;
					if(IsOnline())
						Net_KickNpc(ctx.talker);
				}
				else if(strcmp(de.msg, "give_item_credit") == 0)
				{
					TeamShareItem& tsi = team_shares[ctx.team_share_id];
					if(CheckTeamShareItem(tsi))
					{
						tsi.to->AddItem(tsi.item, 1, false);
						if(tsi.from->IsPlayer())
							tsi.from->weight -= tsi.item->weight;
						tsi.to->hero->kredyt += tsi.item->value/2;
						CheckCredit(true);
						tsi.from->items.erase(tsi.from->items.begin()+tsi.index);
						if(!ctx.is_local && tsi.from == ctx.pc->unit)
						{
							NetChangePlayer& c = Add1(net_changes_player);
							c.type = NetChangePlayer::REMOVE_ITEMS;
							c.pc = tsi.from->player;
							c.id = tsi.index;
							c.ile = 1;
							GetPlayerInfo(c.pc).NeedUpdate();
						}
						if(tsi.to->IsOverloaded())
						{
							// sprzedaj stary przedmiot
							const Item*& old = tsi.to->slots[ItemTypeToSlot(tsi.item->type)];
							if(old)
							{
								tsi.to->gold += old->value/2;
								tsi.to->weight -= old->weight;
								old = NULL;
							}
						}
						UpdateUnitInventory(*tsi.to);
						
					}
				}
				else if(strcmp(de.msg, "sell_item") == 0)
				{
					TeamShareItem& tsi = team_shares[ctx.team_share_id];
					if(CheckTeamShareItem(tsi))
					{
						tsi.to->AddItem(tsi.item, 1, false);
						tsi.from->weight -= tsi.item->weight;
						int value = tsi.item->value/2;
						tsi.to->gold += value;
						tsi.from->gold += value;
						tsi.from->items.erase(tsi.from->items.begin()+tsi.index);
						if(!ctx.is_local)
						{
							NetChangePlayer& c = Add1(net_changes_player);
							c.type = NetChangePlayer::REMOVE_ITEMS;
							c.pc = tsi.from->player;
							c.id = tsi.index;
							c.ile = 1;
							GetPlayerInfo(c.pc).NeedUpdateAndGold();
						}
						if(tsi.to->IsOverloaded())
						{
							// sprzedaj stary przedmiot
							const Item*& old = tsi.to->slots[ItemTypeToSlot(tsi.item->type)];
							if(old)
							{
								tsi.to->gold += old->value/2;
								tsi.to->weight -= old->weight;
								old = NULL;
							}
						}
						UpdateUnitInventory(*tsi.to);
					}
				}
				else if(strcmp(de.msg, "pvp") == 0)
				{
					// walka z towarzyszem
					StartPvp(ctx.pc, ctx.talker);
				}
				else if(strcmp(de.msg, "pay_100") == 0)
				{
					int ile = min(100, ctx.pc->unit->gold);
					if(ile > 0)
					{
						ctx.pc->unit->gold -= ile;
						ctx.talker->gold += ile;
						if(sound_volume)
							PlaySound2d(sMoneta);
						if(!ctx.is_local)
							GetPlayerInfo(ctx.pc->id).UpdateGold();
					}
				}
				else if(strcmp(de.msg, "attack") == 0)
				{
					// ta komenda jest zbyt og�lna, je�li b�dzie kilka takich grup to wystarczy �e jedna tego u�yje to wszyscy zaatakuj�, nie obs�uguje te� budynk�w
					if(ctx.talker->data->grupa == G_SZALENI)
					{
						if(!atak_szalencow)
						{
							atak_szalencow = true;
							if(IsOnline())
							{
								NetChange& c = Add1(net_changes);
								c.type = NetChange::CHANGE_FLAGS;
							}
						}
					}
					else
					{
						for(vector<Unit*>::iterator it = local_ctx.units->begin(), end = local_ctx.units->end(); it != end; ++it)
						{
							if((*it)->dont_attack && IsEnemy(**it, *leader, true))
							{
								(*it)->dont_attack = false;
								(*it)->ai->change_ai_mode = true;
							}
						}
					}
				}
				else if(strcmp(de.msg, "force_attack") == 0)
				{
					ctx.talker->dont_attack = false;
					ctx.talker->attack_team = true;
					ctx.talker->ai->change_ai_mode = true;
				}
				else if(strcmp(de.msg, "rude_wlosy") == 0)
				{
					ctx.pc->unit->human_data->hair_color = g_hair_colors[8];
					if(!ctx.is_local)
					{
						NetChange& c = Add1(net_changes);
						c.type = NetChange::HAIR_COLOR;
						c.unit = ctx.pc->unit;
					}
				}
				else if(strcmp(de.msg, "losowe_wlosy") == 0)
				{
					if(rand2()%2 == 0)
					{
						VEC4 kolor = ctx.pc->unit->human_data->hair_color;
						do
						{
							ctx.pc->unit->human_data->hair_color = g_hair_colors[rand2()%n_hair_colors];
						}
						while(equal(kolor, ctx.pc->unit->human_data->hair_color));
					}
					else
					{
						VEC4 kolor = ctx.pc->unit->human_data->hair_color;
						do
						{
							ctx.pc->unit->human_data->hair_color = VEC4(random(0.f,1.f), random(0.f,1.f), random(0.f,1.f), 1.f);
						}
						while(equal(kolor, ctx.pc->unit->human_data->hair_color));
					}
					if(!ctx.is_local)
					{
						NetChange& c = Add1(net_changes);
						c.type = NetChange::HAIR_COLOR;
						c.unit = ctx.pc->unit;
					}
				}
				else if(strcmp(de.msg, "captive_join") == 0)
				{
					ctx.talker->hero->team_member = true;
					ctx.talker->hero->mode = HeroData::Follow;
					ctx.talker->hero->free = true;
					AddTeamMember(ctx.talker, false);
					ctx.talker->dont_attack = true;
					if(IsOnline())
						Net_RecruitNpc(ctx.talker);
				}
				else if(strcmp(de.msg, "captive_escape") == 0)
				{
					ctx.talker->hero->team_member = false;
					ctx.talker->hero->mode = HeroData::Leave;
					ctx.talker->MakeItemsTeam(true);
					ctx.talker->dont_attack = false;
					if(RemoveElementTry(team, ctx.talker))
					{
						if(IsOnline())
							Net_KickNpc(ctx.talker);
						// aktualizuj TeamPanel o ile otwarty
						if(team_panel->visible)
							team_panel->Changed();
					}
				}
				else if(strcmp(de.msg, "pay_500") == 0)
				{
					ctx.talker->gold += 500;
					ctx.pc->unit->gold -= 500;
					if(sound_volume)
						PlaySound2d(sMoneta);
					if(!ctx.is_local)
						GetPlayerInfo(ctx.pc->id).UpdateGold();
				}
				else if(strcmp(de.msg, "use_arena") == 0)
					arena_free = false;
				else if(strcmp(de.msg, "chlanie_start") == 0)
				{
					chlanie_stan = 3;
					chlanie_czas = 0;
					chlanie_ludzie.clear();
					chlanie_ludzie.push_back(ctx.pc->unit);
				}
				else if(strcmp(de.msg, "chlanie_udzial") == 0)
					chlanie_ludzie.push_back(ctx.pc->unit);
				else if(strcmp(de.msg, "chlanie_nagroda") == 0)
				{
					cstring co[3] = {
						"potion_str",
						"potion_end",
						"potion_dex"
					};
					chlanie_zwyciesca = NULL;
					AddItem(*ctx.pc->unit, FindItem(co[rand2()%3]), 1, false);
					if(ctx.is_local)
						AddGameMsg3(GMS_ADDED_ITEM);
					else
						Net_AddedItemMsg(ctx.pc);
				}
				else if(strcmp(de.msg, "bandyci_daj_paczke") == 0)
				{
					const Item* item = FindItem("q_bandyci_paczka");
					ctx.talker->AddItem(item, 1, true);
					RemoveQuestItem(item);
				}
				else if(strcmp(de.msg, "q_magowie_zaplac") == 0)
				{
					if(ctx.pc->unit->gold)
					{
						ctx.talker->gold += ctx.pc->unit->gold;
						ctx.pc->unit->gold = 0;
						if(sound_volume)
							PlaySound2d(sMoneta);
						if(!ctx.is_local)
							GetPlayerInfo(ctx.pc->id).UpdateGold();
					}
					magowie_zaplacono = true;
				}
				else if(strcmp(de.msg, "news") == 0)
				{
					if(ctx.update_news)
					{
						ctx.update_news = false;
						ctx.active_news = news;
						if(ctx.active_news.empty())
						{
							DialogTalk(ctx, random_string(txNoNews));
							++ctx.dialog_pos;
							return;
						}
					}

					if(ctx.active_news.empty())
					{
						DialogTalk(ctx, random_string(txAllNews));
						++ctx.dialog_pos;
						return;
					}

					int id = rand2()%ctx.active_news.size();
					News* news = ctx.active_news[id];
					ctx.active_news.erase(ctx.active_news.begin()+id);

					DialogTalk(ctx, news->text.c_str());
					++ctx.dialog_pos;
					return;
				}
				else if(strcmp(de.msg, "go_melee") == 0)
				{
					assert(ctx.talker->IsHero());
					ctx.talker->hero->melee = true;
				}
				else if(strcmp(de.msg, "go_ranged") == 0)
				{
					assert(ctx.talker->IsHero());
					ctx.talker->hero->melee = false;
				}
				else if(strcmp(de.msg, "pvp_gather") == 0)
				{
					near_players.clear();
					for(vector<Unit*>::iterator it = active_team.begin(), end = active_team.end(); it != end; ++it)
					{
						if((*it)->IsPlayer() && (*it)->player != ctx.pc && distance2d((*it)->pos, city_ctx->arena_pos) < 5.f)
							near_players.push_back(*it);
					}
					near_players_str.resize(near_players.size());
					for(uint i=0, size=near_players.size(); i!=size; ++i)
						near_players_str[i] = Format(txPvpWith, near_players[i]->player->name.c_str());
				}
				else if(strncmp(de.msg, "pvp", 3) == 0)
				{
					int id = int(de.msg[3]-'1');
					PlayerInfo& info = GetPlayerInfo(near_players[id]->player);
					if(distance2d(info.u->pos, city_ctx->arena_pos) > 5.f)
					{
						ctx.dialog_s_text = Format(txPvpTooFar, info.name.c_str());
						DialogTalk(ctx, ctx.dialog_s_text.c_str());
						++ctx.dialog_pos;
						return;
					}
					else
					{
						if(info.id == my_id)
						{
							DialogInfo info;
							info.event = DialogEvent(this, &Game::Event_Pvp);
							info.name = "pvp";
							info.order = ORDER_TOP;
							info.parent = NULL;
							info.pause = false;
							info.text = Format(txPvp, ctx.pc->name.c_str());
							info.type = DIALOG_YESNO;
							dialog_pvp = GUI.ShowDialog(info);
							pvp_unit = near_players[id];
						}
						else
						{
							NetChangePlayer& c = Add1(net_changes_player);
							c.type = NetChangePlayer::PVP;
							c.pc = near_players[id]->player;
							c.id = ctx.pc->id;
							GetPlayerInfo(near_players[id]->player).NeedUpdate();
						}

						pvp_response.ok = true;
						pvp_response.from = ctx.pc->unit;
						pvp_response.to = info.u;
						pvp_response.timer = 0.f;
					}
				}
				else if(strcmp(de.msg, "ironfist_start") == 0)
				{
					StartTournament(ctx.talker);
					zawody_ludzie.push_back(ctx.pc->unit);
					ctx.pc->unit->gold -= 100;
					if(!ctx.is_local)
						GetPlayerInfo(ctx.pc).UpdateGold();
				}
				else if(strcmp(de.msg, "ironfist_join") == 0)
				{
					zawody_ludzie.push_back(ctx.pc->unit);
					ctx.pc->unit->gold -= 100;
					if(!ctx.is_local)
						GetPlayerInfo(ctx.pc).UpdateGold();
				}
				else if(strcmp(de.msg, "ironfist_train") == 0)
				{
					zawody_zwyciezca = NULL;
					ctx.pc->unit->frozen = 2;
					if(ctx.is_local)
					{
						// lokalny fallback
						fallback_co = FALLBACK_TRAIN;
						fallback_t = -1.f;
						fallback_1 = 2;
						fallback_2 = 0;
					}
					else
					{
						// wy�lij info o trenowaniu
						NetChangePlayer& c = Add1(net_changes_player);
						c.type = NetChangePlayer::TRAIN;
						c.pc = ctx.pc;
						c.id = 2;
						c.ile = 0;
						GetPlayerInfo(ctx.pc).NeedUpdate();
					}
				}
				else if(strcmp(de.msg, "szaleni_powiedzial") == 0)
				{
					ctx.talker->ai->morale = -100.f;
					szaleni_stan = SS_POGADANO_Z_SZALONYM;
				}
				else if(strcmp(de.msg, "szaleni_sprzedaj") == 0)
				{
					const Item* kamien = FindItem("q_szaleni_kamien");
					RemoveItem(*ctx.pc->unit, kamien, 1);
					ctx.pc->unit->gold += 10;
					if(!ctx.is_local)
					{
						NetChangePlayer& c = Add1(net_changes_player);
						c.type = NetChangePlayer::GOLD_MSG;
						c.pc = ctx.pc;
						c.ile = 10;
						c.id = 1;
						GetPlayerInfo(ctx.pc).UpdateGold();
					}
					else
						AddGameMsg(Format(txGoldPlus, 10), 3.f);
				}
				else if(strcmp(de.msg, "give_1") == 0)
				{
					ctx.pc->unit->gold += 1;
					if(!ctx.is_local)
					{
						NetChangePlayer& c = Add1(net_changes_player);
						c.type = NetChangePlayer::GOLD_MSG;
						c.pc = ctx.pc;
						c.ile = 1;
						c.id = 1;
						GetPlayerInfo(ctx.pc).UpdateGold();
					}
					else
						AddGameMsg(Format(txGoldPlus, 1), 3.f);
				}
				else if(strcmp(de.msg, "take_1") == 0)
				{
					ctx.pc->unit->gold -= 1;
					if(!ctx.is_local)
						GetPlayerInfo(ctx.pc).UpdateGold();
				}
				else if(strcmp(de.msg, "crazy_give_item") == 0)
				{
					crazy_give_item = GetRandomItem(100);
					ctx.pc->unit->AddItem(crazy_give_item, 1, false);
					if(!ctx.is_local)
					{
						Net_AddItem(ctx.pc, crazy_give_item, false);
						Net_AddedItemMsg(ctx.pc);
					}
					else
						AddGameMsg3(GMS_ADDED_ITEM);	
				}
				else if(strcmp(de.msg, "sekret_atak") == 0)
				{
					sekret_stan = SS2_WALKA;
					at_arena.clear();

					ctx.talker->in_arena = 1;
					at_arena.push_back(ctx.talker);
					if(IsOnline())
					{
						NetChange& c = Add1(net_changes);
						c.type = NetChange::CHANGE_ARENA_STATE;
						c.unit = ctx.talker;
					}

					for(vector<Unit*>::iterator it = team.begin(), end = team.end(); it != end; ++it)
					{
						(*it)->in_arena = 0;
						at_arena.push_back(*it);
						if(IsOnline())
						{
							NetChange& c = Add1(net_changes);
							c.type = NetChange::CHANGE_ARENA_STATE;
							c.unit = *it;
						}
					}
				}
				else if(strcmp(de.msg, "sekret_daj") == 0)
				{
					sekret_stan = SS2_NAGRODA;
					const Item* item = FindItem("sword_forbidden");
					ctx.pc->unit->AddItem(item, 1, true);
					if(!ctx.is_local)
					{
						Net_AddItem(ctx.pc, item, true);
						Net_AddedItemMsg(ctx.pc);
					}
					else
						AddGameMsg3(GMS_ADDED_ITEM);
				}
				else
				{
					WARN(Format("DT_SPECIAL: %s", de.msg));
					assert(0);
				}
			}
			break;
		case DT_SET_QUEST_PROGRESS:
			if(if_level == ctx.dialog_level)
			{
				assert(ctx.dialog_quest);
				ctx.dialog_quest->SetProgress((int)de.msg);
				if(ctx.dialog_wait > 0.f)
				{
					++ctx.dialog_pos;
					return;
				}
			}
			break;
		case DT_IF_QUEST_TIMEOUT:
			if(if_level == ctx.dialog_level)
			{
				assert(ctx.dialog_quest);
				if(ctx.dialog_quest->IsActive() && ctx.dialog_quest->IsTimedout())
					++ctx.dialog_level;
			}
			++if_level;
			break;
		case DT_IF_RAND:
			if(if_level == ctx.dialog_level)
			{
				if(rand2()%int(de.msg) == 0)
					++ctx.dialog_level;
			}
			++if_level;
			break;
		case DT_IF_ONCE:
			if(if_level == ctx.dialog_level)
			{
				if(ctx.dialog_once)
				{
					ctx.dialog_once = false;
					++ctx.dialog_level;
				}
			}
			++if_level;
			break;
		case DT_DO_ONCE:
			if(if_level == ctx.dialog_level)
				ctx.dialog_once = false;
			break;
		case DT_ELSE:
			if(if_level == ctx.dialog_level)
				--ctx.dialog_level;
			else if(if_level == ctx.dialog_level+1)
				++ctx.dialog_level;
			break;
		case DT_CHECK_QUEST_TIMEOUT:
			if(if_level == ctx.dialog_level)
			{
				Quest* quest = FindQuest(current_location, (int)de.msg);
				if(quest && quest->IsActive() && quest->IsTimedout())
				{
					ctx.dialog_once = false;
					ctx.prev_dialog = ctx.dialog;
					ctx.prev_dialog_pos = ctx.dialog_pos;
					ctx.dialog = quest->GetDialog(QUEST_DIALOG_FAIL);
					ctx.dialog_pos = -1;
					ctx.dialog_quest = quest;
				}
			}
			break;
		case DT_IF_HAVE_QUEST_ITEM:
			if(if_level == ctx.dialog_level)
			{
				if(FindQuestItem2(ctx.pc->unit, de.msg, NULL, NULL))
					++ctx.dialog_level;
			}
			++if_level;
			break;
		case DT_DO_QUEST_ITEM:
			if(if_level == ctx.dialog_level)
			{
				Quest* quest;
				if(FindQuestItem2(ctx.pc->unit, de.msg, &quest, NULL))
				{
					ctx.prev_dialog = ctx.dialog;
					ctx.prev_dialog_pos = ctx.dialog_pos;
					ctx.dialog = quest->GetDialog(QUEST_DIALOG_NEXT);
					ctx.dialog_pos = -1;
					ctx.dialog_quest = quest;
				}
			}
			break;
		case DT_IF_QUEST_PROGRESS:
			if(if_level == ctx.dialog_level)
			{
				assert(ctx.dialog_quest);
				if(ctx.dialog_quest->prog == (int)de.msg)
					++ctx.dialog_level;
			}
			++if_level;
			break;
		case DT_IF_QUEST_PROGRESS_RANGE:
			if(if_level == ctx.dialog_level)
			{
				assert(ctx.dialog_quest);
				int x = int(de.msg)&0xFFFF;
				int y = (int(de.msg)&0xFFFF0000)>>16;
				assert(y > x);
				if(in_range(ctx.dialog_quest->prog, x, y))
					++ctx.dialog_level;
			}
			++if_level;
			break;
		case DT_IF_NEED_TALK:
			if(if_level == ctx.dialog_level)
			{
				for(vector<Quest*>::iterator it = quests.begin(), end = quests.end(); it != end; ++it)
				{
					if((*it)->IsActive() && (*it)->IfNeedTalk(de.msg))
					{
						++ctx.dialog_level;
						break;
					}
				}
			}
			++if_level;
			break;
		case DT_DO_QUEST:
			if(if_level == ctx.dialog_level)
			{
				for(vector<Quest*>::iterator it = quests.begin(), end = quests.end(); it != end; ++it)
				{
					if((*it)->IsActive() && (*it)->IfNeedTalk(de.msg))
					{
						ctx.prev_dialog = ctx.dialog;
						ctx.prev_dialog_pos = ctx.dialog_pos;
						ctx.dialog = (*it)->GetDialog(QUEST_DIALOG_NEXT);
						ctx.dialog_pos = -1;
						ctx.dialog_quest = *it;
						break;
					}
				}
			}
			break;
		case DT_ESCAPE_CHOICE:
			if(if_level == ctx.dialog_level)
				ctx.dialog_esc = (int)ctx.choices.size()-1;
			break;
		case DT_IF_SPECIAL:
			if(if_level == ctx.dialog_level)
			{
				if(strcmp(de.msg, "arena_combat") == 0)
				{
					int t = city_ctx->arena_czas;
					if(t == -1)
						++ctx.dialog_level;
					else
					{
						int rok = t/360;
						t -= rok*360;
						rok += 100;
						int miesiac = t/30;
						t -= miesiac*30;
						int dekadzien = t/10;

						if(rok != year || miesiac != month || dekadzien != day/10)
							++ctx.dialog_level;
					}
				}
				else if(strcmp(de.msg, "have_team") == 0)
				{
					if(HaveTeamMember())
						++ctx.dialog_level;
				}
				else if(strcmp(de.msg, "have_pc_team") == 0)
				{
					if(HaveTeamMemberPC())
						++ctx.dialog_level;
				}
				else if(strcmp(de.msg, "have_npc_team") == 0)
				{
					if(HaveTeamMemberNPC())
						++ctx.dialog_level;
				}
				else if(strcmp(de.msg, "is_drunk") == 0)
				{
					if(IS_SET(ctx.talker->data->flagi, F_AI_PIJAK) && ctx.talker->in_building != -1)
						++ctx.dialog_level;
				}
				else if(strcmp(de.msg, "is_team_member") == 0)
				{
					if(IsTeamMember(*ctx.talker))
						++ctx.dialog_level;
				}
				else if(strcmp(de.msg, "is_not_known") == 0)
				{
					assert(ctx.talker->IsHero());
					if(!ctx.talker->hero->know_name)
						++ctx.dialog_level;
				}
				else if(strcmp(de.msg, "is_inside_dungeon") == 0)
				{
					if(local_ctx.type == LevelContext::Inside)
						++ctx.dialog_level;
				}
				else if(strcmp(de.msg, "is_team_full") == 0)
				{
					if(GetActiveTeamSize() >= MAX_TEAM_SIZE)
						++ctx.dialog_level;
				}
				else if(strcmp(de.msg, "can_join") == 0)
				{
					if(ctx.pc->unit->gold >= ctx.talker->hero->JoinCost())
						++ctx.dialog_level;
				}
				else if(strcmp(de.msg, "is_inside_town") == 0)
				{
					if(city_ctx)
						++ctx.dialog_level;
				}
				else if(strcmp(de.msg, "is_free") == 0)
				{
					assert(ctx.talker->IsHero());
					if(ctx.talker->hero->mode == HeroData::Wander)
						++ctx.dialog_level;
				}
				else if(strcmp(de.msg, "is_not_free") == 0)
				{
					assert(ctx.talker->IsHero());
					if(ctx.talker->hero->mode != HeroData::Wander)
						++ctx.dialog_level;
				}
				else if(strcmp(de.msg, "is_not_follow") == 0)
				{
					assert(ctx.talker->IsHero());
					if(ctx.talker->hero->mode != HeroData::Follow)
						++ctx.dialog_level;
				}
				else if(strcmp(de.msg, "is_not_waiting") == 0)
				{
					assert(ctx.talker->IsHero());
					if(ctx.talker->hero->mode != HeroData::Wait)
						++ctx.dialog_level;
				}
				else if(strcmp(de.msg, "is_safe_location") == 0)
				{
					if(city_ctx)
						++ctx.dialog_level;
					else if(location->outside)
					{
						if(location->state == LS_CLEARED)
							++ctx.dialog_level;
					}
					else
					{
						InsideLocation* inside = (InsideLocation*)location;
						if(inside->IsMultilevel())
						{
							MultiInsideLocation* multi = (MultiInsideLocation*)inside;
							if(multi->IsLevelClear())
							{
								if(dungeon_level == 0)
								{
									if(!multi->from_portal)
										++ctx.dialog_level;
								}
								else
									++ctx.dialog_level;
							}
						}
						else if(location->state == LS_CLEARED)
							++ctx.dialog_level;
					}
				}
				else if(strcmp(de.msg, "is_near_arena") == 0)
				{
					if(city_ctx && city_ctx->type != L_VILLAGE && distance2d(ctx.talker->pos, city_ctx->arena_pos) < 5.f)
						++ctx.dialog_level;
				}
				else if(strcmp(de.msg, "is_lost_pvp") == 0)
				{
					assert(ctx.talker->IsHero());
					if(ctx.talker->hero->lost_pvp)
						++ctx.dialog_level;
				}
				else if(strcmp(de.msg, "is_healthy") == 0)
				{
					if(ctx.talker->GetHpp() >= 0.75f)
						++ctx.dialog_level;
				}
				else if(strcmp(de.msg, "is_arena_free") == 0)
				{
					if(arena_free)
						++ctx.dialog_level;
				}
				else if(strcmp(de.msg, "is_bandit") == 0)
				{
					if(bandyta)
						++ctx.dialog_level;
				}
				else if(strcmp(de.msg, "have_gold_100") == 0)
				{
					if(ctx.pc->unit->gold >= 100)
						++ctx.dialog_level;
				}
				else if(strcmp(de.msg, "czy_rude_wlosy") == 0)
				{
					if(equal(ctx.pc->unit->human_data->hair_color, g_hair_colors[8]))
						++ctx.dialog_level;
				}
				else if(strcmp(de.msg, "czy_lysy") == 0)
				{
					if(ctx.pc->unit->human_data->hair == -1)
						++ctx.dialog_level;
				}
				else if(strcmp(de.msg, "czy_oboz") == 0)
				{
					if(target_loc_is_camp)
						++ctx.dialog_level;
				}
				else if(strcmp(de.msg, "have_500") == 0)
				{
					if(ctx.pc->unit->gold >= 500)
						++ctx.dialog_level;
				}
				else if(strcmp(de.msg, "taken_guards_reward") == 0)
				{
					if(!guards_enc_reward)
					{
						AddGold(250, NULL, true);
						guards_enc_reward = true;
						++ctx.dialog_level;
					}
				}
				else if(strcmp(de.msg, "dont_have_quest") == 0)
				{
					if(ctx.talker->quest_refid == -1)
						++ctx.dialog_level;
				}
				else if(strcmp(de.msg, "have_unaccepted_quest") == 0)
				{
					if(FindUnacceptedQuest(ctx.talker->quest_refid))
						++ctx.dialog_level;
				}
				else if(strcmp(de.msg, "have_completed_quest") == 0)
				{
					Quest* q = FindQuest(ctx.talker->quest_refid, false);
					if(q && !q->IsActive())
						++ctx.dialog_level;
				}
				else if(strcmp(de.msg, "czy_tartak") == 0)
				{
					Quest_Tartak* q = (Quest_Tartak*)FindQuest(tartak_refid);
					if(current_location == q->target_loc)
						++ctx.dialog_level;
				}
				else if(strcmp(de.msg, "udzialy_w_kopalni") == 0)
				{
					if(kopalnia_stan == 2)
						++ctx.dialog_level;
				}
				else if(strcmp(de.msg, "have_10000") == 0)
				{
					if(ctx.pc->unit->gold >= 10000)
						++ctx.dialog_level;
				}
				else if(strcmp(de.msg, "have_12000") == 0)
				{
					if(ctx.pc->unit->gold >= 12000)
						++ctx.dialog_level;
				}
				else if(strcmp(de.msg, "chlanie_bylo") == 0)
				{
					if(chlanie_stan == 1)
						++ctx.dialog_level;
				}
				else if(strcmp(de.msg, "chlanie_tutaj") == 0)
				{
					if(chlanie_gdzie == current_location)
						++ctx.dialog_level;
				}
				else if(strcmp(de.msg, "chlanie_dzisiaj") == 0)
				{
					if(chlanie_stan == 2)
						++ctx.dialog_level;
				}
				else if(strcmp(de.msg, "chlanie_trwa") == 0)
				{
					if(chlanie_stan == 4)
						++ctx.dialog_level;
				}
				else if(strcmp(de.msg, "chlanie_rozpoczeto") == 0)
				{
					if(chlanie_stan == 3)
						++ctx.dialog_level;
				}
				else if(strcmp(de.msg, "chlanie_zapisany") == 0)
				{
					for(vector<Unit*>::iterator it = chlanie_ludzie.begin(), end = chlanie_ludzie.end(); it != end; ++it)
					{
						if(*it == ctx.pc->unit)
						{
							++ctx.dialog_level;
							break;
						}
					}
				}
				else if(strcmp(de.msg, "chlanie_zwyciesca") == 0)
				{
					if(ctx.pc->unit == chlanie_zwyciesca)
						++ctx.dialog_level;
				}
				else if(strcmp(de.msg, "q_bandyci_straznikow_daj") == 0)
				{
					Quest* q = FindQuest(bandyci_refid);
					if(q && q->prog == 6 && current_location == bandyci_miasto)
						++ctx.dialog_level;
				}
				else if(strcmp(de.msg, "q_magowie_zaplacono") == 0)
				{
					if(magowie_zaplacono)
						++ctx.dialog_level;
				}
				else if(strcmp(de.msg, "q_magowie_to_miasto") == 0)
				{
					if(magowie_stan >= MS_POROZMAWIANO_Z_KAPITANEM && current_location == magowie_miasto)
						++ctx.dialog_level;
				}
				else if(strcmp(de.msg, "q_magowie_poinformuj") == 0)
				{
					if(magowie_stan == MS_SPOTKANO_GOLEMA)
						++ctx.dialog_level;
				}
				else if(strcmp(de.msg, "q_magowie_kup_miksture") == 0)
				{
					if(magowie_stan == MS_KUP_MIKSTURE)
						++ctx.dialog_level;
				}
				else if(strcmp(de.msg, "q_magowie_kup") == 0)
				{
					if(ctx.pc->unit->gold >= 150)
					{
						Quest* q = FindQuest(magowie_refid2);
						q->SetProgress(8);
						++ctx.dialog_level;
					}
				}
				else if(strcmp(de.msg, "q_magowie_u_siebie") == 0)
				{
					if(current_location == magowie_gdzie)
						++ctx.dialog_level;
				}
				else if(strcmp(de.msg, "q_magowie_czas") == 0)
				{
					if(magowie_czas >= 30.f)
						++ctx.dialog_level;
				}
				else if(strcmp(de.msg, "q_magowie_u_bossa") == 0)
				{
					Quest_Magowie2* q = (Quest_Magowie2*)FindQuest(magowie_refid2);
					if(q->target_loc == current_location)
						++ctx.dialog_level;
				}
				else if(strcmp(de.msg, "q_orkowie_to_miasto") == 0)
				{
					if(current_location == orkowie_miasto)
						++ctx.dialog_level;
				}
				else if(strcmp(de.msg, "q_orkowie_zaakceptowano") == 0)
				{
					if(orkowie_stan >= OS_ZAAKCEPTOWANO)
						++ctx.dialog_level;
				}
				else if(strcmp(de.msg, "q_magowie_nie_ukonczono") == 0)
				{
					if(magowie_stan != MS_UKONCZONO)
						++ctx.dialog_level;
				}
				else if(strcmp(de.msg, "q_orkowie_nie_ukonczono") == 0)
				{
					if(orkowie_stan < OS_UKONCZONO)
						++ctx.dialog_level;
				}
				else if(strcmp(de.msg, "q_orkowie_dolaczyl") == 0)
				{
					if(orkowie_stan == OS_ORK_DOLACZYL || orkowie_stan == OS_UKONCZONO_DOLACZYL)
						++ctx.dialog_level;
				}
				else if(strcmp(de.msg, "is_free_recruit") == 0)
				{
					if(ctx.talker->level < 6 && free_recruit)
						++ctx.dialog_level;
				}
				else if(strcmp(de.msg, "q_orkowie_woj") == 0)
				{
					if(orkowie_klasa == GORUSH_WOJ)
						++ctx.dialog_level;
				}
				else if(strcmp(de.msg, "q_orkowie_lowca") == 0)
				{
					if(orkowie_klasa == GORUSH_LOWCA)
						++ctx.dialog_level;
				}
				else if(strcmp(de.msg, "masz_unikalne_zadanie") == 0)
				{
					if((orkowie_stan == OS_ZAAKCEPTOWANO || orkowie_stan == OS_ORK_DOLACZYL) && orkowie_miasto == current_location)
						++ctx.dialog_level;
					else if(magowie_stan >= MS_POROZMAWIANO_Z_KAPITANEM && magowie_stan < MS_UKONCZONO && magowie_miasto == current_location)
						++ctx.dialog_level;
				}
				else if(strcmp(de.msg, "q_orkowie_na_miejscu") == 0)
				{
					if(current_location == ((Quest_Orkowie2*)FindQuest(orkowie_refid2))->target_loc)
						++ctx.dialog_level;
				}
				else if(strcmp(de.msg, "have_100") == 0)
				{
					if(ctx.pc->unit->gold >= 100)
						++ctx.dialog_level;
				}
				else if(strcmp(de.msg, "q_gobliny_zapytaj") == 0)
				{
					if(gobliny_stan >= GS_MAG_POGADAL && gobliny_stan < GS_POZNANO_LOKACJE && current_location == gobliny_miasto)
					{
						Quest* q = FindQuest(gobliny_refid);
						if(q->prog != 10)
							++ctx.dialog_level;
					}
				}
				else if(strcmp(de.msg, "q_zlo_kapitan") == 0)
				{
					if(current_location == zlo_gdzie2 && zlo_stan >= ZS_WYGENEROWANO_MAGA && zlo_stan < ZS_ZAMYKANIE_PORTALI)
					{
						Quest* q = FindQuest(zlo_refid);
						if(in_range(q->prog, 6, 8))
							++ctx.dialog_level;
					}
				}
				else if(strcmp(de.msg, "q_zlo_burmistrz") == 0)
				{
					if(current_location == zlo_gdzie2 && zlo_stan >= ZS_WYGENEROWANO_MAGA && zlo_stan < ZS_ZAMYKANIE_PORTALI)
					{
						Quest* q = FindQuest(zlo_refid);
						if(q->prog == 7)
							++ctx.dialog_level;
					}
				}
				else if(strcmp(de.msg, "q_zlo_clear1") == 0)
				{
					if(zlo_stan == ZS_ZAMYKANIE_PORTALI)
					{
						Quest_Zlo* q = (Quest_Zlo*)FindQuest(zlo_refid);
						if(q->closed == 1)
							++ctx.dialog_level;
					}
				}
				else if(strcmp(de.msg, "q_zlo_clear2") == 0)
				{
					if(zlo_stan == ZS_ZAMYKANIE_PORTALI)
					{
						Quest_Zlo* q = (Quest_Zlo*)FindQuest(zlo_refid);
						if(q->closed == 2)
							++ctx.dialog_level;
					}
				}
				else if(strcmp(de.msg, "q_zlo_tutaj") == 0)
				{
					Quest_Zlo* q = (Quest_Zlo*)FindQuest(zlo_refid);
					if(q->prog == 10)
					{
						int d = q->GetLocId(current_location);
						if(d != -1)
						{
							if(q->loc[d].state != 3)
								++ctx.dialog_level;
						}
					}
					else if(q->prog == 12)
					{
						if(current_location == zlo_gdzie)
							++ctx.dialog_level;
					}
				}
				else if(strcmp(de.msg, "is_not_mage") == 0)
				{
					if(ctx.talker->hero->clas != Class::MAGE)
						++ctx.dialog_level;
				}
				else if(strcmp(de.msg, "prefer_melee") == 0)
				{
					if(ctx.talker->hero->melee)
						++ctx.dialog_level;
				}
				else if(strcmp(de.msg, "is_leader") == 0)
				{
					if(ctx.pc->unit == leader)
						++ctx.dialog_level;
				}
				else if(strncmp(de.msg, "have_player", 11) == 0)
				{
					int id = int(de.msg[11]-'1');
					if(id < (int)near_players.size())
						++ctx.dialog_level;
				}
				else if(strcmp(de.msg, "waiting_for_pvp") == 0)
				{
					if(pvp_response.ok)
						++ctx.dialog_level;
				}
				else if(strcmp(de.msg, "in_city") == 0)
				{
					if(city_ctx)
						++ctx.dialog_level;
				}
				else if(strcmp(de.msg, "ironfist_can_start") == 0)
				{
					if(zawody_stan == IS_BRAK && zawody_miasto == current_location && day == 6 && month == 2 && zawody_rok != year)
						++ctx.dialog_level;
				}
				else if(strcmp(de.msg, "ironfist_done") == 0)
				{
					if(zawody_rok == year)
						++ctx.dialog_level;
				}
				else if(strcmp(de.msg, "ironfist_here") == 0)
				{
					if(current_location == zawody_miasto)
						++ctx.dialog_level;
				}
				else if(strcmp(de.msg, "ironfist_preparing") == 0)
				{
					if(zawody_stan == IS_ROZPOCZYNANIE)
						++ctx.dialog_level;
				}
				else if(strcmp(de.msg, "ironfist_started") == 0)
				{
					if(zawody_stan == IS_TRWAJA)
					{
						// zwyci�zca mo�e pogada� i przerwa� gadanie
						if(zawody_zwyciezca == dialog_context.pc->unit && zawody_stan == IS_TRWAJA && zawody_stan2 == 2 && zawody_stan3 == 1)
						{
							zawody_mistrz->look_target = NULL;
							zawody_stan = IS_BRAK;
						}
						else
							++ctx.dialog_level;
					}
				}
				else if(strcmp(de.msg, "ironfist_joined") == 0)
				{
					for(vector<Unit*>::iterator it = zawody_ludzie.begin(), end = zawody_ludzie.end(); it != end; ++it)
					{
						if(*it == ctx.pc->unit)
						{
							++ctx.dialog_level;
							break;
						}
					}
				}
				else if(strcmp(de.msg, "ironfist_winner") == 0)
				{
					if(ctx.pc->unit == zawody_zwyciezca)
						++ctx.dialog_level;
				}
				else if(strcmp(de.msg, "szaleni_nie_zapytano") == 0)
				{
					if(szaleni_stan == SS_BRAK)
						++ctx.dialog_level;
				}
				else if(strcmp(de.msg, "q_szaleni_trzeba_pogadac") == 0)
				{
					if(szaleni_stan == SS_PIERWSZY_ATAK)
						++ctx.dialog_level;
				}
				else if(strcmp(de.msg, "have_1") == 0)
				{
					if(ctx.pc->unit->gold != 0)
						++ctx.dialog_level;
				}
				else if(strcmp(de.msg, "sekret_pierwsza_rozmowa") == 0)
				{
					if(sekret_stan == SS2_WYGENEROWANO2)
					{
						sekret_stan = SS2_POGADANO;
						++ctx.dialog_level;
					}
				}
				else if(strcmp(de.msg, "sekret_moze_walczyc") == 0)
				{
					if(sekret_stan == SS2_POGADANO)
						++ctx.dialog_level;
				}
				else if(strcmp(de.msg, "sekret_wygralo_sie") == 0)
				{
					if(sekret_stan == SS2_WYGRANO || sekret_stan == SS2_NAGRODA)
						++ctx.dialog_level;
				}
				else if(strcmp(de.msg, "sekret_dawaj_nagrode") == 0)
				{
					if(sekret_stan == SS2_WYGRANO)
						++ctx.dialog_level;
				}
				else
				{
					WARN(Format("DT_SPECIAL_IF: %s", de.msg));
					assert(0);
				}
			}
			++if_level;
			break;
		case DT_RANDOM_TEXT:
			if(if_level == ctx.dialog_level)
			{
				int n = (int)de.msg;
				ctx.dialog_skip = ctx.dialog_pos + n + 1;
				ctx.dialog_pos += rand2()%n;
			}
			break;
		case DT_IF_CHOICES:
			if(if_level == ctx.dialog_level)
			{
				if(ctx.choices.size() == (int)de.msg)
					++ctx.dialog_level;
			}
			++if_level;
			break;
		case DT_DO_QUEST2:
			if(if_level == ctx.dialog_level)
			{
				for(vector<Quest*>::iterator it = quests.begin(), end = quests.end(); it != end; ++it)
				{
					if((*it)->IfNeedTalk(de.msg))
					{
						ctx.prev_dialog = ctx.dialog;
						ctx.prev_dialog_pos = ctx.dialog_pos;
						ctx.dialog = (*it)->GetDialog(QUEST_DIALOG_NEXT);
						ctx.dialog_pos = -1;
						ctx.dialog_quest = *it;
						break;
					}
				}

				if(ctx.dialog_pos != -1)
				{
					for(vector<Quest*>::iterator it = unaccepted_quests.begin(), end = unaccepted_quests.end(); it != end; ++it)
					{
						if((*it)->IfNeedTalk(de.msg))
						{
							ctx.prev_dialog = ctx.dialog;
							ctx.prev_dialog_pos = ctx.dialog_pos;
							ctx.dialog = (*it)->GetDialog(QUEST_DIALOG_NEXT);
							ctx.dialog_pos = -1;
							ctx.dialog_quest = *it;
							break;
						}
					}
				}
			}
			break;
		case DT_IF_HAVE_ITEM:
			if(if_level == ctx.dialog_level)
			{
				if(ctx.pc->unit->HaveItem(FindItem(de.msg)))
					++ctx.dialog_level;
			}
			++if_level;
			break;
		case DT_IF_QUEST_EVENT:
			if(if_level == ctx.dialog_level)
			{
				assert(ctx.dialog_quest);
				if(ctx.dialog_quest->IfQuestEvent())
					++ctx.dialog_level;
			}
			++if_level;
			break;
		case DT_END_OF_DIALOG:
			assert(0);
			throw Format("Broken dialog, character '%s' [%d:%d, %d:%d, %d:%d]",  ctx.talker->data->id, ctx.dialog[0].type, (int)ctx.dialog[0].msg, ctx.dialog[1].type, (int)ctx.dialog[1].msg,
				ctx.dialog[2].type, (int)ctx.dialog[2].msg);
		default:
			assert(0 && "Unknown dialog type!");
			break;
		}

		++ctx.dialog_pos;
	}
}

//=============================================================================
// Ruch jednostki, ustawia pozycje Y dla terenu, opuszczanie lokacji
//=============================================================================
void Game::MoveUnit(Unit& unit, bool warped)
{
	if(location->outside)
	{
		if(unit.in_building == -1)
		{
			if(terrain->IsInside(unit.pos))
			{
				terrain->SetH(unit.pos);
				if(warped)
					return;
				if(unit.IsPlayer() && WantExitLevel() && unit.frozen == 0)
				{
					bool allow_exit = false;

					if(city_ctx && city_ctx->have_exit)
					{
						for(vector<EntryPoint>::const_iterator it = city_ctx->entry_points.begin(), end = city_ctx->entry_points.end(); it != end; ++it)
						{
							if(it->exit_area.IsInside(unit.pos))
							{
								if(!IsLeader())
									AddGameMsg3(GMS_NOT_LEADER);
								else
								{
									if(IsLocal())
									{
										int w = CanLeaveLocation(unit);
										if(w == 0)
										{
											allow_exit = true;
											SetExitWorldDir();
										}
										else
											AddGameMsg3(w == 1 ? GMS_GATHER_TEAM : GMS_NOT_IN_COMBAT);
									}
									else
										Net_LeaveLocation(WHERE_OUTSIDE);
								}
								break;
							}
						}
					}
					else if(current_location != sekret_gdzie2 && (unit.pos.x < 33.f || unit.pos.x > 256.f-33.f || unit.pos.z < 33.f || unit.pos.z > 256.f-33.f))
					{
						if(IsLeader())
						{
							if(IsLocal())
							{
								int w = CanLeaveLocation(unit);
								if(w == 0)
								{
									allow_exit = true;
									// opuszczanie otwartego terenu (las/droga/ob�z)
									if(unit.pos.x < 33.f)
										world_dir = lerp(3.f/4.f*PI, 5.f/4.f*PI, 1.f-(unit.pos.z-33.f)/(256.f-66.f));
									else if(unit.pos.x > 256.f-33.f)
									{
										if(unit.pos.z > 128.f)
											world_dir = lerp(0.f, 1.f/4*PI, (unit.pos.z-128.f)/(256.f-128.f-33.f));
										else
											world_dir = lerp(7.f/4*PI, PI*2, (unit.pos.z-33.f)/(256.f-128.f-33.f));
									}
									else if(unit.pos.z < 33.f)
										world_dir = lerp(5.f/4*PI, 7.f/4*PI, (unit.pos.x-33.f)/(256.f-66.f));
									else
										world_dir = lerp(1.f/4*PI, 3.f/4*PI, 1.f-(unit.pos.x-33.f)/(256.f-66.f));
								}
								else if(w == 1)
									AddGameMsg3(w == 1 ? GMS_GATHER_TEAM : GMS_NOT_IN_COMBAT);
							}
							else
								Net_LeaveLocation(WHERE_OUTSIDE);
						}
						else
							AddGameMsg3(GMS_NOT_LEADER);
					}

					if(allow_exit)
					{
						fallback_co = FALLBACK_EXIT;
						fallback_t = -1.f;
						for(vector<Unit*>::iterator it = team.begin(), end = team.end(); it != end; ++it)
							(*it)->frozen = 2;
						if(IsOnline())
							PushNetChange(NetChange::LEAVE_LOCATION);
					}
				}
			}
			else
				unit.pos.y = 0.f;

			if(warped)
				return;

			if(unit.IsPlayer() && OR2_EQ(location->type, L_CITY, L_VILLAGE) && WantExitLevel() && unit.frozen == 0)
			{
				int id = 0;
				for(vector<InsideBuilding*>::iterator it = city_ctx->inside_buildings.begin(), end = city_ctx->inside_buildings.end(); it != end; ++it, ++id)
				{
					if((*it)->enter_area.IsInside(unit.pos))
					{
						if(IsLocal())
						{
							// wejd� do budynku
							fallback_co = FALLBACK_ENTER;
							fallback_t = -1.f;
							fallback_1 = id;
							unit.frozen = 2;
						}
						else
						{
							// info do serwera
							fallback_co = FALLBACK_WAIT_FOR_WARP;
							fallback_t = -1.f;
							unit.frozen = 2;
							NetChange& c = Add1(net_changes);
							c.type = NetChange::ENTER_BUILDING;
							c.id = id;
						}
						break;
					}
				}
			}
		}
		else
		{
			if(warped)
				return;

			// jest w budynku
			// sprawd� czy nie wszed� na wyj�cie (tylko gracz mo�e opuszcza� budynek, na razie)
			City* city = (City*)location;
			InsideBuilding& building = *city->inside_buildings[unit.in_building];

			if(unit.IsPlayer() && building.exit_area.IsInside(unit.pos) && WantExitLevel() && unit.frozen == 0)
			{
				if(IsLocal())
				{
					// opu�� budynek
					fallback_co = FALLBACK_ENTER;
					fallback_t = -1.f;
					fallback_1 = -1;
					unit.frozen = 2;
				}
				else
				{
					// info do serwera
					fallback_co = FALLBACK_WAIT_FOR_WARP;
					fallback_t = -1.f;
					unit.frozen = 2;
					PushNetChange(NetChange::EXIT_BUILDING);
				}
			}
		}
	}
	else
	{
		InsideLocation* inside = (InsideLocation*)location;
		InsideLocationLevel& lvl = inside->GetLevelData();
		INT2 pt = pos_to_pt(unit.pos);

		if(pt == lvl.schody_gora)
		{
			BOX2D box;
			switch(lvl.schody_gora_dir)
			{
			case 0:
				unit.pos.y = (unit.pos.z-2.f*lvl.schody_gora.y)/2;
				box = BOX2D(2.f*lvl.schody_gora.x, 2.f*lvl.schody_gora.y+1.4f, 2.f*(lvl.schody_gora.x+1), 2.f*(lvl.schody_gora.y+1));
				break;
			case 1:
				unit.pos.y = (unit.pos.x-2.f*lvl.schody_gora.x)/2;
				box = BOX2D(2.f*lvl.schody_gora.x+1.4f, 2.f*lvl.schody_gora.y, 2.f*(lvl.schody_gora.x+1), 2.f*(lvl.schody_gora.y+1));
				break;
			case 2:
				unit.pos.y = (2.f*lvl.schody_gora.y-unit.pos.z)/2+1.f;
				box = BOX2D(2.f*lvl.schody_gora.x, 2.f*lvl.schody_gora.y, 2.f*(lvl.schody_gora.x+1), 2.f*lvl.schody_gora.y+0.6f);
				break;
			case 3:
				unit.pos.y = (2.f*lvl.schody_gora.x-unit.pos.x)/2+1.f;
				box = BOX2D(2.f*lvl.schody_gora.x, 2.f*lvl.schody_gora.y, 2.f*lvl.schody_gora.x+0.6f, 2.f*(lvl.schody_gora.y+1));
				break;
			}

			if(warped)
				return;

			if(unit.IsPlayer() && WantExitLevel() && box.IsInside(unit.pos) && unit.frozen == 0)
			{
				if(IsLeader())
				{
					if(IsLocal())
					{
						int w = CanLeaveLocation(unit);
						if(w == 0)
						{
							fallback_co = FALLBACK_CHANGE_LEVEL;
							fallback_t = -1.f;
							fallback_1 = -1;
							for(vector<Unit*>::iterator it = team.begin(), end = team.end(); it != end; ++it)
								(*it)->frozen = 2;
							if(IsOnline())
								PushNetChange(NetChange::LEAVE_LOCATION);
						}
						else
							AddGameMsg3(w == 1 ? GMS_GATHER_TEAM : GMS_NOT_IN_COMBAT);
					}
					else
						Net_LeaveLocation(WHERE_LEVEL_UP);
				}
				else
					AddGameMsg3(GMS_NOT_LEADER);
			}
		}
		else if(pt == lvl.schody_dol)
		{
			BOX2D box;
			switch(lvl.schody_dol_dir)
			{
			case 0:
				unit.pos.y = (unit.pos.z-2.f*lvl.schody_dol.y)*-1.f;
				box = BOX2D(2.f*lvl.schody_dol.x, 2.f*lvl.schody_dol.y+1.4f, 2.f*(lvl.schody_dol.x+1), 2.f*(lvl.schody_dol.y+1));
				break;
			case 1:
				unit.pos.y = (unit.pos.x-2.f*lvl.schody_dol.x)*-1.f;
				box = BOX2D(2.f*lvl.schody_dol.x+1.4f, 2.f*lvl.schody_dol.y, 2.f*(lvl.schody_dol.x+1), 2.f*(lvl.schody_dol.y+1));
				break;
			case 2:
				unit.pos.y = (2.f*lvl.schody_dol.y-unit.pos.z)*-1.f-2.f;
				box = BOX2D(2.f*lvl.schody_dol.x, 2.f*lvl.schody_dol.y, 2.f*(lvl.schody_dol.x+1), 2.f*lvl.schody_dol.y+0.6f);
				break;
			case 3:
				unit.pos.y = (2.f*lvl.schody_dol.x-unit.pos.x)*-1.f-2.f;
				box = BOX2D(2.f*lvl.schody_dol.x, 2.f*lvl.schody_dol.y, 2.f*lvl.schody_dol.x+0.6f, 2.f*(lvl.schody_dol.y+1));
				break;
			}

			if(warped)
				return;

			if(unit.IsPlayer() && WantExitLevel() && box.IsInside(unit.pos) && unit.frozen == 0)
			{
				if(IsLeader())
				{
					if(IsLocal())
					{
						int w = CanLeaveLocation(unit);
						if(w == 0)
						{
							fallback_co = FALLBACK_CHANGE_LEVEL;
							fallback_t = -1.f;
							fallback_1 = +1;
							for(vector<Unit*>::iterator it = team.begin(), end = team.end(); it != end; ++it)
								(*it)->frozen = 2;
							if(IsOnline())
								PushNetChange(NetChange::LEAVE_LOCATION);
						}
						else
							AddGameMsg3(w == 1 ? GMS_GATHER_TEAM : GMS_NOT_IN_COMBAT);
					}
					else
						Net_LeaveLocation(WHERE_LEVEL_DOWN);
				}
				else
					AddGameMsg3(GMS_NOT_LEADER);
			}
		}
		else
			unit.pos.y = 0.f;

		if(warped)
			return;
	}

	// obs�uga portali
	if(unit.frozen == 0 && unit.IsPlayer() && WantExitLevel())
	{
		Portal* portal = location->portal;
		int index = 0;
		while(portal)
		{
			if(portal->target_loc != -1 && distance2d(unit.pos, portal->pos) < 2.f)
			{
				if(CircleToRotatedRectangle(unit.pos.x, unit.pos.z, unit.GetUnitRadius(), portal->pos.x, portal->pos.z, 0.67f, 0.1f, portal->rot))
				{
					if(IsLeader())
					{
						if(IsLocal())
						{
							int w = CanLeaveLocation(unit);
							if(w == 0)
							{
								fallback_co = FALLBACK_USE_PORTAL;
								fallback_t = -1.f;
								fallback_1 = index;
								for(vector<Unit*>::iterator it = team.begin(), end = team.end(); it != end; ++it)
									(*it)->frozen = 2;
								if(IsOnline())
									PushNetChange(NetChange::LEAVE_LOCATION);
							}
							else
								AddGameMsg3(w == 1 ? GMS_GATHER_TEAM : GMS_NOT_IN_COMBAT);
						}
						else
							Net_LeaveLocation(WHERE_PORTAL+index);
					}
					else
						AddGameMsg3(GMS_NOT_LEADER);

					break;
				}
			}
			portal = portal->next_portal;
			++index;
		}
	}

	if(!warped)
	{
		if(IsLocal() || &unit != pc->unit || interpolate_timer <= 0.f)
			unit.visual_pos = unit.pos;
		UpdateUnitPhysics(&unit, unit.pos);
	}
}

bool Game::RayToMesh(const VEC3& _ray_pos, const VEC3& _ray_dir, const VEC3& _obj_pos, float _obj_rot, VertexData* _vd, float& _dist)
{
	assert(_vd);

	// najpierw sprawd� kolizje promienia ze sfer� otaczaj�c� model
	if(!RayToSphere(_ray_pos, _ray_dir, _obj_pos, _vd->radius, _dist))
		return false;

	// przekszta�� promie� o pozycj� i obr�t modelu
	D3DXMatrixTranslation(&m1, _obj_pos);
	D3DXMatrixRotationY(&m2, _obj_rot);
	D3DXMatrixMultiply(&m3, &m2, &m1);
	D3DXMatrixInverse(&m1, NULL, &m3);

	VEC3 ray_pos, ray_dir;
	D3DXVec3TransformCoord(&ray_pos, &_ray_pos, &m1);
	D3DXVec3TransformNormal(&ray_dir, &_ray_dir, &m1);

	// szukaj kolizji
	_dist = 1.01f;
	float dist;
	bool hit = false;

	for(vector<Face>::iterator it = _vd->faces.begin(), end = _vd->faces.end(); it != end; ++it)
	{
		if(RayToTriangle(ray_pos, ray_dir, _vd->verts[it->idx[0]], _vd->verts[it->idx[1]], _vd->verts[it->idx[2]], dist) && dist < _dist && dist >= 0.f)
		{
			hit = true;
			_dist = dist;
		}
	}

	return hit;
}

bool Game::CollideWithStairs(const CollisionObject& _co, const VEC3& _pos, float _radius) const
{
	assert(_co.type == CollisionObject::CUSTOM && _co.check == &Game::CollideWithStairs && !location->outside && _radius > 0.f);

	InsideLocation* inside = (InsideLocation*)location;
	InsideLocationLevel& lvl = inside->GetLevelData();

	int dir;

	if(_co.extra == 0)
	{
		assert(!lvl.schody_dol_w_scianie);
		dir = lvl.schody_dol_dir;
	}
	else
		dir = lvl.schody_gora_dir;

	if(dir != 0)
	{
		if(CircleToRectangle(_pos.x, _pos.z, _radius, _co.pt.x, _co.pt.y-0.95f, 1.f, 0.05f))
			return true;
	}

	if(dir != 1)
	{
		if(CircleToRectangle(_pos.x, _pos.z, _radius, _co.pt.x-0.95f, _co.pt.y, 0.05f, 1.f))
			return true;
	}

	if(dir != 2)
	{
		if(CircleToRectangle(_pos.x, _pos.z, _radius, _co.pt.x, _co.pt.y+0.95f, 1.f, 0.05f))
			return true;
	}

	if(dir != 3)
	{
		if(CircleToRectangle(_pos.x, _pos.z, _radius, _co.pt.x+0.95f, _co.pt.y, 0.05f, 1.f))
			return true;
	}

	return false;
}

bool Game::CollideWithStairsRect(const CollisionObject& _co, const BOX2D& _box) const
{
	assert(_co.type == CollisionObject::CUSTOM && _co.check_rect == &Game::CollideWithStairsRect && !location->outside);

	InsideLocation* inside = (InsideLocation*)location;
	InsideLocationLevel& lvl = inside->GetLevelData();

	int dir;

	if(_co.extra == 0)
	{
		assert(!lvl.schody_dol_w_scianie);
		dir = lvl.schody_dol_dir;
	}
	else
		dir = lvl.schody_gora_dir;

	if(dir != 0)
	{
		if(RectangleToRectangle(_box.v1.x, _box.v1.y, _box.v2.x, _box.v2.y, _co.pt.x-1.f, _co.pt.y-1.f, _co.pt.x+1.f, _co.pt.y-0.9f))
			return true;
	}

	if(dir != 1)
	{
		if(RectangleToRectangle(_box.v1.x, _box.v1.y, _box.v2.x, _box.v2.y, _co.pt.x-1.f, _co.pt.y-1.f, _co.pt.x-0.9f, _co.pt.y+1.f))
			return true;
	}

	if(dir != 2)
	{
		if(RectangleToRectangle(_box.v1.x, _box.v1.y, _box.v2.x, _box.v2.y, _co.pt.x-1.f, _co.pt.y+0.9f, _co.pt.x+1.f, _co.pt.y+1.f))
			return true;
	}

	if(dir != 3)
	{
		if(RectangleToRectangle(_box.v1.x, _box.v1.y, _box.v2.x, _box.v2.y, _co.pt.x+0.9f, _co.pt.y-1.f, _co.pt.x+1.f, _co.pt.y+1.f))
			return true;
	}

	return false;
}

inline bool IsNotNegative(const VEC3& v)
{
	return v.x >= 0.f && v.y >= 0.f && v.z >= 0.f;
}

void Game::TestGameData(bool _major)
{
	/*Unit u;
	for(uint i=0; i<n_base_units; ++i)
	{
		UnitData& ud = g_base_units[i];
		for(int j=ud.level.x; j<=ud.level.y; ++j)
		{
			CreateUnit(ud, j, NULL, false, &u);
			float dmg = u.CalculateAttack(),
				  def = u.CalculateDefense();
			LOG(format("%s, poziom %d, si� %d, kon %d, zr� %d, walka %d, lekki p %d, ci�ki p %d, �uk %d, tarcza %d, hp %g, atak %g, obrona %g, ratio %g", ud.name, j, u.attrib[A_STR],
				u.attrib[A_CON], u.attrib[A_DEX], u.skill[S_WEAPON], u.skill[S_LIGHT_ARMOR], u.skill[S_HEAVY_ARMOR], u.skill[S_BOW], u.skill[S_SHIELD], u.hpmax, dmg, def, u.hpmax/(dmg-def)));
			u.items.clear();
			u.weapon = -1;
			u.armor = -1;
			u.bow = -1;
			u.shield = -1;
		}
	}*/

	//ExportItems();

	LOG("Test: Testing game data. It can take some time...");
	string str;
	uint errors = 0;

	LOG("Test: Checking items...");

	// bronie
	for(uint i=0; i<n_weapons; ++i)
	{
		const Weapon& w = g_weapons[i];
		if(!w.ani)
		{
			ERROR(Format("Test: Weapon %s: missing mesh %s.", w.id, w.mesh));
			++errors;
		}
		else
		{
			Animesh::Point* pt = w.ani->FindPoint("hit");
			if(!pt || !pt->IsBox())
			{
				ERROR(Format("Test: Weapon %s: no hitbox in mesh %s.", w.id, w.mesh));
				++errors;
			}
			else if(!IsNotNegative(pt->size))
			{
				ERROR(Format("Test: Weapon %s: invalid hitbox %g, %g, %g in mesh %s.", w.id, pt->size.x, pt->size.y, pt->size.z, w.mesh));
				++errors;
			}
		}
	}

	// tarcze
	for(uint i=0; i<n_shields; ++i)
	{
		const Shield& s = g_shields[i];
		if(!s.ani)
		{
			ERROR(Format("Test: Shield %s: missing mesh %s.", s.id, s.mesh));
			++errors;
		}
		else
		{
			Animesh::Point* pt = s.ani->FindPoint("hit");
			if(!pt || !pt->IsBox())
			{
				ERROR(Format("Test: Shield %s: no hitbox in mesh %s.", s.id, s.mesh));
				++errors;
			}
			else if(!IsNotNegative(pt->size))
			{
				ERROR(Format("Test: Shield %s: invalid hitbox %g, %g, %g in mesh %s.", s.id, pt->size.x, pt->size.y, pt->size.z, s.mesh));
				++errors;
			}
		}
	}

	// postacie
	LOG("Test: Checking units...");
	for(uint i=0; i<n_base_units; ++i)
	{
		UnitData& ud = g_base_units[i];
		str.clear();

		// przedmioty postaci
		if(ud.items)
			TestItemScript(ud.items, str, errors);

		// czary postaci
		if(ud.spells)
			TestUnitSpells(*ud.spells, str, errors);

		// ataki
		if(ud.frames)
		{
			if(ud.frames->extra)
			{
				if(in_range(ud.frames->attacks, 1, NAMES::max_attacks))
				{
					for(int i=0; i<ud.frames->attacks; ++i)
					{
						if(!in_range2(0.f, ud.frames->extra->e[i].start, ud.frames->extra->e[i].end, 1.f))
						{
							str += Format("\tInvalid values in attack %d (%g, %g).\n", i+1, ud.frames->extra->e[i].start, ud.frames->extra->e[i].end);
							++errors;
						}
					}
				}
				else
				{
					str += Format("\tInvalid attacks count (%d).\n", ud.frames->attacks);
					++errors;
				}
			}
			else
			{
				if(in_range(ud.frames->attacks, 1, 3))
				{
					for(int i=0; i<ud.frames->attacks; ++i)
					{
						if(!in_range2(0.f, ud.frames->t[F_ATTACK1_START+i*2], ud.frames->t[F_ATTACK1_END+i*2], 1.f))
						{
							str += Format("\tInvalid values in attack %d (%g, %g).\n", i+1, ud.frames->t[F_ATTACK1_START+i*2],
								ud.frames->t[F_ATTACK1_END+i*2]);
							++errors;
						}
					}
				}
				else
				{
					str += Format("\tInvalid attacks count (%d).\n", ud.frames->attacks);
					++errors;
				}
			}
			if(!in_range(ud.frames->t[F_BASH], 0.f, 1.f))
			{
				str += Format("\tInvalid attack point '%g' bash.\n", ud.frames->t[F_BASH]);
				++errors;
			}
		}
		else
		{
			str += "\tMissing attacks information.\n";
			++errors;
		}

		// model postaci
		if(!ud.mesh)
		{
			if(!IS_SET(ud.flagi, F_CZLOWIEK))
			{
				str += "\tNo mesh!\n";
				++errors;
			}
		}
		else if(_major)
		{
			if(!ud.ani)
			{
				str += Format("\tMissing mesh %s.\n", ud.mesh);
				++errors;
			}
			else
			{
				Animesh& a = *ud.ani;

				for(uint i=0; i<NAMES::n_ani_base; ++i)
				{
					if(!a.GetAnimation(NAMES::ani_base[i]))
					{
						str += Format("\tMissing animation '%s'.\n", NAMES::ani_base[i]);
						++errors;
					}
				}

				if(!IS_SET(ud.flagi, F_POWOLNY))
				{
					if(!a.GetAnimation(NAMES::ani_run))
					{
						str += Format("\tMissing animation '%s'.\n", NAMES::ani_run);
						++errors;
					}
				}

				if(!IS_SET(ud.flagi, F_NIE_CIERPI))
				{
					if(!a.GetAnimation(NAMES::ani_hurt))
					{
						str += Format("\tMissing animation '%s'.\n", NAMES::ani_hurt);
						++errors;
					}
				}

				if(IS_SET(ud.flagi, F_CZLOWIEK) || IS_SET(ud.flagi, F_HUMANOID))
				{
					for(uint i=0; i<NAMES::n_points; ++i)
					{
						if(!a.GetPoint(NAMES::points[i]))
						{
							str += Format("\tMissing attachment point '%s'.\n", NAMES::points[i]);
							++errors;
						}
					}

					for(uint i=0; i<NAMES::n_ani_humanoid; ++i)
					{
						if(!a.GetAnimation(NAMES::ani_humanoid[i]))
						{
							str += Format("\tMissing animation '%s'.\n", NAMES::ani_humanoid[i]);
							++errors;
						}
					}
				}

				// animacje atak�w
				if(ud.frames)
				{
					if(ud.frames->attacks > NAMES::max_attacks)
					{
						str += Format("\tToo many attacks (%d)!\n", ud.frames->attacks);
						++errors;
					}
					for(int i=0; i<min(ud.frames->attacks, NAMES::max_attacks); ++i)
					{
						if(!a.GetAnimation(NAMES::ani_attacks[i]))
						{
							str += Format("\tMissing animation '%s'.\n", NAMES::ani_attacks[i]);
							++errors;
						}
					}
				}

				// animacje idle
				if(ud.idles)
				{
					for(int i=0; i<ud.idles_count; ++i)
					{
						if(!a.GetAnimation(ud.idles[i]))
						{
							str += Format("\tMissing animation '%s'.\n", ud.idles[i]);
							++errors;
						}
					}
				}

				// punkt czaru
				if(ud.spells && !a.GetPoint(NAMES::point_cast))
				{
					str += Format("\tMissing attachment point '%s'.\n", NAMES::point_cast);
					++errors;
				}
			}
		}

		if(!str.empty())
			ERROR(Format("Test: Unit %s:\n%s", ud.id, str.c_str()));
	}

	LOG("Test: Testing completed.");

	if(errors)
		ERROR(Format("Test: Game errors count: %d!", errors));
}

#define S(x) (*(cstring*)(x))

void Game::TestItemScript(const int* _script, string& _errors, uint& _count)
{
	assert(_script);

	const int* ps = _script;
	const Item* item;
	int a, b, depth=0, elsel = 0;

	while(*ps != PS_KONIEC)
	{
		switch(*ps)
		{
		case PS_DAJ:
			++ps;
			item = FindItem(S(ps), false);
			if(!item)
			{
				_errors += Format("\tMissing item '%s'.\n", S(ps));
				++_count;
			}
			break;
		case PS_JEDEN_Z_WIELU:
			++ps;
			if((a = *ps) == 0)
				_errors += "\tOne from many: 0.\n";
			else
			{
				for(int i=0; i<a; ++i)
				{
					++ps;
					item = FindItem(S(ps), false);
					if(!item)
					{
						_errors += Format("\tMissing item '%s'.\n", S(ps));
						++_count;
					}
				}
			}
			break;
		case PS_SZANSA:
			++ps;
			a = *ps;
			if(a <= 0 || a >= 100)
			{
				_errors += Format("\tChance %d.\n", a);
				++_count;
			}
			++ps;
			item = FindItem(S(ps), false);
			if(!item)
			{
				_errors += Format("\tMissing item '%s'.\n", S(ps));
				++_count;
			}
			break;
		case PS_SZANSA2:
			++ps;
			a = *ps;
			if(a <= 0 || a >= 100)
			{
				_errors += Format("\tChance2 %d.\n", a);
				++_count;
			}
			++ps;
			item = FindItem(S(ps), false);
			if(!item)
			{
				_errors += Format("\tMissing item '%s'.\n", S(ps));
				++_count;
			}
			++ps;
			item = FindItem(S(ps), false);
			if(!item)
			{
				_errors += Format("\tMissing item '%s'.\n", S(ps));
				++_count;
			}
			break;
		case PS_IF_SZANSA:
			++ps;
			a = *ps;
			if(a <= 0 || a >= 100)
			{
				_errors += Format("\tIf chance %d.\n", a);
				++_count;
			}
			CLEAR_BIT(elsel, BIT(depth));
			++depth;
			break;
		case PS_POZIOM:
			++ps;
			a = *ps;
			if(a <= 1)
			{
				_errors += Format("\tLevel %d.\n", a);
				++_count;
			}
			CLEAR_BIT(elsel, BIT(depth));
			++depth;
			break;
		case PS_ELSE:
			if(depth == 0)
			{
				_errors += "\tElse without if.\n";
				++_count;
			}
			else
			{
				if(IS_SET(elsel, BIT(depth-1)))
				{
					_errors += "\tNext else.\n";
					++_count;
				}
				else
					SET_BIT(elsel, BIT(depth-1));
			}
			break;
		case PS_END_IF:
			if(depth == 0)
			{
				_errors += "\tEnd if without if.\n";
				++_count;
			}
			else
				--depth;
			break;
		case PS_DAJ_KILKA:
			++ps;
			a = *ps;
			if(a < 0)
			{
				_errors += Format("\tGive some %d.\n", a);
				++_count;
			}
			++ps;
			item = FindItem(S(ps), false);
			if(!item)
			{
				_errors += Format("\tMissing item '%s'.\n", S(ps));
				++_count;
			}
			break;
		case PS_LOSOWO:
			++ps;
			a = *ps;
			++ps;
			b = *ps;
			++ps;
			if(a < 0 || b < 0 || a >= b)
			{
				_errors += Format("\tGive random %d, %d.", a, b);
				++_count;
			}
			item = FindItem(S(ps), false);
			if(!item)
			{
				_errors += Format("\tMissing item '%s'.\n", S(ps));
				++_count;
			}
			break;
		}

		++ps;
	}

	if(depth != 0)
	{
		_errors += Format("\tUnclosed ifs %d.\n", depth);
		++_count;
	}
}

void Game::TestUnitSpells(const SpellList& _spells, string& _errors, uint& _count)
{
	for(int i=0; i<3; ++i)
	{
		if(_spells.name[i])
		{
			Spell* spell = FindSpell(_spells.name[i]);
			if(!spell)
			{
				_errors += Format("\tMissing spell '%s'.\n", _spells.name[i]);
				++_count;
			}
		}
	}
}

Unit* Game::CreateUnit(UnitData& _base, int level, Human* _human_data, bool create_physics, Unit* test_unit)
{
	Unit* u;
	if(test_unit)
		u = test_unit;
	else
		u = new Unit;

	u->human_data = NULL;

	// typ
	if(!test_unit)
	{
		if(IS_SET(_base.flagi, F_CZLOWIEK))
		{
			u->type = Unit::HUMAN;
			if(_human_data)
				u->human_data = _human_data;
			else
			{
#define HEX(h) VEC4(1.f/256*(((h)&0xFF0000)>>16), 1.f/256*(((h)&0xFF00)>>8), 1.f/256*((h)&0xFF), 1.f)
				u->human_data = new Human;
				u->human_data->beard = rand2()%MAX_BEARD-1;
				u->human_data->hair = rand2()%MAX_HAIR-1;
				u->human_data->height = random(0.9f, 1.1f);
				if(IS_SET(_base.flagi2, F2_STARY))
					u->human_data->hair_color = HEX(0xDED5D0);
				else if(IS_SET(_base.flagi, F_SZALONY))
					u->human_data->hair_color = VEC4(random(0.f,1.f), random(0.f,1.f), random(0.f,1.f), 1.f);
				else if(IS_SET(_base.flagi, F_SZARE_WLOSY))
					u->human_data->hair_color = g_hair_colors[rand2()%4];
				else if(IS_SET(_base.flagi, F_TOMASH))
				{
					u->human_data->beard = 4;
					u->human_data->mustache = -1;
					u->human_data->hair = 0;
					u->human_data->hair_color = g_hair_colors[0];
					u->human_data->height = 1.1f;
				}
				else
					u->human_data->hair_color = g_hair_colors[rand2()%n_hair_colors];
				u->human_data->mustache = rand2()%MAX_MUSTACHE-1;
				u->human_data->ApplyScale(aHumanBase);
#undef HEX
			}

			u->ani = new AnimeshInstance(aHumanBase);
		}
		else
		{
			u->ani = new AnimeshInstance(_base.ani);
			
			if(IS_SET(_base.flagi, F_HUMANOID))
				u->type = Unit::HUMANOID;
			else
				u->type = Unit::ANIMAL;
		}

		u->animacja = u->animacja2 = ANI_STOI;
		u->ani->Play("stoi", PLAY_PRIO1|PLAY_NO_BLEND, 0);

		if(u->ani->ani->head.n_groups > 1)
			u->ani->groups[1].state = 0;

		u->ani->ptr = u;
	}

	u->pos = VEC3(0,0,0);
	u->rot = 0.f;
	u->used_item = NULL;
	u->live_state = Unit::ALIVE;
	for(int i=0; i<SLOT_MAX; ++i)
		u->slots[i] = NULL;
	u->action = A_NONE;
	u->wyjeta = B_BRAK;
	u->chowana = B_BRAK;
	u->stan_broni = BRON_SCHOWANA;
	u->data = &_base;
	if(level == -2)
		u->level = random2(_base.level);
	else if(level == -3)
		u->level = clamp(dungeon_level, _base.level);
	else
		u->level = clamp(level, _base.level);
	u->player = NULL;
	u->ai = NULL;
	u->speed = u->prev_speed = 0.f;
	u->invisible = false;
	u->hurt_timer = 0.f;
	u->talking = false;
	u->useable = NULL;
	u->in_building = -1;
	u->frozen = 0;
	u->in_arena = -1;
	u->atak_w_biegu = false;
	u->event_handler = NULL;
	u->to_remove = false;
	u->temporary = false;
	u->quest_refid = -1;
	u->bubble = NULL;
	u->busy = Unit::Busy_No;
	u->look_target = NULL;
	u->interp = NULL;
	u->dont_attack = false;
	u->assist = false;
	u->auto_talk = 0;
	u->attack_team = false;
	//u->block_power = 1.f;
	u->last_bash = 0.f;
	u->guard_target = NULL;
	u->alcohol = 0.f;

	float t;
	if(_base.level.x == _base.level.y)
		t = 1.f;
	else
		t = float(u->level-_base.level.x)/(_base.level.y-_base.level.x);

	// attributes & skills
	u->data->GetStatProfile().Set(u->level, u->attrib, u->skill);

	// przedmioty
	u->weight = 0;
	u->CalculateLoad();
	if(_base.items)
	{
		ParseItemScript(*u, _base.items);
		SortItems(u->items);
		u->RecalculateWeight();
	}

	// hp
	u->hp = u->hpmax = u->CalculateMaxHp();

	// z�oto
	u->gold = random2(lerp(_base.gold, _base.gold2, t));

	if(!test_unit)
	{
		if(IS_SET(_base.flagi, F_BOHATER))
		{
			u->hero = new HeroData;
			u->hero->Init(*u);
		}
		else
			u->hero = NULL;

		if(IS_SET(u->data->flagi2, F2_BOSS))
			boss_levels.push_back(INT2(current_location, dungeon_level));

		// kolizje
		if(create_physics)
		{
			btCapsuleShape* caps = new btCapsuleShape(u->GetUnitRadius(), max(MIN_H, u->GetUnitHeight()));
			u->cobj = new btCollisionObject;
			u->cobj->setCollisionShape(caps);
			u->cobj->setUserPointer(u);
			u->cobj->setCollisionFlags(CG_UNIT);
			phy_world->addCollisionObject(u->cobj);
		}
		else
			u->cobj = NULL;
	}

	if(IsOnline() && IsServer())
		u->netid = netid_counter++;

	return u;
}

void Game::ParseItemScript(Unit& _unit, const int* _script)
{
	assert(_script);

	const int* ps = _script;
	int a, b, depth=0, depth_if=0;

	while(*ps != PS_KONIEC)
	{
		switch(*ps)
		{
		case PS_DAJ:
			++ps;
			if(depth == depth_if)
				_unit.AddItemAndEquipIfNone(FindItem(S(ps)));
			break;
		case PS_JEDEN_Z_WIELU:
			++ps;
			a = *ps;
			b = rand2()%a;
			if(depth == depth_if)
				_unit.AddItemAndEquipIfNone(FindItem(S(ps+b+1)));
			ps += a;
			break;
		case PS_SZANSA:
			++ps;
			a = *ps;
			++ps;
			if(depth == depth_if && rand2()%100 < a)
				_unit.AddItemAndEquipIfNone(FindItem(S(ps)));
			break;
		case PS_SZANSA2:
			++ps;
			a = *ps;
			++ps;
			if(depth == depth_if)
			{
				if(rand2()%100 < a)
				{
					_unit.AddItemAndEquipIfNone(FindItem(S(ps)));
					++ps;
				}
				else
				{
					++ps;
					_unit.AddItemAndEquipIfNone(FindItem(S(ps)));
				}
			}
			else
				++ps;
			break;
		case PS_IF_SZANSA:
			++ps;
			a = *ps;
			if(depth == depth_if && rand2()%100 < a)
				++depth_if;
			++depth;
			break;
		case PS_POZIOM:
			++ps;
			if(depth == depth_if && _unit.level >= *ps)
				++depth_if;
			++depth;
			break;
		case PS_ELSE:
			if(depth == depth_if)
				--depth_if;
			else if(depth == depth_if+1)
				++depth_if;
			break;
		case PS_END_IF:
			if(depth == depth_if)
				--depth_if;
			--depth;
			break;
		case PS_DAJ_KILKA:
			++ps;
			a = *ps;
			++ps;
			if(depth == depth_if)
			{
				const ItemList* lis = NULL;
				const Item* item = FindItem(S(ps), true, &lis);
				if(!lis || a == 1)
					_unit.AddItemAndEquipIfNone(item, a);
				else
				{
					_unit.AddItemAndEquipIfNone(item);
					for(int i=1; i<a; ++i)
						_unit.AddItemAndEquipIfNone(lis->Get());
				}
			}
			break;
		case PS_LOSOWO:
			++ps;
			a = *ps;
			++ps;
			b = *ps;
			++ps;
			a = random(a,b);
			if(depth == depth_if && a > 0)
			{
				const ItemList* lis = NULL;
				const Item* item = FindItem(S(ps), true, &lis);
				if(!lis || a == 1)
					_unit.AddItemAndEquipIfNone(item, a);
				else
				{
					_unit.AddItemAndEquipIfNone(item);
					for(int i=1; i<a; ++i)
						_unit.AddItemAndEquipIfNone(lis->Get());
				}
			}
			break;
		}

		++ps;
	}
}

bool Game::IsEnemy(Unit &u1, Unit &u2, bool ignore_dont_attack)
{
	if(u1.in_arena == -1 && u2.in_arena == -1)
	{
		if(!ignore_dont_attack)
		{
			if(u1.IsAI() && IsUnitDontAttack(u1))
				return false;
			if(u2.IsAI() && IsUnitDontAttack(u2))
				return false;
		}

		GRUPA g1 = u1.data->grupa,
			g2 = u2.data->grupa;

		if(u1.IsTeamMember())
			g1 = G_DRUZYNA;
		if(u2.IsTeamMember())
			g2 = G_DRUZYNA;

		if(g1 == g2)
			return false;
		else if(g1 == G_MIESZKANCY)
		{
			if(g2 == G_SZALENI)
				return true;
			else if(g2 == G_DRUZYNA)
				return bandyta || WantAttackTeam(u1);
			else
				return true;
		}
		else if(g1 == G_SZALENI)
		{
			if(g2 == G_MIESZKANCY)
				return true;
			else if(g2 == G_DRUZYNA)
				return atak_szalencow || WantAttackTeam(u1);
			else
				return true;
		}
		else if(g1 == G_DRUZYNA)
		{
			if(WantAttackTeam(u2))
				return true;
			else if(g2 == G_MIESZKANCY)
				return bandyta;
			else if(g2 == G_SZALENI)
				return atak_szalencow;
			else
				return true;
		}
		else
			return true;
	}
	else
	{
		if(u1.in_arena == -1 || u2.in_arena == -1)
			return false;
		else if(u1.in_arena == u2.in_arena)
			return false;
		else
			return true;
	}
}

bool Game::IsFriend(Unit& u1, Unit& u2)
{
	if(u1.in_arena == -1 && u2.in_arena == -1)
	{
		if(u1.IsTeamMember())
		{
			if(u2.IsTeamMember())
				return true;
			else if(u2.IsAI() && !bandyta && IsUnitAssist(u2))
				return true;
			else
				return false;
		}
		else if(u2.IsTeamMember())
		{
			if(u1.IsAI() && !bandyta && IsUnitAssist(u1))
				return true;
			else
				return false;
		}
		else
			return (u1.data->grupa == u2.data->grupa);
	}
	else
		return u1.in_arena == u2.in_arena;
}

bool Game::CanSee(Unit& u1, Unit& u2)
{
	if(u1.in_building != u2.in_building)
		return false;

	INT2 tile1(int(u1.pos.x/2),int(u1.pos.z/2)),
		 tile2(int(u2.pos.x/2),int(u2.pos.z/2));

	if(tile1 == tile2)
		return true;

	LevelContext& ctx = GetContext(u1);

	if(ctx.type == LevelContext::Outside)
	{
		const uint _s = 16 * 8;
		OutsideLocation* outside = (OutsideLocation*)location;

		int xmin = max(0, min(tile1.x, tile2.x)),
			xmax = min((int)_s, max(tile1.x, tile2.x)),
			ymin = max(0, min(tile1.y, tile2.y)),
			ymax = min((int)_s, max(tile1.y, tile2.y));

		for(int y=ymin; y<=ymax; ++y)
		{
			for(int x=xmin; x<=xmax; ++x)
			{
				if(outside->tiles[x+y*_s].mode >= TM_BUILDING_BLOCK && LineToRectangle(u1.pos, u2.pos, VEC2(2.f*x, 2.f*y), VEC2(2.f*(x+1),2.f*(y+1))))
					return false;
			}
		}
	}
	else if(ctx.type == LevelContext::Inside)
	{
		InsideLocation* inside = (InsideLocation*)location;
		InsideLocationLevel& lvl = inside->GetLevelData();

		int xmin = max(0, min(tile1.x, tile2.x)),
			xmax = min(lvl.w, max(tile1.x, tile2.x)),
			ymin = max(0, min(tile1.y, tile2.y)),
			ymax = min(lvl.h, max(tile1.y, tile2.y));

		for(int y=ymin; y<=ymax; ++y)
		{
			for(int x=xmin; x<=xmax; ++x)
			{
				if(czy_blokuje2(lvl.mapa[x+y*lvl.w].co) && LineToRectangle(u1.pos, u2.pos, VEC2(2.f*x, 2.f*y), VEC2(2.f*(x+1),2.f*(y+1))))
					return false;
				if(lvl.mapa[x+y*lvl.w].co == DRZWI)
				{
					Door* door = FindDoor(ctx, INT2(x,y));
					if(door && door->IsBlocking())
					{
						// 0.842f, 1.319f, 0.181f
						BOX2D box(door->pos.x, door->pos.z);
						if(door->rot == 0.f || door->rot == PI)
						{
							box.v1.x -= 1.f;
							box.v2.x += 1.f;
							box.v1.y -= 0.181f;
							box.v2.y += 0.181f;
						}
						else
						{
							box.v1.y -= 1.f;
							box.v2.y += 1.f;
							box.v1.x -= 0.181f;
							box.v2.x += 0.181f;
						}

						if(LineToRectangle(u1.pos, u2.pos, box.v1, box.v2))
							return false;
					}
				}
			}
		}
	}
	else
	{
		// kolizje z obiektami
		for(vector<CollisionObject>::iterator it = ctx.colliders->begin(), end = ctx.colliders->end(); it != end; ++it)
		{
			if(it->ptr != CAM_COLLIDER || it->type != CollisionObject::RECTANGLE)
				continue;

			BOX2D box(it->pt.x-it->w, it->pt.y-it->h, it->pt.x+it->w, it->pt.y+it->h);
			if(LineToRectangle(u1.pos, u2.pos, box.v1, box.v2))
				return false;
		}

		// kolizje z drzwiami
		for(vector<Door*>::iterator it = ctx.doors->begin(), end = ctx.doors->end(); it != end; ++it)
		{
			Door& door = **it;
			if(door.IsBlocking())
			{
				BOX2D box(door.pos.x, door.pos.z);
				if(door.rot == 0.f || door.rot == PI)
				{
					box.v1.x -= 1.f;
					box.v2.x += 1.f;
					box.v1.y -= 0.181f;
					box.v2.y += 0.181f;
				}
				else
				{
					box.v1.y -= 1.f;
					box.v2.y += 1.f;
					box.v1.x -= 0.181f;
					box.v2.x += 0.181f;
				}

				if(LineToRectangle(u1.pos, u2.pos, box.v1, box.v2))
					return false;
			}
		}
	}

	return true;
}

bool Game::CanSee(const VEC3& v1, const VEC3& v2)
{
	INT2 tile1(int(v1.x/2),int(v1.z/2)),
		tile2(int(v2.x/2),int(v2.z/2));

	if(tile1 == tile2)
		return true;

	LevelContext& ctx = local_ctx;

	if(ctx.type == LevelContext::Outside)
	{
		const uint _s = 16 * 8;
		OutsideLocation* outside = (OutsideLocation*)location;

		int xmin = max(0, min(tile1.x, tile2.x)),
			xmax = min((int)_s, max(tile1.x, tile2.x)),
			ymin = max(0, min(tile1.y, tile2.y)),
			ymax = min((int)_s, max(tile1.y, tile2.y));

		for(int y=ymin; y<=ymax; ++y)
		{
			for(int x=xmin; x<=xmax; ++x)
			{
				if(outside->tiles[x+y*_s].mode >= TM_BUILDING_BLOCK && LineToRectangle(v1, v2, VEC2(2.f*x, 2.f*y), VEC2(2.f*(x+1),2.f*(y+1))))
					return false;
			}
		}
	}
	else if(ctx.type == LevelContext::Inside)
	{
		InsideLocation* inside = (InsideLocation*)location;
		InsideLocationLevel& lvl = inside->GetLevelData();

		int xmin = max(0, min(tile1.x, tile2.x)),
			xmax = min(lvl.w, max(tile1.x, tile2.x)),
			ymin = max(0, min(tile1.y, tile2.y)),
			ymax = min(lvl.h, max(tile1.y, tile2.y));

		for(int y=ymin; y<=ymax; ++y)
		{
			for(int x=xmin; x<=xmax; ++x)
			{
				if(czy_blokuje2(lvl.mapa[x+y*lvl.w].co) && LineToRectangle(v1, v2, VEC2(2.f*x, 2.f*y), VEC2(2.f*(x+1),2.f*(y+1))))
					return false;
				if(lvl.mapa[x+y*lvl.w].co == DRZWI)
				{
					Door* door = FindDoor(ctx, INT2(x,y));
					if(door && door->IsBlocking())
					{
						// 0.842f, 1.319f, 0.181f
						BOX2D box(door->pos.x, door->pos.z);
						if(door->rot == 0.f || door->rot == PI)
						{
							box.v1.x -= 1.f;
							box.v2.x += 1.f;
							box.v1.y -= 0.181f;
							box.v2.y += 0.181f;
						}
						else
						{
							box.v1.y -= 1.f;
							box.v2.y += 1.f;
							box.v1.x -= 0.181f;
							box.v2.x += 0.181f;
						}

						if(LineToRectangle(v1, v2, box.v1, box.v2))
							return false;
					}
				}
			}
		}
	}
	else
	{
		// kolizje z obiektami
		/*for(vector<CollisionObject>::iterator it = ctx.colliders->begin(), end = ctx.colliders->end(); it != end; ++it)
		{
			if(it->ptr != (void*)1 || it->type != CollisionObject::RECTANGLE)
				continue;

			BOX2D box(it->pt.x-it->w, it->pt.y-it->h, it->pt.x+it->w, it->pt.y+it->h);
			if(LineToRectangle(u1.pos, u2.pos, box.v1, box.v2))
				return false;
		}

		// kolizje z drzwiami
		for(vector<Door*>::iterator it = ctx.doors->begin(), end = ctx.doors->end(); it != end; ++it)
		{
			Door& door = **it;
			if(door.IsBlocking())
			{
				BOX2D box(door.pos.x, door.pos.z);
				if(door.rot == 0.f || door.rot == PI)
				{
					box.v1.x -= 1.f;
					box.v2.x += 1.f;
					box.v1.y -= 0.181f;
					box.v2.y += 0.181f;
				}
				else
				{
					box.v1.y -= 1.f;
					box.v2.y += 1.f;
					box.v1.x -= 0.181f;
					box.v2.x += 0.181f;
				}

				if(LineToRectangle(u1.pos, u2.pos, box.v1, box.v2))
					return false;
			}
		}*/
	}

	return true;
}

bool Game::CheckForHit(LevelContext& ctx, Unit& _unit, Unit*& _hitted, VEC3& _hitpoint)
{	
	// atak broni� lub naturalny

	Animesh::Point* hitbox, *point;

	if(_unit.ani->ani->head.n_groups > 1)
	{
		hitbox = _unit.GetWeapon().ani->FindPoint("hit");
		point = _unit.ani->ani->GetPoint(NAMES::point_weapon);
		assert(point);
	}
	else
	{
		point = NULL;
		hitbox = _unit.ani->ani->GetPoint(Format("hitbox%d", _unit.attack_id+1));
		if(!hitbox)
			hitbox = _unit.ani->ani->FindPoint("hitbox");
	}

	assert(hitbox);

	return CheckForHit(ctx, _unit, _hitted, *hitbox, point, _hitpoint);
}

// Sprawdza czy jest kolizja hitboxa z jak�� postaci�
// S� dwie opcje:
//  - _bone to punkt "bron", _hitbox to hitbox z bronii
//  - _bone jest NULL, _hitbox jest na modelu postaci
// Na drodze test�w ustali�em �e najlepiej dzia�a gdy stosuje si� sam� funkcj� OrientedBoxToOrientedBox
bool Game::CheckForHit(LevelContext& ctx, Unit& _unit, Unit*& _hitted, Animesh::Point& _hitbox, Animesh::Point* _bone, VEC3& _hitpoint)
{
	assert(_hitted && _hitbox.IsBox());

	//BOX box1, box2;

	// ustaw ko�ci
	if(_unit.human_data)
		_unit.ani->SetupBones(&_unit.human_data->mat_scale[0]);
	else
		_unit.ani->SetupBones();

	// oblicz macierz hitbox

	// transformacja postaci
	D3DXMatrixTranslation(&m1, _unit.pos);
	D3DXMatrixRotationY(&m2, _unit.rot);
	D3DXMatrixMultiply(&m1, &m2, &m1); // m1 (World) = Rot * Pos

	if(_bone)
	{
		// transformacja punktu broni
		D3DXMatrixMultiply(&m2, &_bone->mat, &_unit.ani->mat_bones[_bone->bone]); // m2 = PointMatrix * BoneMatrix
		D3DXMatrixMultiply(&m3, &m2, &m1); // m3 = PointMatrix * BoneMatrix * UnitRot * UnitPos

		// transformacja hitboxa broni
		D3DXMatrixMultiply(&m1, &_hitbox.mat, &m3); // m1 = BoxMatrix * PointMatrix * BoneMatrix * UnitRot * UnitPos
	}
	else
	{
		D3DXMatrixMultiply(&m2, &_hitbox.mat, &_unit.ani->mat_bones[_hitbox.bone]);
		D3DXMatrixMultiply(&m3, &m2, &m1);
		m1 = m3;
	}

	// m1 to macierz hitboxa
	// teraz mo�emy stworzy� AABBOX wok� broni
	//CreateAABBOX(box1, m1);

	// przy okazji stw�rz obr�cony BOX
	/*OOBBOX obox1, obox2;
	D3DXMatrixIdentity(&obox2.rot);
	D3DXVec3TransformCoord(&obox1.pos, &VEC3(0,0,0), &m1);
	obox1.size = _hitbox.size*2;
	obox1.rot = m1;
	obox1.rot._11 = obox2.rot._22 = obox2.rot._33 = 1.f;

	// szukaj kolizji
	for(vector<Unit*>::iterator it = ctx.units->begin(), end = ctx.units->end(); it != end; ++it)
	{
		if(*it == &_unit || !(*it)->IsAlive() || distance((*it)->pos, _unit.pos) > 5.f || IsFriend(_unit, **it))
			continue;

		BOX box2;
		(*it)->GetBox(box2);

		//bool jest1 = BoxToBox(box1, box2);

		// wst�pnie mo�e by� tu jaka� kolizja, trzeba sprawdzi� dok�adniejszymi metodami
		// stw�rz obr�cony box
		obox2.pos = box2.Midpoint();
		obox2.size = box2.Size()*2;

		// sprawd� czy jest kolizja
		//bool jest2 = OrientedBoxToOrientedBox(obox1, obox2, &_hitpoint);


		//if(jest1 && jest2)
		if(OrientedBoxToOrientedBox(obox1, obox2, &_hitpoint))
		{
			// teraz z kolei hitpoint jest za daleko >:(
			float dist = distance2d((*it)->pos, _hitpoint);
			float r = (*it)->GetUnitRadius()*3/2;
			if(dist > r)
			{
				VEC3 dir = _hitpoint - (*it)->pos;
				D3DXVec3Length(&dir);
				dir *= r;
				dir.y = _hitpoint.y;
				_hitpoint = dir;
				_hitpoint.x += (*it)->pos.x;
				_hitpoint.z += (*it)->pos.z;
			}
			if(_hitpoint.y > (*it)->pos.y + (*it)->GetUnitHeight())
				_hitpoint.y = (*it)->pos.y + (*it)->GetUnitHeight();
			// jest kolizja
			_hitted = *it;
			return true;
		}
	}*/

	// a - hitbox broni, b - hitbox postaci
	OOB a, b;
	m1._11 = m1._22 = m1._33 = 1.f;
	D3DXVec3TransformCoord(&a.c, &VEC3(0,0,0), &m1);
	a.e = _hitbox.size;
	a.u[0] = VEC3(m1._11, m1._12, m1._13);
	a.u[1] = VEC3(m1._21, m1._22, m1._23);
	a.u[2] = VEC3(m1._31, m1._32, m1._33);
	b.u[0] = VEC3(1,0,0);
	b.u[1] = VEC3(0,1,0);
	b.u[2] = VEC3(0,0,1);

	// szukaj kolizji
	for(vector<Unit*>::iterator it = ctx.units->begin(), end = ctx.units->end(); it != end; ++it)
	{
		if(*it == &_unit || !(*it)->IsAlive() || distance((*it)->pos, _unit.pos) > 5.f || IsFriend(_unit, **it))
			continue;

		BOX box2;
		(*it)->GetBox(box2);
		b.c = box2.Midpoint();
		b.e = box2.Size()/2;

		if(OOBToOOB(b, a))
		{
			_hitpoint = a.c;
			_hitted = *it;
			return true;
		}
	}

	return false;
}

void Game::UpdateParticles(LevelContext& ctx, float dt)
{
	// aktualizuj cz�steczki
	bool deletions = false;
	for(vector<ParticleEmitter*>::iterator it = ctx.pes->begin(), end = ctx.pes->end(); it != end; ++it)
	{
		ParticleEmitter& pe = **it;

		if(pe.emisions == 0 || (pe.life > 0 && (pe.life -= dt) <= 0.f))
			pe.destroy = true;

		if(pe.destroy && pe.alive == 0)
		{
			deletions = true;
			if(pe.manual_delete == 0)
				delete *it;
			else
				pe.manual_delete = 2;
			*it = NULL;
			continue;
		}

		// aktualizuj cz�steczki
		for(vector<Particle>::iterator it2 = pe.particles.begin(), end2 = pe.particles.end(); it2 != end2; ++it2)
		{
			Particle& p = *it2;
			if(!p.exists)
				continue;

			if((p.life -= dt) <= 0.f)
			{
				p.exists = false;
				--pe.alive;
			}
			else
			{
				p.pos += p.speed * dt;
				p.speed.y -= p.gravity * dt;
			}
		}

		// emisja
		if(!pe.destroy && (pe.emisions == -1 || pe.emisions > 0) && ((pe.time += dt) >= pe.emision_interval))
		{
			if(pe.emisions > 0)
				--pe.emisions;
			pe.time -= pe.emision_interval;

			int ile = min(random(pe.spawn_min, pe.spawn_max), pe.max_particles-pe.alive);
			vector<Particle>::iterator it2 = pe.particles.begin();

			for(int i=0; i<ile; ++i)
			{
				while(it2->exists)
					++it2;

				Particle& p = *it2;
				p.exists = true;
				p.gravity = 9.81f;
				p.life = pe.particle_life;
				p.pos = pe.pos + random(pe.pos_min, pe.pos_max);
				p.speed = random(pe.speed_min, pe.speed_max);
			}

			pe.alive += ile;
		}
	}

	if(deletions)
	{
		ctx.pes->erase(std::remove_if(ctx.pes->begin(), ctx.pes->end(), is_null<ParticleEmitter*>), ctx.pes->end());
	}

	// aktualizuj cz�steczki szlakowe
	deletions = false;

	for(vector<TrailParticleEmitter*>::iterator it = ctx.tpes->begin(), end = ctx.tpes->end(); it != end; ++it)
	{
		if((*it)->Update(dt, NULL, NULL))
		{
			delete *it;
			*it = NULL;
			deletions = true;
		}
	}

	if(deletions)
	{
		ctx.tpes->erase(std::remove_if(ctx.tpes->begin(), ctx.tpes->end(), is_null<TrailParticleEmitter*>), ctx.tpes->end());
	}
}

int ObliczModyfikator(int typ, int flagi)
{
	// -2 bez zmian
	// -1 podatno��
	// 0 normalny
	// 1 odporno��

	int mod = -2;

	if(IS_SET(typ, DMG_SLASH))
	{
		if(IS_SET(flagi, F_CIETE25))
			mod = 1;
		else if(IS_SET(flagi, F_CIETE_MINUS25))
			mod = -1;
		else
			mod = 0;
	}

	if(IS_SET(typ, DMG_PIERCE))
	{
		if(IS_SET(flagi, F_KLUTE25))
		{
			if(mod == -2)
				mod = 1;
		}
		else if(IS_SET(flagi, F_KLUTE_MINUS25))
			mod = -1;
		else if(mod != -1)
			mod = 0;
	}
	
	if(IS_SET(typ, DMG_BLUNT))
	{
		if(IS_SET(flagi, F_OBUCHOWE25))
		{
			if(mod == -2)
				mod = 1;
		}
		else if(IS_SET(flagi, F_OBUCHOWE_MINUS25))
			mod = -1;
		else if(mod != -1)
			mod = 0;
	}

	assert(mod != -2);
	return mod;
}

Game::ATTACK_RESULT Game::DoAttack(LevelContext& ctx, Unit& unit)
{
	VEC3 hitpoint;
	Unit* hitted;

	if(!CheckForHit(ctx, unit, hitted, hitpoint))
		return ATTACK_NOT_HIT;

	return DoGenericAttack(ctx, unit, *hitted, hitpoint, unit.CalculateAttack()*unit.attack_power, unit.GetDmgType(), Skill::ONE_HANDED_WEAPON);
}

void Game::GiveDmg(LevelContext& ctx, Unit* _giver, float _dmg, Unit& _taker, const VEC3* _hitpoint, int dmg_flags, bool trained_armor)
{
	if(!IS_SET(dmg_flags, DMG_NO_BLOOD))
	{
		// krew
		ParticleEmitter* pe = new ParticleEmitter;
		pe->tex = tKrew[_taker.data->blood];
		pe->emision_interval = 0.01f;
		pe->life = 5.f;
		pe->particle_life = 0.5f;
		pe->emisions = 1;
		pe->spawn_min = 10;
		pe->spawn_max = 15;
		pe->max_particles = 15;
		if(_hitpoint)
			pe->pos = *_hitpoint;
		else
		{
			pe->pos = _taker.pos;
			pe->pos.y += _taker.GetUnitHeight() * 2.f/3;
		}
		pe->speed_min = VEC3(-1,0,-1);
		pe->speed_max = VEC3(1,1,1);
		pe->pos_min = VEC3(-0.1f,-0.1f,-0.1f);
		pe->pos_max = VEC3(0.1f,0.1f,0.1f);
		pe->size = 0.3f;
		pe->op_size = POP_LINEAR_SHRINK;
		pe->alpha = 0.9f;
		pe->op_alpha = POP_LINEAR_SHRINK;
		pe->mode = 0;
		pe->Init();
		ctx.pes->push_back(pe);

		if(IsOnline())
		{
			NetChange& c = Add1(net_changes);
			c.type = NetChange::SPAWN_BLOOD;
			c.id = _taker.data->blood;
			c.pos = pe->pos;
		}
	}

	if(IS_SET(dmg_flags, DMG_MAGICAL))
		_dmg *= _taker.CalculateMagicResistance();

	if(_giver && _giver->IsPlayer())
	{
		_giver->player->dmg_done += (int)_dmg;
		if(IsOnline())
			_giver->player->stat_flags |= STAT_DMG_DONE;
	}
	if(_taker.IsPlayer())
	{
		_taker.player->dmg_taken += (int)_dmg;
		if(IsOnline())
			_taker.player->stat_flags |= STAT_DMG_TAKEN;
	}

	if((_taker.hp -= _dmg) <= 0.f && !_taker.IsImmortal())
	{
		if(_giver && _giver->IsPlayer())
			AttackReaction(_taker, *_giver);
		UnitDie(_taker, &ctx, _giver);
	}
	else
	{
		// d�wi�k
		if(_taker.hurt_timer <= 0.f && _taker.data->sounds->sound[SOUND_PAIN])
		{
			if(sound_volume)
				PlayAttachedSound(_taker, _taker.data->sounds->sound[SOUND_PAIN], 2.f, 15.f);
			_taker.hurt_timer = random(1.f, 1.5f);
			if(IS_SET(dmg_flags, DMG_NO_BLOOD))
				_taker.hurt_timer += 1.f;
			if(IsOnline())
			{
				NetChange& c2 = Add1(net_changes);
				c2.type = NetChange::HURT_SOUND;
				c2.unit = &_taker;
			}
		}

		// szkol gracza w kondycji
		if(_taker.IsPlayer())
		{
			if(!trained_armor)
				_taker.player->Train2(Train_Hurt, float(_dmg)/_taker.hpmax, 4.f);

			// obw�dka b�lu
			_taker.player->last_dmg += float(_dmg);
		}

		// wkurw na atakujacego
		if(_giver && _giver->IsPlayer())
			AttackReaction(_taker, *_giver);

		// nie�miertelno��
		if(_taker.hp <= 0.f)
			_taker.hp = 1.f;

		if(IsOnline())
		{
			NetChange& c2 = Add1(net_changes);
			c2.type = NetChange::UPDATE_HP;
			c2.unit = &_taker;
		}
	}
}

//=================================================================================================
// Aktualizuj jednostki na danym poziomie (pomieszczeniu)
//=================================================================================================
void Game::UpdateUnits(LevelContext& ctx, float dt)
{
	PROFILER_BLOCK("UpdateUnits");
	for(vector<Unit*>::iterator it = ctx.units->begin(), end = ctx.units->end(); it != end; ++it)
	{
		Unit& u = **it;
		//assert(u.rot >= 0.f && u.rot < PI*2);

// 		u.block_power += dt/10;
// 		if(u.block_power > 1.f)
// 			u.block_power = 1.f;

		// licznik okrzyku od ostatniego trafienia
		u.hurt_timer -= dt;
		u.last_bash -= dt;

		// aktualizuj efekty i machanie ustami
		if(u.IsAlive())
		{
			if(IsLocal())
				u.UpdateEffects(dt);
			if(u.IsStanding() && u.talking)
			{
				u.talk_timer += dt;
				u.ani->need_update = true;
			}
		}

		// zmie� podstawow� animacj�
		if(u.animacja != u.animacja2)
		{
			u.changed = true;
			switch(u.animacja)
			{
			case ANI_IDZIE:
				u.ani->Play(NAMES::ani_move, PLAY_PRIO1|PLAY_RESTORE, 0);
				if(!IsClient2())
					u.ani->groups[0].speed = u.GetWalkSpeed() / u.data->walk_speed;
				break;
			case ANI_IDZIE_TYL:
				u.ani->Play(NAMES::ani_move, PLAY_BACK|PLAY_PRIO1|PLAY_RESTORE, 0);
				if(!IsClient2())
					u.ani->groups[0].speed = u.GetWalkSpeed() / u.data->walk_speed;
				break;
			case ANI_BIEGNIE:
				u.ani->Play(NAMES::ani_run, PLAY_PRIO1|PLAY_RESTORE, 0);
				if(!IsClient2())
					u.ani->groups[0].speed = u.GetRunSpeed() / u.data->run_speed;
				break;
			case ANI_W_LEWO:
				u.ani->Play(NAMES::ani_left, PLAY_PRIO1|PLAY_RESTORE, 0);
				if(!IsClient2())
					u.ani->groups[0].speed = u.GetRotationSpeed() / u.data->rot_speed;
				break;
			case ANI_W_PRAWO:
				u.ani->Play(NAMES::ani_right, PLAY_PRIO1|PLAY_RESTORE, 0);
				if(!IsClient2())
					u.ani->groups[0].speed = u.GetRotationSpeed() / u.data->rot_speed;
				break;
			case ANI_STOI:
				u.ani->Play(NAMES::ani_stand, PLAY_PRIO1, 0);
				break;
			case ANI_BOJOWA:
				u.ani->Play(NAMES::ani_battle, PLAY_PRIO1, 0);
				break;
			case ANI_BOJOWA_LUK:
				u.ani->Play(NAMES::ani_battle_bow, PLAY_PRIO1, 0);
				break;
			case ANI_UMIERA:
				u.ani->Play(NAMES::ani_die, PLAY_STOP_AT_END|PLAY_ONCE|PLAY_PRIO3, 0);
				break;
			case ANI_ODTWORZ:
				break;
			case ANI_IDLE:
				break;
			case ANI_KLEKA:
				u.ani->Play("kleka", PLAY_STOP_AT_END|PLAY_ONCE|PLAY_PRIO3, 0);
				break;
			default:
				assert(0);
				break;
			}
			u.animacja2 = u.animacja;
		}

		// koniec animacji idle
		if(u.animacja == ANI_IDLE && u.ani->frame_end_info)
		{
			u.ani->Play(NAMES::ani_stand, PLAY_PRIO1, 0);
			u.animacja = ANI_STOI;
		}

		// aktualizuj animacj�
		u.ani->Update(dt);

		// zmie� stan z umiera na umar� i stw�rz krew (chyba �e tylko upad�)
		if(!u.IsStanding())
		{
			if(u.ani->frame_end_info)
			{
				if(u.live_state == Unit::DYING)
				{
					u.live_state = Unit::DEAD;
					CreateBlood(ctx, u);
				}
				else if(u.live_state == Unit::FALLING)
					u.live_state = Unit::FALL;
				u.ani->frame_end_info = false;
			}
			if(u.action != A_POSITION)
				continue;
		}

		// aktualizuj akcj�
		switch(u.action)
		{
		case A_NONE:
			break;
		case A_TAKE_WEAPON:
			if(u.stan_broni == BRON_WYJMUJE)
			{
				if(u.etap_animacji == 0 && (u.ani->GetProgress2() >= u.data->frames->t[F_TAKE_WEAPON] || u.ani->frame_end_info2))
					u.etap_animacji = 1;
				if(u.ani->frame_end_info2)
				{
					u.stan_broni = BRON_WYJETA;
					if(u.useable)
					{
						u.action = A_ANIMATION2;
						u.etap_animacji = 1;
					}
					else
						u.action = A_NONE;
					u.ani->Deactivate(1);
					u.ani->frame_end_info2 = false;
				}
			}
			else
			{
				// chowanie broni
				if(u.etap_animacji == 0 && (u.ani->GetProgress2() <= u.data->frames->t[F_TAKE_WEAPON] || u.ani->frame_end_info2))
					u.etap_animacji = 1;
				if(u.wyjeta != B_BRAK && (u.etap_animacji == 1 || u.ani->frame_end_info2))
				{
					u.ani->Play(u.GetTakeWeaponAnimation(u.wyjeta == B_JEDNORECZNA), PLAY_ONCE|PLAY_PRIO1, 1);
					u.stan_broni = BRON_WYJMUJE;
					u.chowana = B_BRAK;
					u.etap_animacji = 1;
					u.ani->frame_end_info2 = false;
					u.etap_animacji = 0;

					if(IsOnline())
					{
						NetChange& c = Add1(net_changes);
						c.unit = &u;
						c.id = 0;
						c.type = NetChange::TAKE_WEAPON;
					}
				}
				else if(u.ani->frame_end_info2)
				{
					u.stan_broni = BRON_SCHOWANA;
					u.chowana = B_BRAK;
					u.action = A_NONE;
					u.ani->Deactivate(1);
					u.ani->frame_end_info2 = false;

					if(&u == pc->unit)
					{
						switch(pc->po_akcja)
						{
						// zdejmowanie za�o�onego przedmiotu
						case PO_ZDEJMIJ:
							assert(Inventory::lock_id == LOCK_MY);
							Inventory::lock_id = LOCK_NO;
							if(Inventory::lock_index != LOCK_REMOVED)
								inventory->RemoveSlotItem(IIndexToSlot(Inventory::lock_index));
							break;
						// zak�adanie przedmiotu po zdj�ciu innego
						case PO_ZALOZ:
							assert(Inventory::lock_id == LOCK_MY);
							Inventory::lock_id = LOCK_NO;
							if(Inventory::lock_id != LOCK_REMOVED)
								inventory->EquipSlotItem(Inventory::lock_index);
							break;
						// wyrzucanie za�o�onego przedmiotu
						case PO_WYRZUC:
							assert(Inventory::lock_id == LOCK_MY);
							Inventory::lock_id = LOCK_NO;
							if(Inventory::lock_index != LOCK_REMOVED)
								inventory->DropSlotItem(IIndexToSlot(Inventory::lock_index));
							break;
						// wypijanie miksturki
						case PO_WYPIJ:
							assert(Inventory::lock_id == LOCK_MY);
							Inventory::lock_id = LOCK_NO;
							if(Inventory::lock_index != LOCK_REMOVED)
								inventory->ConsumeItem(Inventory::lock_index);
							break;
						// u�yj obiekt
						case PO_UZYJ:
							if(before_player == BP_USEABLE && before_player_ptr.useable == pc->po_akcja_useable)
								PlayerUseUseable(pc->po_akcja_useable, true);
							break;
						// sprzedawanie za�o�onego przedmiotu
						case PO_SPRZEDAJ:
							assert(Inventory::lock_id == LOCK_TRADE_MY);
							Inventory::lock_id = LOCK_NO;
							if(Inventory::lock_index != LOCK_REMOVED)
								inv_trade_mine->SellSlotItem(IIndexToSlot(Inventory::lock_index));
							break;
						// chowanie za�o�onego przedmiotu do kontenera
						case PO_SCHOWAJ:
							assert(Inventory::lock_id == LOCK_TRADE_MY);
							Inventory::lock_id = LOCK_NO;
							if(Inventory::lock_index != LOCK_REMOVED)
								inv_trade_mine->PutSlotItem(IIndexToSlot(Inventory::lock_index));
							break;
						// daj przedmiot po schowaniu
						case PO_DAJ:
							assert(Inventory::lock_id == LOCK_TRADE_MY);
							Inventory::lock_id = LOCK_NO;
							if(Inventory::lock_index != LOCK_REMOVED)
								inv_trade_mine->GiveSlotItem(IIndexToSlot(Inventory::lock_index));
							break;
						}
						pc->po_akcja = PO_BRAK;
						assert(Inventory::lock_id == LOCK_NO);

						if(u.action == A_NONE && u.useable)
						{
							u.action = A_ANIMATION2;
							u.etap_animacji = 1;
						}
					}
					else if(IsLocal() && u.IsAI() && u.ai->potion != -1)
					{
						u.ConsumeItem(u.ai->potion);
						u.ai->potion = -1;
					}
				}
			}
			break;
		case A_SHOOT:
			if(u.etap_animacji == 0)
			{
				if(u.ani->GetProgress2() > 20.f/40)
					u.ani->groups[1].time = 20.f/40*u.ani->groups[1].anim->length;
			}
			else if(u.etap_animacji == 1)
			{
				if(IsLocal() && !u.trafil && u.ani->GetProgress2() > 20.f/40)
				{
					u.trafil = true;
					Bullet& b = Add1(ctx.bullets);
					b.level = u.GetLevel(Train_Shot);

					if(u.human_data)
						u.ani->SetupBones(&u.human_data->mat_scale[0]);
					else
						u.ani->SetupBones();
					
					Animesh::Point* point = u.ani->ani->GetPoint(NAMES::point_weapon);
					assert(point);

					D3DXMatrixTranslation(&m1, u.pos);
					D3DXMatrixRotationY(&m2, u.rot);
					D3DXMatrixMultiply(&m1, &m2, &m1);
					m2 = point->mat * u.ani->mat_bones[point->bone] * m1;

					VEC3 coord;
					D3DXVec3TransformCoord(&b.pos, &VEC3(0,0,0), &m2);

					b.attack = u.CalculateAttack(&u.GetBow());
					b.rot = VEC3(PI/2, u.rot+PI, 0);
					b.mesh = aArrow;
					b.speed = ARROW_SPEED;
					b.timer = ARROW_TIMER;
					b.owner = &u;
					b.remove = false;
					b.pe = NULL;
					b.spell = NULL;
					b.tex = NULL;
					b.poison_attack = 0.f;
					b.start_pos = b.pos;

					if(u.IsPlayer())
					{
						if(&u == pc->unit)
							b.yspeed = PlayerAngleY()*36;
						else
							b.yspeed = GetPlayerInfo(u.player->id).yspeed;
					}
					else
					{
						b.yspeed = u.ai->shoot_yspeed;
						if(u.ai->state == AIController::Idle && u.ai->idle_action == AIController::Idle_TrainBow)
							b.attack = -100.f;
					}

					// losowe odchylenie
					int sk = u.skill[(int)Skill::BOW];
					if(u.IsPlayer())
						sk += 10;
					if(sk < 50)
					{
						int szansa;
						float odchylenie_x, odchylenie_y;
						if(sk < 10)
						{
							szansa = 100;
							odchylenie_x = PI/16;
							odchylenie_y = 5.f;
						}
						else if(sk < 20)
						{
							szansa = 80;
							odchylenie_x = PI/20;
							odchylenie_y = 4.f;
						}
						else if(sk < 30)
						{
							szansa = 60;
							odchylenie_x = PI/26;
							odchylenie_y = 3.f;
						}
						else if(sk < 40)
						{
							szansa = 40;
							odchylenie_x = PI/34;
							odchylenie_y = 2.f;
						}
						else
						{
							szansa = 20;
							odchylenie_x = PI/48;
							odchylenie_y = 1.f;
						}

						if(rand2()%100 < szansa)
							b.rot.y += random_normalized(odchylenie_x);
						if(rand2()%100 < szansa)
							b.yspeed += random_normalized(odchylenie_y);
					}

					b.rot.y = clip(b.rot.y);

					TrailParticleEmitter* tpe = new TrailParticleEmitter;
					tpe->fade = 0.3f;
					tpe->color1 = VEC4(1,1,1,0.5f);
					tpe->color2 = VEC4(1,1,1,0);
					tpe->Init(50);
					ctx.tpes->push_back(tpe);
					b.trail = tpe;

					TrailParticleEmitter* tpe2 = new TrailParticleEmitter;
					tpe2->fade = 0.3f;
					tpe2->color1 = VEC4(1,1,1,0.5f);
					tpe2->color2 = VEC4(1,1,1,0);
					tpe2->Init(50);
					ctx.tpes->push_back(tpe2);
					b.trail2 = tpe2;

					if(sound_volume)
						PlaySound3d(sBow[rand2()%2], b.pos, 2.f, 8.f);

					if(IsOnline())
					{
						NetChange& c = Add1(net_changes);
						c.type = NetChange::SHOT_ARROW;
						c.unit = &u;
						c.pos = b.start_pos;
						c.f[0] = b.rot.y;
						c.f[1] = b.yspeed;
						c.f[2] = b.rot.x;
					}
				}
				if(u.ani->GetProgress2() > 20.f/40)
					u.etap_animacji = 2;
			}
			else if(u.ani->GetProgress2() > 35.f/40)
			{
				u.etap_animacji = 3;
				if(u.ani->frame_end_info2)
				{
koniec_strzelania:
					u.ani->Deactivate(1);
					u.ani->frame_end_info2 = false;
					u.action = A_NONE;
					assert(u.bow_instance);
					bow_instances.push_back(u.bow_instance);
					u.bow_instance = NULL;
					if(IsLocal() && u.IsAI())
					{
						float v = 1.f - float(u.skill[(int)Skill::ONE_HANDED_WEAPON]) / 100;
						u.ai->next_attack = random(v/2, v);
					}
					break;
				}
			}
			if(!u.ani)
			{
				// fix na skutek, nie na przyczyn� ;(
#ifdef IS_DEV
				WARN(Format("Unit %s dont have shooting animation, LS:%d A:%D ANI:%d PANI:%d ETA:%d.", u.GetName(), u.live_state, u.action, u.animacja, u.animacja2, u.etap_animacji));
				AddGameMsg("Unit don't have shooting animation!", 5.f);
#endif
				goto koniec_strzelania;
			}
			u.bow_instance->groups[0].time = min(u.ani->groups[1].time, u.bow_instance->groups[0].anim->length);
			u.bow_instance->need_update = true;
			break;
		case A_ATTACK:
			if(u.etap_animacji == 0)
			{
				float t = u.GetAttackFrame(0);
				if(u.ani->ani->head.n_groups == 1)
				{
					if(u.ani->GetProgress() > t)
					{
						if(IsLocal() && u.IsAI())
						{
							u.ani->groups[0].speed = 1.f + u.GetAttackSpeed();
							u.attack_power = 2.f;
							++u.etap_animacji;
							if(IsOnline())
							{
								NetChange& c = Add1(net_changes);
								c.type = NetChange::ATTACK;
								c.unit = &u;
								c.id = AID_Attack;
								c.f[1] = u.ani->groups[1].speed;
							}
						}
						else
							u.ani->groups[0].time = t*u.ani->groups[0].anim->length;
					}
				}
				else
				{
					if(u.ani->GetProgress2() > t)
					{
						if(IsLocal() && u.IsAI())
						{
							u.ani->groups[1].speed = 1.f + u.GetAttackSpeed();
							u.attack_power = 2.f;
							++u.etap_animacji;
							if(IsOnline())
							{
								NetChange& c = Add1(net_changes);
								c.type = NetChange::ATTACK;
								c.unit = &u;
								c.id = AID_Attack;
								c.f[1] = u.ani->groups[1].speed;
							}
						}
						else
							u.ani->groups[1].time = t*u.ani->groups[1].anim->length;
					}
				}
			}
			else
			{
				if(u.ani->ani->head.n_groups > 1)
				{
					if(u.etap_animacji == 1 && u.ani->GetProgress2() > u.GetAttackFrame(0))
					{
						if(IsLocal() && !u.trafil && u.ani->GetProgress2() >= u.GetAttackFrame(1))
						{
							ATTACK_RESULT result = DoAttack(ctx, u);
							if(result != ATTACK_NOT_HIT)
							{
								/*if(result == ATTACK_PARRIED)
								{
									u.ani->frame_end_info2 = false;
									u.ani->Play(rand2()%2 == 0 ? "atak1_p" : "atak2_p", PLAY_PRIO1|PLAY_ONCE, 1);
									u.action = A_PAIN;
									if(IsLocal() && u.IsAI())
									{
										float v = 1.f - float(u.skill[S_WEAPON])/100;
										u.ai->next_attack = 1.f+random(v/2, v);
									}
								}
								else*/
								u.trafil = true;
							}
						}
						if(u.ani->GetProgress2() >= u.GetAttackFrame(2) || u.ani->frame_end_info2)
						{
							// koniec mo�liwego ataku
							u.etap_animacji = 2;
							u.ani->groups[1].speed = 1.f;
							u.atak_w_biegu = false;
						}
					}
					if(u.etap_animacji == 2 && u.ani->frame_end_info2)
					{
						u.atak_w_biegu = false;
						u.ani->Deactivate(1);
						u.ani->frame_end_info2 = false;
						u.action = A_NONE;
						if(IsLocal() && u.IsAI())
						{
							float v = 1.f - float(u.skill[(int)Skill::ONE_HANDED_WEAPON]) / 100;
							u.ai->next_attack = random(v/2, v);
						}
					}
				}
				else
				{
					if(u.etap_animacji == 1 && u.ani->GetProgress() > u.GetAttackFrame(0))
					{
						if(IsLocal() && !u.trafil && u.ani->GetProgress() >= u.GetAttackFrame(1))
						{
							ATTACK_RESULT result = DoAttack(ctx, u);
							if(result != ATTACK_NOT_HIT)
							{
								/*u.ani->Deactivate(0);
								u.atak_w_biegu = false;
								u.action = A_NONE;
								if(IsLocal() && u.IsAI())
								{
									float v = 1.f - float(u.skill[S_WEAPON])/100;
									u.ai->next_attack = random(v/2, v);
								}*/
								u.trafil = true;
							}
						}
						if(u.ani->GetProgress() >= u.GetAttackFrame(2) || u.ani->frame_end_info)
						{
							// koniec mo�liwego ataku
							u.etap_animacji = 2;
							u.ani->groups[0].speed = 1.f;
							u.atak_w_biegu = false;
						}
					}
					if(u.etap_animacji == 2 && u.ani->frame_end_info)
					{
						u.atak_w_biegu = false;
						u.animacja = ANI_BOJOWA;
						u.animacja2 = ANI_STOI;
						u.action = A_NONE;
						if(IsLocal() && u.IsAI())
						{
							float v = 1.f - float(u.skill[(int)Skill::ONE_HANDED_WEAPON]) / 100;
							u.ai->next_attack = random(v/2, v);
						}
					}
				}
			}
			break;
		case A_BLOCK:
			break;
		case A_BASH:
			if(u.etap_animacji == 0)
			{
				if(u.ani->GetProgress2() >= u.data->frames->t[F_BASH])
					u.etap_animacji = 1;
			}
			if(IsLocal() && u.etap_animacji == 1 && !u.trafil)
			{
				if(DoShieldSmash(ctx, u))
					u.trafil = true;
			}
			if(u.ani->frame_end_info2)
			{
				u.action = A_NONE;
				u.ani->frame_end_info2 = false;
				u.ani->Deactivate(1);
			}
			break;
		/*case A_PAROWANIE:
			if(u.ani->frame_end_info2)
			{
				u.action = A_NONE;
				u.ani->frame_end_info2 = false;
				u.ani->Deactivate(1);
			}
			break;*/
		case A_DRINK:
			{
				float p = u.ani->GetProgress2();
				if(p >= 28.f/52.f && u.etap_animacji == 0)
				{
					if(sound_volume)
						PlayUnitSound(u, sGulp);
					u.etap_animacji = 1;
					if(IsLocal())
						u.ApplyConsumeableEffect(u.used_item->ToConsumeable());
				}
				if(p >= 49.f/52.f && u.etap_animacji == 1)
				{
					u.etap_animacji = 2;
					u.used_item = NULL;
				}
				if(u.ani->frame_end_info2)
				{
					if(u.useable)
					{
						u.etap_animacji = 1;
						u.action = A_ANIMATION2;
					}
					else
						u.action = A_NONE;
					u.ani->frame_end_info2 = false;
					u.ani->Deactivate(1);
				}
			}
			break;
		case A_EAT:
			{
				float p = u.ani->GetProgress2();
				if(p >= 32.f/70 && u.etap_animacji == 0)
				{
					u.etap_animacji = 1;
					if(sound_volume)
						PlayUnitSound(u, sEat);
				}
				if(p >= 48.f/70 && u.etap_animacji == 1)
				{
					u.etap_animacji = 2;
					if(IsLocal())
						u.ApplyConsumeableEffect(u.used_item->ToConsumeable());
				}
				if(p >= 60.f/70 && u.etap_animacji == 2)
				{
					u.etap_animacji = 3;
					u.used_item = NULL;
				}
				if(u.ani->frame_end_info2)
				{
					if(u.useable)
					{
						u.etap_animacji = 1;
						u.action = A_ANIMATION2;
					}
					else
						u.action = A_NONE;
					u.ani->frame_end_info2 = false;
					u.ani->Deactivate(1);
				}
			}
			break;
		case A_PAIN:
			if(u.ani->ani->head.n_groups == 2)
			{
				if(u.ani->frame_end_info2)
				{
					u.action = A_NONE;
					u.ani->frame_end_info2 = false;
					u.ani->Deactivate(1);
				}
			}
			else if(u.ani->frame_end_info)
			{
				u.action = A_NONE;
				u.animacja = ANI_BOJOWA;
				u.ani->frame_end_info = false;
			}
			break;
		case A_CAST:
			if(u.ani->ani->head.n_groups == 2)
			{
				if(IsLocal() && u.etap_animacji == 0 && u.ani->GetProgress2() >= u.data->frames->t[F_CAST])
				{
					u.etap_animacji = 1;
					CastSpell(ctx, u);
				}
				if(u.ani->frame_end_info2)
				{
					u.action = A_NONE;
					u.ani->frame_end_info2 = false;
					u.ani->Deactivate(1);
				}
			}
			else
			{
				if(IsLocal() && u.etap_animacji == 0 && u.ani->GetProgress() >= u.data->frames->t[F_CAST])
				{
					u.etap_animacji = 1;
					CastSpell(ctx, u);
				}
				if(u.ani->frame_end_info)
				{
					u.action = A_NONE;
					u.animacja = ANI_BOJOWA;
					u.ani->frame_end_info = false;
				}
			}
			break;
		case A_ANIMATION:
			if(u.ani->frame_end_info)
			{
				u.action = A_NONE;
				u.animacja = ANI_STOI;
				u.animacja2 = (ANIMACJA)-1;
			}
			break;
		case A_ANIMATION2:
			{
				bool allow_move = true;
				if(IsOnline())
				{
					if(IsServer())
					{
						if(u.IsPlayer() && &u != pc->unit)
							allow_move = false;
					}
					else
					{
						if(!u.IsPlayer() || &u != pc->unit)
							allow_move = false;
					}
				}
				if(u.etap_animacji == 3)
				{
					u.timer += dt;
					if(allow_move && u.timer >= 0.5f)
					{
						u.visual_pos = u.pos = u.target_pos;
						u.useable->user = NULL;
						if(IsOnline() && IsServer())
						{
							NetChange& c = Add1(net_changes);
							c.type = NetChange::USE_USEABLE;
							c.unit = &u;
							c.id = u.useable->netid;
							c.ile = 0;
						}
						u.useable = NULL;
						u.action = A_NONE;
						break;
					}

					if(allow_move)
					{
						// przesu� posta�
						u.visual_pos = u.pos = lerp(u.target_pos2, u.target_pos, u.timer*2);

						// obr�t
						float target_rot = lookat_angle(u.target_pos, u.useable->pos);
						float dif = angle_dif(u.rot, target_rot);
						if(not_zero(dif))
						{
							const float rot_speed = u.GetRotationSpeed() * 2 * dt;
							const float arc = shortestArc(u.rot, target_rot);

							if(dif <= rot_speed)
								u.rot = target_rot;
							else
								u.rot = clip(u.rot + sign(arc) * rot_speed);
						}
					}
				}
				else
				{
					BaseUsable& bu = g_base_usables[u.useable->type];

					if(u.etap_animacji > 0)
					{
						// odtwarzanie d�wi�ku
						if(bu.sound)
						{
							if(u.ani->GetProgress() >= bu.sound_timer)
							{
								if(u.etap_animacji == 1)
								{
									u.etap_animacji = 2;
									if(sound_volume)
										PlaySound3d(bu.sound, u.GetCenter(), 2.f, 5.f);
									if(IsOnline() && IsServer())
									{
										NetChange& c = Add1(net_changes);
										c.type = NetChange::USEABLE_SOUND;
										c.unit = &u;
									}
								}
							}
							else if(u.etap_animacji == 2)
								u.etap_animacji = 1;
						}
					}
					else if(IsLocal() || &u == pc->unit)
					{
						// ustal docelowy obr�t postaci
						float target_rot;
						if(bu.limit_rot == 0)
							target_rot = u.rot;
						else if(bu.limit_rot == 1)
						{
							float rot1 = clip(u.use_rot + PI/2),
								dif1 = angle_dif(rot1, u.useable->rot),
								rot2 = clip(u.useable->rot+PI),
								dif2 = angle_dif(rot1, rot2);

							if(dif1 < dif2)
								target_rot = u.useable->rot;
							else
								target_rot = rot2;
						}
						else if(bu.limit_rot == 2)
							target_rot = u.useable->rot;
						else if(bu.limit_rot == 3)
						{
							float rot1 = clip(u.use_rot + PI),
								dif1 = angle_dif(rot1, u.useable->rot),
								rot2 = clip(u.useable->rot+PI),
								dif2 = angle_dif(rot1, rot2);

							if(dif1 < dif2)
								target_rot = u.useable->rot;
							else
								target_rot = rot2;
						}
						else
							target_rot = u.useable->rot+PI;
						target_rot = clip(target_rot);

						// obr�t w strone obiektu
						const float dif = angle_dif(u.rot, target_rot);
						const float rot_speed = u.GetRotationSpeed()*2;
						if(allow_move && not_zero(dif))
						{
							const float rot_speed_dt = rot_speed * dt;
							if(dif <= rot_speed_dt)
								u.rot = target_rot;
							else
							{
								const float arc = shortestArc(u.rot, target_rot);
								u.rot = clip(u.rot + sign(arc) * rot_speed_dt);
							}
						}

						// czy musi si� obraca� zanim zacznie si� przesuwa�?
						if(dif < rot_speed*0.5f)
						{
							u.timer += dt;
							if(u.timer >= 0.5f)
							{
								u.timer = 0.5f;
								++u.etap_animacji;
							}

							// przesu� posta� i fizyk�
							if(allow_move)
							{
								u.visual_pos = u.pos = lerp(u.target_pos, u.target_pos2, u.timer*2);
								global_col.clear();
								float my_radius = u.GetUnitRadius();
								bool ok = true;
								for(vector<Unit*>::iterator it2 = ctx.units->begin(), end2 = ctx.units->end(); it2 != end2; ++it2)
								{
									if(&u == *it2 || !(*it2)->IsStanding())
										continue;

									float radius = (*it2)->GetUnitRadius();
									if(distance((*it2)->pos.x, (*it2)->pos.z, u.pos.x, u.pos.z) <= radius+my_radius)
									{
										ok = false;
										break;
									}
								}
								if(ok)
									UpdateUnitPhysics(&u, u.pos);
							}
						}
					}
				}
			}
			break;
		case A_POSITION:
			u.timer += dt;
			if(u.etap_animacji == 1)
			{
				// obs�uga animacji cierpienia
				if(u.ani->ani->head.n_groups == 2)
				{
					if(u.ani->frame_end_info2 || u.timer >= 0.5f)
					{
						u.ani->frame_end_info2 = false;
						u.ani->Deactivate(1);
						u.etap_animacji = 2;
					}
				}
				else if(u.ani->frame_end_info || u.timer >= 0.5f)
				{
					u.animacja = ANI_BOJOWA;
					u.ani->frame_end_info = false;
					u.etap_animacji = 2;
				}
			}
			if(u.timer >= 0.5f)
			{
				u.visual_pos = u.pos = u.target_pos;
				u.useable->user = NULL;
				if(IsOnline() && IsServer())
				{
					NetChange& c = Add1(net_changes);
					c.type = NetChange::USE_USEABLE;
					c.unit = &u;
					c.id = u.useable->netid;
					c.ile = 0;
				}
				u.useable = NULL;
				u.action = A_NONE;
			}
			else
				u.visual_pos = u.pos = lerp(u.target_pos2, u.target_pos, u.timer*2);
			break;
		case A_PICKUP:
			if(u.ani->frame_end_info)
			{
				u.action = A_NONE;
				u.animacja = ANI_STOI;
				u.animacja2 = (ANIMACJA)-1;
			}
			break;
		default:
			assert(0);
			break;
		}
	}
}

// dzia�a tylko dla cz�onk�w dru�yny!
void Game::UpdateUnitInventory(Unit& u)
{
	bool changes = false;
	int index = 0;
	const Item* prev_slots[SLOT_MAX];
	for(int i=0; i<SLOT_MAX; ++i)
		prev_slots[i] = u.slots[i];

	for(vector<ItemSlot>::iterator it = u.items.begin(), end = u.items.end(); it != end; ++it, ++index)
	{
		if(!it->item || it->team_count != 0)
			continue;

		switch(it->item->type)
		{
		case IT_WEAPON:
			if(!u.HaveWeapon())
			{
				u.slots[SLOT_WEAPON] = it->item;
				it->item = NULL;
				changes = true;
			}
			else if(IS_SET(u.data->flagi, F_MAG))
			{
				if(IS_SET(it->item->flags, ITEM_MAGE))
				{
					if(IS_SET(u.GetWeapon().flags, ITEM_MAGE))
					{
						if(u.GetWeapon().value < it->item->value)
						{
							std::swap(u.slots[SLOT_WEAPON], it->item);
							changes = true;
						}
					}
					else
					{
						std::swap(u.slots[SLOT_WEAPON], it->item);
						changes = true;
					}
				}
				else
				{
					if(!IS_SET(u.GetWeapon().flags, ITEM_MAGE) && u.IsBetterWeapon(it->item->ToWeapon()))
					{
						std::swap(u.slots[SLOT_WEAPON], it->item);
						changes = true;
					}
				}
			}
			else if(u.IsBetterWeapon(it->item->ToWeapon()))
			{
				std::swap(u.slots[SLOT_WEAPON], it->item);
				changes = true;
			}
			break;
		case IT_BOW:
			if(!u.HaveBow())
			{
				u.slots[SLOT_BOW] = it->item;
				it->item = NULL;
				changes = true;
			}
			else if(u.GetBow().value < it->item->value)
			{
				std::swap(u.slots[SLOT_BOW], it->item);
				changes = true;
			}
			break;
		case IT_ARMOR:
			if(!u.HaveArmor())
			{
				u.slots[SLOT_ARMOR] = it->item;
				it->item = NULL;
				changes = true;
			}
			else if(IS_SET(u.data->flagi, F_MAG))
			{
				if(IS_SET(it->item->flags, ITEM_MAGE))
				{
					if(IS_SET(u.GetArmor().flags, ITEM_MAGE))
					{
						if(it->item->value > u.GetArmor().value)
						{
							std::swap(u.slots[SLOT_ARMOR], it->item);
							changes = true;
						}
					}
					else
					{
						std::swap(u.slots[SLOT_ARMOR], it->item);
						changes = true;
					}
				}
				else
				{
					if(!IS_SET(u.GetArmor().flags, ITEM_MAGE) && u.IsBetterArmor(it->item->ToArmor()))
					{
						std::swap(u.slots[SLOT_ARMOR], it->item);
						changes = true;
					}
				}
			}
			else if(u.IsBetterArmor(it->item->ToArmor()))
			{
				std::swap(u.slots[SLOT_ARMOR], it->item);
				changes = true;
			}
			break;
		case IT_SHIELD:
			if(!u.HaveShield())
			{
				u.slots[SLOT_SHIELD] = it->item;
				it->item = NULL;
				changes = true;
			}
			else if(u.GetShield().value < it->item->value)
			{
				std::swap(u.slots[SLOT_SHIELD], it->item);
				changes = true;
			}
			break;
		default:
			break;
		}
	}

	if(changes)
	{
		RemoveNullItems(u.items);
		SortItems(u.items);

		if(IsOnline() && players > 1)
		{
			for(int i=0; i<SLOT_MAX; ++i)
			{
				if(u.slots[i] != prev_slots[i])
				{
					NetChange& c = Add1(net_changes);
					c.unit = &u;
					c.type = NetChange::CHANGE_EQUIPMENT;
					c.id = i;
				}
			}
		}
	}
}

// sprawdza czy kto� z dru�yny jeszcze �yje
bool Game::IsAnyoneAlive() const
{
	for(vector<Unit*>::const_iterator it = team.begin(), end = team.end(); it != end; ++it)
	{
		if((*it)->IsAlive() || (*it)->in_arena != -1)
			return true;
	}
	return false;
}

bool Game::DoShieldSmash(LevelContext& ctx, Unit& _attacker)
{
	assert(_attacker.HaveShield());

	VEC3 hitpoint;
	Unit* hitted;

	if(!CheckForHit(ctx, _attacker, hitted, *_attacker.GetShield().ani->FindPoint("hit"), _attacker.ani->ani->GetPoint(NAMES::point_shield), hitpoint))
		return false;

	if(!IS_SET(hitted->data->flagi, F_NIE_CIERPI) && hitted->last_bash <= 0.f)
	{
		hitted->last_bash = 1.f + float(hitted->attrib[(int)Attribute::CON]) / 50.f;

		BreakAction(*hitted);

		if(hitted->action != A_POSITION)
			hitted->action = A_PAIN;
		else
			hitted->etap_animacji = 1;

		if(hitted->ani->ani->head.n_groups == 2)
		{
			hitted->ani->frame_end_info2 = false;
			hitted->ani->Play(NAMES::ani_hurt, PLAY_PRIO1|PLAY_ONCE, 1);
			hitted->ani->groups[1].speed = 1.f;
		}
		else
		{
			hitted->ani->frame_end_info = false;
			hitted->ani->Play(NAMES::ani_hurt, PLAY_PRIO3|PLAY_ONCE, 0);
			hitted->ani->groups[0].speed = 1.f;
			hitted->animacja = ANI_ODTWORZ;
		}

		if(IsOnline())
		{
			NetChange& c = Add1(net_changes);
			c.unit = hitted;
			c.type = NetChange::STUNNED;
		}

		/*static float t = 1.f;

		if(Key.Down('0'))
			t = 1.f;
		if(Key.Down('7'))
			t = 0.f;
		if(Key.Down('8'))
		{
			t -= 0.1f;
			if(t < 0.f)
				t = 0.f;
		}
		if(Key.Down('9'))
		{
			t += 0.1f;
			if(t > 1.f)
				t = 1.f;
		}

		Animesh::Animation* a = hitted->ani->ani->GetAnimation("trafiony2");
		assert(a);

		AnimeshInstance::Group& g0 = hitted->ani->groups[0];
		g0.anim = a;
		g0.blend_time = 0.f;
		g0.state = AnimeshInstance::FLAG_GROUP_ACTIVE;
		g0.time = t*a->length;
		g0.used_group = 0;
		g0.prio = 3;

		if(hitted->ani->ani->head.n_groups > 1)
		{
			for(int i=1; i<hitted->ani->ani->head.n_groups; ++i)
			{
				AnimeshInstance::Group& gi = hitted->ani->groups[i];
				gi.anim = NULL;
				gi.state = 0;
				gi.used_group = 0;
			}
		}

		hitted->ani->need_update = true;

		hitted->ani->SetupBones();

		hitted->ani->groups[0].state = 0;*/
	}

	DoGenericAttack(ctx, _attacker, *hitted, hitpoint, _attacker.CalculateShieldAttack(), DMG_BLUNT, Skill::SHIELD);

	return true;
}

VEC4 Game::GetFogColor()
{
	return fog_color;
}

VEC4 Game::GetFogParams()
{
	if(cl_fog)
		return fog_params;
	else
		return VEC4(draw_range, draw_range+1, 1, 0);
}

VEC4 Game::GetAmbientColor()
{
	if(!cl_lighting)
		return VEC4(1, 1, 1, 1);
	//return VEC4(0.1f,0.1f,0.1f,1);
	return ambient_color;
}

VEC4 Game::GetLightDir()
{
// 	VEC3 light_dir(1, 1, 1);
// 	D3DXVec3Normalize(&light_dir, &light_dir);
// 	return VEC4(light_dir, 1);

	VEC3 light_dir(sin(light_angle), 2.f, cos(light_angle));
	D3DXVec3Normalize(&light_dir, &light_dir);
	return VEC4(light_dir, 1);
}

VEC4 Game::GetLightColor()
{
	return VEC4(1,1,1,1);
	//return VEC4(0.5f,0.5f,0.5f,1);
}

inline bool CanRemoveBullet(const Bullet& b)
{
	return b.remove;
}

struct BulletCallback : public btCollisionWorld::ContactResultCallback
{
	BulletCallback(btCollisionObject* _bullet, btCollisionObject* _ignore) : target(NULL), hit(false), bullet(_bullet), ignore(_ignore), hit_unit(false)
	{
		assert(_bullet);
	}

	btScalar addSingleResult(btManifoldPoint& cp, const btCollisionObjectWrapper* colObj0Wrap, int partId0, int index0, const btCollisionObjectWrapper* colObj1Wrap, int partId1, int index1)
	{
		if(colObj0Wrap->getCollisionObject() == bullet)
		{
			if(colObj1Wrap->getCollisionObject() == ignore)
				return 1.f;
			hitpoint = ToVEC3(cp.getPositionWorldOnA());
			if(!target)
			{
				if(IS_SET(colObj1Wrap->getCollisionObject()->getCollisionFlags(), CG_UNIT))
					hit_unit = true;
				// to nie musi by� jednostka :/
				target = (Unit*)colObj1Wrap->getCollisionObject()->getUserPointer();
			}
		}
		else
		{
			if(colObj0Wrap->getCollisionObject() == ignore)
				return 1.f;
			hitpoint = ToVEC3(cp.getPositionWorldOnB());
			if(!target)
			{
				if(IS_SET(colObj0Wrap->getCollisionObject()->getCollisionFlags(), CG_UNIT))
					hit_unit = true;
				// to nie musi by� jednostka :/
				target = (Unit*)colObj0Wrap->getCollisionObject()->getUserPointer();
			}
		}

		hit = true;
		return 0.f;
	}

	btCollisionObject* bullet, *ignore;
	Unit* target;
	VEC3 hitpoint;
	bool hit, hit_unit;
};

void Game::UpdateBullets(LevelContext& ctx, float dt)
{
	bool deletions = false;

	for(vector<Bullet>::iterator it = ctx.bullets->begin(), end = ctx.bullets->end(); it != end; ++it)
	{
		it->pos += VEC3(sin(it->rot.y)*it->speed*dt, 0, cos(it->rot.y)*it->speed*dt);

		// pozycja y
		it->pos.y += it->yspeed * dt;
		if(it->spell && it->spell->type == Spell::Ball)
			it->yspeed -= 10.f*dt;

		// aktualizuj trail
		if(it->trail)
		{
			VEC3 pt1 = it->pos;
			pt1.y += 0.05f;
			VEC3 pt2 = it->pos;
			pt2.y -= 0.05f;
			it->trail->Update(0, &pt1, &pt2);

			pt1 = it->pos;
			pt1.x += sin(it->rot.y+PI/2)*0.05f;
			pt1.z += cos(it->rot.y+PI/2)*0.05f;
			pt2 = it->pos;
			pt2.x -= sin(it->rot.y+PI/2)*0.05f;
			pt2.z -= cos(it->rot.y+PI/2)*0.05f;
			it->trail2->Update(0, &pt1, &pt2);
		}

		// aktualizuj cz�steczki
		if(it->pe)
			it->pe->pos = it->pos;

		if((it->timer -= dt) <= 0.f)
		{
			it->remove = true;
			deletions = true;
			if(it->trail)
			{
				it->trail->destroy = true;
				it->trail2->destroy = true;
			}
			if(it->pe)
				it->pe->destroy = true;
		}
		else
		{
			btCollisionObject* cobj;

			if(!it->spell)
				cobj = obj_arrow;
			else
			{
				cobj = obj_spell;
				cobj->setCollisionShape(it->spell->shape);
			}
				
			btTransform& tr = cobj->getWorldTransform();
			tr.setOrigin(ToVector3(it->pos));
			tr.setRotation(btQuaternion(it->rot.y, it->rot.x, it->rot.z));

			BulletCallback callback(cobj, it->owner ? it->owner->cobj : NULL);
			phy_world->contactTest(cobj, callback);

			/*Unit* owner[2] = {it->owner, NULL};
			global_col3d.clear();
			GatherCollisionObjects3D(global_col3d, it->obj.pos, it->obj.mesh->head.radius, owner);

			Animesh::Point* hitbox = it->obj.mesh->FindPoint("hit");
			assert(hitbox && hitbox->IsBox());

			OOBBOX obox;
			obox.size = hitbox->size/2;
			obox.pos = it->obj.pos;
			D3DXMatrixRotation(&obox.rot, it->obj.rot);

			VEC3 hitpoint;
			int id = Collide3D(global_col3d, obox, hitpoint);*/

			if(callback.hit)
			{
				Unit* hitted = callback.target;

				it->remove = true;
				deletions = true;
				if(it->trail)
				{
					it->trail->destroy = true;
					it->trail2->destroy = true;
				}
				if(it->pe)
					it->pe->destroy = true;

				if(callback.hit_unit && hitted)
				{
					if(!it->spell)
					{
						if(!IsLocal())
							continue;

						if(it->owner && IsFriend(*it->owner, *hitted) || it->attack < -50.f)
						{
							// frendly fire
							if(hitted->action == A_BLOCK && angle_dif(clip(it->rot.y+PI), hitted->rot) < PI*2/5)
							{
								MATERIAL_TYPE mat = hitted->GetShield().material;
								if(sound_volume)
									PlaySound3d(GetMaterialSound(MAT_IRON, mat), callback.hitpoint, 2.f, 10.f);
								if(IsOnline())
								{
									NetChange& c = Add1(net_changes);
									c.type = NetChange::HIT_SOUND;
									c.id = MAT_IRON;
									c.ile = mat;
									c.pos = callback.hitpoint;
								}
							}
							else
								PlayHitSound(MAT_IRON, hitted->GetBodyMaterial(), callback.hitpoint, 2.f, false);
							continue;
						}

						if(it->owner && hitted->IsAI())
							AI_HitReaction(*hitted, it->start_pos);

						// trafienie w posta�
						float dmg = it->attack,
							  def = hitted->CalculateDefense();

						int mod = ObliczModyfikator(DMG_PIERCE, hitted->data->flagi);
						float m = 1.f;
						if(mod == -1)
							m += 0.25f;
						else if(mod == 1)
							m -= 0.25f;
						if(hitted->IsNotFighting())
							m += 0.1f; // 10% do dmg je�li ma schowan� bro�

						// premia za atak w plecy
						float kat = angle_dif(it->rot.y, hitted->rot);
						m += kat/PI*0.25f;

						// modyfikator obra�e�
						dmg *= m;
						float base_dmg = dmg;

						if(hitted->action == A_BLOCK && kat < PI*2/5)
						{
							float blocked = hitted->CalculateBlock(&hitted->GetShield()) * hitted->ani->groups[1].GetBlendT() * (1.f - kat/(PI*2/5));
							dmg -= blocked;

							MATERIAL_TYPE mat = hitted->GetShield().material;
							if(sound_volume)
								PlaySound3d(GetMaterialSound(MAT_IRON, mat), callback.hitpoint, 2.f, 10.f);
							if(IsOnline())
							{
								NetChange& c = Add1(net_changes);
								c.type = NetChange::HIT_SOUND;
								c.id = MAT_IRON;
								c.ile = mat;
								c.pos = callback.hitpoint;
							}

							if(hitted->IsPlayer())
							{
								// gracz zablokowa� pocisk, szkol go
								hitted->player->Train2(Train_Block, C_TRAIN_BLOCK, 0.f, it->level);
							}

							if(dmg < 0)
							{
								// dzi�ki tarczy zablokowano
								if(it->owner && it->owner->IsPlayer())
								{
									// wr�g zablokowa� strza� z �uku, szkol si�
									it->owner->player->Train2(Train_Shot, C_TRAIN_SHOT_BLOCK, hitted->GetLevel(Train_Block), it->level);
									// wkurw na atakuj�cego
									AttackReaction(*hitted, *it->owner);
								}
								continue;
							}
						}

						// odporno��/pancerz
						dmg -= def;

						// szkol gracza w pancerzu/hp
						if(hitted->IsPlayer())
							hitted->player->Train2(Train_Hurt, C_TRAIN_ARMOR_HURT * base_dmg / hitted->hpmax, it->level);
						// d�wi�k trafienia
						PlayHitSound(MAT_IRON, hitted->GetBodyMaterial(), callback.hitpoint, 2.f, dmg>0.f);

						if(dmg < 0)
						{
							if(it->owner && it->owner->IsPlayer())
							{
								// cios zaabsorbowany, szkol
								it->owner->player->Train2(Train_Shot, C_TRAIN_SHOT_BLOCK, hitted->GetLevel(Train_Hurt), it->level);
								// wkurw na atakuj�cego
								AttackReaction(*hitted, *it->owner);
							}
							continue;
						}

						// szkol gracza w �uku
						if(it->owner && it->owner->IsPlayer())
						{
							float v = dmg/hitted->hpmax;
							if(hitted->hp - dmg < 0.f && !hitted->IsImmortal())
								v = max(C_TRAIN_KILL_RATIO, v);
							if(v > 1.f)
								v = 1.f;
							it->owner->player->Train2(Train_Shot, v*C_TRAIN_SHOT, hitted->GetLevel(Train_Hurt), it->level);
						}

						GiveDmg(ctx, it->owner, dmg, *hitted, &callback.hitpoint, 0, true);

						// obra�enia od trucizny
						if(it->poison_attack > 0.f && !IS_SET(hitted->data->flagi, F_ODPORNOSC_NA_TRUCIZNY))
						{
							Effect& e = Add1(hitted->effects);
							e.power = it->poison_attack/5;
							e.time = 5.f;
							e.effect = E_POISON;
						}
					}
					else
					{
						if(!IsLocal())
							continue;

						// trafienie w posta� z czara
						if(it->owner && IsFriend(*it->owner, *hitted))
						{
							// frendly fire
							SpellHitEffect(ctx, *it, callback.hitpoint, hitted);

							// d�wi�k trafienia w posta�
							if(hitted->action == A_BLOCK && angle_dif(clip(it->rot.y+PI), hitted->rot) < PI*2/5)
							{
								MATERIAL_TYPE mat = hitted->GetShield().material;
								if(sound_volume)
									PlaySound3d(GetMaterialSound(MAT_IRON, mat), callback.hitpoint, 2.f, 10.f);
								if(IsOnline())
								{
									NetChange& c = Add1(net_changes);
									c.type = NetChange::HIT_SOUND;
									c.id = MAT_IRON;
									c.ile = mat;
									c.pos = callback.hitpoint;
								}
							}
							else
								PlayHitSound(MAT_IRON, hitted->GetBodyMaterial(), callback.hitpoint, 2.f, false);
							continue;
						}

						if(hitted->IsAI())
							AI_HitReaction(*hitted, it->start_pos);

						float dmg = it->attack;
						if(it->owner)
							dmg += it->owner->level * it->spell->dmg_premia;
						float kat = angle_dif(clip(it->rot.y+PI), hitted->rot);

						if(hitted->action == A_BLOCK && kat < PI*2/5)
						{
							float blocked = hitted->CalculateBlock(&hitted->GetShield()) * hitted->ani->groups[1].GetBlendT() * (1.f - kat/(PI*2/5));
							dmg -= blocked/2;

							if(hitted->IsPlayer())
							{
								// gracz zablokowa� pocisk, szkol go
								hitted->player->Train2(Train_Block, C_TRAIN_BLOCK, 0.f, it->level);
							}

							if(dmg < 0)
							{
								// dzi�ki tarczy zablokowano
								SpellHitEffect(ctx, *it, callback.hitpoint, hitted);
								continue;
							}
						}

						GiveDmg(ctx, it->owner, dmg, *hitted, &callback.hitpoint, !IS_SET(it->spell->flags, Spell::Poison) ? DMG_MAGICAL : 0);

						// obra�enia od trucizny
						if(IS_SET(it->spell->flags, Spell::Poison) && !IS_SET(hitted->data->flagi, F_ODPORNOSC_NA_TRUCIZNY))
						{
							Effect& e = Add1(hitted->effects);
							e.power = dmg/5;
							e.time = 5.f;
							e.effect = E_POISON;
						}

						SpellHitEffect(ctx, *it, callback.hitpoint, hitted);
					}
				}
				else
				{
					// trafiono w obiekt
					if(!it->spell)
					{
						if(sound_volume)
							PlaySound3d(GetMaterialSound(MAT_IRON, MAT_ROCK), callback.hitpoint, 2.f, 10.f);

						ParticleEmitter* pe = new ParticleEmitter;
						pe->tex = tIskra;
						pe->emision_interval = 0.01f;
						pe->life = 5.f;
						pe->particle_life = 0.5f;
						pe->emisions = 1;
						pe->spawn_min = 10;
						pe->spawn_max = 15;
						pe->max_particles = 15;
						pe->pos = callback.hitpoint;
						pe->speed_min = VEC3(-1,0,-1);
						pe->speed_max = VEC3(1,1,1);
						pe->pos_min = VEC3(-0.1f,-0.1f,-0.1f);
						pe->pos_max = VEC3(0.1f,0.1f,0.1f);
						pe->size = 0.3f;
						pe->op_size = POP_LINEAR_SHRINK;
						pe->alpha = 0.9f;
						pe->op_alpha = POP_LINEAR_SHRINK;
						pe->mode = 0;
						pe->Init();
						ctx.pes->push_back(pe);

						if(IsLocal())
						{
							if(it->owner && it->owner->IsPlayer())
								it->owner->player->Train2(Train_Shot, C_TRAIN_SHOT_MISSED, 1.f, it->level);

							if(in_tutorial && callback.target)
							{
								void* ptr = (void*)callback.target;
								if((ptr == tut_tarcza || ptr == tut_tarcza2) && tut_state == 12)
								{
									Train(*pc->unit, true, (int)Skill::BOW, true);
									tut_state = 13;
									int unlock = 6;
									int activate = 8;
									for(vector<TutorialText>::iterator it = ttexts.begin(), end = ttexts.end(); it != end; ++it)
									{
										if(it->id == activate)
										{
											it->state = 1;
											break;
										}
									}
									for(vector<Door*>::iterator it = local_ctx.doors->begin(), end = local_ctx.doors->end(); it != end; ++it)
									{
										if((*it)->locked == LOCK_SAMOUCZEK+unlock)
										{
											(*it)->locked = LOCK_NONE;
											break;
										}
									}
								}
							}
						}
					}
					else
					{
						// trafienie czarem w obiekt
						SpellHitEffect(ctx, *it, callback.hitpoint, NULL);
					}
				}
			}
		}
	}

	if(deletions)
	{
		ctx.bullets->erase(std::remove_if(ctx.bullets->begin(), ctx.bullets->end(), CanRemoveBullet), ctx.bullets->end());
	}
}

void Game::SpawnDungeonColliders()
{
	assert(!location->outside);

	InsideLocation* inside = (InsideLocation*)location;
	InsideLocationLevel& lvl = inside->GetLevelData();
	Pole* m = lvl.mapa;
	int w = lvl.w,
		h = lvl.h;

	btCollisionObject* cobj;

	for(int y=1; y<h-1; ++y)
	{
		for(int x=1; x<w-1; ++x)
		{
			if(czy_blokuje2(m[x+y*w]) && (!czy_blokuje2(m[x-1+(y-1)*w]) || !czy_blokuje2(m[x+(y-1)*w]) || !czy_blokuje2(m[x+1+(y-1)*w]) ||
				!czy_blokuje2(m[x-1+y*w]) || !czy_blokuje2(m[x+1+y*w]) ||
				!czy_blokuje2(m[x-1+(y+1)*w]) || !czy_blokuje2(m[x+(y+1)*w]) || !czy_blokuje2(m[x+1+(y+1)*w])))
			{
				cobj = new btCollisionObject;
				cobj->setCollisionShape(shape_wall);
				cobj->getWorldTransform().getOrigin().setValue(2.f*x+1.f, 2.f, 2.f*y+1.f);
				cobj->setCollisionFlags(CG_WALL);
				phy_world->addCollisionObject(cobj);
			}
		}
	}

	// lewa/prawa �ciana
	for(int i=1; i<h-1; ++i)
	{
		// lewa
		if(czy_blokuje2(m[i*w]) && !czy_blokuje2(m[1+i*w]))
		{
			cobj = new btCollisionObject;
			cobj->setCollisionShape(shape_wall);
			cobj->getWorldTransform().getOrigin().setValue(1.f, 2.f, 2.f*i+1.f);
			cobj->setCollisionFlags(CG_WALL);
			phy_world->addCollisionObject(cobj);
		}

		// prawa
		if(czy_blokuje2(m[i*w+w-1]) && !czy_blokuje2(m[w-2+i*w]))
		{
			cobj = new btCollisionObject;
			cobj->setCollisionShape(shape_wall);
			cobj->getWorldTransform().getOrigin().setValue(2.f*(w-1)+1.f, 2.f, 2.f*i+1.f);
			cobj->setCollisionFlags(CG_WALL);
			phy_world->addCollisionObject(cobj);
		}
	}

	// przednia/tylna �ciana
	for(int i=1; i<lvl.w-1; ++i)
	{
		// przednia
		if(czy_blokuje2(m[i+(h-1)*w]) && !czy_blokuje2(m[i+(h-2)*w]))
		{
			cobj = new btCollisionObject;
			cobj->setCollisionShape(shape_wall);
			cobj->getWorldTransform().getOrigin().setValue(2.f*i+1.f, 2.f, 2.f*(h-1)+1.f);
			cobj->setCollisionFlags(CG_WALL);
			phy_world->addCollisionObject(cobj);
		}

		// tylna
		if(czy_blokuje2(m[i]) && !czy_blokuje2(m[i+w]))
		{
			cobj = new btCollisionObject;
			cobj->setCollisionShape(shape_wall);
			cobj->getWorldTransform().getOrigin().setValue(2.f*i+1.f, 2.f, 1.f);
			cobj->setCollisionFlags(CG_WALL);
			phy_world->addCollisionObject(cobj);
		}
	}

	// schody w g�r�
	if(inside->HaveUpStairs())
	{
		cobj = new btCollisionObject;
		cobj->setCollisionShape(shape_schody);
		cobj->getWorldTransform().getOrigin().setValue(2.f*lvl.schody_gora.x+1.f, 0.f, 2.f*lvl.schody_gora.y+1.f);
		cobj->getWorldTransform().setRotation(btQuaternion(dir_to_rot(lvl.schody_gora_dir), 0, 0));
		cobj->setCollisionFlags(CG_WALL);
		phy_world->addCollisionObject(cobj);
	}
}

void Game::RemoveColliders()
{
	if(phy_world)
		phy_world->Reset();

	DeleteElements(shapes);
}

void Game::CreateCollisionShapes()
{
	shape_wall = new btBoxShape(btVector3(1.f, 2.f, 1.f));
	shape_low_celling = new btBoxShape(btVector3(1.f, 0.5f, 1.f));
	shape_celling = new btStaticPlaneShape(btVector3(0.f, -1.f, 0.f), 4.f);
	shape_floor = new btStaticPlaneShape(btVector3(0.f, 1.f, 0.f), 0.f);
	shape_door = new btBoxShape(btVector3(0.842f, 1.319f, 0.181f));
	shape_block = new btBoxShape(btVector3(1.f,4.f,1.f));
	btCompoundShape* s = new btCompoundShape;
	btBoxShape* b = new btBoxShape(btVector3(1.f,2.f,0.1f));
	shape_schody_c[0] = b;
	btTransform tr;
	tr.setIdentity();
	tr.setOrigin(btVector3(0.f,2.f,0.95f));
	s->addChildShape(tr, b);
	b = new btBoxShape(btVector3(0.1f,2.f,1.f));
	shape_schody_c[1] = b;
	tr.setOrigin(btVector3(-0.95f,2.f,0.f));
	s->addChildShape(tr, b);
	tr.setOrigin(btVector3(0.95f,2.f,0.f));
	s->addChildShape(tr, b);
	shape_schody = s;

	Animesh::Point* point = aArrow->FindPoint("Empty");
	assert(point && point->IsBox());

	btBoxShape* box = new btBoxShape(ToVector3(point->size));

	/*btCompoundShape* comp = new btCompoundShape;
	btTransform tr(btQuaternion(), btVector3(0.f,point->size.y,0.f));
	comp->addChildShape(tr, box);*/

	obj_arrow = new btCollisionObject;
	obj_arrow->setCollisionShape(box);

	obj_spell = new btCollisionObject;
}

inline float dot2d(const VEC3& v1, const VEC3& v2)
{
	return (v1.x*v2.x + v1.z*v2.z);
}

inline float dot2d(const VEC3& v1)
{
	return (v1.x*v1.x + v1.z*v1.z);
}

VEC3 Game::PredictTargetPos(const Unit& me, const Unit& target, float bullet_speed) const
{
	if(bullet_speed == 0.f)
		return target.GetCenter();
	else
	{
		VEC3 vel = target.pos - target.prev_pos;
		vel *= 60;

		float a = bullet_speed*bullet_speed - dot2d(vel);
		float b = -2*dot2d(vel, target.pos-me.pos);
		float c = -dot2d(target.pos-me.pos);

		float delta = b*b-4*a*c;
		// brak rozwi�zania, nie mo�e trafi� wi�c strzel w aktualn� pozycj�
		if(delta < 0)
			return target.GetCenter();

		VEC3 pos = target.pos + ((b+std::sqrt(delta))/(2*a)) * VEC3(vel.x,0,vel.z);
		pos.y += target.GetUnitHeight()/2;
		return pos;
	}
}

struct BulletRaytestCallback : public btCollisionWorld::RayResultCallback
{
	BulletRaytestCallback(const void* _ignore1, const void* _ignore2) : ignore1(_ignore1), ignore2(_ignore2), clear(true)
	{
	}

	btScalar addSingleResult(btCollisionWorld::LocalRayResult& rayResult, bool normalInWorldSpace)
	{
		if(OR2_EQ(rayResult.m_collisionObject->getUserPointer(), ignore1, ignore2))
			return 1.f;
		clear = false;
		return 0.f;
	}

	bool clear;
	const void* ignore1, *ignore2;
};

struct BulletRaytestCallback2 : public btCollisionWorld::RayResultCallback
{
	BulletRaytestCallback2() : clear(true)
	{
	}

	btScalar addSingleResult(btCollisionWorld::LocalRayResult& rayResult, bool normalInWorldSpace)
	{
		if(IS_SET(rayResult.m_collisionObject->getCollisionFlags(), CG_UNIT))
			return 1.f;
		clear = false;
		return 0.f;
	}

	bool clear;
};

bool Game::CanShootAtLocation(const VEC3& from, const VEC3& to) const
{
	BulletRaytestCallback2 callback;
	phy_world->rayTest(ToVector3(from), ToVector3(to), callback);
	return callback.clear;
}

bool Game::CanShootAtLocation2(const Unit& me, const void* ptr, const VEC3& to) const
{
	BulletRaytestCallback callback(&me, ptr);
	phy_world->rayTest(btVector3(me.pos.x, me.pos.y+1.f, me.pos.z), btVector3(to.x, to.y+1.f, to.z), callback);
	return callback.clear;
}

void Game::LoadItems(Item* _items, uint _count, uint _stride)
{
	assert(_items && _count && _stride >= sizeof(Item));
	byte* ptr = (byte*)_items;

	for(uint i=0; i<_count; ++i)
	{
		Item& item = *(Item*)ptr;

		if(IS_SET(item.flags, ITEM_TEX_ONLY))
		{
			item.ani = NULL;
			load_tasks.push_back(LoadTask(item.mesh, &item.tex));
		}
		else
			load_tasks.push_back(LoadTask(item.mesh, &item));

		ptr += _stride;
	}
}

void Game::SpawnTerrainCollider()
{
	if(terrain_shape)
		delete terrain_shape;

	terrain_shape = new btHeightfieldTerrainShape(OutsideLocation::size+1, OutsideLocation::size+1, terrain->GetHeightMap(), 1.f, 0.f, 10.f, 1, PHY_FLOAT, false);
	terrain_shape->setLocalScaling(btVector3(2.f, 1.f, 2.f));

	obj_terrain = new btCollisionObject;
	obj_terrain->setCollisionShape(terrain_shape);
	obj_terrain->getWorldTransform().setOrigin(btVector3(float(OutsideLocation::size), 5.f, float(OutsideLocation::size)));
	obj_terrain->setCollisionFlags(CG_WALL);
	phy_world->addCollisionObject(obj_terrain);
}

void Game::GenerateDungeonObjects()
{
	InsideLocation* inside = (InsideLocation*)location;
	InsideLocationLevel& lvl = inside->GetLevelData();
	BaseLocation& base = g_base_locations[inside->cel];
	static vector<Chest*> room_chests;
	static vector<VEC3> on_wall;
	static vector<INT2> blocks;
	IgnoreObjects ignore = {0};
	ignore.ignore_blocks = true;
	int chest_lvl = GetDungeonLevelChest();
	
	// dotyczy tylko pochodni
	int flags = 0;
	if(IS_SET(base.options, BLO_MAGIC_LIGHT))
		flags = SOE_MAGIC_LIGHT;

	bool wymagany = false;
	if(base.wymagany.room)
		wymagany = true;

	for(vector<Pokoj>::iterator it = lvl.pokoje.begin(), end = lvl.pokoje.end(); it != end; ++it)
	{
		if(it->korytarz)
			continue;

		RoomType* rt;

		// dodaj blokady
		for(int x=0; x<it->size.x; ++x)
		{
			// g�rna kraw�d�
			POLE co = lvl.mapa[it->pos.x+x+(it->pos.y+it->size.y-1)*lvl.w].co;
			if(co == PUSTE || co == KRATKA || co == KRATKA_PODLOGA || co == KRATKA_SUFIT || co == DRZWI || co == OTWOR_NA_DRZWI)
			{
				blocks.push_back(INT2(it->pos.x+x, it->pos.y+it->size.y-1));
				blocks.push_back(INT2(it->pos.x+x, it->pos.y+it->size.y-2));
			}
			else if(co == SCIANA || co == BLOKADA_SCIANA)
				blocks.push_back(INT2(it->pos.x+x, it->pos.y+it->size.y-1));

			// dolna kraw�d�
			co = lvl.mapa[it->pos.x+x+it->pos.y*lvl.w].co;
			if(co == PUSTE || co == KRATKA || co == KRATKA_PODLOGA || co == KRATKA_SUFIT || co == DRZWI || co == OTWOR_NA_DRZWI)
			{
				blocks.push_back(INT2(it->pos.x+x, it->pos.y));
				blocks.push_back(INT2(it->pos.x+x, it->pos.y+1));
			}
			else if(co == SCIANA || co == BLOKADA_SCIANA)
				blocks.push_back(INT2(it->pos.x+x, it->pos.y));
		}
		for(int y=0; y<it->size.y; ++y)
		{
			// lewa kraw�d�
			POLE co = lvl.mapa[it->pos.x+(it->pos.y+y)*lvl.w].co;
			if(co == PUSTE || co == KRATKA || co == KRATKA_PODLOGA || co == KRATKA_SUFIT || co == DRZWI || co == OTWOR_NA_DRZWI)
			{
				blocks.push_back(INT2(it->pos.x, it->pos.y+y));
				blocks.push_back(INT2(it->pos.x+1, it->pos.y+y));
			}
			else if(co == SCIANA || co == BLOKADA_SCIANA)
				blocks.push_back(INT2(it->pos.x, it->pos.y+y));

			// prawa kraw�d�
			co = lvl.mapa[it->pos.x+it->size.x-1+(it->pos.y+y)*lvl.w].co;
			if(co == PUSTE || co == KRATKA || co == KRATKA_PODLOGA || co == KRATKA_SUFIT || co == DRZWI || co == OTWOR_NA_DRZWI)
			{
				blocks.push_back(INT2(it->pos.x+it->size.x-1, it->pos.y+y));
				blocks.push_back(INT2(it->pos.x+it->size.x-2, it->pos.y+y));
			}
			else if(co == SCIANA || co == BLOKADA_SCIANA)
				blocks.push_back(INT2(it->pos.x+it->size.x-1, it->pos.y+y));
		}
		if(it->cel != POKOJ_CEL_BRAK)
		{
			if(it->cel == POKOJ_CEL_SKARBIEC)
				rt = FindRoomType("krypta_skarb");
			else if(it->cel == POKOJ_CEL_TRON)
				rt = FindRoomType("tron");
			else if(it->cel == POKOJ_CEL_PORTAL_STWORZ)
				rt = FindRoomType("portal");
			else
			{
				INT2 pt;
				if(it->cel == POKOJ_CEL_SCHODY_DOL)
					pt = lvl.schody_dol;
				else if(it->cel == POKOJ_CEL_SCHODY_GORA)
					pt = lvl.schody_gora;
				else if(it->cel == POKOJ_CEL_PORTAL)
				{
					if(inside->portal)
						pt = pos_to_pt(inside->portal->pos);
					else
					{
						Object* o = local_ctx.FindObject(FindObject("portal"));
						if(o)
							pt = pos_to_pt(o->pos);
						else
							pt = pos_to_pt(it->GetCenter());
					}
				}

				for(int y=-1; y<=1; ++y)
				{
					for(int x=-1; x<=1; ++x)
						blocks.push_back(INT2(pt.x+x, pt.y+y));
				}

				if(base.schody.room)
					rt = base.schody.room;
				else
					rt = base.GetRandomRoomType();
			}
		}
		else
		{
			if(wymagany)
				rt = base.wymagany.room;
			else
				rt = base.GetRandomRoomType();
		}

		int fail = 10;
		bool wymagany_obiekt = false;

		for(int i=0; i<rt->count && fail > 0; ++i)
		{
			bool is_variant = false;
			Obj* obj = FindObject(rt->objs[i].id, &is_variant);
			if(!obj)
				continue;

			int count = random(rt->objs[i].count);

			for(int j=0; j<count && fail > 0; ++j)
			{
				VEC3 pos;
				float rot;
				VEC2 shift;

				if(obj->type == OBJ_CYLINDER)
				{
					shift.x = obj->r + obj->extra_dist;
					shift.y = obj->r + obj->extra_dist;
				}
				else
					shift = obj->size + VEC2(obj->extra_dist, obj->extra_dist);

				if(IS_SET(obj->flagi, OBJ_PRZY_SCIANIE))
				{
					INT2 tile;
					int dir;
					if(!lvl.GetRandomNearWallTile(*it, tile, dir, IS_SET(obj->flagi, OBJ_NA_SCIANIE)))
					{
						if(IS_SET(obj->flagi, OBJ_WAZNE))
							--j;
						--fail;
						continue;
					}
					
					rot = dir_to_rot(dir);

					if(dir == 2 || dir == 3)
						pos = VEC3(2.f*tile.x+sin(rot)*(2.f-shift.y-0.01f)+2, 0.f, 2.f*tile.y+cos(rot)*(2.f-shift.y-0.01f)+2);
					else
						pos = VEC3(2.f*tile.x+sin(rot)*(2.f-shift.y-0.01f), 0.f, 2.f*tile.y+cos(rot)*(2.f-shift.y-0.01f));

					if(IS_SET(obj->flagi, OBJ_NA_SCIANIE))
					{
						switch(dir)
						{
						case 0:
							pos.x += 1.f;
							break;
						case 1:
							pos.z += 1.f;
							break;
						case 2:
							pos.x -= 1.f;
							break;
						case 3:
							pos.z -= 1.f;
							break;
						}

						bool ok = true;

						for(vector<VEC3>::iterator it2 = on_wall.begin(), end2 = on_wall.end(); it2 != end2; ++it2)
						{
							float dist = distance2d(*it2, pos);
							if(dist < 2.f)
							{
								ok = false;
								continue;
							}
						}

						if(!ok)
						{
							if(IS_SET(obj->flagi, OBJ_WAZNE))
								--j;
							fail = true;
							continue;
						}
					}
					else
					{
						switch(dir)
						{
						case 0:
							pos.x += random(0.2f,1.8f);
							break;
						case 1:
							pos.z += random(0.2f,1.8f);
							break;
						case 2:
							pos.x -= random(0.2f,1.8f);
							break;
						case 3:
							pos.z -= random(0.2f,1.8f);
							break;
						}
					}
				}
				else if(IS_SET(obj->flagi, OBJ_NA_SRODKU))
				{
					rot = PI/2*(rand2()%4);
					pos = it->GetCenter();
					switch(rand2()%4)
					{
					case 0:
						pos.x += 2;
						break;
					case 1:
						pos.x -= 2;
						break;
					case 2:
						pos.z += 2;
						break;
					case 3:
						pos.z -= 2;
						break;
					}
				}
				else
				{
					rot = random(MAX_ANGLE);

					if(obj->type == OBJ_CYLINDER)
						pos = it->GetRandomPos(max(obj->size.x, obj->size.y));
					else
						pos = it->GetRandomPos(obj->r);
				}

				if(IS_SET(obj->flagi, OBJ_WYSOKO))
					pos.y += 1.5f;

				if(obj->type == OBJ_HITBOX)
				{
					// sprawd� kolizje z blokami
					if(!IS_SET(obj->flagi, OBJ_BRAK_FIZYKI))
					{
						bool ok = true;
						if(not_zero(rot))
						{
							RotRect r1, r2;
							r1.center.x = pos.x;
							r1.center.y = pos.z;
							r1.rot = rot;
							r1.size = shift;
							r2.rot = 0;
							r2.size = VEC2(1,1);
							for(vector<INT2>::iterator b_it = blocks.begin(), b_end = blocks.end(); b_it != b_end; ++b_it)
							{
								r2.center.x = 2.f*b_it->x+1.f;
								r2.center.y = 2.f*b_it->y+1.f;
								if(RotatedRectanglesCollision(r1, r2))
								{
									ok = false;
									break;
								}
							}
						}
						else
						{
							for(vector<INT2>::iterator b_it = blocks.begin(), b_end = blocks.end(); b_it != b_end; ++b_it)
							{
								if(RectangleToRectangle(pos.x-shift.x, pos.z-shift.y, pos.x+shift.x, pos.z+shift.y, 2.f*b_it->x, 2.f*b_it->y, 2.f*(b_it->x+1), 2.f*(b_it->y+1)))
								{
									ok = false;
									break;
								}
							}
						}
						if(!ok)
						{
							if(IS_SET(obj->flagi, OBJ_WAZNE))
								--j;
							--fail;
							continue;
						}
					}

					// sprawd� kolizje z innymi obiektami
					global_col.clear();
					GatherCollisionObjects(local_ctx, global_col, pos, max(shift.x, shift.y) * SQRT_2, &ignore);
					if(!global_col.empty() && Collide(global_col, BOX2D(pos.x-shift.x, pos.z-shift.y, pos.x+shift.x, pos.z+shift.y), 0.8f, rot))
					{
						if(IS_SET(obj->flagi, OBJ_WAZNE))
							--j;
						--fail;
						continue;
					}
				}
				else
				{
					// sprawd� kolizje z blokami
					if(!IS_SET(obj->flagi, OBJ_BRAK_FIZYKI))
					{
						bool ok = true;
						for(vector<INT2>::iterator b_it = blocks.begin(), b_end = blocks.end(); b_it != b_end; ++b_it)
						{
							if(CircleToRectangle(pos.x, pos.z, shift.x, 2.f*b_it->x+1.f, 2.f*b_it->y+1.f, 1.f, 1.f))
							{
								ok = false;
								break;
							}
						}
						if(!ok)
						{
							if(IS_SET(obj->flagi, OBJ_WAZNE))
								--j;
							--fail;
							continue;
						}
					}

					// sprawd� kolizje z innymi obiektami
					global_col.clear();
					GatherCollisionObjects(local_ctx, global_col, pos, obj->r, &ignore);
					if(!global_col.empty() && Collide(global_col, pos, obj->r+0.8f))
					{
						if(IS_SET(obj->flagi, OBJ_WAZNE))
							--j;
						--fail;
						continue;
					}
				}

				if(IS_SET(obj->flagi, OBJ_STOL))
				{
					Obj* stol = FindObject(rand2()%2 == 0 ? "table" : "table2");

					// st�
					{
						Object& o = Add1(local_ctx.objects);
						o.mesh = stol->ani;
						o.rot = VEC3(0,rot,0);
						o.pos = pos;
						o.scale = 1;
						o.base = stol;

						SpawnObjectExtras(local_ctx, stol, pos, rot, NULL, NULL);
					}

					// sto�ki
					Obj* stolek = FindObject("stool");
					int ile = random(2, 4);
					int d[4] = {0,1,2,3};
					for(int i=0; i<4; ++i)
						std::swap(d[rand2()%4], d[rand2()%4]);

					for(int i=0; i<ile; ++i)
					{
						float sdir, slen;
						switch(d[i])
						{
						case 0:
							sdir = 0.f;
							slen = stol->size.y+0.3f;
							break;
						case 1:
							sdir = PI/2;
							slen = stol->size.x+0.3f;
							break;
						case 2:
							sdir = PI;
							slen = stol->size.y+0.3f;
							break;
						case 3:
							sdir = PI*3/2;
							slen = stol->size.x+0.3f;
							break;
						default:
							assert(0);
							break;
						}

						sdir += rot;
						
						Useable* u = new Useable;
						local_ctx.useables->push_back(u);
						u->type = U_STOLEK;
						u->pos = pos + VEC3(sin(sdir)*slen, 0, cos(sdir)*slen);
						u->rot = sdir;
						u->user = NULL;

						if(IsOnline())
							u->netid = useable_netid_counter++;

						SpawnObjectExtras(local_ctx, stolek, u->pos, u->rot, u, NULL);
					}
				}
				else
				{
					void* obj_ptr = NULL;
					void** result_ptr = NULL;

					if(IS_SET(obj->flagi, OBJ_SKRZYNIA))
					{
						Chest* chest = new Chest;
						chest->ani = new AnimeshInstance(aSkrzynia);
						chest->pos = pos;
						chest->rot = rot;
						chest->handler = NULL;
						chest->looted = false;
						room_chests.push_back(chest);
						local_ctx.chests->push_back(chest);
						if(IsOnline())
							chest->netid = chest_netid_counter++;
					}
					else if(IS_SET(obj->flagi, OBJ_UZYWALNY))
					{
						int typ;
						if(IS_SET(obj->flagi, OBJ_LAWA))
							typ = U_LAWA;
						else if(IS_SET(obj->flagi, OBJ_KOWADLO))
							typ = U_KOWADLO;
						else if(IS_SET(obj->flagi, OBJ_KRZESLO))
							typ = U_KRZESLO;
						else if(IS_SET(obj->flagi, OBJ_KOCIOLEK))
							typ = U_KOCIOLEK;
						else if(IS_SET(obj->flagi, OBJ_ZYLA_ZELAZA))
							typ = U_ZYLA_ZELAZA;
						else if(IS_SET(obj->flagi, OBJ_ZYLA_ZLOTA))
							typ = U_ZYLA_ZLOTA;
						else if(IS_SET(obj->flagi, OBJ_TRON))
							typ = U_TRON;
						else if(IS_SET(obj->flagi, OBJ_STOLEK))
							typ = U_STOLEK;
						else if(IS_SET(obj->flagi2, OBJ2_LAWA_DIR))
							typ = U_LAWA_DIR;
						else
						{
							assert(0);
							typ = U_KRZESLO;
						}

						Useable* u = new Useable;
						u->type = typ;
						u->pos = pos;
						u->rot = rot;
						u->user = NULL;
						Obj* base_obj = u->GetBase()->obj;
						if(IS_SET(base_obj->flagi2, OBJ2_VARIANT))
						{
							// extra code for bench
							if(typ == U_LAWA || typ == U_LAWA_DIR)
							{
								switch(location->type)
								{
								case L_CITY:
								case L_VILLAGE:
									u->variant = 0;
									break;
								case L_DUNGEON:
								case L_CRYPT:
									u->variant = rand2()%2;
									break;
								default:
									u->variant = rand2()%2+2;
									break;
								}
							}
							else
								u->variant = random<int>(base_obj->variant->count-1);
						}
						else
							u->variant = -1;
						local_ctx.useables->push_back(u);
						obj_ptr = u;

						if(IsOnline())
							u->netid = useable_netid_counter++;
					}
					else
					{
						Object& o = Add1(local_ctx.objects);
						o.mesh = obj->ani;
						o.rot = VEC3(0,rot,0);
						o.pos = pos;
						o.scale = 1;
						o.base = obj;
						result_ptr = &o.ptr;
						obj_ptr = &o;
					}

					SpawnObjectExtras(local_ctx, obj, pos, rot, obj_ptr, (btCollisionObject**)result_ptr, 1.f, flags);

					if(IS_SET(obj->flagi, OBJ_WYMAGANE))
						wymagany_obiekt = true;

					if(IS_SET(obj->flagi, OBJ_NA_SCIANIE))
						on_wall.push_back(pos);
				}

				if(is_variant)
					obj = FindObject(rt->objs[i].id);
			}
		}

		if(wymagany && wymagany_obiekt && it->cel == POKOJ_CEL_BRAK)
			wymagany = false;

		if(!room_chests.empty())
		{
			int gold;
			if(room_chests.size() == 1)
			{
				vector<ItemSlot>& items = room_chests.front()->items;
				GenerateTreasure(chest_lvl, 10, items, gold);
				InsertItemBare(items, &gold_item, (uint)gold, (uint)gold);
				SortItems(items);
			}
			else
			{
				static vector<ItemSlot> items;
				GenerateTreasure(chest_lvl, 9+2*room_chests.size(), items, gold);
				SplitTreasure(items, gold, &room_chests[0], room_chests.size());
			}
			
			room_chests.clear();
		}

		on_wall.clear();
		blocks.clear();
	}

	if(wymagany)
		throw "Failed to generate required object!";

	if(local_ctx.chests->empty())
	{
		// niech w podziemiach b�dzie minimum 1 skrzynia
		Obj* obj = FindObject("chest");
		for(int i=0; i<100; ++i)
		{
			on_wall.clear();
			blocks.clear();
			Pokoj& p = lvl.pokoje[rand2()%lvl.pokoje.size()];
			if(p.cel == POKOJ_CEL_BRAK)
			{
				// dodaj blokady
				for(int x=0; x<p.size.x; ++x)
				{
					// g�rna kraw�d�
					POLE co = lvl.mapa[p.pos.x+x+(p.pos.y+p.size.y-1)*lvl.w].co;
					if(co == PUSTE || co == KRATKA || co == KRATKA_PODLOGA || co == KRATKA_SUFIT || co == DRZWI || co == OTWOR_NA_DRZWI)
					{
						blocks.push_back(INT2(p.pos.x+x, p.pos.y+p.size.y-1));
						blocks.push_back(INT2(p.pos.x+x, p.pos.y+p.size.y-2));
					}
					else if(co == SCIANA || co == BLOKADA_SCIANA)
						blocks.push_back(INT2(p.pos.x+x, p.pos.y+p.size.y-1));

					// dolna kraw�d�
					co = lvl.mapa[p.pos.x+x+p.pos.y*lvl.w].co;
					if(co == PUSTE || co == KRATKA || co == KRATKA_PODLOGA || co == KRATKA_SUFIT || co == DRZWI || co == OTWOR_NA_DRZWI)
					{
						blocks.push_back(INT2(p.pos.x+x, p.pos.y));
						blocks.push_back(INT2(p.pos.x+x, p.pos.y+1));
					}
					else if(co == SCIANA || co == BLOKADA_SCIANA)
						blocks.push_back(INT2(p.pos.x+x, p.pos.y));
				}
				for(int y=0; y<p.size.y; ++y)
				{
					// lewa kraw�d�
					POLE co = lvl.mapa[p.pos.x+(p.pos.y+y)*lvl.w].co;
					if(co == PUSTE || co == KRATKA || co == KRATKA_PODLOGA || co == KRATKA_SUFIT || co == DRZWI || co == OTWOR_NA_DRZWI)
					{
						blocks.push_back(INT2(p.pos.x, p.pos.y+y));
						blocks.push_back(INT2(p.pos.x+1, p.pos.y+y));
					}
					else if(co == SCIANA || co == BLOKADA_SCIANA)
						blocks.push_back(INT2(p.pos.x, p.pos.y+y));

					// prawa kraw�d�
					co = lvl.mapa[p.pos.x+p.size.x-1+(p.pos.y+y)*lvl.w].co;
					if(co == PUSTE || co == KRATKA || co == KRATKA_PODLOGA || co == KRATKA_SUFIT || co == DRZWI || co == OTWOR_NA_DRZWI)
					{
						blocks.push_back(INT2(p.pos.x+p.size.x-1, p.pos.y+y));
						blocks.push_back(INT2(p.pos.x+p.size.x-2, p.pos.y+y));
					}
					else if(co == SCIANA || co == BLOKADA_SCIANA)
						blocks.push_back(INT2(p.pos.x+p.size.x-1, p.pos.y+y));
				}

				VEC3 pos;
				float rot;
				VEC2 shift;

				if(obj->type == OBJ_CYLINDER)
				{
					shift.x = obj->r;
					shift.y = obj->r;
				}
				else
					shift = obj->size;

				if(IS_SET(obj->flagi, OBJ_PRZY_SCIANIE))
				{
					INT2 tile;
					int dir;
					if(!lvl.GetRandomNearWallTile(p, tile, dir, IS_SET(obj->flagi, OBJ_NA_SCIANIE)))
						continue;

					rot = dir_to_rot(dir);
					if(dir == 2 || dir == 3)
						pos = VEC3(2.f*tile.x+sin(rot)*(2.f-shift.y-0.01f)+2, 0.f, 2.f*tile.y+cos(rot)*(2.f-shift.y-0.01f)+2);
					else
						pos = VEC3(2.f*tile.x+sin(rot)*(2.f-shift.y-0.01f), 0.f, 2.f*tile.y+cos(rot)*(2.f-shift.y-0.01f));

					switch(dir)
					{
					case 0:
						pos.x += random(0.2f,1.8f);
						break;
					case 1:
						pos.z += random(0.2f,1.8f);
						break;
					case 2:
						pos.x -= random(0.2f,1.8f);
						break;
					case 3:
						pos.z -= random(0.2f,1.8f);
						break;
					}
				}
				else if(IS_SET(obj->flagi, OBJ_NA_SRODKU))
				{
					rot = PI/2*(rand2()%4);
					pos = p.GetCenter();
					switch(rand2()%4)
					{
					case 0:
						pos.x += 2;
						break;
					case 1:
						pos.x -= 2;
						break;
					case 2:
						pos.z += 2;
						break;
					case 3:
						pos.z -= 2;
						break;
					}
				}
				else
				{
					rot = random(MAX_ANGLE);

					if(obj->type == OBJ_CYLINDER)
						pos = p.GetRandomPos(max(obj->size.x, obj->size.y));
					else
						pos = p.GetRandomPos(obj->r);
				}

				if(IS_SET(obj->flagi, OBJ_WYSOKO))
					pos.y += 1.5f;

				if(obj->type == OBJ_HITBOX)
				{
					// sprawd� kolizje z blokami
					bool ok = true;
					if(not_zero(rot))
					{
						RotRect r1, r2;
						r1.center.x = pos.x;
						r1.center.y = pos.z;
						r1.rot = rot;
						r1.size = shift;
						r2.rot = 0;
						r2.size = VEC2(1,1);
						for(vector<INT2>::iterator b_it = blocks.begin(), b_end = blocks.end(); b_it != b_end; ++b_it)
						{
							r2.center.x = 2.f*b_it->x+1.f;
							r2.center.y = 2.f*b_it->y+1.f;
							if(RotatedRectanglesCollision(r1, r2))
							{
								ok = false;
								break;
							}
						}
					}
					else
					{
						for(vector<INT2>::iterator b_it = blocks.begin(), b_end = blocks.end(); b_it != b_end; ++b_it)
						{
							if(RectangleToRectangle(pos.x-shift.x, pos.z-shift.y, pos.x+shift.x, pos.z+shift.y, 2.f*b_it->x, 2.f*b_it->y, 2.f*(b_it->x+1), 2.f*(b_it->y+1)))
							{
								ok = false;
								break;
							}
						}
					}
					if(!ok)
						continue;

					// sprawd� kolizje z innymi obiektami
					global_col.clear();
					GatherCollisionObjects(local_ctx, global_col, pos, max(shift.x, shift.y) * SQRT_2, &ignore);
					if(!global_col.empty() && Collide(global_col, BOX2D(pos.x-shift.x, pos.z-shift.y, pos.x+shift.x, pos.z+shift.y), 0.4f, rot))
						continue;
				}
				else
				{
					// sprawd� kolizje z blokami
					bool ok = true;
					for(vector<INT2>::iterator b_it = blocks.begin(), b_end = blocks.end(); b_it != b_end; ++b_it)
					{
						if(CircleToRectangle(pos.x, pos.z, shift.x, 2.f*b_it->x+1.f, 2.f*b_it->y+1.f, 1.f, 1.f))
						{
							ok = false;
							break;
						}
					}
					if(!ok)
						continue;

					// sprawd� kolizje z innymi obiektami
					global_col.clear();
					GatherCollisionObjects(local_ctx, global_col, pos, obj->r, &ignore);
					if(!global_col.empty() && Collide(global_col, pos, obj->r+0.4f))
						continue;
				}

				Chest* chest = new Chest;
				chest->ani = new AnimeshInstance(aSkrzynia);
				chest->pos = pos;
				chest->rot = rot;
				chest->handler = NULL;
				chest->looted = false;
				local_ctx.chests->push_back(chest);
				if(IsOnline())
					chest->netid = chest_netid_counter++;

				SpawnObjectExtras(local_ctx, obj, pos, rot, NULL, NULL, 1.f, flags);

				vector<ItemSlot>& items = chest->items;
				int gold;
				GenerateTreasure(chest_lvl, 10, items, gold);
				InsertItemBare(items, &gold_item, (uint)gold, (uint)gold);
				SortItems(items);

				break;
			}
		}
	}
}

int wartosc_skarbu[] = {
	10, //0
	50, //1
	100, //2
	200, //3
	350, //4
	500, //5
	700, //6
	900, //7
	1200, //8
	1500, //9
	1900, //10
	2400, //11
	2900, //12
	3400, //13
	4000, //14
	4600, //15
	5300, //16
	6000, //17
	6800, //18
	7600, //19
	8500 //20
};

void Game::GenerateTreasure(int level, int _count, vector<ItemSlot>& items, int& gold)
{
	assert(in_range(level, 1, 20));

	int value = random(wartosc_skarbu[level-1], wartosc_skarbu[level]);

	items.clear();

	const Item* item;
	uint count;

	for(int tries = 0; tries < _count; ++tries)
	{
		switch(rand2()%IT_MAX_GEN)
		{
		case IT_WEAPON:
			item = &g_weapons[rand2()%n_weapons];
			count = 1;
			break;
		case IT_ARMOR:
			item = &g_armors[rand2()%n_armors];
			count = 1;
			break;
		case IT_BOW:
			item = &g_bows[rand2()%n_bows];
			count = 1;
			break;
		case IT_SHIELD:
			item = &g_shields[rand2()%n_shields];
			count = 1;
			break;
		case IT_CONSUMEABLE:
			item = &g_consumeables[rand2()%n_consumeables];
			count = random(2,5);
			break;
		case IT_OTHER:
			item = &g_others[rand2()%n_others];
			count = random(1,4);
			break;
		default:
			assert(0);
			item = NULL;
			break;
		}

		if(!item->CanBeGenerated())
			continue;

		int cost = item->value * count;
		if(cost > value)
			continue;

		value -= cost;

		InsertItemBare(items, item, count, count);
	}

	gold = value+level*5;
}

Item* gold_item_ptr;

void Game::SplitTreasure(vector<ItemSlot>& items, int gold, Chest** chests, int count)
{
	assert(gold >= 0 && count > 0 && chests);

	while(!items.empty())
	{
		for(int i=0; i<count && !items.empty(); ++i)
		{
			chests[i]->items.push_back(items.back());
			items.pop_back();
		}
	}

	int ile = gold/count-1;
	if(ile < 0)
		ile = 0;
	gold -= ile * count;

	for(int i=0; i<count; ++i)
	{
		if(i == count-1)
			ile += gold;
		ItemSlot& slot = Add1(chests[i]->items);
		slot.Set(gold_item_ptr, ile, ile);
		SortItems(chests[i]->items);
	}
}

struct EnemyGroupPart
{
	vector<EnemyEntry> enemies;
	int total, max_level;
};

void Game::GenerateDungeonUnits()
{
	SPAWN_GROUP spawn_group;
	int base_level;

	if(location->spawn != SG_WYZWANIE)
	{
		spawn_group = location->spawn;
		base_level = GetDungeonLevel();
	}
	else
	{
		base_level = location->st;
		if(dungeon_level == 0)
			spawn_group = SG_ORKOWIE;
		else if(dungeon_level == 1)
			spawn_group = SG_MAGOWIE_I_GOLEMY;
		else
			spawn_group = SG_ZLO;
	}
	
	SpawnGroup& spawn = g_spawn_groups[spawn_group];
	if(spawn.id == -1)
		return;
	EnemyGroup& group = g_enemy_groups[spawn.id];
	static vector<EnemyGroupPart> groups;

	int level = base_level;

	// poziomy 1..3
	for(int i=0; i<3; ++i)
	{
		EnemyGroupPart& part = Add1(groups);
		part.total = 0;
		part.max_level = level;

		for(int j=0; j<group.count; ++j)
		{
			if(group.enemies[j].unit->level.x <= level)
			{
				part.enemies.push_back(group.enemies[j]);
				part.total += group.enemies[j].count;
			}
		}

		level = min(base_level-i-1, base_level*4/(i+5));
	}

	// opcje wej�ciowe (p�ki co tu)
	// musi by� w sumie 100%
	int szansa_na_brak = 10,
		szansa_na_1 = 20,
		szansa_na_2 = 30,
		szansa_na_3 = 40,
		szansa_na_wrog_w_korytarz = 25;

	assert(in_range(szansa_na_brak, 0, 100) && in_range(szansa_na_1, 0, 100) && in_range(szansa_na_2, 0, 100) && in_range(szansa_na_3, 0, 100) && in_range(szansa_na_wrog_w_korytarz, 0, 100) &&
		szansa_na_brak + szansa_na_1 + szansa_na_2 + szansa_na_3 == 100);

	int szansa[3] = {szansa_na_brak, szansa_na_brak+szansa_na_1, szansa_na_brak+szansa_na_1+szansa_na_2};

	// dodaj jednostki
	InsideLocation* inside = (InsideLocation*)location;
	InsideLocationLevel& lvl = inside->GetLevelData();
	INT2 pt = lvl.schody_gora, pt2 = lvl.schody_dol;
	if(!inside->HaveDownStairs())
		pt2 = INT2(-1000,-1000);
	if(inside->from_portal)
		pt = pos_to_pt(inside->portal->pos);

	for(vector<Pokoj>::iterator it = lvl.pokoje.begin(), end = lvl.pokoje.end(); it != end; ++it)
	{
		int ile;

		if(it->cel == POKOJ_CEL_SKARBIEC || it->cel == POKOJ_CEL_WIEZIENIE)
			continue;

		if(it->korytarz)
		{
			if(rand2()%100 < szansa_na_wrog_w_korytarz)
				ile = 1;
			else
				continue;
		}
		else
		{
			int x = rand2()%100;
			if(x < szansa[0])
				continue;
			else if(x < szansa[1])
				ile = 1;
			else if(x < szansa[2])
				ile = 2;
			else
				ile = 3;
		}

		EnemyGroupPart& part = groups[ile-1];
		if(part.total == 0)
			continue;

		for(int i=0; i<ile; ++i)
		{
			int x = rand2()%part.total,
				y = 0;

			for(int j=0; j<(int)part.enemies.size(); ++j)
			{
				y += part.enemies[j].count;

				if(x < y)
				{
					// dodaj
					SpawnUnitInsideRoom(*it, *part.enemies[j].unit, random(part.max_level/2, part.max_level), pt, pt2);
					break;
				}
			}
		}
	}

	// posprz�taj
	groups.clear();
}

void Game::SetUnitPointers()
{
	for(int i=0; i<n_enemy_groups; ++i)
	{
		EnemyGroup& g = g_enemy_groups[i];
		for(int j=0; j<g.count; ++j)
		{
			g.enemies[j].unit = FindUnitData(g.enemies[j].id);
		}
	}
}

const VEC2 POISSON_DISC_2D[] = {
	VEC2(-0.6271834f, -0.3647562f),
	VEC2(-0.6959124f, -0.1932297f),
	VEC2(-0.425675f, -0.4331925f),
	VEC2(-0.8259574f, -0.3775373f),
	VEC2(-0.4134415f, -0.2794108f),
	VEC2(-0.6711653f, -0.5842927f),
	VEC2(-0.505241f, -0.5710775f),
	VEC2(-0.5399489f, -0.1941965f),
	VEC2(-0.2056243f, -0.3328375f),
	VEC2(-0.2721521f, -0.4913186f),
	VEC2(0.009952361f, -0.4938473f),
	VEC2(-0.3341284f, -0.7402002f),
	VEC2(-0.009171869f, -0.1417411f),
	VEC2(-0.05370279f, -0.3561031f),
	VEC2(-0.2042215f, -0.1395438f),
	VEC2(0.1491909f, -0.7528881f),
	VEC2(-0.09437386f, -0.6736782f),
	VEC2(0.2218135f, -0.5837499f),
	VEC2(0.1357503f, -0.2823138f),
	VEC2(0.1759486f, -0.4372835f),
	VEC2(-0.8812768f, -0.1270963f),
	VEC2(-0.5861077f, -0.7143953f),
	VEC2(-0.4840448f, -0.8610057f),
	VEC2(-0.1953385f, -0.9313949f),
	VEC2(-0.3544169f, -0.1299241f),
	VEC2(0.4259588f, -0.3359875f),
	VEC2(0.1780135f, -0.006630601f),
	VEC2(0.3781602f, -0.174012f),
	VEC2(-0.6535406f, 0.07830032f),
	VEC2(-0.4176719f, 0.006290245f),
	VEC2(-0.2157413f, 0.1043319f),
	VEC2(-0.3825159f, 0.1611559f),
	VEC2(-0.04609891f, 0.1563928f),
	VEC2(-0.2525779f, 0.3147326f),
	VEC2(0.6283897f, -0.2800752f),
	VEC2(0.5242329f, -0.4569906f),
	VEC2(0.5337259f, -0.1482658f),
	VEC2(0.4243455f, -0.6266792f),
	VEC2(-0.8479414f, 0.08037262f),
	VEC2(-0.5815527f, 0.3148638f),
	VEC2(-0.790419f, 0.2343442f),
	VEC2(-0.4226354f, 0.3095743f),
	VEC2(-0.09465869f, 0.3677911f),
	VEC2(0.3935578f, 0.04151043f),
	VEC2(0.2390065f, 0.1743644f),
	VEC2(0.02775179f, 0.01711585f),
	VEC2(-0.3588479f, 0.4862351f),
	VEC2(-0.7332007f, 0.3809305f),
	VEC2(-0.5283061f, 0.5106883f),
	VEC2(0.7347565f, -0.04643056f),
	VEC2(0.5254471f, 0.1277963f),
	VEC2(-0.1984853f, 0.6903372f),
	VEC2(-0.1512452f, 0.5094652f),
	VEC2(-0.5878937f, 0.6584677f),
	VEC2(-0.4450369f, 0.7685395f),
	VEC2(0.691914f, -0.552465f),
	VEC2(0.293443f, -0.8303219f),
	VEC2(0.5147449f, -0.8018763f),
	VEC2(0.3373911f, -0.4752345f),
	VEC2(-0.7731022f, 0.6132235f),
	VEC2(-0.9054359f, 0.3877104f),
	VEC2(0.1200563f, -0.9095488f),
	VEC2(-0.05998399f, -0.8304204f),
	VEC2(0.1212275f, 0.4447584f),
	VEC2(-0.04844639f, 0.8149281f),
	VEC2(-0.1576151f, 0.9731216f),
	VEC2(-0.2921374f, 0.8280436f),
	VEC2(0.8305115f, -0.3373946f),
	VEC2(0.7025464f, -0.7087887f),
	VEC2(-0.9783711f, 0.1895637f),
	VEC2(-0.9950094f, 0.03602472f),
	VEC2(-0.02693105f, 0.6184058f),
	VEC2(-0.3686568f, 0.6363685f),
	VEC2(0.07644552f, 0.9160427f),
	VEC2(0.2174875f, 0.6892526f),
	VEC2(0.09518065f, 0.2284235f),
	VEC2(0.2566459f, 0.8855528f),
	VEC2(0.2196656f, -0.1571368f),
	VEC2(0.9549446f, -0.2014009f),
	VEC2(0.4562157f, 0.7741205f),
	VEC2(0.3333389f, 0.413012f),
	VEC2(0.5414181f, 0.2789065f),
	VEC2(0.7839744f, 0.2456573f),
	VEC2(0.6805856f, 0.1255756f),
	VEC2(0.3859844f, 0.2440029f),
	VEC2(0.4403853f, 0.600696f),
	VEC2(0.6249176f, 0.6072751f),
	VEC2(0.5145468f, 0.4502719f),
	VEC2(0.749785f, 0.4564187f),
	VEC2(0.9864355f, -0.0429658f),
	VEC2(0.8654963f, 0.04940263f),
	VEC2(0.9577024f, 0.1808657f)
};
const int poisson_disc_count = countof(POISSON_DISC_2D);

Unit* Game::SpawnUnitInsideRoom(Pokoj &p, UnitData &unit, int level, const INT2& stairs_pt, const INT2& stairs_down_pt)
{
	const float radius = unit.GetRadius();
	VEC3 stairs_pos(2.f*stairs_pt.x+1.f, 0.f, 2.f*stairs_pt.y+1.f);

	for(int i=0; i<10; ++i)
	{
		VEC3 pt = p.GetRandomPos(radius);

		if(distance(stairs_pos, pt) < 10.f)
			continue;

		INT2 my_pt = INT2(int(pt.x/2),int(pt.y/2));
		if(my_pt == stairs_down_pt)
			continue;

		global_col.clear();
		GatherCollisionObjects(local_ctx, global_col, pt, radius, NULL);

		if(!Collide(global_col, pt, radius))
		{
			float rot = random(MAX_ANGLE);
			return CreateUnitWithAI(local_ctx, unit, level, NULL, &pt, &rot);
		}
	}

	return NULL;
}

Unit* Game::SpawnUnitNearLocation(LevelContext& ctx, const VEC3 &pos, UnitData &unit, const VEC3* look_at, int level, float extra_radius)
{
	const float radius = unit.GetRadius();

	global_col.clear();
	GatherCollisionObjects(ctx, global_col, pos, extra_radius+radius, NULL);

	VEC3 tmp_pos = pos;

	for(int i=0; i<10; ++i)
	{
		if(!Collide(global_col, tmp_pos, radius))
		{
			float rot;
			if(look_at)
				rot = lookat_angle(tmp_pos, *look_at);
			else
				rot = random(MAX_ANGLE);
			return CreateUnitWithAI(ctx, unit, level, NULL, &tmp_pos, &rot);
		}

		int j = rand2()%poisson_disc_count;
		tmp_pos = pos;
		tmp_pos.x += POISSON_DISC_2D[j].x*extra_radius;
		tmp_pos.z += POISSON_DISC_2D[j].y*extra_radius;
	}
	
	return NULL;
}

Unit* Game::CreateUnitWithAI(LevelContext& ctx, UnitData& unit, int level, Human* human_data, const VEC3* pos, const float* rot, AIController** ai)
{
	Unit* u = CreateUnit(unit, level, human_data);
	u->in_building = ctx.building_id;

	if(pos)
	{
		if(ctx.type == LevelContext::Outside)
		{
			VEC3 pt = *pos;
			terrain->SetH(pt);
			u->pos = pt;
		}
		else
			u->pos = *pos;
		UpdateUnitPhysics(u, u->pos);
		u->visual_pos = u->pos;
	}
	if(rot)
		u->rot = *rot;

	ctx.units->push_back(u);

	AIController* a = new AIController;
	a->Init(u);

	ais.push_back(a);

	if(ai)
		*ai = a;

	return u;
}

const INT2 g_kierunekk[4] = {
	INT2(0,-1),
	INT2(-1,0),
	INT2(0,1),
	INT2(1,0)
};

void Game::ChangeLevel(int gdzie)
{
	assert(gdzie == 1 || gdzie == -1);

	location_event_handler = NULL;
	UpdateDungeonMinimap(false);

	if(szaleni_stan >= SS_WZIETO_KAMIEN && szaleni_stan < SS_KONIEC)
		SzaleniSprawdzKamien();

	if(IsOnline() && players > 1)
	{
		int poziom = dungeon_level;
		if(gdzie == -1)
			--poziom;
		else
			++poziom;
		if(poziom >= 0)
		{
			packet_data.resize(3);
			packet_data[0] = ID_CHANGE_LEVEL;
			packet_data[1] = (byte)current_location;
			packet_data[2] = (byte)poziom;
			int ack = peer->Send((cstring)&packet_data[0], 3, HIGH_PRIORITY, RELIABLE_WITH_ACK_RECEIPT, 0, UNASSIGNED_SYSTEM_ADDRESS, true);
			for(vector<PlayerInfo>::iterator it = game_players.begin(), end = game_players.end(); it != end; ++it)
			{
				if(it->id == my_id)
					it->state = PlayerInfo::IN_GAME;
				else
				{
					it->state = PlayerInfo::WAITING_FOR_RESPONSE;
					it->ack = ack;
					it->timer = 5.f;
				}
			}
		}

		Net_FilterServerChanges();
	}

	if(gdzie == -1)
	{
		// poziom w g�r�
		if(dungeon_level == 0)
		{
			if(in_tutorial)
			{
				TutEvent(3);
				fallback_co = FALLBACK_CLIENT;
				fallback_t = 0.f;
				return;
			}

			// wyj�cie na powierzchni�
			ExitToMap();
			return;
		}
		else
		{
			LoadingStart(4);
			LoadingStep(txLevelUp);

			MultiInsideLocation* inside = (MultiInsideLocation*)location;
			LeaveLevel();
			--dungeon_level;
			inside->SetActiveLevel(dungeon_level);
			EnterLevel(false, false, true, -1, false);
			OnEnterLevelOrLocation();
		}
	}
	else
	{
		MultiInsideLocation* inside = (MultiInsideLocation*)location;
		
		int steps = 3;
		if(dungeon_level > inside->generated)
			steps += 2;
		else
			++steps;

		LoadingStart(steps);
		LoadingStep(txLevelDown);

		// poziom w d�
		LeaveLevel();
		++dungeon_level;

		inside->SetActiveLevel(dungeon_level);

		bool first = false;

		// czy to pierwsza wizyta?
		if(dungeon_level >= inside->generated)
		{
			if(next_seed != 0)
			{
				srand2(next_seed);
				next_seed = 0;
				next_seed_extra = false;
			}
			else if(force_seed != 0 && force_seed_all)
				srand2(force_seed);

			inside->generated = dungeon_level+1;
			inside->infos[dungeon_level].seed = rand_r2();

			LOG(Format("Generating location '%s', seed %u.", location->name.c_str(), rand_r2()));
			LOG(Format("Generating dungeon, level %d, target %d.", dungeon_level+1, inside->cel));

			LoadingStep(txGeneratingMap);
			GenerateDungeon(*location);
			first = true;
		}

		EnterLevel(first, false, false, -1, false);
		OnEnterLevelOrLocation();
	}

	local_ctx_valid = true;
	location->last_visit = worldtime;
	CheckIfLocationCleared();
	LoadingStep(txLoadingComplete);

	if(IsOnline() && players > 1)
	{
		net_mode = NM_SERVER_SEND;
		net_state = 0;
		PrepareLevelData(net_stream);
		LOG(Format("Generated location packet: %d.", net_stream.GetNumberOfBytesUsed()));
		info_box->Show(txWaitingForPlayers);
	}
	else
	{
		clear_color = clear_color2;
		game_state = GS_LEVEL;
		load_screen->visible = false;
		main_menu->visible = false;
		game_gui_container->visible = true;
		game_messages->visible = true;
	}

#ifdef IS_DEV
	ValidateTeamItems();
#endif

	LOG(Format("Randomness integrity: %d", rand2_tmp()));
}

void Game::AddPlayerTeam(const VEC3& pos, float rot, bool reenter, bool hide_weapon)
{
	for(vector<Unit*>::iterator it = team.begin(), end = team.end(); it != end; ++it)
	{
		Unit& u = **it;

		if(!reenter)
		{
			local_ctx.units->push_back(&u);

			btCapsuleShape* caps = new btCapsuleShape(u.GetUnitRadius(), max(MIN_H, u.GetUnitHeight()));
			u.cobj = new btCollisionObject;
			u.cobj->setCollisionShape(caps);
			u.cobj->setUserPointer(&u);
			u.cobj->setCollisionFlags(CG_UNIT);
			phy_world->addCollisionObject(u.cobj);

			if(u.IsHero())
				ais.push_back(u.ai);
		}

		if(hide_weapon || u.stan_broni == BRON_CHOWA)
		{
			u.stan_broni = BRON_SCHOWANA;
			u.wyjeta = B_BRAK;
			u.chowana = B_BRAK;
		}
		else if(u.stan_broni == BRON_WYJMUJE)
			u.stan_broni = BRON_WYJETA;

		u.rot = rot;
		u.animacja = u.animacja2 = ANI_STOI;
		u.ani->Play(NAMES::ani_stand, PLAY_PRIO1, 0);
		BreakAction(u);
		u.ani->SetToEnd();
		u.in_building = -1;

		if(u.IsAI())
		{
			u.ai->state = AIController::Idle;
			u.ai->idle_action = AIController::Idle_None;
			u.ai->target = NULL;
			u.ai->alert_target = NULL;
			u.ai->timer = random(2.f,5.f);
		}

		WarpNearLocation(local_ctx, u, pos, city_ctx ? 4.f : 2.f, true, 20);
		u.visual_pos = u.pos;

		if(!location->outside)
			DungeonReveal(INT2(int(u.pos.x/2), int(u.pos.z/2)));

		if(u.interp)
			u.interp->Reset(u.pos, u.rot);
	}
}

void Game::OpenDoorsByTeam(const INT2& pt)
{
	InsideLocation* inside = (InsideLocation*)location;
	InsideLocationLevel& lvl = inside->GetLevelData();
	for(vector<Unit*>::iterator it = team.begin(), end = team.end(); it != end; ++it)
	{
		INT2 unit_pt = pos_to_pt((*it)->pos);
		if(FindPath(local_ctx, unit_pt, pt, tmp_path))
		{
			for(vector<INT2>::iterator it2 = tmp_path.begin(), end2 = tmp_path.end(); it2 != end2; ++it2)
			{
				Pole& p = lvl.mapa[(*it2)(lvl.w)];
				if(p.co == DRZWI)
				{
					Door* door = lvl.FindDoor(*it2);					
					if(door && door->state == Door::Closed)
					{
						door->state = Door::Open;
						btVector3& pos = door->phy->getWorldTransform().getOrigin();
						pos.setY(pos.y() - 100.f);
						door->ani->SetToEnd(&door->ani->ani->anims[0]);
					}
				}
			}
		}
		else
			WARN(Format("OpenDoorsByTeam: Can't find path from unit %s (%d,%d) to spawn point (%d,%d).", (*it)->data->id, unit_pt.x, unit_pt.y, pt.x, pt.y));
	}
}

void Game::ExitToMap()
{
	// zamknij gui layer
	CloseAllPanels();

	clear_color = BLACK;
	game_state = GS_WORLDMAP;
	if(open_location != -1 && location->type == L_ENCOUNTER)
		LeaveLocation();
	
	if(world_state == WS_ENCOUNTER)
		world_state = WS_TRAVEL;
	else if(world_state != WS_TRAVEL)
		world_state = WS_MAIN;

	SetMusic(MUSIC_TRAVEL);

	if(IsOnline() && IsServer())
		PushNetChange(NetChange::EXIT_TO_MAP);

	show_mp_panel = true;
	world_map->visible = true;
	game_gui_container->visible = false;
}

void Game::RespawnObjectColliders(bool spawn_pes)
{
	RespawnObjectColliders(local_ctx, spawn_pes);

	if(city_ctx)
	{
		for(vector<InsideBuilding*>::iterator it = city_ctx->inside_buildings.begin(), end = city_ctx->inside_buildings.end(); it != end; ++it)
			RespawnObjectColliders((*it)->ctx, spawn_pes);
	}
}

// wbrew nazwie tworzy te� ogie� :3
void Game::RespawnObjectColliders(LevelContext& ctx, bool spawn_pes)
{
	Texture flare_tex;

	int flags = SOE_DONT_CREATE_LIGHT;
	if(!spawn_pes)
		flags |= SOE_DONT_SPAWN_PARTICLES;

	// dotyczy tylko pochodni
	if(ctx.type == LevelContext::Inside)
	{
		InsideLocation* inside = (InsideLocation*)location;
		BaseLocation& base = g_base_locations[inside->cel];
		if(IS_SET(base.options, BLO_MAGIC_LIGHT))
			flags |= SOE_MAGIC_LIGHT;
	}

	for(vector<Object>::iterator it = ctx.objects->begin(), end = ctx.objects->end(); it != end; ++it)
	{
		if(!it->base)
			continue;

		Obj* obj = it->base;

		if(IS_SET(obj->flagi, OBJ_BUDYNEK))
		{
			float rot = it->rot.y;
			int roti;
			if(equal(rot, 0))
				roti = 0;
			else if(equal(rot, PI/2))
				roti = 1;
			else if(equal(rot, PI))
				roti = 2;
			else if(equal(rot, PI*3/2))
				roti = 3;
			else
			{
				assert(0);
				roti = 0;
				rot = 0.f;
			}

			ProcessBuildingObjects(ctx, NULL, NULL, obj->ani, NULL, rot, roti, it->pos, B_NONE, NULL, true);
		}
		else
			SpawnObjectExtras(ctx, obj, it->pos, it->rot.y, &*it, (btCollisionObject**)&it->ptr, it->scale, flags);
	}

	if(ctx.chests)
	{
		Obj* chest = FindObject("chest");
		for(vector<Chest*>::iterator it = ctx.chests->begin(), end = ctx.chests->end(); it != end; ++it)
			SpawnObjectExtras(ctx, chest, (*it)->pos, (*it)->rot, NULL, NULL, 1.f, flags);
	}

	for(vector<Useable*>::iterator it = ctx.useables->begin(), end = ctx.useables->end(); it != end; ++it)
		SpawnObjectExtras(ctx, g_base_usables[(*it)->type].obj, (*it)->pos, (*it)->rot, *it, NULL, 1.f, flags);
}

void Game::SetRoomPointers()
{
	for(uint i=0; i<n_base_locations; ++i)
	{
		BaseLocation& base = g_base_locations[i];

		if(base.schody.id)
			base.schody.room = FindRoomType(base.schody.id);

		if(base.wymagany.id)
			base.wymagany.room = FindRoomType(base.wymagany.id);

		if(base.rooms)
		{
			base.room_total = 0;
			for(int j=0; j<base.room_count; ++j)
			{
				base.rooms[j].room = FindRoomType(base.rooms[j].id);
				base.room_total += base.rooms[j].chance;
			}
		}
	}
}

SOUND Game::GetMaterialSound(MATERIAL_TYPE atakuje, MATERIAL_TYPE trafiony)
{
	switch(trafiony)
	{
	case MAT_BODY:
		return sBody[rand2()%5];
	case MAT_BONE:
		return sBone;
	case MAT_CLOTH:
	case MAT_SKIN:
		return sSkin;
	case MAT_IRON:
		return sMetal;
	case MAT_WOOD:
		return sWood;
	case MAT_ROCK:
		return sRock;
	case MAT_CRYSTAL:
		return sCrystal;
	default:
		assert(0);
		return NULL;
	}
}

void Game::PlayAttachedSound(Unit& unit, SOUND sound, float smin, float smax)
{
	assert(sound && sound_volume > 0);

	VEC3 pos = unit.GetHeadSoundPos();

	if(&unit == pc->unit)
		PlaySound2d(sound);
	else
	{
		FMOD::Channel* channel;
		fmod_system->playSound(FMOD_CHANNEL_FREE, sound, true, &channel);
		channel->set3DAttributes((const FMOD_VECTOR*)&pos, NULL);
		channel->set3DMinMaxDistance(smin, 10000.f/*smax*/);
		channel->setPaused(false);

		AttachedSound& s = Add1(attached_sounds);
		s.channel = channel;
		s.unit = &unit;
	}
}

float Game::GetLevelDiff(Unit& player, Unit& enemy)
{
	return float(enemy.level) / player.level;
}

Game::ATTACK_RESULT Game::DoGenericAttack(LevelContext& ctx, Unit& attacker, Unit& hitted, const VEC3& hitpoint, float base_dmg, int dmg_type, Skill weapon_skill)
{
	int mod = ObliczModyfikator(dmg_type, hitted.data->flagi);
	float m = 1.f;
	if(mod == -1)
		m += 0.25f;
	else if(mod == 1)
		m -= 0.25f;
	if(hitted.IsNotFighting())
		m += 0.1f; // 10% do dmg je�li ma schowan� bro�
	else if(hitted.IsHoldingBow())
		m += 0.05f;
	if(hitted.action == A_PAIN)
		m += 0.1f;

	// premia za atak w plecy
	float kat = angle_dif(clip(attacker.rot+PI), hitted.rot);
	float backstab_mod = 0.25f;
	if(IS_SET(attacker.data->flagi, F2_CIOS_W_PLECY))
		backstab_mod += 0.25f;
	if(attacker.HaveWeapon() && IS_SET(attacker.GetWeapon().flags, ITEM_BACKSTAB))
		backstab_mod += 0.25f;
	if(IS_SET(hitted.data->flagi2, F2_ODPORNOSC_NA_CIOS_W_PLECY))
		backstab_mod /= 2;
	m += kat/PI*backstab_mod;

	// uwzgl�dnij modyfikatory
	float dmg = base_dmg * m;

	// oblicz obron�
	float armor_def = hitted.CalculateArmorDefense(),
		  dex_def = hitted.CalculateDexterityDefense(),
		  base_def = hitted.CalculateBaseDefense();

	// blokowanie tarcz�
	if(hitted.action == A_BLOCK && kat < PI/2)
	{
// 		int block_skill =  int(hitted.ani->groups[1].GetBlendT() * hitted.skill[S_SHIELD] + hitted.CalculateDexterity()/4)
// 			- attacker.skill[S_WEAPON] - int(attacker.CalculateDexterity()/4);// + random(-10,10);
// 		if(kat > PI/4)
// 			block_skill -= 10;
// 		
// 		int block_value;
// 		
// 		if(block_skill > 10)
// 		{
// 			// dobry blok
// 			block_value = 6;
// 		}
// 		else if(block_skill > -10)
// 		{
// 			// �redni blok
// 			block_value = 4;
// 		}
// 		else
// 		{
// 			// zepsuty blok
// 			block_value = 2;
// 		}

		int block_value = 3;
		MATERIAL_TYPE hitted_mat;
		if(IS_SET(attacker.data->flagi2, F2_OMIJA_BLOK))
			--block_value;
		if(attacker.attack_power >= 1.9f)
			--block_value;

		// odejmij dmg
		float blocked;
		//if(hitted.HaveShield())
		//{
			blocked = hitted.CalculateBlock(&hitted.GetShield());
			//if(hitted.HaveWeapon())
			//	blocked += hitted.CalculateWeaponBlock()/5;
			hitted_mat = hitted.GetShield().material;
		//}
// 		else
// 		{
// 			blocked = hitted.CalculateWeaponBlock();
// 			hitted_mat = hitted.GetWeapon().material;
// 		}

		blocked *= block_value;

		//LOG(Format("Blocked %g", blocked));
		dmg -= blocked;

		// d�wi�k bloku
		MATERIAL_TYPE weapon_mat = (weapon_skill == Skill::ONE_HANDED_WEAPON ? attacker.GetWeaponMaterial() : attacker.GetShield().material);
		if(IsServer())
		{
			NetChange& c = Add1(net_changes);
			c.type = NetChange::HIT_SOUND;
			c.pos = hitpoint;
			c.id = weapon_mat;
			c.ile = hitted_mat;
		}
		if(sound_volume)
			PlaySound3d(GetMaterialSound(weapon_mat, hitted_mat), hitpoint, 2.f, 10.f);

		if(hitted.IsPlayer())
		{
			// gracz zablokowa� cios tarcz�, szkol go w tarczy i sile, zr�czno�ci
			hitted.player->Train2(Train_Block, C_TRAIN_BLOCK, attacker.GetLevel(Train_Hit));
		}

		/*if((dmg > hitted.hpmax/4 || attacker.attack_power > 1.9f) && !IS_SET(hitted.data->flagi, F_NIE_CIERPI))
		{
			BreakAction(hitted);

			if(hitted.action != A_POSITION)
				hitted.action = A_PAIN;
			else
				hitted.etap_animacji = 1;

			if(hitted.ani->ani->head.n_groups == 2)
			{
				hitted.ani->frame_end_info2 = false;
				hitted.ani->Play(hitted.HaveShield() ? NAMES::ani_hurt : (rand2()%2 == 0 ? "atak1_p" : "atak2_p"), PLAY_PRIO1|PLAY_ONCE, 1);
			}
			else
			{
				hitted.ani->frame_end_info = false;
				hitted.ani->Play(hitted.HaveShield() ? NAMES::ani_hurt : (rand2()%2 == 0 ? "atak1_p" : "atak2_p"), PLAY_PRIO3|PLAY_ONCE, 0);
				hitted.animacja = ANI_ODTWORZ;
			}
		}*/

		if(attacker.attack_power >= 1.9f && !IS_SET(hitted.data->flagi, F_NIE_CIERPI))
		{
			BreakAction(hitted);

			if(hitted.action != A_POSITION)
				hitted.action = A_PAIN;
			else
				hitted.etap_animacji = 1;

			if(hitted.ani->ani->head.n_groups == 2)
			{
				hitted.ani->frame_end_info2 = false;
				hitted.ani->Play(NAMES::ani_hurt, PLAY_PRIO1|PLAY_ONCE, 1);
			}
			else
			{
				hitted.ani->frame_end_info = false;
				hitted.ani->Play(NAMES::ani_hurt, PLAY_PRIO3|PLAY_ONCE, 0);
				hitted.animacja = ANI_ODTWORZ;
			}
		}

		if(dmg < 0)
		{
			// dzi�ki tarczy zablokowano
			if(attacker.IsPlayer())
			{
				// wr�g zablokowa� cios gracza, trenuj walk� broni�, si�� i zr�czno��
				attacker.player->Train2(weapon_skill == Skill::ONE_HANDED_WEAPON ? Train_Hit : Train_Bash, C_TRAIN_NO_DMG, hitted.GetLevel(Train_Hurt), 0.f);
				// wkurw na atakuj�cego
				AttackReaction(hitted, attacker);
			}
			return ATTACK_BLOCKED;
		}
	}
// 	else if(hitted.action == A_PAROWANIE && kat < PI/4)
// 	{
// 		// d�wi�k
// 		if(sound_volume)
// 			PlaySound3d(GetMaterialSound(attacker.GetWeaponMaterial(), hitted.GetWeaponMaterial()), hitpoint, 2.f);
// 
// 		hitted.action = A_ATTACK;
// 		hitted.attack_id = hitted.GetRandomAttack();
// 		hitted.ani->Play(NAMES::ani_attacks[hitted.attack_id], PLAY_PRIO1|PLAY_ONCE|PLAY_BLEND_WAIT|PLAY_BLEND_WAIT, 1);
// 		hitted.ani->groups[1].speed = hitted.GetAttackSpeed()*2.5f;
// 		hitted.ani->groups[1].blend_max = 0.1f;
// 		hitted.etap_animacji = 1;
// 		hitted.atak_w_biegu = false;
// 		hitted.trafil = false;
// 		hitted.attack_power = 0.75f;
// 
// 		/*if(IsOnline())
// 		{
// 			NetChange& c = Add1(net_changes);
// 			c.type = NetChange::ATTACK;
// 			c.unit = pc->unit;
// 			c.id = 1;
// 		}*/
// 
// 		return ATTACK_PARRIED;
// 	}
	else if(hitted.HaveShield() && hitted.action != A_PAIN)
	{
		// premia do obrony z tarczy
		base_def += hitted.CalculateBlock(&hitted.GetShield()) / 5;
		//LOG(Format("Shield bonus: %g", hitted.CalculateBlock(&hitted.GetShield()) / 5));
	}

	bool clean_hit = false;

	if(hitted.action == A_PAIN)
	{
		dex_def = 0.f;
		if(hitted.HaveArmor())
		{
			const Armor& a = hitted.GetArmor();
			if(a.IsHeavy())
				armor_def *= 0.5f;
			else
				armor_def *= 0.25f;
		}
		clean_hit = true;
	}

	// odporno�� pancerza
	//LOG(Format("Armor: %g, Dex: %g, Base: %g", armor_def, dex_def, base_def));
	dmg -= (armor_def + dex_def + base_def);

	// d�wi�k trafienia
	PlayHitSound(weapon_skill == Skill::ONE_HANDED_WEAPON ? attacker.GetWeaponMaterial() : attacker.GetShield().material, hitted.GetBodyMaterial(), hitpoint, 2.f, dmg>0.f);

	// szkolenie w pancerzu
	if(hitted.IsPlayer())
		hitted.player->Train2(Train_Hurt, C_TRAIN_ARMOR_HURT * base_dmg / hitted.hpmax, attacker.GetLevel(Train_Hit));

	if(dmg < 0)
	{
		if(attacker.IsPlayer())
		{
			// wr�g zaabsorbowa� cios gracza, trenuj
			attacker.player->Train2(weapon_skill == Skill::ONE_HANDED_WEAPON ? Train_Hit : Train_Bash, C_TRAIN_NO_DMG, hitted.GetLevel(Train_Hurt));
			// wkurw na atakuj�cego
			AttackReaction(hitted, attacker);
		}
		return ATTACK_HIT;
	}

	if(attacker.IsPlayer())
	{
		// gracz trafi� kogo� (cios zabijaj�cy szkoli minimum 10% zadanych obra�e�, inaczej dmg/hpmax)
		float dmgf = (float)dmg;
		float ratio;
		if(hitted.hp - dmgf <= 0.f && !hitted.IsImmortal())
			ratio = max(C_TRAIN_KILL_RATIO, dmgf/hitted.hpmax);
		else
			ratio = dmgf/hitted.hpmax;
		attacker.player->Train2(weapon_skill == Skill::ONE_HANDED_WEAPON ? Train_Hit : Train_Bash, ratio*C_TRAIN_GIVE_DMG, hitted.GetLevel(Train_Hurt));
	}

	GiveDmg(ctx, &attacker, dmg, hitted, &hitpoint, 0, true);

	// obra�enia od trucizny
	if(IS_SET(attacker.data->flagi, F_TRUJACY_ATAK) && !IS_SET(hitted.data->flagi, F_ODPORNOSC_NA_TRUCIZNY))
	{
		Effect& e = Add1(hitted.effects);
		e.power = dmg/10;
		e.time = 5.f;
		e.effect = E_POISON;
	}

	return clean_hit ? ATTACK_CLEAN_HIT : ATTACK_HIT;
}

void Game::GenerateLabirynthUnits()
{
	// ustal jakie jednostki mo�na tu wygenerowa�
	cstring group_id;
	int count, tries;
	if(location->spawn == SG_UNK)
	{
		group_id = "unk";
		count = 30;
		tries = 150;
	}
	else
	{
		group_id = "labirynth";
		count = 20;
		tries = 100;
	}
	EnemyGroup& group = FindEnemyGroup(group_id);
	static vector<EnemyEntry> enemies;

	int level = GetDungeonLevel();
	int total = 0;

	for(int i=0; i<group.count; ++i)
	{
		if(group.enemies[i].unit->level.x <= level)
		{
			enemies.push_back(group.enemies[i]);
			total += group.enemies[i].count;
		}
	}

	// generuj jednostki
	InsideLocationLevel& lvl = ((InsideLocation*)location)->GetLevelData();
	for(int added=0; added<count && tries; --tries)
	{
		INT2 pt(random(1,lvl.w-2), random(1,lvl.h-2));
		if(czy_blokuje21(lvl.mapa[pt.x+pt.y*lvl.w]))
			continue;
		if(distance(pt, lvl.schody_gora) < 5)
			continue;

		// co wygenerowa�
		int x = rand2()%total,
			y = 0;

		for(int i=0; i<int(enemies.size()); ++i)
		{
			y += enemies[i].count;

			if(x < y)
			{
				// dodaj
				if(SpawnUnitNearLocation(local_ctx, VEC3(2.f*pt.x+1.f, 0, 2.f*pt.y+1.f), *enemies[i].unit, NULL, random(level/2, level)))
					++added;
				break;
			}
		}
	}

	// wrogowie w skarbcu
	if(location->spawn == SG_UNK)
	{
		for(int i=0; i<3; ++i)
			SpawnUnitInsideRoom(lvl.pokoje[0], *enemies[0].unit, random(level/2, level));
	}

	// posprz�taj
	enemies.clear();
}

void Game::GenerateCave(Location& l)
{
	CaveLocation* cave = (CaveLocation*)&l;
	InsideLocationLevel& lvl = cave->GetLevelData();

	generate_cave(lvl.mapa, 52, lvl.schody_gora, lvl.schody_gora_dir, cave->holes, &cave->ext);
	
	lvl.w = lvl.h = 52;
}

void Game::GenerateCaveObjects()
{
	CaveLocation* cave = (CaveLocation*)location;
	InsideLocationLevel& lvl = cave->GetLevelData();

	// �wiat�a
	for(vector<INT2>::iterator it = cave->holes.begin(), end = cave->holes.end(); it != end; ++it)
	{
		Light& s = Add1(lvl.lights);
		s.pos = VEC3(2.f*it->x+1.f, 3.f, 2.f*it->y+1.f);
		s.range = 5;
		s.color = VEC3(1.f,1.0f,1.0f);
	}

	// stalaktyty
	Obj* obj = FindObject("stalactite");
	static vector<INT2> sta;
	for(int count=0, tries=200; count<50 && tries>0; --tries)
	{
		INT2 pt = cave->GetRandomTile();
		if(lvl.mapa[pt.x+pt.y*lvl.w].co != PUSTE)
			continue;

		bool ok = true;
		for(vector<INT2>::iterator it = sta.begin(), end = sta.end(); it != end; ++it)
		{
			if(pt == *it)
			{
				ok = false;
				break;
			}
		}

		if(ok)
		{
			++count;

			Object& o = Add1(local_ctx.objects);
			o.base = obj;
			o.mesh = obj->ani;
			o.scale = random(1.f,2.f);
			o.rot = VEC3(0,random(MAX_ANGLE),0);
			o.pos = VEC3(2.f*pt.x+1.f, 4.f, 2.f*pt.y+1.f);

			sta.push_back(pt);
		}
	}

	// krzaki
	obj = FindObject("plant2");
	for(int i=0; i<150; ++i)
	{
		INT2 pt = cave->GetRandomTile();

		if(lvl.mapa[pt.x+pt.y*lvl.w].co == PUSTE)
		{
			Object& o = Add1(local_ctx.objects);
			o.base = obj;
			o.mesh = obj->ani;
			o.scale = 1.f;
			o.rot = VEC3(0,random(MAX_ANGLE),0);
			o.pos = VEC3(2.f*pt.x+random(0.1f,1.9f), 0.f, 2.f*pt.y+random(0.1f,1.9f));
		}
	}

	// grzyby
	obj = FindObject("mushrooms");
	for(int i=0; i<100; ++i)
	{
		INT2 pt = cave->GetRandomTile();

		if(lvl.mapa[pt.x+pt.y*lvl.w].co == PUSTE)
		{
			Object& o = Add1(local_ctx.objects);
			o.base = obj;
			o.mesh = obj->ani;
			o.scale = 1.f;
			o.rot = VEC3(0,random(MAX_ANGLE),0);
			o.pos = VEC3(2.f*pt.x+random(0.1f,1.9f), 0.f, 2.f*pt.y+random(0.1f,1.9f));
		}
	}

	// kamienie
	obj = FindObject("rock");
	sta.clear();
	for(int i=0; i<80; ++i)
	{
		INT2 pt = cave->GetRandomTile();

		if(lvl.mapa[pt.x+pt.y*lvl.w].co == PUSTE)
		{
			bool ok = true;

			for(vector<INT2>::iterator it = sta.begin(), end = sta.end(); it != end; ++it)
			{
				if(*it == pt)
				{
					ok = false;
					break;
				}
			}

			if(ok)
			{
				Object& o = Add1(local_ctx.objects);
				o.base = obj;
				o.mesh = obj->ani;
				o.scale = 1.f;
				o.rot = VEC3(0,random(MAX_ANGLE),0);
				o.pos = VEC3(2.f*pt.x+random(0.1f,1.9f), 0.f, 2.f*pt.y+random(0.1f,1.9f));

				if(obj->shape)
				{
					CollisionObject& c = Add1(local_ctx.colliders);

					btCollisionObject* cobj = new btCollisionObject;
					cobj->setCollisionShape(obj->shape);

					if(obj->type == OBJ_CYLINDER)
					{
						cobj->getWorldTransform().setOrigin(btVector3(o.pos.x, o.pos.y+obj->h/2, o.pos.z));
						c.type = CollisionObject::SPHERE;
						c.pt = VEC2(o.pos.x, o.pos.z);
						c.radius = obj->r;
					}
					else
					{
						btTransform& tr = cobj->getWorldTransform();
						VEC3 zero(0,0,0), pos2;
						D3DXVec3TransformCoord(&pos2, &zero, obj->matrix);
						pos2 += o.pos;
						//VEC3 pos2 = o.pos;
						tr.setOrigin(ToVector3(pos2));
						tr.setRotation(btQuaternion(o.rot.y, 0, 0));
						//tr.setBasis(btMatrix3x3(obj->matrix->_11, obj->matrix->_12, obj->matrix->_13, obj->matrix->_21, obj->matrix->_22, obj->matrix->_23, obj->matrix->_31, obj->matrix->_32, obj->matrix->_33));

						c.pt = VEC2(pos2.x, pos2.z);
						c.w = obj->size.x;
						c.h = obj->size.y;
						if(not_zero(o.rot.y))
						{
							c.type = CollisionObject::RECTANGLE_ROT;
							c.rot = o.rot.y;
							c.radius = max(c.w, c.h) * SQRT_2;
						}
						else
							c.type = CollisionObject::RECTANGLE;
					}
					
					cobj->setCollisionFlags(CG_WALL);
					phy_world->addCollisionObject(cobj);
				}

				sta.push_back(pt);
			}
		}
	}
	sta.clear();
}

struct TGroup
{
	vector<EnemyEntry*> enemies;
	int total;
};

void Game::GenerateCaveUnits()
{
	// zbierz grupy
	EnemyGroup* groups[3];
	groups[0] = &FindEnemyGroup("cave_wolfs");
	groups[1] = &FindEnemyGroup("cave_spiders");
	groups[2] = &FindEnemyGroup("cave_rats");

	static vector<INT2> tiles;
	CaveLocation* cave = (CaveLocation*)location;
	InsideLocationLevel& lvl = cave->GetLevelData();
	int level = GetDungeonLevel();
	static TGroup ee[3];
	tiles.push_back(lvl.schody_gora);

	// ustal wrog�w
	for(int i=0; i<3; ++i)
	{
		ee[i].total = 0;
		for(int j=0; j<groups[i]->count; ++j)
		{
			if(groups[i]->enemies[j].unit->level.x <= level)
			{
				ee[i].total += groups[i]->enemies[j].count;
				ee[i].enemies.push_back(&groups[i]->enemies[j]);
			}
		}
	}

	for(int added=0, tries=50; added<8 && tries>0; --tries)
	{
		INT2 pt = cave->GetRandomTile();
		if(lvl.mapa[pt.x+pt.y*lvl.w].co != PUSTE)
			continue;

		bool ok = true;
		for(vector<INT2>::iterator it = tiles.begin(), end = tiles.end(); it != end; ++it)
		{
			if(distance(pt, *it) < 10)
			{
				ok = false;
				break;
			}
		}

		if(ok)
		{
			// losuj grupe
			TGroup& group = ee[rand2()%3];
			if(group.enemies.empty())
				continue;

			tiles.push_back(pt);
			++added;

			// postaw jednostki
			int levels = level * 2;
			while(levels > 0)
			{
				int k = rand2()%group.total, l = 0;
				UnitData* ud;

				DEBUG_DO(ud = NULL);

				for(vector<EnemyEntry*>::iterator it = group.enemies.begin(), end = group.enemies.end(); it != end; ++it)
				{
					l += (*it)->count;
					if(k < l)
					{
						ud = (*it)->unit;
						break;
					}
				}

				assert(ud);

				if(ud->level.x > levels)
					break;

				int enemy_level = random2(ud->level.x, min3(ud->level.y, levels, level));
				if(!SpawnUnitNearLocation(local_ctx, VEC3(2.f*pt.x+1.f, 0, 2.f*pt.y+1.f), *ud, NULL, enemy_level, 3.f))
					break;
				levels -= enemy_level;
			}
		}
	}

	tiles.clear();
	for(int i=0; i<3; ++i)
		ee[i].enemies.clear();
}

void Game::CastSpell(LevelContext& ctx, Unit& u)
{
	Spell& spell = *u.data->spells->spell[u.attack_id];

	Animesh::Point* point = u.ani->ani->GetPoint(NAMES::point_cast);
	assert(point);

	if(u.human_data)
		u.ani->SetupBones(&u.human_data->mat_scale[0]);
	else
		u.ani->SetupBones();

	D3DXMatrixTranslation(&m1, u.pos);
	D3DXMatrixRotationY(&m2, u.rot);
	D3DXMatrixMultiply(&m1, &m2, &m1);
	m2 = point->mat * u.ani->mat_bones[point->bone] * m1;

	VEC3 coord;
	D3DXVec3TransformCoord(&coord, &VEC3(0,0,0), &m2);

	if(spell.type == Spell::Ball || spell.type == Spell::Point)
	{
		int ile = 1;
		if(IS_SET(spell.flags, Spell::Triple))
			ile = 3;

		for(int i=0; i<ile; ++i)
		{
			Bullet& b = Add1(ctx.bullets);

			b.level = (float)(u.level+u.CalculateMagicPower());
			b.pos = coord;
			b.attack = float(spell.dmg);
			b.rot = VEC3(0, clip(u.rot+PI+random(-0.05f,0.05f)), 0);
			b.mesh = spell.mesh;
			b.tex = spell.tex;
			b.tex_size = spell.size;
			b.speed = spell.speed;
			b.timer = spell.range/(spell.speed-1);
			b.owner = &u;
			b.remove = false;
			b.trail = NULL;
			b.trail2 = NULL;
			b.pe = NULL;
			b.spell = &spell;
			b.start_pos = b.pos;

			// ustal z jak� si�� rzuci� kul�
			if(spell.type == Spell::Ball)
			{
				float dist = distance2d(u.pos, u.target_pos);
				float t = dist / spell.speed;
				float h = (u.target_pos.y + random(-0.5f, 0.5f)) - b.pos.y;
				b.yspeed = h/t + (10.f*t)/2;
			}
			else if(spell.type == Spell::Point)
			{
				float dist = distance2d(u.pos, u.target_pos);
				float t = dist / spell.speed;
				float h = (u.target_pos.y + random(-0.5f, 0.5f)) - b.pos.y;
				b.yspeed = h/t;
			}

			if(spell.tex2)
			{
				ParticleEmitter* pe = new ParticleEmitter;
				pe->tex = spell.tex2;
				pe->emision_interval = 0.1f;
				pe->life = -1;
				pe->particle_life = 0.5f;
				pe->emisions = -1;
				pe->spawn_min = 3;
				pe->spawn_max = 4;
				pe->max_particles = 50;
				pe->pos = b.pos;
				pe->speed_min = VEC3(-1,-1,-1);
				pe->speed_max = VEC3(1,1,1);
				pe->pos_min = VEC3(-spell.size, -spell.size, -spell.size);
				pe->pos_max = VEC3(spell.size, spell.size, spell.size);
				pe->size = spell.size2;
				pe->op_size = POP_LINEAR_SHRINK;
				pe->alpha = 1.f;
				pe->op_alpha = POP_LINEAR_SHRINK;
				pe->mode = 1;
				pe->Init();
				ctx.pes->push_back(pe);
				b.pe = pe;
			}

			if(IsOnline())
			{
				NetChange& c = Add1(net_changes);
				c.type = NetChange::CREATE_SPELL_BALL;
				c.unit = &u;
				c.pos = b.start_pos;
				c.f[0] = b.rot.y;
				c.f[1] = b.yspeed;
				c.i = spell.id;
			}
		}
	}
	else
	{
		if(IS_SET(spell.flags, Spell::Jump))
		{
			Electro* e = new Electro;
			e->hitted.push_back(&u);
			e->dmg = float(spell.dmg + spell.dmg_premia * (u.CalculateMagicPower() + u.level));
			e->owner = &u;
			e->spell = &spell;
			e->start_pos = u.pos;

			VEC3 hitpoint;
			Unit* hitted;

			u.target_pos.y += random(-0.5f,0.5f);
			VEC3 dir = u.target_pos - coord;
			D3DXVec3Normalize(&dir, &dir);
			VEC3 target = coord+dir*spell.range;

			if(RayTest(coord, target, &u, hitpoint, hitted))
			{
				if(hitted)
				{
					// trafiono w cel
					e->valid = true;
					e->hitsome = true;
					e->hitted.push_back(hitted);
					e->AddLine(coord, hitpoint);
				}
				else
				{
					// trafienie w obiekt
					e->valid = false;
					e->hitsome = true;
					e->AddLine(coord, hitpoint);
				}
			}
			else
			{
				// w nic nie trafiono
				e->valid = false;
				e->hitsome = false;
				e->AddLine(coord, target);
			}

			ctx.electros->push_back(e);

			if(IsOnline())
			{
				e->netid = electro_netid_counter++;

				NetChange& c = Add1(net_changes);
				c.type = NetChange::CREATE_ELECTRO;
				c.e_id = e->netid;
				c.pos = e->lines[0].pts.front();
				memcpy(c.f, &e->lines[0].pts.back(), sizeof(VEC3));
			}
		}
		else if(IS_SET(spell.flags, Spell::Drain))
		{
			VEC3 hitpoint;
			Unit* hitted;

			u.target_pos.y += random(-0.5f,0.5f);

			if(RayTest(coord, u.target_pos, &u, hitpoint, hitted) && hitted)
			{
				// trafiono w cel
				if(!IS_SET(hitted->data->flagi2, F2_BRAK_KRWII) && !IsFriend(u, *hitted))
				{
					Drain& drain = Add1(ctx.drains);
					drain.from = hitted;
					drain.to = &u;

					GiveDmg(ctx, &u, float(spell.dmg+(u.CalculateMagicPower()+u.level)*spell.dmg_premia), *hitted, NULL, DMG_MAGICAL, true);

					drain.pe = ctx.pes->back();
					drain.t = 0.f;
					drain.pe->manual_delete = 1;
					drain.pe->speed_min = VEC3(-3,0,-3);
					drain.pe->speed_max = VEC3(3,3,3);

					u.hp += float(spell.dmg);
					if(u.hp > u.hpmax)
						u.hp = u.hpmax;

					if(IsOnline())
					{
						NetChange& c = Add1(net_changes);
						c.type = NetChange::UPDATE_HP;
						c.unit = drain.to;

						NetChange& c2 = Add1(net_changes);
						c2.type = NetChange::CREATE_DRAIN;
						c2.unit = drain.to;
					}
				}
			}
		}
		else if(IS_SET(spell.flags, Spell::Raise))
		{
			for(vector<Unit*>::iterator it = ctx.units->begin(), end = ctx.units->end(); it != end; ++it)
			{
				if((*it)->live_state == Unit::DEAD && !IsEnemy(u, **it) && IS_SET((*it)->data->flagi, F_NIEUMARLY) && distance(u.target_pos, (*it)->pos) < 0.5f)
				{
					Unit& u2 = **it;
					u2.hp = u2.hpmax;
					u2.live_state = Unit::ALIVE;
					u2.stan_broni = BRON_SCHOWANA;
					u2.animacja = ANI_STOI;
					u2.etap_animacji = 0;
					u2.wyjeta = B_BRAK;
					u2.chowana = B_BRAK;

					// za�� przedmioty / dodaj z�oto
					ReequipItemsMP(u2);

					// przenie� fizyke
					WarpUnit(u2, u2.pos);

					// resetuj ai
					u2.ai->Reset();

					// efekt cz�steczkowy
					ParticleEmitter* pe = new ParticleEmitter;
					pe->tex = spell.tex3;
					pe->emision_interval = 0.01f;
					pe->life = 0.f;
					pe->particle_life = 0.5f;
					pe->emisions = 1;
					pe->spawn_min = 16;
					pe->spawn_max = 25;
					pe->max_particles = 25;
					pe->pos = u2.pos;
					pe->pos.y += u2.GetUnitHeight()/2;
					pe->speed_min = VEC3(-1.5f,-1.5f,-1.5f);
					pe->speed_max = VEC3(1.5f,1.5f,1.5f);
					pe->pos_min = VEC3(-spell.size, -spell.size, -spell.size);
					pe->pos_max = VEC3(spell.size, spell.size, spell.size);
					pe->size = spell.size2;
					pe->op_size = POP_LINEAR_SHRINK;
					pe->alpha = 1.f;
					pe->op_alpha = POP_LINEAR_SHRINK;
					pe->mode = 1;
					pe->Init();
					ctx.pes->push_back(pe);

					if(IsOnline())
					{
						NetChange& c = Add1(net_changes);
						c.type = NetChange::RAISE_EFFECT;
						c.pos = pe->pos;

						NetChange& c2 = Add1(net_changes);
						c2.type = NetChange::STAND_UP;
						c2.unit = &u2;

						NetChange& c3 = Add1(net_changes);
						c3.type = NetChange::UPDATE_HP;
						c3.unit = &u2;
					}

					break;
				}
			}
			u.ai->state = AIController::Idle;
		}
		else if(IS_SET(spell.flags, Spell::Heal))
		{
			for(vector<Unit*>::iterator it = ctx.units->begin(), end = ctx.units->end(); it != end; ++it)
			{
				if(!IsEnemy(u, **it) && !IS_SET((*it)->data->flagi, F_NIEUMARLY) && distance(u.target_pos, (*it)->pos) < 0.5f)
				{
					Unit& u2 = **it;
					u2.hp += float(spell.dmg+spell.dmg_premia*(u.level+u.CalculateMagicPower()));
					if(u2.hp > u2.hpmax)
						u2.hp = u2.hpmax;
					
					// efekt cz�steczkowy
					ParticleEmitter* pe = new ParticleEmitter;
					pe->tex = spell.tex3;
					pe->emision_interval = 0.01f;
					pe->life = 0.f;
					pe->particle_life = 0.5f;
					pe->emisions = 1;
					pe->spawn_min = 16;
					pe->spawn_max = 25;
					pe->max_particles = 25;
					pe->pos = u2.pos;
					pe->pos.y += u2.GetUnitHeight()/2;
					pe->speed_min = VEC3(-1.5f,-1.5f,-1.5f);
					pe->speed_max = VEC3(1.5f,1.5f,1.5f);
					pe->pos_min = VEC3(-spell.size, -spell.size, -spell.size);
					pe->pos_max = VEC3(spell.size, spell.size, spell.size);
					pe->size = spell.size2;
					pe->op_size = POP_LINEAR_SHRINK;
					pe->alpha = 1.f;
					pe->op_alpha = POP_LINEAR_SHRINK;
					pe->mode = 1;
					pe->Init();
					ctx.pes->push_back(pe);

					if(IsOnline())
					{
						NetChange& c = Add1(net_changes);
						c.type = NetChange::HEAL_EFFECT;
						c.pos = pe->pos;

						NetChange& c3 = Add1(net_changes);
						c3.type = NetChange::UPDATE_HP;
						c3.unit = &u2;
					}

					break;
				}
			}
			u.ai->state = AIController::Idle;
		}
	}

	// d�wi�k
	if(spell.sound)
	{
		if(sound_volume)
			PlaySound3d(spell.sound, coord, spell.sound_dist.x, spell.sound_dist.y);
		if(IsOnline())
		{
			NetChange& c = Add1(net_changes);
			c.type = NetChange::SPELL_SOUND;
			c.pos = coord;
			c.id = spell.id;
		}
	}
}

void Game::SpellHitEffect(LevelContext& ctx, Bullet& bullet, const VEC3& pos, Unit* hitted)
{
	Spell& spell = *bullet.spell;

	// d�wi�k
	if(spell.sound2 && sound_volume)
		PlaySound3d(spell.sound2, pos, spell.sound_dist2.x, spell.sound_dist2.y);

	// cz�steczki
	if(spell.tex2 && spell.type == Spell::Ball)
	{
		ParticleEmitter* pe = new ParticleEmitter;
		pe->tex = spell.tex2;
		pe->emision_interval = 0.01f;
		pe->life = 0.f;
		pe->particle_life = 0.5f;
		pe->emisions = 1;
		pe->spawn_min = 8;
		pe->spawn_max = 12;
		pe->max_particles = 12;
		pe->pos =pos;
		pe->speed_min = VEC3(-1.5f,-1.5f,-1.5f);
		pe->speed_max = VEC3(1.5f,1.5f,1.5f);
		pe->pos_min = VEC3(-spell.size, -spell.size, -spell.size);
		pe->pos_max = VEC3(spell.size, spell.size, spell.size);
		pe->size = spell.size/2;
		pe->op_size = POP_LINEAR_SHRINK;
		pe->alpha = 1.f;
		pe->op_alpha = POP_LINEAR_SHRINK;
		pe->mode = 1;
		pe->Init();
		//pe->gravity = true;
		ctx.pes->push_back(pe);
	}

	// wybuch
	if(IsLocal() && spell.tex3 && IS_SET(spell.flags, Spell::Explode))
	{
		Explo* explo = new Explo;
		explo->dmg = (float)spell.dmg;
		if(bullet.owner)
			explo->dmg += float((bullet.owner->level + bullet.owner->CalculateMagicPower()) * spell.dmg_premia);
		explo->size = 0.f;
		explo->sizemax = spell.explode_range;
		explo->pos = pos;
		explo->tex = spell.tex3;
		explo->owner = bullet.owner;
		if(hitted)
			explo->hitted.push_back(hitted);
		ctx.explos->push_back(explo);

		if(IsOnline())
		{
			NetChange& c = Add1(net_changes);
			c.type = NetChange::CREATE_EXPLOSION;
			c.id = spell.id;
			c.pos = explo->pos;
		}
	}
}

extern vector<uint> _to_remove;

void Game::UpdateExplosions(LevelContext& ctx, float dt)
{
	uint index = 0;
	for(vector<Explo*>::iterator it = ctx.explos->begin(), end = ctx.explos->end(); it != end; ++it, ++index)
	{
		Explo& e = **it;
		bool delete_me = false;

		// powi�ksz
		e.size += e.sizemax*dt;
		if(e.size >= e.sizemax)
		{
			delete_me = true;
			e.size = e.sizemax;
			_to_remove.push_back(index);
		}

		float dmg = e.dmg * lerp(1.f, 0.1f, e.size/e.sizemax);

		if(IsLocal())
		{
			// zadaj obra�enia
			for(vector<Unit*>::iterator it2 = ctx.units->begin(), end2 = ctx.units->end(); it2 != end2; ++it2)
			{
				if(!(*it2)->IsAlive() || ((*it)->owner && IsFriend(*(*it)->owner, **it2)))
					continue;

				// sprawd� czy ju� nie zosta� trafiony
				bool jest = false;
				for(vector<Unit*>::iterator it3 = e.hitted.begin(), end3 = e.hitted.end(); it3 != end3; ++it3)
				{
					if(*it2 == *it3)
					{
						jest = true;
						break;
					}
				}

				// nie zosta� trafiony
				if(!jest)
				{
					BOX box;
					(*it2)->GetBox(box);

					if(SphereToBox(e.pos, e.size, box))
					{
						// zadaj obra�enia
						GiveDmg(ctx, e.owner, dmg, **it2, NULL, DMG_NO_BLOOD|DMG_MAGICAL);
						e.hitted.push_back(*it2);
					}
				}
			}
		}

		if(delete_me)
			delete *it;
	}

	while(!_to_remove.empty())
	{
		index = _to_remove.back();
		_to_remove.pop_back();
		if(index == ctx.explos->size()-1)
			ctx.explos->pop_back();
		else
		{
			std::iter_swap(ctx.explos->begin()+index, ctx.explos->end()-1);
			ctx.explos->pop_back();
		}
	}
}

void Game::UpdateTraps(LevelContext& ctx, float dt)
{
	const bool is_local = IsLocal();
	uint index = 0;
	for(vector<Trap*>::iterator it = ctx.traps->begin(), end = ctx.traps->end(); it != end; ++it, ++index)
	{
		Trap& trap = **it;

		if(trap.state == -1)
		{
			trap.time -= dt;
			if(trap.time <= 0.f)
				trap.state = 0;
			continue;
		}

		switch(trap.base->type)
		{
		case TRAP_SPEAR:
			if(trap.state == 0)
			{
				// sprawd� czy kto� nadepn��
				bool jest = false;
				if(is_local)
				{
					for(vector<Unit*>::iterator it2 = ctx.units->begin(), end2 = ctx.units->end(); it2 != end2; ++it2)
					{
						if((*it2)->IsStanding() && !IS_SET((*it2)->data->flagi, F_ZA_LEKKI) && CircleToCircle(trap.pos.x, trap.pos.z, trap.base->rw, (*it2)->pos.x, (*it2)->pos.z, (*it2)->GetUnitRadius()))
						{
							jest = true;
							break;
						}
					}
				}
				else if(trap.trigger)
				{
					jest = true;
					trap.trigger = false;
				}

				if(jest)
				{
					// kto� nadepn��
					if(sound_volume)
						PlaySound3d(trap.base->sound, trap.pos, 1.f, 4.f);
					trap.state = 1;
					trap.time = random(0.5f,0.75f);

					if(IsOnline() && IsServer())
					{
						NetChange& c = Add1(net_changes);
						c.type = NetChange::TRIGGER_TRAP;
						c.id = trap.netid;
					}
				}
			}
			else if(trap.state == 1)
			{
				// odliczaj czas do wyj�cia w��czni
				trap.time -= dt;
				if(trap.time <= 0.f)
				{
					trap.state = 2;
					trap.time = 0.f;

					// d�wi�k wychodzenia
					if(sound_volume)
						PlaySound3d(trap.base->sound2, trap.pos, 2.f, 8.f);
				}
			}
			else if(trap.state == 2)
			{
				// wychodzenie w��czni
				bool koniec = false;
				trap.time += dt;
				if(trap.time >= 0.27f)
				{
					trap.time = 0.27f;
					koniec = true;
				}

				trap.obj2.pos.y = trap.obj.pos.y - 2.f + 2.f*(trap.time/0.27f);

				if(is_local)
				{
					for(vector<Unit*>::iterator it2 = ctx.units->begin(), end2 = ctx.units->end(); it2 != end2; ++it2)
					{
						if(!(*it2)->IsAlive())
							continue;
						if(CircleToCircle(trap.obj2.pos.x, trap.obj2.pos.z, trap.base->rw, (*it2)->pos.x, (*it2)->pos.z, (*it2)->GetUnitRadius()))
						{
							bool jest = false;
							for(vector<Unit*>::iterator it3 = trap.hitted->begin(), end3 = trap.hitted->end(); it3 != end3; ++it3)
							{
								if(*it2 == *it3)
								{
									jest = true;
									break;
								}
							}

							if(!jest)
							{
								Unit* hitted = *it2;

								// uderzenie w��czniami
								float dmg = float(trap.base->dmg),
									def = hitted->CalculateDefense();

								int mod = ObliczModyfikator(DMG_PIERCE, hitted->data->flagi);
								float m = 1.f;
								if(mod == -1)
									m += 0.25f;
								else if(mod == 1)
									m -= 0.25f;

								// modyfikator obra�e�
								float base_dmg = dmg;
								dmg *= m;
								dmg -= def;

								// d�wi�k trafienia
								if(sound_volume)
									PlaySound3d(GetMaterialSound(MAT_IRON, hitted->GetBodyMaterial()), hitted->pos + VEC3(0,1.f,0), 2.f, 8.f);

								// szkol gracza w pancerzu
								if(hitted->IsPlayer())
									hitted->player->Train2(Train_Hurt, C_TRAIN_ARMOR_HURT * (base_dmg / hitted->hpmax), 4.f);

								// obra�enia
								if(dmg > 0)
									GiveDmg(ctx, NULL, dmg, **it2, NULL, 0, true);

								trap.hitted->push_back(hitted);
							}
						}
					}
				}

				if(koniec)
				{
					trap.state = 3;
					if(is_local)
						trap.hitted->clear();
					trap.time = 1.f;
				}
			}
			else if(trap.state == 3)
			{
				// w��cznie wysz�y, odliczaj czas do chowania
				trap.time -= dt;
				if(trap.time <= 0.f)
				{
					trap.state = 4;
					trap.time = 1.5f;
					if(sound_volume)
						PlaySound3d(trap.base->sound3, trap.pos, 1.f, 4.f);
				}
			}
			else if(trap.state == 4)
			{
				// chowanie w��czni
				trap.time -= dt;
				if(trap.time <= 0.f)
				{
					trap.time = 0.f;
					trap.state = 5;
				}

				trap.obj2.pos.y = trap.obj.pos.y - 2.f + trap.time / 1.5f * 2.f;
			}
			else if(trap.state == 5)
			{
				// w��cznie schowane, odliczaj czas do ponownej aktywacji
				trap.time -= dt;
				if(trap.time <= 0.f)
				{
					if(is_local)
					{
						// je�li kto� stoi to nie aktywuj
						bool ok = true;
						for(vector<Unit*>::iterator it2 = local_ctx.units->begin(), end2 = local_ctx.units->end(); it2 != end2; ++it2)
						{
							if(!IS_SET((*it2)->data->flagi, F_ZA_LEKKI) && CircleToCircle(trap.obj2.pos.x, trap.obj2.pos.z, trap.base->rw, (*it2)->pos.x, (*it2)->pos.z, (*it2)->GetUnitRadius()))
							{
								ok = false;
								break;
							}
						}
						if(ok)
							trap.state = 0;
					}
					else
						trap.state = 0;
				}
			}
			break;
		case TRAP_ARROW:
		case TRAP_POISON:
			if(trap.state == 0)
			{
				// sprawd� czy kto� nadepn��
				bool jest = false;
				if(is_local)
				{
					for(vector<Unit*>::iterator it2 = ctx.units->begin(), end2 = ctx.units->end(); it2 != end2; ++it2)
					{
						if((*it2)->IsStanding() && !IS_SET((*it2)->data->flagi, F_ZA_LEKKI) &&
							CircleToRectangle((*it2)->pos.x, (*it2)->pos.z, (*it2)->GetUnitRadius(), trap.pos.x, trap.pos.z, trap.base->rw, trap.base->h))
						{
							jest = true;
							break;
						}
					}
				}
				else if(trap.trigger)
				{
					jest = true;
					trap.trigger = false;
				}

				if(jest)
				{
					// kto� nadepn��, wystrzel strza��
					trap.state = 1;
					trap.time = random(5.f, 7.5f);

					if(is_local)
					{
						Bullet& b = Add1(ctx.bullets);
						b.level = 4.f;
						b.attack = float(trap.base->dmg);
						b.mesh = aArrow;
						b.pos = VEC3(2.f*trap.tile.x+trap.pos.x-float(int(trap.pos.x/2)*2)+random(-trap.base->rw, trap.base->rw)-1.2f*g_kierunek2[trap.dir].x,
							random(0.5f,1.5f),
							2.f*trap.tile.y+trap.pos.z-float(int(trap.pos.z/2)*2)+random(-trap.base->h, trap.base->h)-1.2f*g_kierunek2[trap.dir].y);
						b.rot = VEC3(0,dir_to_rot(trap.dir),0);
						b.owner = NULL;
						b.pe = NULL;
						b.remove = false;
						b.speed = ARROW_SPEED;
						b.spell = NULL;
						b.tex = NULL;
						b.tex_size = 0.f;
						b.timer = ARROW_TIMER;
						b.yspeed = 0.f;
						b.poison_attack = (trap.base->type == TRAP_POISON ? float(trap.base->dmg) : 0.f);

						TrailParticleEmitter* tpe = new TrailParticleEmitter;
						tpe->fade = 0.3f;
						tpe->color1 = VEC4(1,1,1,0.5f);
						tpe->color2 = VEC4(1,1,1,0);
						tpe->Init(50);
						ctx.tpes->push_back(tpe);
						b.trail = tpe;

						TrailParticleEmitter* tpe2 = new TrailParticleEmitter;
						tpe2->fade = 0.3f;
						tpe2->color1 = VEC4(1,1,1,0.5f);
						tpe2->color2 = VEC4(1,1,1,0);
						tpe2->Init(50);
						ctx.tpes->push_back(tpe2);
						b.trail2 = tpe2;

						if(sound_volume)
						{
							// d�wi�k nadepni�cia
							PlaySound3d(trap.base->sound, trap.pos, 1.f, 4.f);
							// d�wi�k strza�u
							PlaySound3d(sBow[rand2()%2], b.pos, 2.f, 8.f);
						}

						if(IsOnline() && IsServer())
						{
							NetChange& c = Add1(net_changes);
							c.type = NetChange::SHOT_ARROW;
							c.unit = NULL;
							c.pos = b.start_pos;
							c.f[0] = b.rot.y;
							c.f[1] = 0.f;
							c.f[2] = 0.f;

							NetChange& c2 = Add1(net_changes);
							c2.type = NetChange::TRIGGER_TRAP;
							c2.id = trap.netid;
						}
					}
				}
			}
			else if(trap.state == 1)
			{
				trap.time -= dt;
				if(trap.time <= 0.f)
					trap.state = 2;
			}
			else
			{
				// sprawd� czy zeszli
				bool jest = false;
				if(is_local)
				{
					for(vector<Unit*>::iterator it2 = ctx.units->begin(), end2 = ctx.units->end(); it2 != end2; ++it2)
					{
						if(!IS_SET((*it2)->data->flagi, F_ZA_LEKKI) && 
							CircleToRectangle((*it2)->pos.x, (*it2)->pos.z, (*it2)->GetUnitRadius(), trap.pos.x, trap.pos.z, trap.base->rw, trap.base->h))
						{
							jest = true;
							break;
						}
					}
				}
				else if(trap.trigger)
				{
					jest = true;
					trap.trigger = false;
				}

				if(!jest)
				{
					trap.state = 0;
					if(IsOnline() && IsServer())
					{
						NetChange& c = Add1(net_changes);
						c.type = NetChange::TRIGGER_TRAP;
						c.id = trap.netid;
					}
				}
			}
			break;
		case TRAP_FIREBALL:
			{
				if(!is_local)
					break;

				bool jest = false;
				for(vector<Unit*>::iterator it2 = ctx.units->begin(), end2 = ctx.units->end(); it2 != end2; ++it2)
				{
					if((*it2)->IsStanding() && CircleToRectangle((*it2)->pos.x, (*it2)->pos.z, (*it2)->GetUnitRadius(), trap.pos.x, trap.pos.z, trap.base->rw, trap.base->h))
					{
						jest = true;
						break;
					}
				}

				if(jest)
				{
					_to_remove.push_back(index);

					Spell* fireball = FindSpell("fireball");

					Explo* explo = new Explo;
					explo->pos = trap.pos;
					explo->pos.y += 0.2f;
					explo->size = 0.f;
					explo->sizemax = 2.f;
					explo->dmg = float(trap.base->dmg);
					explo->tex = fireball->tex3;
					explo->owner = NULL;

					if(sound_volume)
						PlaySound3d(fireball->sound2, explo->pos, fireball->sound_dist2.x, fireball->sound_dist2.y);

					ctx.explos->push_back(explo);

					if(IsOnline())
					{
						NetChange& c = Add1(net_changes);
						c.type = NetChange::CREATE_EXPLOSION;
						c.id = 2;
						c.pos = explo->pos;

						NetChange& c2 = Add1(net_changes);
						c2.type = NetChange::REMOVE_TRAP;
						c2.id = trap.netid;
					}

					delete *it;
				}
			}
			break;
		default:
			assert(0);
			break;
		}
	}

	while(!_to_remove.empty())
	{
		index = _to_remove.back();
		_to_remove.pop_back();
		if(index == ctx.traps->size()-1)
			ctx.traps->pop_back();
		else
		{
			std::iter_swap(ctx.traps->begin()+index, ctx.traps->end()-1);
			ctx.traps->pop_back();
		}
	}
}

struct TrapLocation
{
	INT2 pt;
	int dist, dir;
};

inline bool SortTrapLocations(TrapLocation& pt1, TrapLocation& pt2)
{
	return abs(pt1.dist-5) < abs(pt2.dist-5);
}

Trap* Game::CreateTrap(INT2 pt, TRAP_TYPE type, bool timed)
{
	Trap* t = new Trap;
	Trap& trap = *t;
	local_ctx.traps->push_back(t);

	trap.base = &g_traps[type];
	trap.hitted = NULL;
	trap.state = 0;
	trap.pos = VEC3(2.f*pt.x+random(trap.base->rw, 2.f-trap.base->rw), 0.f, 2.f*pt.y+random(trap.base->h, 2.f-trap.base->h));
	trap.obj.base = NULL;
	trap.obj.mesh = trap.base->mesh;
	trap.obj.pos = trap.pos;
	trap.obj.scale = 1.f;
	trap.netid = trap_netid_counter++;

	if(type == TRAP_ARROW || type == TRAP_POISON)
	{
		trap.obj.rot = VEC3(0,0,0);

		static vector<TrapLocation> possible;

		InsideLocationLevel& lvl = ((InsideLocation*)location)->GetLevelData();

		// ustal tile i dir
		for(int i=0; i<4; ++i)
		{
			bool ok = false;
			int j;

			for(j=1; j<=10; ++j)
			{
				if(czy_blokuje2(lvl.mapa[pt.x+g_kierunek2[i].x*j+(pt.y+g_kierunek2[i].y*j)*lvl.w]))
				{
					if(j != 1)
						ok = true;
					break;
				}
			}

			if(ok)
			{
				trap.tile = pt + g_kierunek2[i] * j;

				if(CanShootAtLocation(VEC3(trap.pos.x+(2.f*j-1.2f)*g_kierunek2[i].x, 1.f, trap.pos.z+(2.f*j-1.2f)*g_kierunek2[i].y), VEC3(trap.pos.x, 1.f, trap.pos.z)))
				{
					TrapLocation& tr = Add1(possible);
					tr.pt = trap.tile;
					tr.dist = j;
					tr.dir = i;
				}
			}
		}

		if(!possible.empty())
		{
			if(possible.size() > 1)
				std::sort(possible.begin(), possible.end(), SortTrapLocations);

			trap.tile = possible[0].pt;
			trap.dir = possible[0].dir;

			possible.clear();
		}
		else
		{
			local_ctx.traps->pop_back();
			delete t;
			return NULL;
		}
	}
	else if(type == TRAP_SPEAR)
	{
		trap.obj.rot = VEC3(0,random(MAX_ANGLE),0);
		trap.obj2.base = NULL;
		trap.obj2.mesh = trap.base->mesh2;
		trap.obj2.pos = trap.obj.pos;
		trap.obj2.rot = trap.obj.rot;
		trap.obj2.scale = 1.f;
		trap.obj2.pos.y -= 2.f;
		trap.hitted = new vector<Unit*>;
	}
	else
	{
		trap.obj.rot = VEC3(0,PI/2*(rand2()%4),0);
		trap.obj.base = &obj_alpha;
	}

	if(timed)
	{
		trap.state = -1;
		trap.time = 2.f;
	}

	return &trap;
}

struct BulletRaytestCallback3 : public btCollisionWorld::RayResultCallback
{
	BulletRaytestCallback3(Unit* ignore) : hit(false), ignore(ignore), hitted(NULL), fraction(1.01f)
	{
	}

	btScalar addSingleResult(btCollisionWorld::LocalRayResult& rayResult, bool normalInWorldSpace)
	{
		Unit* u = (Unit*)rayResult.m_collisionObject->getUserPointer();
		if(ignore && u == ignore)
			return 1.f;

		if(rayResult.m_hitFraction < fraction)
		{
			hit = true;
			hitted = u;
			fraction = rayResult.m_hitFraction;
		}

		return 0.f;
	}

	bool hit;
	Unit* ignore, *hitted;
	float fraction;
};

bool Game::RayTest(const VEC3& from, const VEC3& to, Unit* ignore, VEC3& hitpoint, Unit*& hitted)
{
	BulletRaytestCallback3 callback(ignore);
	phy_world->rayTest(ToVector3(from), ToVector3(to), callback);

	if(callback.hit)
	{
		hitpoint = from + (to-from) * callback.fraction;
		hitted = callback.hitted;
		return true;
	}
	else
		return false;
}

inline bool SortElectroTargets(const std::pair<Unit*, float>& target1, const std::pair<Unit*, float>& target2)
{
	return target1.second < target2.second;
}

void Game::UpdateElectros(LevelContext& ctx, float dt)
{
	uint index = 0;
	for(vector<Electro*>::iterator it = ctx.electros->begin(), end = ctx.electros->end(); it != end; ++it, ++index)
	{
		Electro& e = **it;

		for(vector<ElectroLine>::iterator it2 = e.lines.begin(), end2 = e.lines.end(); it2 != end2; ++it2)
			it2->t += dt;

		if(!IsLocal())
		{
			if(e.lines.back().t >= 0.5f)
			{
				_to_remove.push_back(index);
				delete *it;
			}
		}
		else if(e.valid)
		{
			if(e.lines.back().t >= 0.25f)
			{
				// zadaj obra�enia
				if(!IsFriend(*e.owner, *e.hitted.back()))
				{
					if(e.hitted.back()->IsAI())
						AI_HitReaction(*e.hitted.back(), e.start_pos);
					GiveDmg(ctx, e.owner, e.dmg, *e.hitted.back(), NULL, DMG_NO_BLOOD|DMG_MAGICAL);
				}

				if(sound_volume && e.spell->sound2)
					PlaySound3d(e.spell->sound2, e.lines.back().pts.back(), e.spell->sound_dist2.x, e.spell->sound_dist2.y);

				// cz�steczki
				if(e.spell->tex2)
				{
					ParticleEmitter* pe = new ParticleEmitter;
					pe->tex = e.spell->tex2;
					pe->emision_interval = 0.01f;
					pe->life = 0.f;
					pe->particle_life = 0.5f;
					pe->emisions = 1;
					pe->spawn_min = 8;
					pe->spawn_max = 12;
					pe->max_particles = 12;
					pe->pos = e.lines.back().pts.back();
					pe->speed_min = VEC3(-1.5f,-1.5f,-1.5f);
					pe->speed_max = VEC3(1.5f,1.5f,1.5f);
					pe->pos_min = VEC3(-e.spell->size, -e.spell->size, -e.spell->size);
					pe->pos_max = VEC3(e.spell->size, e.spell->size, e.spell->size);
					pe->size = e.spell->size2;
					pe->op_size = POP_LINEAR_SHRINK;
					pe->alpha = 1.f;
					pe->op_alpha = POP_LINEAR_SHRINK;
					pe->mode = 1;
					pe->Init();
					//pe->gravity = true;
					ctx.pes->push_back(pe);
				}

				if(IsOnline())
				{
					NetChange& c = Add1(net_changes);
					c.type = NetChange::ELECTRO_HIT;
					c.pos = e.lines.back().pts.back();
				}

				if(e.dmg >= 10.f)
				{
					static vector<std::pair<Unit*, float> > targets;

					// traf w kolejny cel
					for(vector<Unit*>::iterator it2 = ctx.units->begin(), end2 = ctx.units->end(); it2 != end2; ++it2)
					{
						if(!(*it2)->IsAlive() || IsInside(e.hitted, *it2))
							continue;

						float dist = distance((*it2)->pos, e.hitted.back()->pos);
						if(dist <= 5.f)
							targets.push_back(std::pair<Unit*, float>(*it2, dist));
					}

					if(!targets.empty())
					{
						if(targets.size() > 1)
							std::sort(targets.begin(), targets.end(), SortElectroTargets);

						Unit* target = NULL;
						float dist;

						for(vector<std::pair<Unit*, float> >::iterator it2 = targets.begin(), end2 = targets.end(); it2 != end2; ++it2)
						{
							VEC3 hitpoint;
							Unit* hitted;

							if(RayTest(e.lines.back().pts.back(), it2->first->GetCenter(), e.hitted.back(), hitpoint, hitted))
							{
								if(hitted == it2->first)
								{
									target = it2->first;
									dist = it2->second;
									break;
								}
							}
						}

						if(target)
						{
							// kolejny cel
							e.dmg = min(e.dmg/2, lerp(e.dmg, e.dmg/5, dist/5));
							e.valid = true;
							e.hitted.push_back(target);
							VEC3 from = e.lines.back().pts.back();
							VEC3 to = target->GetCenter();
							e.AddLine(from, to);

							if(IsOnline())
							{
								NetChange& c = Add1(net_changes);
								c.type = NetChange::UPDATE_ELECTRO;
								c.e_id = e.netid;
								c.pos = to;
							}
						}
						else
						{
							// brak kolejnego celu
							e.valid = false;
						}

						targets.clear();
					}
					else
						e.valid = false;
				}
				else
				{
					// trafi� ju� wystarczaj�co du�o postaci
					e.valid = false;
				}
			}
		}
		else
		{
			if(e.hitsome && e.lines.back().t >= 0.25f)
			{
				e.hitsome = false;

				if(sound_volume && e.spell->sound2)
					PlaySound3d(e.spell->sound2, e.lines.back().pts.back(), e.spell->sound_dist2.x, e.spell->sound_dist2.y);

				// cz�steczki
				if(e.spell->tex2)
				{
					ParticleEmitter* pe = new ParticleEmitter;
					pe->tex = e.spell->tex2;
					pe->emision_interval = 0.01f;
					pe->life = 0.f;
					pe->particle_life = 0.5f;
					pe->emisions = 1;
					pe->spawn_min = 8;
					pe->spawn_max = 12;
					pe->max_particles = 12;
					pe->pos = e.lines.back().pts.back();
					pe->speed_min = VEC3(-1.5f,-1.5f,-1.5f);
					pe->speed_max = VEC3(1.5f,1.5f,1.5f);
					pe->pos_min = VEC3(-e.spell->size, -e.spell->size, -e.spell->size);
					pe->pos_max = VEC3(e.spell->size, e.spell->size, e.spell->size);
					pe->size = e.spell->size2;
					pe->op_size = POP_LINEAR_SHRINK;
					pe->alpha = 1.f;
					pe->op_alpha = POP_LINEAR_SHRINK;
					pe->mode = 1;
					pe->Init();
					//pe->gravity = true;
					ctx.pes->push_back(pe);
				}

				if(IsOnline())
				{
					NetChange& c = Add1(net_changes);
					c.type = NetChange::ELECTRO_HIT;
					c.pos = e.lines.back().pts.back();
				}
			}
			if(e.lines.back().t >= 0.5f)
			{
				_to_remove.push_back(index);
				delete *it;
			}
		}
	}

	// usu�
	while(!_to_remove.empty())
	{
		index = _to_remove.back();
		_to_remove.pop_back();
		if(index == ctx.electros->size()-1)
			ctx.electros->pop_back();
		else
		{
			std::iter_swap(ctx.electros->begin()+index, ctx.electros->end()-1);
			ctx.electros->pop_back();
		}
	}
}

void Game::UpdateDrains(LevelContext& ctx, float dt)
{
	uint index = 0;
	for(vector<Drain>::iterator it = ctx.drains->begin(), end = ctx.drains->end(); it != end; ++it, ++index)
	{
		Drain& d = *it;
		d.t += dt;
		VEC3 center = d.to->GetCenter();

		if(d.pe->manual_delete == 2)
		{
			delete d.pe;
			_to_remove.push_back(index);
		}
		else
		{
			for(vector<Particle>::iterator it2 = d.pe->particles.begin(), end2 = d.pe->particles.end(); it2 != end2; ++it2)
				it2->pos = lerp(it2->pos, center, d.t/1.5f);
		}
	}

	// usu�
	while(!_to_remove.empty())
	{
		index = _to_remove.back();
		_to_remove.pop_back();
		if(index == ctx.drains->size()-1)
			ctx.drains->pop_back();
		else
		{
			std::iter_swap(ctx.drains->begin()+index, ctx.drains->end()-1);
			ctx.drains->pop_back();
		}
	}
}

void Game::UpdateAttachedSounds(float dt)
{
	uint index = 0;
	for(vector<AttachedSound>::iterator it = attached_sounds.begin(), end = attached_sounds.end(); it != end; ++it, ++index)
	{
		bool is_playing;
		if(it->channel->isPlaying(&is_playing) == FMOD_OK && is_playing)
		{
			VEC3 pos = it->unit->GetHeadSoundPos();
			it->channel->set3DAttributes((const FMOD_VECTOR*)&pos, NULL);
		}
		else
			_to_remove.push_back(index);
	}

	// usu�
	while(!_to_remove.empty())
	{
		index = _to_remove.back();
		_to_remove.pop_back();
		if(index == attached_sounds.size()-1)
			attached_sounds.pop_back();
		else
		{
			std::iter_swap(attached_sounds.begin()+index, attached_sounds.end()-1);
			attached_sounds.pop_back();
		}
	}
}



vector<Unit*> Unit::refid_table;
vector<std::pair<Unit**, int> > Unit::refid_request;
vector<ParticleEmitter*> ParticleEmitter::refid_table;
vector<TrailParticleEmitter*> TrailParticleEmitter::refid_table;
vector<Useable*> Useable::refid_table;
vector<UseableRequest> Useable::refid_request;

void Game::BuildRefidTables()
{
	// jednostki i u�ywalne
	Unit::refid_table.clear();
	Useable::refid_table.clear();
	for(vector<Location*>::iterator it = locations.begin(), end = locations.end(); it != end; ++it)
	{
		if(*it)
			(*it)->BuildRefidTable();
	}

	// cz�steczki
	ParticleEmitter::refid_table.clear();
	TrailParticleEmitter::refid_table.clear();
	if(open_location != -1)
	{
		for(vector<ParticleEmitter*>::iterator it2 = local_ctx.pes->begin(), end2 = local_ctx.pes->end(); it2 != end2; ++it2)
		{
			(*it2)->refid = (int)ParticleEmitter::refid_table.size();
			ParticleEmitter::refid_table.push_back(*it2);
		}
		for(vector<TrailParticleEmitter*>::iterator it2 = local_ctx.tpes->begin(), end2 = local_ctx.tpes->end(); it2 != end2; ++it2)
		{
			(*it2)->refid = (int)TrailParticleEmitter::refid_table.size();
			TrailParticleEmitter::refid_table.push_back(*it2);
		}
		if(city_ctx)
		{
			for(vector<InsideBuilding*>::iterator it = city_ctx->inside_buildings.begin(), end = city_ctx->inside_buildings.end(); it != end; ++it)
			{
				for(vector<ParticleEmitter*>::iterator it2 = (*it)->ctx.pes->begin(), end2 = (*it)->ctx.pes->end(); it2 != end2; ++it2)
				{
					(*it2)->refid = (int)ParticleEmitter::refid_table.size();
					ParticleEmitter::refid_table.push_back(*it2);
				}
				for(vector<TrailParticleEmitter*>::iterator it2 = (*it)->ctx.tpes->begin(), end2 = (*it)->ctx.tpes->end(); it2 != end2; ++it2)
				{
					(*it2)->refid = (int)TrailParticleEmitter::refid_table.size();
					TrailParticleEmitter::refid_table.push_back(*it2);
				}
			}
		}
	}
}

void Game::ClearGameVarsOnNewGameOrLoad()
{
	inventory_mode = I_NONE;
	dialog_context.dialog_mode = false;
	dialog_context.is_local = true;
	death_screen = 0;
	koniec_gry = false;
	selected_unit = NULL;
	selected_target = NULL;
	before_player = BP_NONE;
	first_frame = true;
	minimap_reveal.clear();
	minimap->city = NULL;
	team_shares.clear();
	team_share_id = -1;
	draw_flags = 0xFFFFFFFF;
	unit_views.clear();
	to_remove.clear();
	journal->Reset();
	arena_viewers.clear();
	debug_info = false;
	debug_info2 = false;
	dialog_enc = NULL;
	dialog_pvp = NULL;
	game_gui_container->visible = false;
	game_messages->visible = false;
	Inventory::lock_id = LOCK_NO;
	Inventory::lock_give = false;
	picked_location = -1;
	post_effects.clear();
	grayout = 0.f;
	rot_buf = 0.f;

#ifdef DRAW_LOCAL_PATH
	marked = NULL;
#endif

	// odciemnianie na pocz�tku
	fallback_co = FALLBACK_NONE;
	fallback_t = 0.f;
}

void Game::ClearGameVarsOnNewGame()
{
	ClearGameVarsOnNewGameOrLoad();

	used_cheats = false;
	cheats = false;
	cl_fog = true;
	cl_lighting = true;
	draw_particle_sphere = false;
	draw_unit_radius = false;
	draw_hitbox = false;
	noai = false;
	draw_phy = false;
	draw_col = false;
	rotY = 4.2875104f;
	cam_dist = 3.5f;
	speed = 1.f;
	dungeon_level = 0;
	quest_counter = 0;
	notes.clear();
	year = 100;
	day = 0;
	month = 0;
	worldtime = 0;
	gt_hour = 0;
	gt_minute = 0;
	gt_second = 0;
	gt_tick = 0.f;
	team_gold = 0;
	dont_wander = false;
	szansa_na_spotkanie = 0.f;
	atak_szalencow = false;
	bandyta = false;
	create_camp = 0;
	arena_fighter = NULL;
	ile_plotek_questowych = P_MAX;
	for(int i=0; i<P_MAX; ++i)
		plotka_questowa[i] = false;
	plotki.clear();
	free_recruit = true;
	first_city = true;
	unique_quests_completed = 0;
	unique_completed_show = false;
	news.clear();
	picking_item_state = 0;
	arena_tryb = Arena_Brak;
	world_state = WS_MAIN;
	open_location = -1;
	picked_location = -1;
	arena_free = true;
	DeleteElements(quests);
	DeleteElements(unaccepted_quests);
	quests_timeout.clear();
	total_kills = 0;
	world_dir = random(MAX_ANGLE);
	timed_units.clear();
	LoadGui();
	ClearGui();
	mp_box->visible = sv_online;
	drunk_anim = 0.f;
	light_angle = random(PI*2);

#ifdef _DEBUG
	cheats = true;
	noai = true;
	dont_wander = true;
#endif
}

void Game::ClearGameVarsOnLoad()
{
	ClearGameVarsOnNewGameOrLoad();
}

Quest* Game::FindQuest(int refid, bool active)
{
	for(vector<Quest*>::iterator it = quests.begin(), end = quests.end(); it != end; ++it)
	{
		if((!active || (*it)->IsActive()) && (*it)->refid == refid)
			return *it;
	}

	return NULL;
}

Quest* Game::FindQuest(int loc, int source)
{
	for(vector<Quest*>::iterator it = quests.begin(), end = quests.end(); it != end; ++it)
	{
		if((*it)->IsActive() && (*it)->start_loc == loc && (*it)->type == source)
			return *it;
	}

	return NULL;
}

Quest* Game::FindUnacceptedQuest(int loc, int source)
{
	for(vector<Quest*>::iterator it = unaccepted_quests.begin(), end = unaccepted_quests.end(); it != end; ++it)
	{
		if((*it)->start_loc == loc && (*it)->type == source)
			return *it;
	}

	return NULL;
}

Quest* Game::FindUnacceptedQuest(int refid)
{
	for(vector<Quest*>::iterator it = unaccepted_quests.begin(), end = unaccepted_quests.end(); it != end; ++it)
	{
		if((*it)->refid == refid)
			return *it;
	}

	return NULL;
}

int Game::GetRandomCityLocation(int this_city)
{
	int id = rand2()%cities;
	if(id == this_city)
	{
		++id;
		if(id == (int)cities)
			id = 0;
	}

	return id;
}

int Game::GetFreeRandomCityLocation(int this_city)
{
	int id = rand2()%cities, tries = (int)cities;
	while((id == this_city || locations[id]->active_quest) && tries>0)
	{
		++id;
		--tries;
		if(id == (int)cities)
			id = 0;
	}

	return (tries == 0 ? -1 : id);
}

int Game::GetRandomCity(int this_city)
{
	// policz ile jest miast
	int ile = 0;
	while(locations[ile]->type == L_CITY)
		++ile;

	int id = rand2()%ile;
	if(id == this_city)
	{
		++id;
		if(id == ile)
			id = 0;
	}

	return id;
}

void Game::LoadQuests(vector<Quest*>& v_quests, HANDLE file)
{
	uint count;
	ReadFile(file, &count, sizeof(count), &tmp, NULL);
	v_quests.resize(count);
	for(uint i=0; i<count; ++i)
	{
		QUEST q;
		ReadFile(file, &q, sizeof(q), &tmp, NULL);
		Quest* quest;

		if(LOAD_VERSION == V_0_2)
		{
			if(q > Q_ZLO)
				q = (QUEST)(q-1);
		}

		switch(q)
		{
		case Q_DOSTARCZ_LIST:
			quest = new Quest_DostarczList;
			break;
		case Q_DOSTARCZ_PACZKE:
			quest = new Quest_DostarczPaczke;
			break;
		case Q_ROZNIES_WIESCI:
			quest = new Quest_RozniesWiesci;
			break;
		case Q_ODZYSKAJ_PACZKE:
			quest = new Quest_OdzyskajPaczke;
			break;
		case Q_URATUJ_PORWANA_OSOBE:
			quest = new Quest_UratujPorwanaOsobe;
			break;
		case Q_BANDYCI_POBIERAJA_OPLATE:
			quest = new Quest_BandyciPobierajaOplate;
			break;
		case Q_OBOZ_KOLO_MIASTA:
			quest = new Quest_ObozKoloMiasta;
			break;
		case Q_ZABIJ_ZWIERZETA:
			quest = new Quest_ZabijZwierzeta;
			break;
		case Q_PRZYNIES_ARTEFAKT:
			quest = new Quest_ZnajdzArtefakt;
			break;
		case Q_ZGUBIONY_PRZEDMIOT:
			quest = new Quest_ZgubionyPrzedmiot;
			break;
		case Q_UKRADZIONY_PRZEDMIOT:
			quest = new Quest_UkradzionyPrzedmiot;
			break;
		case Q_TARTAK:
			quest = new Quest_Tartak;
			break;
		case Q_KOPALNIA:
			quest = new Quest_Kopalnia;
			break;
		case Q_BANDYCI:
			quest = new Quest_Bandyci;
			break;
		case Q_MAGOWIE:
			quest = new Quest_Magowie;
			break;
		case Q_MAGOWIE2:
			quest = new Quest_Magowie2;
			break;
		case Q_ORKOWIE:
			quest = new Quest_Orkowie;
			break;
		case Q_ORKOWIE2:
			quest = new Quest_Orkowie2;
			break;
		case Q_GOBLINY:
			quest = new Quest_Gobliny;
			break;
		case Q_ZLO:
			quest = new Quest_Zlo;
			break;
		case Q_SZALENI:
			quest = new Quest_Szaleni;
			break;
		case Q_LIST_GONCZY:
			quest = new Quest_ListGonczy;
			break;
		default:
			quest = NULL;
			assert(0);
			break;
		}

		quest->quest_id = q;
		quest->Load(file);
		v_quests[i] = quest;
	}
}

// czyszczenie gry
void Game::ClearGame()
{
	LOG("Clearing game.");

	draw_batch.Clear();

	LeaveLocation(true);

	if((game_state == GS_WORLDMAP || prev_game_state == GS_WORLDMAP) && open_location == -1 && IsLocal() && !was_client)
	{
		for(vector<Unit*>::iterator it = team.begin(), end = team.end(); it != end; ++it)
		{
			Unit* unit = *it;

			if(unit->bow_instance)
				bow_instances.push_back(unit->bow_instance);
			if(unit->interp)
				interpolators.Free(unit->interp);

			delete unit->ai;
			delete unit;
		}

		prev_game_state = GS_LOAD;
	}

	if(!net_talk.empty())
		StringPool.Free(net_talk);

	// usu� lokalizacje
	DeleteElements(locations);
	open_location = -1;
	local_ctx_valid = false;
	city_ctx = NULL;

	// usu� quest
	DeleteElements(quests);
	DeleteElements(unaccepted_quests);
	DeleteElements(quest_items);

	DeleteElements(news);
	DeleteElements(encs);

	ClearGui();
}

cstring Game::FormatString(DialogContext& ctx, const string& str_part)
{
	if(str_part == "burmistrzem")
		return location->type == L_CITY ? "burmistrzem" : "so�tysem";
	else if(str_part == "mayor")
		return location->type == L_CITY ? "mayor" : "soltys";
	else if(str_part == "rcitynhere")
		return locations[GetRandomCityLocation(current_location)]->name.c_str();
	else if(str_part == "name")
	{
		assert(ctx.talker->IsHero());
		return ctx.talker->hero->name.c_str();
	}
	else if(str_part == "join_cost")
	{
		assert(ctx.talker->IsHero());
		return Format("%d", ctx.talker->hero->JoinCost());
	}
	else if(str_part == "item")
	{
		assert(ctx.team_share_id != -1);
		return ctx.team_share_item->name.c_str();
	}
	else if(str_part == "item_value")
	{
		assert(ctx.team_share_id != -1);
		return Format("%d", ctx.team_share_item->value/2);
	}
	else if(str_part == "chlanie_loc")
		return locations[chlanie_gdzie]->name.c_str();
	else if(str_part == "player_name")
		return current_dialog->pc->name.c_str();
	else if(str_part == "ironfist_city")
		return locations[zawody_miasto]->name.c_str();
	else if(str_part == "rhero")
	{
		static string str;
		GenerateHeroName(ClassInfo::GetRandom(), rand2()%4==0, str);
		return str.c_str();
	}
	else if(str_part == "ritem")
		return crazy_give_item->name.c_str();
	else
	{
		assert(0);
		return "";
	}
}

int Game::GetNearestLocation(const VEC2& pos, bool not_quest, bool not_city)
{
	float best_dist = 999.f;
	int best_loc = -1;

	int index = 0;
	for(vector<Location*>::iterator it = locations.begin()+(not_city ? cities : 0), end = locations.end(); it != end; ++it, ++index)
	{
		if(!*it)
			continue;
		float dist = distance((*it)->pos, pos);
		if(dist < best_dist)
		{
			best_loc = index;
			best_dist = dist;
		}
	}

	return best_loc;
}

void Game::AddGameMsg(cstring msg, float time)
{
	assert(msg && time > 0.f);

	GameMsg& m = Add1(game_messages->msgs);
	m.msg = msg;
	m.time = time;
	m.fade = -0.1f;
	m.size = CalcRect(font, msg, DT_CENTER|DT_WORDBREAK);
	m.size.y += 6;
	if(game_messages->msgs.size() == 1u)
		m.pos = VEC2(float(wnd_size.x)/2, float(wnd_size.y)/2);
	else
	{
		list<GameMsg>::reverse_iterator it = game_messages->msgs.rbegin();
		++it;
		GameMsg& prev = *it;
		m.pos = VEC2(float(wnd_size.x)/2, prev.pos.y+prev.size.y);
	}
	m.type = 0;

	game_messages->msgs_h += m.size.y;
}

void Game::AddGameMsg2(cstring msg, float time, int id)
{
	assert(id > 0);

	bool jest = false;
	for(list<GameMsg>::iterator it = game_messages->msgs.begin(), end = game_messages->msgs.end(); it != end; ++it)
	{
		if(it->type == id)
		{
			jest = true;
			it->time = time;
			break;
		}
	}

	if(!jest)
	{
		AddGameMsg(msg, time);
		game_messages->msgs.back().type = id;
	}
}

INT2 Game::CalcRect(FONT font, cstring text, int flags)
{
	assert(font && text);

	RECT rect = {0};
	font->DrawTextA(sprite, text, -1, &rect, flags|DT_CALCRECT, WHITE);

	return INT2(abs(rect.right-rect.left), abs(rect.bottom-rect.top));
}

void Game::CreateCityMinimap()
{
	D3DLOCKED_RECT lock;
	tMinimap->LockRect(0, &lock, NULL, 0);

	OutsideLocation* loc = (OutsideLocation*)location;

	for(int y=0; y<OutsideLocation::size; ++y)
	{
		DWORD* pix = (DWORD*)(((byte*)lock.pBits)+lock.Pitch*y);
		for(int x=0; x<OutsideLocation::size; ++x)
		{
			const TerrainTile& t = loc->tiles[x+(OutsideLocation::size-1-y)*OutsideLocation::size];
			DWORD col;
			if(t.mode >= TM_BUILDING)
				col = COLOR_RGB(128,64,0);
			else if(t.alpha == 0)
			{
				if(t.t == TT_GRASS)
					col = COLOR_RGB(0,128,0);
				else if(t.t == TT_ROAD)
					col = COLOR_RGB(128,128,128);
				else if(t.t == TT_FIELD)
					col = COLOR_RGB(200,200,100);
				else
					col = COLOR_RGB(128,128,64);
			}
			else
			{
				int r, g, b, r2, g2, b2;
				switch(t.t)
				{
				case TT_GRASS:
					r = 0;
					g = 128;
					b = 0;
					break;
				case TT_ROAD:
					r = 128;
					g = 128;
					b = 128;
					break;
				case TT_FIELD:
					r = 200;
					g = 200;
					b = 0;
				case TT_SAND:
				default:
					r = 128;
					g = 128;
					b = 64;
					break;
				}
				switch(t.t2)
				{
				case TT_GRASS:
					r2 = 0;
					g2 = 128;
					b2 = 0;
					break;
				case TT_ROAD:
					r2 = 128;
					g2 = 128;
					b2 = 128;
					break;
				case TT_FIELD:
					r2 = 200;
					g2 = 200;
					b2 = 0;
				case TT_SAND:
				default:
					r2 = 128;
					g2 = 128;
					b2 = 64;
					break;
				}
				const float T = float(t.alpha)/255;
				col = COLOR_RGB(lerp(r,r2,T), lerp(g,g2,T), lerp(b,b2,T));
			}
			if(x < 16 || x > 128-16 || y < 16 || y > 128-16)
			{
				col = ((col & 0xFF) / 2) |
					((((col & 0xFF00) >> 8) / 2) << 8) |
					((((col & 0xFF0000) >> 16) / 2) << 16) |
					0xFF000000;
			}
			*pix = col;
			++pix;
		}
	}

	tMinimap->UnlockRect(0);

	minimap->minimap_size = OutsideLocation::size;
}

void Game::CreateDungeonMinimap()
{
	D3DLOCKED_RECT lock;
	tMinimap->LockRect(0, &lock, NULL, 0);

	InsideLocationLevel& lvl = ((InsideLocation*)location)->GetLevelData();

	for(int y=0; y<lvl.h; ++y)
	{
		DWORD* pix = (DWORD*)(((byte*)lock.pBits)+lock.Pitch*y);
		for(int x=0; x<lvl.w; ++x)
		{
			Pole& p = lvl.mapa[x+(lvl.w-1-y)*lvl.w];
			if(IS_SET(p.flagi, Pole::F_ODKRYTE))
			{
				if(OR2_EQ(p.co, SCIANA, BLOKADA_SCIANA))
					*pix = COLOR_RGB(100,100,100);
				else if(p.co == DRZWI)
					*pix = COLOR_RGB(127,51,0);
				else
					*pix = COLOR_RGB(220,220,240);
			}
			else
				*pix = 0;
			++pix;
		}
	}

	tMinimap->UnlockRect(0);

	minimap->minimap_size = lvl.w;
}

void Game::RebuildMinimap()
{
	if(game_state == GS_LEVEL)
	{
		switch(location->type)
		{
		case L_CITY:
		case L_VILLAGE:
			CreateCityMinimap();
			break;
		case L_DUNGEON:
		case L_CRYPT:
		case L_CAVE:
			CreateDungeonMinimap();
			break;
		case L_FOREST:
		case L_CAMP:
		case L_ENCOUNTER:
		case L_MOONWELL:
			CreateForestMinimap();
			break;
		default:
			assert(0);
			WARN(Format("RebuildMinimap: unknown location %d.", location->type));
			break;
		}
	}
}

void Game::UpdateDungeonMinimap(bool send)
{
	if(minimap_reveal.empty())
		return;

	D3DLOCKED_RECT lock;
	tMinimap->LockRect(0, &lock, NULL, 0);

	InsideLocationLevel& lvl = ((InsideLocation*)location)->GetLevelData();

	for(vector<INT2>::iterator it = minimap_reveal.begin(), end = minimap_reveal.end(); it != end; ++it)
	{
		Pole& p = lvl.mapa[it->x+(lvl.w-it->y-1)*lvl.w];
		SET_BIT(p.flagi, Pole::F_ODKRYTE);
		DWORD* pix = ((DWORD*)(((byte*)lock.pBits)+lock.Pitch*it->y))+it->x;
		if(OR2_EQ(p.co, SCIANA, BLOKADA_SCIANA))
			*pix = COLOR_RGB(100,100,100);
		else if(p.co == DRZWI)
			*pix = COLOR_RGB(127,51,0);
		else
			*pix = COLOR_RGB(220,220,240);
	}
	
	if(IsLocal())
	{
		if(send)
		{
			if(IsOnline())
				minimap_reveal_mp.insert(minimap_reveal_mp.end(), minimap_reveal.begin(), minimap_reveal.end());
		}
		else
			minimap_reveal_mp.clear();
	}

	minimap_reveal.clear();

	tMinimap->UnlockRect(0);
}

Door* Game::FindDoor(LevelContext& ctx, const INT2& pt)
{
	for(vector<Door*>::iterator it = ctx.doors->begin(), end = ctx.doors->end(); it != end; ++it)
	{
		if((*it)->pt == pt)
			return *it;
	}

	return NULL;
}

const Item* Game::FindQuestItem(cstring name, int refid)
{
	for(vector<Quest*>::iterator it = quests.begin(), end = quests.end(); it != end; ++it)
	{
		if(refid == (*it)->refid)
			return (*it)->GetQuestItem();
	}
	assert(0);
	return NULL;
}

void Game::AddGroundItem(LevelContext& ctx, GroundItem* item)
{
	assert(item);

	if(ctx.type == LevelContext::Outside)
		terrain->SetH(item->pos);
	ctx.items->push_back(item);

	if(IsOnline())
	{
		item->netid = item_netid_counter++;
		NetChange& c = Add1(net_changes);
		c.type = NetChange::SPAWN_ITEM;
		c.item = item;
	}
}

SOUND Game::GetItemSound(const Item* item)
{
	assert(item);

	switch(item->type)
	{
	case IT_WEAPON:
		return sItem[6];
	case IT_ARMOR:
		if(item->ToArmor().IsHeavy())
			return sItem[2];
		else
			return sItem[1];
	case IT_BOW:
		return sItem[4];
	case IT_SHIELD:
		return sItem[5];
	case IT_CONSUMEABLE:
		if(item->ToConsumeable().cons_type != Food)
			return sItem[0];
		else
			return sItem[7];
	case IT_OTHER:
		if(IS_SET(item->flags, ITEM_CRYSTAL_SOUND))
			return sItem[3];
		else
			return sItem[7];
	case IT_GOLD:
		return sMoneta;
	default:
		return sItem[7];
	}
}

cstring Game::GetCurrentLocationText()
{
	if(game_state == GS_LEVEL)
	{
		if(location->outside)
			return location->name.c_str();
		else
		{
			InsideLocation* inside = (InsideLocation*)location;
			if(inside->IsMultilevel())
				return Format(txLocationText, location->name.c_str(), dungeon_level+1);
			else
				return location->name.c_str();
		}
	}
	else
		return Format(txLocationTextMap, locations[current_location]->name.c_str());
}

void Game::Unit_StopUsingUseable(LevelContext& ctx, Unit& u, bool send)
{
	u.animacja = ANI_STOI;
	u.etap_animacji = 3;
	u.timer = 0.f;
	u.used_item = NULL;

	const float unit_radius = u.GetUnitRadius();

	global_col.clear();
	IgnoreObjects ignore = {0};
	const Unit* ignore_units[2] = {&u, NULL};
	ignore.ignored_units = ignore_units;
	GatherCollisionObjects(ctx, global_col, u.pos, 2.f+unit_radius, &ignore);

	VEC3 tmp_pos = u.target_pos;
	bool ok = false;
	float radius = unit_radius;

	for(int i=0; i<20; ++i)
	{
		if(!Collide(global_col, tmp_pos, unit_radius))
		{
			if(i != 0 && ctx.have_terrain)
				tmp_pos.y = terrain->GetH(tmp_pos);
			u.target_pos = tmp_pos;
			ok = true;
			break;
		}

		int j = rand2()%poisson_disc_count;
		tmp_pos = u.target_pos;
		tmp_pos.x += POISSON_DISC_2D[j].x*radius;
		tmp_pos.z += POISSON_DISC_2D[j].y*radius;

		if(i < 10)
			radius += 0.2f;
	}

	assert(ok);

	if(u.cobj)
		UpdateUnitPhysics(&u, u.target_pos);

	if(send && IsOnline())
	{
		if(IsLocal())
		{
			NetChange& c = Add1(net_changes);
			c.type = NetChange::USE_USEABLE;
			c.unit = &u;
			c.id = u.useable->netid;
			c.ile = 3;
		}
		else if(&u == pc->unit)
		{
			NetChange& c = Add1(net_changes);
			c.type = NetChange::USE_USEABLE;
			c.id = u.useable->netid;
			c.ile = 0;
		}
	}
}

// ponowne wej�cie na poziom podziemi
void Game::OnReenterLevel(LevelContext& ctx)
{
	// odtw�rz skrzynie
	if(ctx.chests)
	{
		for(vector<Chest*>::iterator it = ctx.chests->begin(), end = ctx.chests->end(); it != end; ++it)
		{
			Chest& chest = **it;

			chest.ani = new AnimeshInstance(aSkrzynia);
		}
	}

	// odtw�rz drzwi
	if(ctx.doors)
	{
		for(vector<Door*>::iterator it = ctx.doors->begin(), end = ctx.doors->end(); it != end; ++it)
		{
			Door& door = **it;

			// animowany model
			door.ani = new AnimeshInstance(door.door2 ? aDrzwi2 : aDrzwi);
			door.ani->groups[0].speed = 2.f;

			// fizyka
			door.phy = new btCollisionObject;
			door.phy->setCollisionShape(shape_door);
			btTransform& tr = door.phy->getWorldTransform();
			VEC3 pos = door.pos;
			pos.y += 1.319f;
			tr.setOrigin(ToVector3(pos));
			tr.setRotation(btQuaternion(door.rot, 0, 0));
			door.phy->setCollisionFlags(CG_WALL);
			phy_world->addCollisionObject(door.phy);

			// czy otwarte
			if(door.state == Door::Open)
			{
				btVector3& pos = door.phy->getWorldTransform().getOrigin();
				pos.setY(pos.y() - 100.f);
				door.ani->SetToEnd(door.ani->ani->anims[0].name.c_str());
			}
		}
	}
}

void Game::ApplyToTexturePack(TexturePack& tp, cstring diffuse, cstring normal, cstring specular)
{
	tp.diffuse = LoadTexResource(diffuse);
	if(normal)
		tp.normal = LoadTexResource(normal);
	else
		tp.normal = NULL;
	if(specular)
		tp.specular = LoadTexResource(specular);
	else
		tp.specular = NULL;
}

void ApplyTexturePackToSubmesh(Animesh::Submesh& sub, TexturePack& tp)
{
	sub.tex = tp.diffuse;
	sub.tex_normal = tp.normal;
	sub.tex_specular = tp.specular;
}

void ApplyDungeonLightToMesh(Animesh& mesh)
{
	for(int i=0; i<mesh.head.n_subs; ++i)
	{
		mesh.subs[i].specular_color = VEC3(1,1,1);
		mesh.subs[i].specular_intensity = 0.2f;
		mesh.subs[i].specular_hardness = 10;
	}
}

void Game::SetDungeonParamsAndTextures(BaseLocation& base)
{
	// ustawienia t�a
	draw_range = base.draw_range;
	fog_params = VEC4(base.fog_range.x, base.fog_range.y, base.fog_range.y-base.fog_range.x, 0);
	fog_color = VEC4(base.fog_color, 1);
	ambient_color = VEC4(base.ambient_color, 1);
	clear_color2 = COLOR_RGB(int(fog_color.x*255), int(fog_color.y*255), int(fog_color.z*255));

	// tekstury podziemi
	if(base.tex_floor)
		ApplyToTexturePack(tFloor[0], base.tex_floor, base.tex_floor_nrm, base.tex_floor_spec);
	else
		tFloor[0] = tFloorBase;
	if(base.tex_wall)
		ApplyToTexturePack(tWall[0], base.tex_wall, base.tex_wall_nrm, base.tex_wall_spec);
	else
		tWall[0] = tWallBase;
	if(base.tex_ceil)
		ApplyToTexturePack(tCeil[0], base.tex_ceil, base.tex_ceil_nrm, base.tex_ceil_spec);
	else
		tCeil[0] = tCeilBase;

	// tekstury schod�w / pu�apek
	ApplyTexturePackToSubmesh(aSchodyDol->subs[0], tFloor[0]);
	ApplyTexturePackToSubmesh(aSchodyDol->subs[2], tWall[0]);
	ApplyTexturePackToSubmesh(aSchodyDol2->subs[0], tFloor[0]);
	ApplyTexturePackToSubmesh(aSchodyDol2->subs[2], tWall[0]);
	ApplyTexturePackToSubmesh(aSchodyGora->subs[0], tFloor[0]);
	ApplyTexturePackToSubmesh(aSchodyGora->subs[2], tWall[0]);
	ApplyTexturePackToSubmesh(aNaDrzwi->subs[0], tWall[0]);
	ApplyTexturePackToSubmesh(g_traps[TRAP_ARROW].mesh->subs[0], tFloor[0]);
	ApplyTexturePackToSubmesh(g_traps[TRAP_POISON].mesh->subs[0], tFloor[0]);
	ApplyDungeonLightToMesh(*aSchodyDol);
	ApplyDungeonLightToMesh(*aSchodyDol2);
	ApplyDungeonLightToMesh(*aSchodyGora);
	ApplyDungeonLightToMesh(*aNaDrzwi);
	ApplyDungeonLightToMesh(*aNaDrzwi2);
	ApplyDungeonLightToMesh(*g_traps[TRAP_ARROW].mesh);
	ApplyDungeonLightToMesh(*g_traps[TRAP_POISON].mesh);
	
	// druga tekstura
	if(base.tex2 != -1)
	{
		BaseLocation& base2 = g_base_locations[base.tex2];
		if(base2.tex_floor)
			ApplyToTexturePack(tFloor[1], base2.tex_floor, base2.tex_floor_nrm, base2.tex_floor_spec);
		else
			tFloor[1] = tFloor[0];
		if(base2.tex_wall)
			ApplyToTexturePack(tWall[1], base2.tex_wall, base2.tex_wall_nrm, base2.tex_wall_spec);
		else
			tWall[1] = tWall[0];
		if(base2.tex_ceil)
			ApplyToTexturePack(tCeil[1], base2.tex_ceil, base2.tex_ceil_nrm, base2.tex_ceil_spec);
		else
			tCeil[1] = tCeil[0];
		ApplyTexturePackToSubmesh(aNaDrzwi2->subs[0], tWall[1]);
	}
	else
	{
		tFloor[1] = tFloorBase;
		tCeil[1] = tCeilBase;
		tWall[1] = tWallBase;
		ApplyTexturePackToSubmesh(aNaDrzwi2->subs[0], tWall[0]);
	}

	// ustawienia uv podziemi
	bool new_tex_wrap = !IS_SET(base.options, BLO_NO_TEX_WRAP);
	if(new_tex_wrap != dungeon_tex_wrap)
	{
		dungeon_tex_wrap = new_tex_wrap;
		ChangeDungeonTexWrap();
	}
}

void Game::EnterLevel(bool first, bool reenter, bool from_lower, int from_portal, bool from_outside)
{
	if(!first)
		LOG(Format("Entering location '%s' level %d.", location->name.c_str(), dungeon_level+1));

	show_mp_panel = true;
	Inventory::lock_id = LOCK_NO;
	Inventory::lock_give = false;

	InsideLocation* inside = (InsideLocation*)location;
	inside->SetActiveLevel(dungeon_level);
	int days;
	bool need_reset = inside->CheckUpdate(days, worldtime);
	InsideLocationLevel& lvl = inside->GetLevelData();
	BaseLocation& base = g_base_locations[inside->cel];

	if(from_portal != -1)
		enter_from = ENTER_FROM_PORTAL+from_portal;
	else if(from_outside)
		enter_from = ENTER_FROM_OUTSIDE;
	else if(from_lower)
		enter_from = ENTER_FROM_DOWN_LEVEL;
	else
		enter_from = ENTER_FROM_UP_LEVEL;

	// ustaw wska�niki
	if(!reenter)
		ApplyContext(inside, local_ctx);

	SetDungeonParamsAndTextures(base);

	// czy to pierwsza wizyta?
	if(first)
	{
		if(location->type == L_CAVE)
		{
			LoadingStep(txGeneratingObjects);

			// jaskinia
			// schody w g�r�
			Object& o = Add1(local_ctx.objects);
			o.mesh = aSchodyGora;
			o.pos = pt_to_pos(lvl.schody_gora);
			o.rot = VEC3(0, dir_to_rot(lvl.schody_gora_dir), 0);
			o.scale = 1;
			o.base = NULL;

			GenerateCaveObjects();
			if(current_location == kopalnia_gdzie)
				GenerujKopalnie();

			LoadingStep(txGeneratingUnits);
			GenerateCaveUnits();
			GenerateMushrooms();
		}
		else
		{
			// podziemia, wygeneruj schody, drzwi, kratki
			LoadingStep(txGeneratingObjects);
			GenerateDungeonObjects2();
			GenerateDungeonObjects();
			GenerateTraps();

			if(!IS_SET(base.options, BLO_LABIRYNTH))
			{
				LoadingStep(txGeneratingUnits);
				GenerateDungeonUnits();
				GenerateDungeonFood();
				ResetCollisionPointers();
			}
			else
			{
				Obj* obj = FindObject("torch");

				// pochodnia przy �cianie
				{
					Object& o = Add1(lvl.objects);
					o.base = obj;
					o.rot = VEC3(0,random(MAX_ANGLE),0);
					o.scale = 1.f;
					o.mesh = obj->ani;

					INT2 pt = lvl.GetUpStairsFrontTile();
					if(czy_blokuje2(lvl.mapa[pt.x-1+pt.y*lvl.w].co))
						o.pos = VEC3(2.f*pt.x+obj->size.x+0.1f, 0.f, 2.f*pt.y+1.f);
					else if(czy_blokuje2(lvl.mapa[pt.x+1+pt.y*lvl.w].co))
						o.pos = VEC3(2.f*(pt.x+1)-obj->size.x-0.1f, 0.f, 2.f*pt.y+1.f);
					else if(czy_blokuje2(lvl.mapa[pt.x+(pt.y-1)*lvl.w].co))
						o.pos = VEC3(2.f*pt.x+1.f, 0.f, 2.f*pt.y+obj->size.y+0.1f);
					else if(czy_blokuje2(lvl.mapa[pt.x+(pt.y+1)*lvl.w].co))
						o.pos = VEC3(2.f*pt.x+1.f, 0.f, 2.f*(pt.y+1)+obj->size.y-0.1f);

					Light& s = Add1(lvl.lights);
					s.pos = o.pos;
					s.pos.y += obj->centery;
					s.range = 5;
					s.color = VEC3(1.f,0.9f,0.9f);

					ParticleEmitter* pe = new ParticleEmitter;
					pe->tex = tFlare;
					pe->alpha = 0.8f;
					pe->emision_interval = 0.1f;
					pe->emisions = -1;
					pe->life = -1;
					pe->max_particles = 50;
					pe->op_alpha = POP_LINEAR_SHRINK;
					pe->op_size = POP_LINEAR_SHRINK;
					pe->particle_life = 0.5f;
					pe->pos = s.pos;
					pe->pos_min = VEC3(0,0,0);
					pe->pos_max = VEC3(0,0,0);
					pe->size = IS_SET(obj->flagi, OBJ_OGNISKO) ? .7f : .5f;
					pe->spawn_min = 1;
					pe->spawn_max = 3;
					pe->speed_min = VEC3(-1,3,-1);
					pe->speed_max = VEC3(1,4,1);
					pe->mode = 1;
					pe->Init();
					local_ctx.pes->push_back(pe);
				}

				// pochodnia w skarbie
				{
					Object& o = Add1(lvl.objects);
					o.base = obj;
					o.rot = VEC3(0,random(MAX_ANGLE),0);
					o.scale = 1.f;
					o.mesh = obj->ani;
					o.pos = lvl.pokoje[0].Srodek();

					Light& s = Add1(lvl.lights);
					s.pos = o.pos;
					s.pos.y += obj->centery;
					s.range = 5;
					s.color = VEC3(1.f,0.9f,0.9f);

					ParticleEmitter* pe = new ParticleEmitter;
					pe->tex = tFlare;
					pe->alpha = 0.8f;
					pe->emision_interval = 0.1f;
					pe->emisions = -1;
					pe->life = -1;
					pe->max_particles = 50;
					pe->op_alpha = POP_LINEAR_SHRINK;
					pe->op_size = POP_LINEAR_SHRINK;
					pe->particle_life = 0.5f;
					pe->pos = s.pos;
					pe->pos_min = VEC3(0,0,0);
					pe->pos_max = VEC3(0,0,0);
					pe->size = IS_SET(obj->flagi, OBJ_OGNISKO) ? .7f : .5f;
					pe->spawn_min = 1;
					pe->spawn_max = 3;
					pe->speed_min = VEC3(-1,3,-1);
					pe->speed_max = VEC3(1,4,1);
					pe->mode = 1;
					pe->Init();
					local_ctx.pes->push_back(pe);
				}

				LoadingStep(txGeneratingUnits);
				GenerateLabirynthUnits();
			}
		}
	}
	else if(!reenter)
	{
		LoadingStep(txRegeneratingLevel);

		if(days > 0)
			UpdateLocation(local_ctx, days, base.door_open, need_reset);

		bool respawn_units = true;

		if(location->type == L_CAVE)
		{
			if(current_location == kopalnia_gdzie)
				respawn_units = GenerujKopalnie();
			if(days > 0)
				GenerateMushrooms(min(days, 10));
		}
		else if(days >= 10 && IS_SET(base.options, BLO_LABIRYNTH))
			GenerateDungeonFood();

		if(need_reset)
		{
			// usu� �ywe jednostki
			if(days != 0)
			{
				for(vector<Unit*>::iterator it = local_ctx.units->begin(), end = local_ctx.units->end(); it != end; ++it)
				{
					if((*it)->IsAlive())
					{
						delete *it;
						*it = NULL;
					}
				}
				local_ctx.units->erase(std::remove_if(local_ctx.units->begin(), local_ctx.units->end(), is_null<Unit*>), local_ctx.units->end());
			}

			// usu� zawarto�� skrzyni
			for(vector<Chest*>::iterator it = local_ctx.chests->begin(), end = local_ctx.chests->end(); it != end; ++it)
				(*it)->items.clear();

			// nowa zawarto�� skrzyni
			int dlevel = GetDungeonLevel();
			for(vector<Chest*>::iterator it = local_ctx.chests->begin(), end = local_ctx.chests->end(); it != end; ++it)
			{
				Chest& chest = **it;
				if(!chest.items.empty())
					continue;
				Pokoj* p = lvl.GetNearestRoom(chest.pos);
				static vector<Chest*> room_chests;
				room_chests.push_back(&chest);
				for(vector<Chest*>::iterator it2 = it+1; it2 != end; ++it2)
				{
					if(p->IsInside((*it2)->pos))
						room_chests.push_back(*it2);
				}
				int gold;
				if(room_chests.size() == 1)
				{
					vector<ItemSlot>& items = room_chests.front()->items;
					GenerateTreasure(dlevel, 10, items, gold);
					InsertItemBare(items, &gold_item, (uint)gold, (uint)gold);
					SortItems(items);
				}
				else
				{
					static vector<ItemSlot> items;
					GenerateTreasure(dlevel, 9+2*room_chests.size(), items, gold);
					SplitTreasure(items, gold, &room_chests[0], room_chests.size());
				}
				room_chests.clear();
			}

			// nowe jednorazowe pu�apki
			RegenerateTraps();
		}

		OnReenterLevel(local_ctx);

		// odtw�rz jednostki
		if(respawn_units)
			RespawnUnits();
		RespawnTraps();

		// odtw�rz fizyk�
		RespawnObjectColliders();

		if(need_reset)
		{
			// nowe jednostki
			if(location->type == L_CAVE)
				GenerateCaveUnits();
			else if(inside->cel == LABIRYNTH)
				GenerateLabirynthUnits();
			else
				GenerateDungeonUnits();
		}
	}

	// questowe rzeczy
	if(inside->active_quest && inside->active_quest != (Quest_Dungeon*)ACTIVE_QUEST_HOLDER)
	{
		Quest_Event* event = inside->active_quest->GetEvent(current_location);
		if(event && event->at_level == dungeon_level)
		{
			if(!event->done)
			{
				event->done = true;
				Unit* spawned = NULL, *spawned2 = NULL;
				if(event->unit_to_spawn)
				{
					Pokoj& pokoj = GetRoom(lvl, event->spawn_unit_room, inside->HaveDownStairs());
					spawned = SpawnUnitInsideRoomOrNear(lvl, pokoj, *event->unit_to_spawn, event->unit_spawn_level);
					if(!spawned)
						throw "Failed to spawn quest unit!";
					spawned->dont_attack = event->unit_dont_attack;
					spawned->auto_talk = (event->unit_auto_talk ? 1 : 0);
					spawned->event_handler = event->unit_event_handler;
					DEBUG_LOG(Format("Wygenerowano jednostk� %s (%g,%g).", event->unit_to_spawn->name, spawned->pos.x, spawned->pos.z));

					if(IS_SET(spawned->data->flagi2, F2_OBSTAWA))
					{
						for(vector<Unit*>::iterator it = local_ctx.units->begin(), end = local_ctx.units->end(); it != end; ++it)
						{
							if((*it) != spawned && IsFriend(**it, *spawned) && lvl.GetRoom(pos_to_pt((*it)->pos)) == &pokoj)
							{
								(*it)->dont_attack = spawned->dont_attack;
								(*it)->guard_target = spawned;
							}
						}
					}
				}
				if(event->unit_to_spawn2)
				{
					Pokoj* pokoj;
					if(event->spawn_2_guard_1)
						pokoj = lvl.GetRoom(pos_to_pt(spawned->pos));
					else
						pokoj = &GetRoom(lvl, POKOJ_CEL_BRAK, inside->HaveDownStairs());
					spawned2 = SpawnUnitInsideRoomOrNear(lvl, *pokoj, *event->unit_to_spawn2, event->unit_spawn_level2);
					if(!spawned2)
						throw "Failed to spawn quest unit 2!";
					DEBUG_LOG(Format("Wygenerowano jednostk� %s (%g,%g).", event->unit_to_spawn2->name, spawned2->pos.x, spawned2->pos.z));
					if(event->spawn_2_guard_1)
					{
						spawned2->dont_attack = spawned->dont_attack;
						spawned2->guard_target = spawned;
					}
				}
				if(event->spawn_item == Quest_Dungeon::Item_GiveStrongest)
				{
					Unit* best = NULL;
					for(vector<Unit*>::iterator it = local_ctx.units->begin(), end = local_ctx.units->end(); it != end; ++it)
					{
						if((*it)->IsAlive() && IsEnemy(**it, *pc->unit) && (!best || (*it)->level > best->level))
							best = *it;
					}
					assert(best);
					if(best)
					{
						best->AddItem(event->item_to_give[0], 1, true);
						DEBUG_LOG(Format("Przekazano przedmiot %s jednostce %s (%g,%g).", event->item_to_give[0]->id, best->data->name, best->pos.x, best->pos.z));
					}
				}
				else if(event->spawn_item == Quest_Dungeon::Item_GiveSpawned)
				{
					assert(spawned);
					if(spawned)
					{
						spawned->AddItem(event->item_to_give[0], 1, true);
						DEBUG_LOG(Format("Przekazano przedmiot %s jednostce %s (%g,%g).", event->item_to_give[0]->id, spawned->data->name, spawned->pos.x, spawned->pos.z));
					}
				}
				else if(event->spawn_item == Quest_Dungeon::Item_GiveSpawned2)
				{
					assert(spawned2);
					if(spawned2)
					{
						spawned2->AddItem(event->item_to_give[0], 1, true);
						DEBUG_LOG(Format("Przekazano przedmiot %s jednostce %s (%g,%g).", event->item_to_give[0]->id, spawned2->data->name, spawned2->pos.x, spawned2->pos.z));
					}
				}
				else if(event->spawn_item == Quest_Dungeon::Item_InTreasure)
				{
					Chest* chest = NULL;

					if(inside->type == L_CRYPT)
					{
						Pokoj& p = lvl.pokoje[inside->specjalny_pokoj];
						vector<Chest*> chests;
						for(vector<Chest*>::iterator it = lvl.chests.begin(), end = lvl.chests.end(); it != end; ++it)
						{
							if(p.IsInside((*it)->pos))
								chests.push_back(*it);
						}

						if(!chests.empty())
							chest = chests[rand2()%chests.size()];
					}
					else
					{
						assert(inside->cel == LABIRYNTH);
						chest = lvl.chests[rand2()%lvl.chests.size()];
					}

					assert(chest);
					if(chest)
					{
						chest->AddItem(event->item_to_give[0]);
						DEBUG_LOG(Format("Wygenerowano przedmiot %s w skrzyni (%g,%g).", event->item_to_give[0]->id, chest->pos.x, chest->pos.z));
					}
				}
				else if(event->spawn_item == Quest_Dungeon::Item_OnGround)
				{
					GroundItem* item = SpawnGroundItemInsideAnyRoom(lvl, event->item_to_give[0]);
					DEBUG_LOG(Format("Wygenerowano przedmiot %s na ziemi (%g,%g).", event->item_to_give[0]->id, item->pos.x, item->pos.z));
				}
				else if(event->spawn_item == Quest_Dungeon::Item_InChest)
				{
					Chest* chest = local_ctx.GetRandomFarChest(GetSpawnPoint());
					chest->AddItem(event->item_to_give[0]);
					if(event->item_to_give[1])
					{
						chest->AddItem(event->item_to_give[1]);
						if(event->item_to_give[2])
						{
							chest->AddItem(event->item_to_give[2]);
							DEBUG_LOG(Format("Dodano przedmioty %s, %s i %s do skrzyni (%g,%g).", event->item_to_give[0]->id, event->item_to_give[1]->id, event->item_to_give[2]->id,
								chest->pos.x, chest->pos.z));
						}
						else
							DEBUG_LOG(Format("Dodano przedmioty %s i %s do skrzyni (%g,%g).", event->item_to_give[0]->id, event->item_to_give[1]->id, chest->pos.x, chest->pos.z));
					}
					else
						DEBUG_LOG(Format("Dodano przedmiot %s do skrzyni (%g,%g).", event->item_to_give[0]->id, chest->pos.x, chest->pos.z));
					chest->handler = event->chest_event_handler;
				}

				if(event->callback)
					event->callback();

				// generowanie ork�w
				if(current_location == orkowie_gdzie && orkowie_stan == OS_GENERUJ_ORKI)
				{
					orkowie_stan = OS_WYGENEROWANO_ORKI;
					UnitData* ud = FindUnitData("q_orkowie_slaby");
					for(vector<Pokoj>::iterator it = lvl.pokoje.begin(), end = lvl.pokoje.end(); it != end; ++it)
					{
						if(!it->korytarz && rand2()%2 == 0)
						{
							Unit* u = SpawnUnitInsideRoom(*it, *ud, -2, INT2(-999,-999), INT2(-999,-999));
							if(u)
								u->dont_attack = true;
						}
					}

					Unit* u = SpawnUnitInsideRoom(lvl.GetFarRoom(false), *FindUnitData("q_orkowie_kowal"), -2, INT2(-999,-999), INT2(-999,-999));
					if(u)
						u->dont_attack = true;

					InsertItemBare(orkowie_towar, FindItem("sword_orcish"), random(1,3));
					InsertItemBare(orkowie_towar, FindItem("axe_orcish"), random(1,3));
					InsertItemBare(orkowie_towar, FindItem("blunt_orcish"), random(1,3));
					InsertItemBare(orkowie_towar, FindItem("blunt_orcish_shaman"), 1);
					InsertItemBare(orkowie_towar, FindItem("bow_long"), 1);
					InsertItemBare(orkowie_towar, FindItem("shield_iron"), 1);
					InsertItemBare(orkowie_towar, FindItem("shield_steel"), 1);
					InsertItemBare(orkowie_towar, FindItem("armor_orcish_leather"), random(1,2));
					InsertItemBare(orkowie_towar, FindItem("armor_orcish_chainmail"), random(1,2));
					InsertItemBare(orkowie_towar, FindItem("armor_orcish_shaman"), 1);
					SortItems(orkowie_towar);
				}
			}

			location_event_handler = event->location_event_handler;
		}
		else if(inside->active_quest->whole_location_event_handler)
			location_event_handler = event->location_event_handler;
	}

	bool debug_do = false;
#ifdef _DEBUG
	debug_do = true;
#endif

	if((first || need_reset) && (rand2()%50 == 0 || (debug_do && Key.Down('C'))) && location->type != L_CAVE && inside->cel != LABIRYNTH && !location->active_quest && dungeon_level == 0)
		SpawnHeroesInsideDungeon();

	// stw�rz obiekty kolizji
	if(!reenter)
		SpawnDungeonColliders();

	// generuj minimap�
	LoadingStep(txGeneratingMinimap);
	CreateDungeonMinimap();

	// sekret
	if(current_location == sekret_gdzie && !inside->HaveDownStairs() && sekret_stan == SS2_WRZUCONO_KAMIEN)
	{
		sekret_stan = SS2_WYGENEROWANO;
		DEBUG_LOG("Generated secret room.");

		Pokoj& p = inside->GetLevelData().pokoje[0];

		if(hardcore_mode && !used_cheats)
		{
			Object* o = local_ctx.FindObject(FindObject("portal"));

			OutsideLocation* loc = new OutsideLocation;
			loc->active_quest = (Quest_Dungeon*)ACTIVE_QUEST_HOLDER;
			loc->pos = VEC2(-999,-999);
			loc->st = 20;
			loc->name = txHiddenPlace;
			loc->type = L_FOREST;
			int loc_id = AddLocation(loc);

			Portal* portal = new Portal;
			portal->at_level = 2;
			portal->next_portal = NULL;
			portal->pos = o->pos;
			portal->rot = o->rot.y;
			portal->target = 0;
			portal->target_loc = loc_id;

			inside->portal = portal;
			sekret_gdzie2 = loc_id;
		}
		else
		{
			// dodaj kartk� (overkill sprawdzania!)
			const Item* kartka = FindItem("sekret_kartka2");
			assert(kartka);
			Chest* c = local_ctx.FindChestInRoom(p);
			assert(c);
			if(c)
				c->AddItem(kartka, 1, 1);
			else
			{
				Object* o = local_ctx.FindObject(FindObject("portal"));
				assert(0);
				if(o)
				{
					GroundItem* item = new GroundItem;
					item->count = 1;
					item->team_count = 1;
					item->item = kartka;
					item->netid = item_netid_counter++;
					item->pos = o->pos;
					item->rot = random(MAX_ANGLE);
					local_ctx.items->push_back(item);
				}
				else
					SpawnGroundItemInsideRoom(p, kartka);
			}

			sekret_stan = SS2_ZAMKNIETO;
		}
	}

	// dodaj gracza i jego dru�yn�
	INT2 spawn_pt;
	VEC3 spawn_pos;
	float spawn_rot;

	if(from_portal == -1)
	{
		if(from_lower)
		{
			spawn_pt = lvl.schody_dol+g_kierunek2[lvl.schody_dol_dir];
			spawn_rot = dir_to_rot(lvl.schody_dol_dir);
		}
		else
		{
			spawn_pt = lvl.schody_gora+g_kierunek2[lvl.schody_gora_dir];
			spawn_rot = dir_to_rot(lvl.schody_gora_dir);
		}
		spawn_pos = pt_to_pos(spawn_pt);
	}
	else
	{
		Portal* portal = inside->GetPortal(from_portal);
		spawn_pos = portal->GetSpawnPos();
		spawn_rot = clip(portal->rot+PI);
		spawn_pt = pos_to_pt(spawn_pos);
	}

	AddPlayerTeam(spawn_pos, spawn_rot, reenter, from_outside);
	OpenDoorsByTeam(spawn_pt);
	SetMusic();
	OnEnterLevel();

	if(!first)
		LOG("Entered level.");
}

void Game::LeaveLevel(bool clear)
{
	if(game_gui)
	{
		for(vector<SpeechBubble*>::iterator it = game_gui->speech_bbs.begin(), end = game_gui->speech_bbs.end(); it != end; ++it)
		{
			if((*it)->unit)
				(*it)->unit->bubble = NULL;
			delete *it;
		}
		game_gui->speech_bbs.clear();
	}

	warp_to_inn.clear();

	if(local_ctx_valid)
	{
		LeaveLevel(local_ctx, clear);
		tmp_ctx_pool.Free(local_ctx.tmp_ctx);
		local_ctx.tmp_ctx = NULL;
		local_ctx_valid = false;
	}

	if(city_ctx)
	{
		for(vector<InsideBuilding*>::iterator it = city_ctx->inside_buildings.begin(), end = city_ctx->inside_buildings.end(); it != end; ++it)
		{
			if(!*it)
				continue;
			LeaveLevel((*it)->ctx, clear);
			if((*it)->ctx.require_tmp_ctx)
			{
				tmp_ctx_pool.Free((*it)->ctx.tmp_ctx);
				(*it)->ctx.tmp_ctx = NULL;
			}
		}

		if(!IsLocal())
			DeleteElements(city_ctx->inside_buildings);
	}

	for(vector<Unit*>::iterator it = warp_to_inn.begin(), end = warp_to_inn.end(); it != end; ++it)
		WarpToInn(**it);

	ais.clear();
	RemoveColliders();
	unit_views.clear();
	attached_sounds.clear();
	cam_colliders.clear();

	ClearQuadtree();

	CloseAllPanels();

	first_frame = true;
	rot_buf = 0.f;
	selected_unit = NULL;
	dialog_context.dialog_mode = false;
	inventory_mode = I_NONE;
	before_player = BP_NONE;

#ifdef IS_DEV
	if(!clear)
		ValidateTeamItems();
#endif
}

void Game::LeaveLevel(LevelContext& ctx, bool clear)
{
	// posprz�taj jednostki
	// usu� AnimeshInstance i btCollisionShape
	if(IsLocal() && !clear)
	{
		for(vector<Unit*>::iterator it = ctx.units->begin(), end = ctx.units->end(); it != end; ++it)
		{
			// fizyka
			delete (*it)->cobj->getCollisionShape();
			(*it)->cobj = NULL;

			// je�li u�ywa jakiego� obiektu to przesu�
			if((*it)->useable)
			{
				Unit_StopUsingUseable(ctx, **it);
				(*it)->useable->user = NULL;
				(*it)->useable = NULL;
				(*it)->visual_pos = (*it)->pos = (*it)->target_pos;
			}

			if((*it)->bow_instance)
			{
				bow_instances.push_back((*it)->bow_instance);
				(*it)->bow_instance = NULL;
			}

			// model
			if((*it)->IsAI())
			{
				if((*it)->IsFollower())
				{
					if(!(*it)->IsAlive())
					{
						(*it)->hp = 1.f;
						(*it)->live_state = Unit::ALIVE;
					}
					(*it)->hero->mode = HeroData::Follow;
					(*it)->talking = false;
					(*it)->ani->need_update = true;
					(*it)->ai->Reset();
					*it = NULL;
				}
				else
				{
					if((*it)->ai->goto_inn && city_ctx)
					{
						// przenie� do karczmy przy opuszczaniu lokacji
						WarpToInn(**it);
					}
					delete (*it)->ani;
					(*it)->ani = NULL;
					delete (*it)->ai;
					(*it)->ai = NULL;
					(*it)->EndEffects();

					if((*it)->useable)
						(*it)->pos = (*it)->target_pos;
				}
			}
			else
			{
				(*it)->talking = false;
				(*it)->ani->need_update = true;
				(*it)->useable = NULL;
				*it = NULL;
			}
		}

		// usu� jednostki kt�re przenios�y si� na inny poziom
		ctx.units->erase(std::remove_if(ctx.units->begin(), ctx.units->end(), is_null<Unit*>), ctx.units->end());
	}
	else
	{
		for(vector<Unit*>::iterator it = ctx.units->begin(), end = ctx.units->end(); it != end; ++it)
		{
			if(!*it)
				continue;

			if((*it)->cobj)
				delete (*it)->cobj->getCollisionShape();

			if((*it)->bow_instance)
				bow_instances.push_back((*it)->bow_instance);

			// zwolnij EntityInterpolator
			if((*it)->interp)
				interpolators.Free((*it)->interp);

			delete (*it)->ai;
			delete *it;
		}

		ctx.units->clear();
	}

	// usu� tymczasowe obiekty
	DeleteElements(ctx.explos);
	DeleteElements(ctx.electros);
	ctx.drains->clear();
	ctx.bullets->clear();
	ctx.colliders->clear();
	
	// cz�steczki
	DeleteElements(ctx.pes);
	DeleteElements(ctx.tpes);

	if(IsLocal())
	{
		// usu� modele skrzyni
		if(ctx.chests)
		{
			for(vector<Chest*>::iterator it = ctx.chests->begin(), end = ctx.chests->end(); it != end; ++it)
			{
				delete (*it)->ani;
				(*it)->ani = NULL;
			}
		}

		// usu� modele drzwi
		if(ctx.doors)
		{
			for(vector<Door*>::iterator it = ctx.doors->begin(), end = ctx.doors->end(); it != end; ++it)
			{
				Door& door = **it;
				if(door.state == Door::Closing || door.state == Door::Closing2)
					door.state = Door::Closed;
				else if(door.state == Door::Opening || door.state == Door::Opening2)
					door.state = Door::Open;
				delete door.ani;
				door.ani = NULL;
			}
		}
	}
	else
	{
		// usu� obiekty
		if(ctx.chests)
			DeleteElements(ctx.chests);
		if(ctx.doors)
			DeleteElements(ctx.doors);
		if(ctx.traps)
			DeleteElements(ctx.traps);
		if(ctx.useables)
			DeleteElements(ctx.useables);
	}

	// maksymalny rozmiar plamy krwi
	for(vector<Blood>::iterator it = ctx.bloods->begin(), end = ctx.bloods->end(); it != end; ++it)
		it->size = 1.f;
}

void Game::CreateBlood(LevelContext& ctx, const Unit& u, bool fully_created)
{
	if(!tKrewSlad[u.data->blood] || IS_SET(u.data->flagi2, F2_BRAK_KRWII))
		return;

	Blood& b = Add1(ctx.bloods);
	if(u.human_data)
		u.ani->SetupBones(&u.human_data->mat_scale[0]);
	else
		u.ani->SetupBones();
	b.pos = u.GetLootCenter();
	b.type = u.data->blood;
	b.rot = random(MAX_ANGLE);
	b.size = (fully_created ? 1.f : 0.f);

	if(ctx.have_terrain)
	{
		b.pos.y = terrain->GetH(b.pos) + 0.05f;
		terrain->GetAngle(b.pos.x, b.pos.z, b.normal);
	}
	else
	{
		b.pos.y = u.pos.y+0.05f;
		b.normal = VEC3(0,1,0);
	}
}

void Game::WarpUnit(Unit& unit, const VEC3& pos)
{
	const float unit_radius = unit.GetUnitRadius();

	global_col.clear();
	LevelContext& ctx = GetContext(unit);
	IgnoreObjects ignore = {0};
	const Unit* ignore_units[2] = {&unit, NULL};
	ignore.ignored_units = ignore_units;
	GatherCollisionObjects(ctx, global_col, pos, 2.f+unit_radius, &ignore);

	VEC3 tmp_pos = pos;
	bool ok = false;
	float radius = unit_radius;

	for(int i=0; i<20; ++i)
	{
		if(!Collide(global_col, tmp_pos, unit_radius))
		{
			unit.pos = tmp_pos;
			ok = true;
			break;
		}

		int j = rand2()%poisson_disc_count;
		tmp_pos = pos;
		tmp_pos.x += POISSON_DISC_2D[j].x*radius;
		tmp_pos.z += POISSON_DISC_2D[j].y*radius;

		if(i < 10)
			radius += 0.25f;
	}

	assert(ok);

	if(ctx.have_terrain)
	{
		if(terrain->IsInside(unit.pos))
			terrain->SetH(unit.pos);
	}

	if(unit.cobj)
		UpdateUnitPhysics(&unit, unit.pos);

	unit.visual_pos = unit.pos;

	if(IsOnline())
	{
		if(unit.interp)
			unit.interp->Reset(unit.pos, unit.rot);
		NetChange& c = Add1(net_changes);
		c.type = NetChange::WARP;
		c.unit = &unit;
		if(unit.IsPlayer())
			GetPlayerInfo(unit.player->id).warping = true;
	}
}

void Game::ProcessUnitWarps()
{
	bool warped_to_arena = false;

	for(vector<UnitWarpData>::iterator it = unit_warp_data.begin(), end = unit_warp_data.end(); it != end; ++it)
	{
		if(it->where == -1)
		{
			if(city_ctx)
			{
				// powr�� na g��wn� map�
				InsideBuilding& building = *city_ctx->inside_buildings[it->unit->in_building];
				RemoveElement(building.units, it->unit);
				it->unit->in_building = -1;
				WarpUnit(*it->unit, building.outside_spawn);
				it->unit->rot = building.outside_rot;
				local_ctx.units->push_back(it->unit);
			}
			else
			{
				// jednostka opu�ci�a podziemia
				if(it->unit->event_handler)
					it->unit->event_handler->HandleUnitEvent(UnitEventHandler::LEAVE, it->unit);
				it->unit->to_remove = true;
				to_remove.push_back(it->unit);
				if(IsOnline())
					Net_RemoveUnit(it->unit);
			}
		}
		else if(it->where == -2)
		{
			// na arene
			InsideBuilding& building = *GetArena();
			RemoveElement(GetContext(*it->unit).units, it->unit);
			it->unit->in_building = building.ctx.building_id;
			VEC3 pos;
			if(!WarpToArea(building.ctx, (it->unit->in_arena == 0 ? building.arena1 : building.arena2), it->unit->GetUnitRadius(), pos, 20))
			{
				// nie uda�o si�, wrzu� go z areny
				it->unit->in_building = -1;
				WarpUnit(*it->unit, building.outside_spawn);
				it->unit->rot = building.outside_rot;
				local_ctx.units->push_back(it->unit);
				RemoveElement(at_arena, it->unit);
			}
			else
			{
				WarpUnit(*it->unit, pos);
				it->unit->rot = (it->unit->in_arena == 0 ? PI : 0);
				building.units.push_back(it->unit);

				warped_to_arena = true;
			}
		}
		else
		{
			// wejd� do budynku
			InsideBuilding& building = *city_ctx->inside_buildings[it->where];
			if(it->unit->in_building == -1)
				RemoveElement(local_ctx.units, it->unit);
			else
				RemoveElement(city_ctx->inside_buildings[it->unit->in_building]->units, it->unit);
			it->unit->in_building = it->where;
			WarpUnit(*it->unit, building.inside_spawn);
			it->unit->rot = PI;
			building.units.push_back(it->unit);
		}

		if(it->unit == pc->unit)
		{
			first_frame = true;
			rot_buf = 0.f;

			if(fallback_co == FALLBACK_ARENA)
			{
				pc->unit->frozen = 1;
				fallback_co = FALLBACK_ARENA2;
			}
			else if(fallback_co == FALLBACK_ARENA_EXIT)
			{
				pc->unit->frozen = 0;
				fallback_co = FALLBACK_NONE;
			}
		}
	}

	unit_warp_data.clear();

	if(warped_to_arena)
	{
		VEC3 pt1(0,0,0), pt2(0,0,0);
		int count1=0, count2=0;

		for(vector<Unit*>::iterator it = at_arena.begin(), end = at_arena.end(); it != end; ++it)
		{
			if((*it)->in_arena == 0)
			{
				pt1 += (*it)->pos;
				++count1;
			}
			else
			{
				pt2 += (*it)->pos;
				++count2;
			}
		}

		pt1 /= (float)count1;
		pt2 /= (float)count2;

		for(vector<Unit*>::iterator it = at_arena.begin(), end = at_arena.end(); it != end; ++it)
			(*it)->rot = lookat_angle((*it)->pos, (*it)->in_arena == 0 ? pt2 : pt1);
	}
}

void Game::ApplyContext(ILevel* level, LevelContext& ctx)
{
	assert(level);

	level->ApplyContext(ctx);
	if(ctx.require_tmp_ctx && !ctx.tmp_ctx)
		ctx.SetTmpCtx(tmp_ctx_pool.Get());
}

void Game::UpdateContext(LevelContext& ctx, float dt)
{
	// aktualizuj postacie
	UpdateUnits(ctx, dt);

	if(ctx.lights)
		UpdateLights(*ctx.lights);

	// aktualizuj skrzynie
	if(ctx.chests && !ctx.chests->empty())
	{
		for(vector<Chest*>::iterator it = ctx.chests->begin(), end = ctx.chests->end(); it != end; ++it)
			(*it)->ani->Update(dt);
	}

	// aktualizuj drzwi
	if(ctx.doors && !ctx.doors->empty())
	{
		for(vector<Door*>::iterator it = ctx.doors->begin(), end = ctx.doors->end(); it != end; ++it)
		{
			Door& door = **it;
			door.ani->Update(dt);
			if(door.state == Door::Opening || door.state == Door::Opening2)
			{
				if(door.state == Door::Opening)
				{
					if(door.ani->frame_end_info || door.ani->GetProgress() >= 0.25f)
					{
						door.state = Door::Opening2;
						btVector3& pos = door.phy->getWorldTransform().getOrigin();
						pos.setY(pos.y() - 100.f);
					}
				}
				if(door.ani->frame_end_info)
					door.state = Door::Open;
			}
			else if(door.state == Door::Closing || door.state == Door::Closing2)
			{
				if(door.state == Door::Closing)
				{
					if(door.ani->frame_end_info || door.ani->GetProgress() <= 0.25f)
					{
						bool blocking = false;

						for(vector<Unit*>::iterator it = ctx.units->begin(), end = ctx.units->end(); it != end; ++it)
						{
							if((*it)->IsAlive() && CircleToRotatedRectangle((*it)->pos.x, (*it)->pos.z, (*it)->GetUnitRadius(), door.pos.x, door.pos.z, 0.842f, 0.181f, door.rot))
							{
								blocking = true;
								break;
							}
						}

						if(!blocking)
						{
							door.state = Door::Closing2;
							btVector3& pos = door.phy->getWorldTransform().getOrigin();
							pos.setY(pos.y() + 100.f);
						}
					}
				}
				if(door.ani->frame_end_info)
				{
					if(door.state == Door::Closing2)
						door.state = Door::Closed;
					else
					{
						// nie mo�na zamkna� drzwi bo co� blokuje
						door.state = Door::Opening2;
						door.ani->Play(&door.ani->ani->anims[0], PLAY_ONCE|PLAY_NO_BLEND|PLAY_STOP_AT_END, 0);
						door.ani->frame_end_info = false;
						// mo�na by da� lepszy punkt d�wi�ku
						if(sound_volume)
							PlaySound3d(sDoorBudge, door.pos, 2.f, 5.f);
					}
				}
			}
		}
	}

	// aktualizuj pu�apki
	if(ctx.traps && !ctx.traps->empty())
		UpdateTraps(ctx, dt);

	// aktualizuj pociski i efekty czar�w
	UpdateBullets(ctx, dt);
	UpdateElectros(ctx, dt);
	UpdateExplosions(ctx, dt);
	UpdateParticles(ctx, dt);
	UpdateDrains(ctx, dt);

	// aktualizuj krew
	for(vector<Blood>::iterator it = ctx.bloods->begin(), end = ctx.bloods->end(); it != end; ++it)
	{
		it->size += dt;
		if(it->size >= 1.f)
			it->size = 1.f;
	}
}

SPAWN_GROUP Game::RandomSpawnGroup(const BaseLocation& base)
{
	int n = rand2()%100;
	int k = base.schance1;
	SPAWN_GROUP sg = SG_LOSOWO;
	if(base.schance1 > 0 && n < k)
		sg = base.sg1;
	else
	{
		k += base.schance2;
		if(base.schance2 > 0 && n < k)
			sg = base.sg2;
		else
		{
			k += base.schance3;
			if(base.schance3 > 0 && n < k)
				sg = base.sg3;
		}
	}

	if(sg == SG_LOSOWO)
	{
		switch(rand2()%10)
		{
		case 0:
		case 1:
			return SG_GOBLINY;
		case 2:
		case 3:
			return SG_ORKOWIE;
		case 4:
		case 5:
			return SG_BANDYCI;
		case 6:
			return SG_MAGOWIE;
		case 7:
			return SG_NEKRO;
		case 8:
			return SG_ZLO;
		case 9:
			return SG_BRAK;
		default:
			assert(0);
			return SG_BRAK;
		}
	}
	else
		return sg;
}

int Game::GetDungeonLevel()
{
	if(location->outside)
		return location->st;
	else
	{
		InsideLocation* inside = (InsideLocation*)location;
		if(inside->IsMultilevel())
			return (int)lerp(max(3.f, float(inside->st)/2), float(inside->st), float(dungeon_level)/(((MultiInsideLocation*)inside)->levels.size()-1));
		else
			return inside->st;
	}
}

int Game::GetDungeonLevelChest()
{
	int level = GetDungeonLevel();
	if(location->spawn == SG_BRAK)
	{
		if(level > 10)
			return 3;
		else if(level > 5)
			return 2;
		else
			return 1;
	}
	else
		return level;
}

void Game::PlayHitSound(MATERIAL_TYPE mat2, MATERIAL_TYPE mat, const VEC3& hitpoint, float range, bool dmg)
{
	if(sound_volume)
	{
		PlaySound3d(GetMaterialSound(mat2, mat), hitpoint, range, 10.f);

		if(mat != MAT_BODY && dmg)
			PlaySound3d(GetMaterialSound(mat2, MAT_BODY), hitpoint, range, 10.f);
	}

	if(IsOnline())
	{
		NetChange& c = Add1(net_changes);
		c.type = NetChange::HIT_SOUND;
		c.pos = hitpoint;
		c.id = mat2;
		c.ile = mat;

		if(mat != MAT_BODY && dmg)
		{
			NetChange& c2 = Add1(net_changes);
			c2.type = NetChange::HIT_SOUND;
			c2.pos = hitpoint;
			c2.id = mat2;
			c2.ile = MAT_BODY; 
		}
	}
}

void Game::LoadingStart(int steps)
{
	loading_t.Reset();
	loading_dt = 0.f;
	loading_steps = steps;
	loading_index = 0;
	clear_color = BLACK;
	game_state = GS_LOAD;
	load_screen->visible = true;
	main_menu->visible = false;
	game_gui_container->visible = false;
	game_messages->visible = false;
}

void Game::LoadingStep(cstring text)
{
	++loading_index;
	loading_dt += loading_t.Tick();
	if(loading_dt >= 1.f/30 || loading_index == loading_steps)
	{
		loading_dt = 0.f;
		load_game_progress = float(loading_index) / loading_steps;
		if(text)
			load_game_text = text;
		DoPseudotick();
		loading_t.Tick();
	}
}

cstring arena_slabi[] = {
	"orcs",
	"goblins",
	"undead",
	"bandits",
	"cave_wolfs",
	"cave_spiders"
};

cstring arena_sredni[] = {
	"orcs",
	"goblins",
	"undead",
	"evils",
	"bandits",
	"mages",
	"golems",
	"mages_n_golems",
	"hunters"
};

cstring arena_silni[] = {
	"orcs",
	"necros",
	"evils",
	"mages",
	"golems",
	"mages_n_golems",
	"crazies"
};

void Game::StartArenaCombat(int level)
{
	assert(in_range(level, 1, 3));

	int ile, lvl;

	switch(rand2()%5)
	{
	case 0:
		ile = 1;
		lvl = level*5+1;
		break;
	case 1:
		ile = 1;
		lvl = level*5;
		break;
	case 2:
		ile = 2;
		lvl = level*5-1;
		break;
	case 3:
		ile = 2;
		lvl = level*5-2;
		break;
	case 4:
		ile = 3;
		lvl = level*5-3;
		break;
	}

	arena_free = false;
	arena_tryb = Arena_Walka;
	arena_etap = Arena_OdliczanieDoPrzeniesienia;
	arena_t = 0.f;
	arena_poziom = level;
	city_ctx->arena_czas = worldtime;
	at_arena.clear();

	// dodaj gracza na aren�
	if(current_dialog->is_local)
	{
		fallback_co = FALLBACK_ARENA;
		fallback_t = -1.f;
	}
	else
	{
		NetChangePlayer& c = Add1(net_changes_player);
		c.type = NetChangePlayer::ENTER_ARENA;
		c.pc = current_dialog->pc;
		current_dialog->pc->arena_fights++;
		GetPlayerInfo(current_dialog->pc).NeedUpdate();
	}

	current_dialog->pc->unit->frozen = 2;
	current_dialog->pc->unit->in_arena = 0;
	at_arena.push_back(current_dialog->pc->unit);

	if(IsOnline())
	{
		NetChange& c = Add1(net_changes);
		c.type = NetChange::CHANGE_ARENA_STATE;
		c.unit = current_dialog->pc->unit;
	}

	for(vector<Unit*>::iterator it = team.begin(), end = team.end(); it != end; ++it)
	{
		if((*it)->frozen || distance2d((*it)->pos, city_ctx->arena_pos) > 5.f)
			continue;
		if((*it)->IsPlayer())
		{
			if((*it)->frozen == 0)
			{
				BreakPlayerAction((*it)->player);

				(*it)->frozen = 2;
				(*it)->in_arena = 0;
				at_arena.push_back(*it);

				(*it)->player->arena_fights++;
				if(IsOnline())
					(*it)->player->stat_flags |= STAT_ARENA_FIGHTS;

				if((*it)->player == pc)
				{
					fallback_co = FALLBACK_ARENA;
					fallback_t = -1.f;
				}
				else
				{
					NetChangePlayer& c = Add1(net_changes_player);
					c.type = NetChangePlayer::ENTER_ARENA;
					c.pc = (*it)->player;
					GetPlayerInfo(c.pc).NeedUpdate();
				}

				NetChange& c = Add1(net_changes);
				c.type = NetChange::CHANGE_ARENA_STATE;
				c.unit = *it;
			}
		}
		else if((*it)->IsHero() && (*it)->CanFollow())
		{
			(*it)->frozen = 2;
			(*it)->in_arena = 0;
			(*it)->hero->following = current_dialog->pc->unit;
			at_arena.push_back(*it);

			if(IsOnline())
			{
				NetChange& c = Add1(net_changes);
				c.type = NetChange::CHANGE_ARENA_STATE;
				c.unit = *it;
			}
		}
	}

	if(at_arena.size() > 3)
	{
		lvl += (at_arena.size()-3)/2+1;
		while(lvl > level*5+2)
		{
			lvl -= 2;
			++ile;
		}
	}

	cstring grupa;
	switch(level)
	{
	default:
	case 1:
		grupa = arena_slabi[rand2()%countof(arena_slabi)];
		SpawnArenaViewers(1);
		break;
	case 2:
		grupa = arena_sredni[rand2()%countof(arena_sredni)];
		SpawnArenaViewers(3);
		break;
	case 3:
		grupa = arena_silni[rand2()%countof(arena_silni)];
		SpawnArenaViewers(5);
		break;
	}

	EnemyGroup& g = FindEnemyGroup(grupa);
	EnemyGroupPart part;
	part.total = 0;
	for(int i=0; i<g.count; ++i)
	{
		if(lvl >= g.enemies[i].unit->level.x)
		{
			EnemyEntry& ee = Add1(part.enemies);
			ee.unit = g.enemies[i].unit;
			ee.count = g.enemies[i].count;
			if(lvl < g.enemies[i].unit->level.y)
				ee.count = max(1, ee.count/2);
			part.total += ee.count;
		}
	}

	InsideBuilding* arena = GetArena();

	for(int i=0; i<ile; ++i)
	{
		int x = rand2()%part.total, y = 0;
		for(int j=0; j<g.count; ++j)
		{
			y += part.enemies[j].count;
			if(x < y)
			{
				Unit* u = SpawnUnitInsideArea(arena->ctx, arena->arena2, *part.enemies[j].unit, lvl);
				u->rot = 0.f;
				u->in_arena = 1;
				u->frozen = 2;
				at_arena.push_back(u);

				if(IsOnline())
					Net_SpawnUnit(u);

				break;
			}
		}
	}
}

Unit* Game::SpawnUnitInsideArea(LevelContext& ctx, const BOX2D& area, UnitData& unit, int level)
{
	VEC3 pos;

	if(!WarpToArea(ctx, area, unit.GetRadius(), pos))
		return NULL;

	return CreateUnitWithAI(ctx, unit, level, NULL, &pos);
}

bool Game::WarpToArea(LevelContext& ctx, const BOX2D& area, float radius, VEC3& pos, int tries)
{
	for(int i=0; i<tries; ++i)
	{
		pos = area.GetRandomPos3();

		global_col.clear();
		GatherCollisionObjects(ctx, global_col, pos, radius, NULL);

		if(!Collide(global_col, pos, radius))
			return true;
	}

	return false;
}

void Game::DeleteUnit(Unit* unit)
{
	assert(unit);

	RemoveElement(GetContext(*unit).units, unit);
	for(vector<UnitView>::iterator it = unit_views.begin(), end = unit_views.end(); it != end; ++it)
	{
		if(it->unit == unit)
		{
			unit_views.erase(it);
			break;
		}
	}
	if(before_player == BP_UNIT && before_player_ptr.unit == unit)
		before_player = BP_NONE;
	if(unit == selected_target)
		selected_target = NULL;
	if(unit == selected_unit)
		selected_unit = NULL;

	if(unit->bubble)
		unit->bubble->unit = NULL;

	if(unit->bow_instance)
		bow_instances.push_back(unit->bow_instance);

	if(unit->ai)
	{
		RemoveElement(ais, unit->ai);
		delete unit->ai;
	}
	delete unit->cobj->getCollisionShape();
	phy_world->removeCollisionObject(unit->cobj);
	delete unit->cobj;
	delete unit;
}

void Game::DialogTalk(DialogContext& ctx, cstring msg)
{
	assert(msg);

	ctx.dialog_text = msg;
	ctx.dialog_wait = 1.f+float(strlen(ctx.dialog_text))/20;

	int ani;
	if(!ctx.talker->useable && ctx.talker->type == Unit::HUMAN && rand2()%3 != 0)
	{
		ani = rand2()%2+1;
		ctx.talker->ani->Play(ani == 1 ? "i_co" : "pokazuje", PLAY_ONCE|PLAY_PRIO2, 0);
		ctx.talker->animacja = ANI_ODTWORZ;
		ctx.talker->action = A_ANIMATION;
	}
	else
		ani = 0;

	game_gui->AddSpeechBubble(ctx.talker, ctx.dialog_text);

	if(IsOnline())
	{
		NetChange& c = Add1(net_changes);
		c.type = NetChange::TALK;
		c.unit = ctx.talker;
		c.str = StringPool.Get();
		*c.str = msg;
		c.id = ani;
		c.ile = skip_id_counter++;
		ctx.skip_id = c.ile;
		net_talk.push_back(c.str);
	}
}

void Game::CreateForestMinimap()
{
	D3DLOCKED_RECT lock;
	tMinimap->LockRect(0, &lock, NULL, 0);

	OutsideLocation* loc = (OutsideLocation*)location;

	for(int y=0; y<OutsideLocation::size; ++y)
	{
		DWORD* pix = (DWORD*)(((byte*)lock.pBits)+lock.Pitch*y);
		for(int x=0; x<OutsideLocation::size; ++x)
		{
			TERRAIN_TILE t = loc->tiles[x+(OutsideLocation::size-1-y)*OutsideLocation::size].t;
			DWORD col;
			if(t == TT_GRASS)
				col = COLOR_RGB(0,128,0);
			else if(t == TT_ROAD)
				col = COLOR_RGB(128,128,128);
			else if(t == TT_SAND)
				col = COLOR_RGB(128,128,64);
			else if(t == TT_GRASS2)
				col = COLOR_RGB(105,128,89);
			else if(t == TT_GRASS3)
				col = COLOR_RGB(127,51,0);
			else
				col = COLOR_RGB(255,0,0);
			if(x < 16 || x > 128-16 || y < 16 || y > 128-16)
			{
				col = ((col & 0xFF) / 2) |
					  ((((col & 0xFF00) >> 8) / 2) << 8) |
					  ((((col & 0xFF0000) >> 16) / 2) << 16) |
					  0xFF000000;
			}

			*pix = col;
			++pix;
		}
	}

	tMinimap->UnlockRect(0);

	minimap->minimap_size = OutsideLocation::size;
}

void Game::SpawnOutsideBariers()
{
	const float size = 256.f;
	const float size2 = size/2;
	const float border = 32.f;
	const float border2 = border/2;

	// g�ra
	{
		CollisionObject& cobj = Add1(local_ctx.colliders);
		cobj.type = CollisionObject::RECTANGLE;
		cobj.pt = VEC2(size2, border2);
		cobj.w = size2;
		cobj.h = border2;
	}

	// d�
	{
		CollisionObject& cobj = Add1(local_ctx.colliders);
		cobj.type = CollisionObject::RECTANGLE;
		cobj.pt = VEC2(size2, size-border2);
		cobj.w = size2;
		cobj.h = border2;
	}

	// lewa
	{
		CollisionObject& cobj = Add1(local_ctx.colliders);
		cobj.type = CollisionObject::RECTANGLE;
		cobj.pt = VEC2(border2, size2);
		cobj.w = border2;
		cobj.h = size2;
	}

	// prawa
	{
		CollisionObject& cobj = Add1(local_ctx.colliders);
		cobj.type = CollisionObject::RECTANGLE;
		cobj.pt = VEC2(size-border2, size2);
		cobj.w = border2;
		cobj.h = size2;
	}
}

bool Game::HaveTeamMember()
{
	return GetActiveTeamSize() > 1;
}

bool Game::HaveTeamMemberNPC()
{
	if(GetActiveTeamSize() < 2)
		return false;
	for(vector<Unit*>::iterator it = active_team.begin(), end = active_team.end(); it != end; ++it)
	{
		if(!(*it)->IsPlayer())
			return true;
	}
	return false;
}

bool Game::HaveTeamMemberPC()
{
	if(!IsOnline())
		return false;
	if(GetActiveTeamSize() < 2)
		return false;
	for(vector<Unit*>::iterator it = active_team.begin(), end = active_team.end(); it != end; ++it)
	{
		if((*it)->IsPlayer() && *it != pc->unit)
			return true;
	}
	return false;
}

bool Game::IsTeamMember(Unit& unit)
{
	if(unit.IsPlayer())
		return true;
	else if(unit.IsHero())
		return unit.hero->team_member;
	else
		return false;
}

inline int& GetCredit(Unit& u)
{
	if(u.IsPlayer())
		return u.player->kredyt;
	else
	{
		assert(u.IsFollower());
		return u.hero->kredyt;
	}
}

int Game::GetPCShare()
{
	int ile_pc = 0, ile_npc = 0;
	for(vector<Unit*>::iterator it = active_team.begin(), end = active_team.end(); it != end; ++it)
	{
		if((*it)->IsPlayer())
			++ile_pc;
		else if(!(*it)->hero->free)
			++ile_npc;
	}
	int r = 100 - ile_npc*10;
	return r/ile_pc;
}

int Game::GetPCShare(int ile_pc, int ile_npc)
{
	int r = 100 - ile_npc*10;
	return r/ile_pc;
}

VEC2 Game::GetMapPosition(Unit& unit)
{
	if(unit.in_building == -1)
		return VEC2(unit.pos.x, unit.pos.z);
	else
	{
		BUILDING type = city_ctx->inside_buildings[unit.in_building]->type;
		for(vector<CityBuilding>::iterator it = city_ctx->buildings.begin(), end = city_ctx->buildings.end(); it != end; ++it)
		{
			if(it->type == type)
				return VEC2(float(it->pt.x*2), float(it->pt.y*2));
		}
	}
	return VEC2(-1000,-1000);
}

//=============================================================================
// Rozdziela z�oto pomi�dzy cz�onk�w dru�yny
//=============================================================================
void Game::AddGold(int ile, vector<Unit*>* units, bool show, cstring msg, float time, bool defmsg)
{
	if(!units)
		units = &active_team;

	// reszta z poprzednich podzia��w, dodaje si� tylko jak jest wi�ksza ni� 10 bo inaczej npc nic nie dostaje przy ma�ych kwotach
	int a = team_gold/10;
	if(a)
	{
		ile += a*10;
		team_gold -= a*10;
	}

	if(units->size() == 1)
	{
		Unit& u = *(*units)[0];
		u.gold += ile;
		if(show && u.IsPlayer())
		{
			if(&u == pc->unit)
				AddGameMsg(Format(msg, ile), time);
			else
			{
				NetChangePlayer& c = Add1(net_changes_player);
				c.type = NetChangePlayer::GOLD_MSG;
				c.id = (defmsg ? 1 : 0);
				c.ile = ile;
				c.pc = u.player;
				GetPlayerInfo(u.player->id).update_flags |= (PlayerInfo::UF_NET_CHANGES | PlayerInfo::UF_GOLD);
			}
		}
		else if(!u.IsPlayer() && u.busy == Unit::Busy_Trading)
		{
			Unit* trader = FindPlayerTradingWithUnit(u);
			if(trader != pc->unit)
			{
				NetChangePlayer& c = Add1(net_changes_player);
				c.type = NetChangePlayer::UPDATE_TRADER_GOLD;
				c.pc = trader->player;
				c.id = u.netid;
				c.ile = u.gold;
				GetPlayerInfo(c.pc).NeedUpdate();
			}
		}
		return;
	}

	int kasa = 0, ile_pc = 0, ile_npc = 0;
	bool credit_info = false;

	for(vector<Unit*>::iterator it = units->begin(), end = units->end(); it != end; ++it)
	{
		Unit& u = **it;
		if(u.IsPlayer())
		{
			++ile_pc;
			u.player->na_kredycie = false;
			u.player->gold_get = 0;
		}
		else
		{
			++ile_npc;
			u.hero->na_kredycie = false;
			u.hero->gained_gold = false;
		}
	}

	for(int i=0; i<2 && ile > 0; ++i)
	{
		int pc_share, npc_share;
		if(ile_pc > 0)
		{
			pc_share = GetPCShare(ile_pc, ile_npc),
			npc_share = 10;
		}
		else
			npc_share = 100/ile_npc;

		for(vector<Unit*>::iterator it = units->begin(), end = units->end(); it != end; ++it)
		{
			Unit& u = **it;
			int& kredyt = (u.IsPlayer() ? u.player->kredyt : u.hero->kredyt);
			bool& na_kredycie = (u.IsPlayer() ? u.player->na_kredycie : u.hero->na_kredycie);
			if(na_kredycie)
				continue;

			int zysk = ile * (u.IsHero() ? npc_share : pc_share) / 100;
			if(kredyt > zysk)
			{
				credit_info = true;
				kredyt -= zysk;
				na_kredycie = true;
				if(u.IsPlayer())
					--ile_pc;
				else
					--ile_npc;
			}
			else if(kredyt)
			{
				credit_info = true;
				zysk -= kredyt;
				kredyt = 0;
				u.gold += zysk;
				kasa += zysk;
				if(u.IsPlayer())
					u.player->gold_get += zysk;
				else
					u.hero->gained_gold = true;
			}
			else
			{
				u.gold += zysk;
				kasa += zysk;
				if(u.IsPlayer())
					u.player->gold_get += zysk;
				else
					u.hero->gained_gold = true;
			}
		}

		ile -= kasa;
		kasa = 0;
	}

	team_gold += ile;
	assert(team_gold >= 0);

	if(IsOnline())
	{
		for(vector<Unit*>::iterator it = units->begin(), end = units->end(); it != end; ++it)
		{
			Unit& u = **it;
			if(u.IsPlayer())
			{
				if(u.player != pc)
				{
					if(u.player->gold_get)
					{
						GetPlayerInfo(u.player->id).update_flags |= PlayerInfo::UF_GOLD;
						if(show)
						{
							NetChangePlayer& c = Add1(net_changes_player);
							c.type = NetChangePlayer::GOLD_MSG;
							c.id = (defmsg ? 1 : 0);
							c.ile = u.player->gold_get;
							c.pc = u.player;
							GetPlayerInfo(c.pc).NeedUpdate();
						}
					}
				}
				else
				{
					if(show)
						AddGameMsg(Format(msg, pc->gold_get), time);
				}
			}
			else if(u.hero->gained_gold && u.busy == Unit::Busy_Trading)
			{
				Unit* trader = FindPlayerTradingWithUnit(u);
				if(trader != pc->unit)
				{
					NetChangePlayer& c = Add1(net_changes_player);
					c.type = NetChangePlayer::UPDATE_TRADER_GOLD;
					c.pc = trader->player;
					c.id = u.netid;
					c.ile = u.gold;
					GetPlayerInfo(c.pc).NeedUpdate();
				}
			}
		}

		if(credit_info)
			PushNetChange(NetChange::UPDATE_CREDIT);
	}
	else if(show)
		AddGameMsg(Format(msg, pc->gold_get), time);
}

int Game::CalculateQuestReward(int gold)
{
	return gold * (90 + active_team.size()*10) / 100;
}

void Game::AddGoldArena(int ile)
{
	vector<Unit*> v;
	for(vector<Unit*>::iterator it = at_arena.begin(), end = at_arena.end(); it != end; ++it)
	{
		if((*it)->in_arena == 0)
			v.push_back(*it);
	}

	AddGold(ile * (85 + v.size()*15) / 100, &v, true);
}

void Game::EventTakeItem(int id)
{
	if(id == BUTTON_YES)
	{
		ItemSlot& slot = pc->unit->items[take_item_id];
		pc->kredyt += slot.item->value/2;
		slot.team_count = 0;

		if(IsLocal())
			CheckCredit(true);
		else
		{
			NetChange& c = Add1(net_changes);
			c.type = NetChange::TAKE_ITEM_CREDIT;
			c.id = take_item_id;
		}
	}
}

// powinno sortowa� w takiej kolejno�ci:
// najlepsza bro�, gorsza bro�, najgorsza bro�, najlepszy �uk, �reni �uk, najgorszy �uk, najlepszy pancerz, �redni pancerz, najgorszy pancerz, najlepsza tarcza,
//	�rednia tarcza, najgorsza tarcza
const int item_type_prio[4] = {
	0, // IT_WEAPON
	1, // IT_BOW
	3, // IT_SHIELD
	2  // IT_ARMOR
};

struct SortTeamShares
{
	Unit* unit;

	SortTeamShares(Unit* unit) : unit(unit)
	{

	}

	bool operator () (const TeamShareItem& t1, const TeamShareItem& t2) const
	{
		if(t1.item->type == t2.item->type)
			return t1.value > t2.value;
		else
			return item_type_prio[t1.item->type] < item_type_prio[t2.item->type];
	}
};

bool UniqueTeamShares(const TeamShareItem& t1, const TeamShareItem& t2)
{
	return t1.to == t2.to && t1.from == t2.from && t1.item == t2.item;
}

void Game::CheckTeamItems()
{
	if(!HaveTeamMemberNPC())
	{
		team_share_id = -1;
		return;
	}

	team_shares.clear();
	uint pos_a, pos_b;

	for(vector<Unit*>::iterator it = active_team.begin(), end = active_team.end(); it != end; ++it)
	{
		if((*it)->IsPlayer())
			continue;
		pos_a = team_shares.size();
		for(vector<Unit*>::iterator it2 = active_team.begin(), end2 = active_team.end(); it2 != end2; ++it2)
		{
			if(*it == *it2)
				continue;
			int index = 0;
			for(vector<ItemSlot>::iterator it3 = (*it2)->items.begin(), end3 = (*it2)->items.end(); it3 != end3; ++it3, ++index)
			{
				if(it3->item && it3->item->IsWearable())
				{
					if(it3->team_count == 0 && it3->item->value/2 > (*it)->gold)
						continue;

					int value;
					if(IsBetterItem(**it, it3->item, &value))
					{
						TeamShareItem& tsi = Add1(team_shares);
						tsi.from = *it2;
						tsi.to = *it;
						tsi.item = it3->item;
						tsi.index = index;
						tsi.value = value;
					}
				}
			}
		}
		pos_b = team_shares.size();
		if(pos_b - pos_a > 1)
		{
			std::vector<TeamShareItem>::iterator it2 = std::unique(team_shares.begin()+pos_a, team_shares.end(), UniqueTeamShares);
			team_shares.resize(std::distance(team_shares.begin(), it2));
			std::sort(team_shares.begin()+pos_a, team_shares.end(), SortTeamShares(*it));
		}
	}

	if(team_shares.empty())
		team_share_id = -1;
	else
		team_share_id = 0;
}

bool Game::IsBetterItem(Unit& unit, const Item* item, int* value)
{
	assert(item && item->IsWearable());

	switch(item->type)
	{
	case IT_WEAPON:
		if(!IS_SET(unit.data->flagi, F_MAG))
			return unit.IsBetterWeapon(item->ToWeapon(), value);
		else 
		{
			if(IS_SET(item->flags, ITEM_MAGE) && item->value > unit.GetWeapon().value)
			{
				if(value)
					*value = item->value;
				return true;
			}
			else
				return false;
		}
	case IT_BOW:
		if(!unit.HaveBow())
		{
			if(value)
				*value = 100;
			return true;
		}
		else
		{
			if(unit.GetBow().value < item->value)
			{
				if(value)
					*value = item->value;
				return true;
			}
			else
				return false;
		}
	case IT_ARMOR:
		if(!IS_SET(unit.data->flagi, F_MAG))
			return unit.IsBetterArmor(item->ToArmor(), value);
		else
		{
			if(IS_SET(item->flags, ITEM_MAGE) && item->value > unit.GetArmor().value)
			{
				if(value)
					*value = item->value;
				return true;
			}
			else
				return false;
		}
	case IT_SHIELD:
		if(!unit.HaveShield())
		{
			if(value)
				*value = 100;
			return true;
		}
		else
		{
			if(unit.GetShield().value < item->value)
			{
				if(value)
					*value = item->value;
				return true;
			}
			else
				return false;
		}
	default:
		assert(0);
		return false;
	}
}

void Game::BuyTeamItems()
{
	const Item* hp1 = FindItem("potion_smallheal");
	const Item* hp2 = FindItem("potion_mediumheal");
	const Item* hp3 = FindItem("potion_bigheal");

	for(vector<Unit*>::iterator it = active_team.begin(), end = active_team.end(); it != end; ++it)
	{
		Unit& u = **it;
		if(u.IsPlayer())
			continue;

		// sprzedaj stare przedmioty, policz miksturki
		int ile_hp=0, ile_hp2=0, ile_hp3=0;
		for(vector<ItemSlot>::iterator it2 = u.items.begin(), end2 = u.items.end(); it2 != end2;)
		{
			assert(it2->item);
			if(it2->item->type == IT_CONSUMEABLE)
			{
				if(it2->item == hp1)
					ile_hp += it2->count;
				else if(it2->item == hp2)
					ile_hp2 += it2->count;
				else if(it2->item == hp3)
					ile_hp3 += it2->count;
				++it2;
			}
			else if(it2->item->IsWearable() && it2->team_count == 0)
			{
				u.weight -= it2->item->weight;
				u.gold += it2->item->value/2;
				if(it2+1 == end2)
				{
					u.items.pop_back();
					break;
				}
				else
				{
					it2 = u.items.erase(it2);
					end2 = u.items.end();
				}
			}
			else
				++it2;
		}

		// kup miksturki
		int p1, p2, p3;
		if(u.level < 4)
		{
			p1 = 5;
			p2 = 0;
			p3 = 0;
		}
		else if(u.level < 8)
		{
			p1 = 5;
			p2 = 2;
			p3 = 0;
		}
		else if(u.level < 12)
		{
			p1 = 6;
			p2 = 3;
			p3 = 1;
		}
		else if(u.level < 16)
		{
			p1 = 6;
			p2 = 4;
			p3 = 2;
		}
		else
		{
			p1 = 6;
			p2 = 5;
			p3 = 4;
		}

		while(ile_hp3 < p3 && u.gold >= hp3->value/2)
		{
			u.AddItem(hp3, 1, false);
			u.gold -= hp3->value/2;
			++ile_hp3;
		}
		while(ile_hp2 < p2 && (*it)->gold >= hp2->value/2)
		{
			u.AddItem(hp2, 1, false);
			u.gold -= hp2->value/2;
			++ile_hp2;
		}
		while(ile_hp < p1 && (*it)->gold >= hp1->value/2)
		{
			u.AddItem(hp1, 1, false);
			u.gold -= hp1->value/2;
			++ile_hp;
		}

		// darmowe miksturki dla biedak�w
		int ile = p1/2 - ile_hp;
		if(ile > 0)
			u.AddItem(hp1, (uint)ile, false);
		ile = p2/2 - ile_hp2;
		if(ile > 0)
			u.AddItem(hp2, (uint)ile, false);
		ile = p3/2 - ile_hp3;
		if(ile > 0)
			u.AddItem(hp3, (uint)ile, false);

		// kup przedmioty
		// kup bro�
		if(!u.HaveWeapon())
		{
			if(IS_SET(u.data->flagi, F_MAG))
				u.AddItem(FindItem("wand_1"), 1, false);
			else
			{
				cstring bron[] = {"dagger_short", "sword_long", "axe_small", "blunt_club"};
				u.AddItem(FindItem(bron[rand2()%countof(bron)]), 1, false);
			}
		}
		else
		{
			const Item* item = GetBetterItem(&u.GetWeapon());
			if(item && u.gold >= item->value && u.IsBetterWeapon(item->ToWeapon()))
			{
				u.AddItem(item, 1, false);
				u.gold -= item->value;
			}
		}

		// kup �uk
		const Item* item;
		if(!u.HaveBow())
			item = FindItem("bow_short");
		else
			item = GetBetterItem(&u.GetBow());
		if(item && u.gold >= item->value)
		{
			u.AddItem(item, 1, false);
			u.gold -= item->value;
		}

		// kup pancerz
		if(!u.HaveArmor())
		{
			if(IS_SET(u.data->flagi, F_MAG))
				item = FindItem("armor_mage_1");
			else if(u.skill[(int)Skill::LIGHT_ARMOR] > u.skill[(int)Skill::HEAVY_ARMOR])
				item = FindItem("armor_leather");
			else
				item = FindItem("armor_chainmail");
		}
		else
			item = GetBetterItem(&u.GetArmor());
		if(item && u.gold >= item->value && u.IsBetterArmor(item->ToArmor()))
		{
			u.AddItem(item, 1, false);
			u.gold -= item->value;
		}

		// kup tarcze
		if(!u.HaveShield())
			item = FindItem("shield_wood");
		else
			item = GetBetterItem(&u.GetShield());
		if(item && u.gold >= item->value)
		{
			u.AddItem(item, 1, false);
			u.gold -= item->value;
		}

		// za�� nowe przedmioty
		UpdateUnitInventory(u);
		u.ai->have_potion = 2;

		// sprzedaj stare przedmioty
		for(vector<ItemSlot>::iterator it2 = u.items.begin(), end2 = u.items.end(); it2 != end2;)
		{
			if(it2->item && it2->item->type != IT_CONSUMEABLE && it2->item->IsWearable() && it2->team_count == 0)
			{
				u.weight -= it2->item->weight;
				u.gold += it2->item->value/2;
				if(it2+1 == end2)
				{
					u.items.pop_back();
					break;
				}
				else
				{
					it2 = u.items.erase(it2);
					end2 = u.items.end();
				}
			}
			else
				++it2;
		}
	}

	// kupowanie miksturek przez starego maga
	if(magowie_uczony && (magowie_stan == MS_MAG_ZREKRUTOWANY || magowie_stan == MS_STARY_MAG_DOLACZYL || magowie_stan == MS_PRZYPOMNIAL_SOBIE || magowie_stan == MS_KUP_MIKSTURE))
	{
		int ile = max(0, 3-magowie_uczony->CountItem(hp2));
		if(ile)
		{
			magowie_uczony->AddItem(hp2, ile, false);
			magowie_uczony->ai->have_potion = 2;
		}
	}

	// kupowanie miksturek przez gorusha
	if(orkowie_gorush && (orkowie_stan == OS_ORK_DOLACZYL || orkowie_stan >= OS_UKONCZONO_DOLACZYL))
	{
		int ile1, ile2;
		switch(orkowie_klasa)
		{
		case GORUSH_BRAK:
			ile1 = 6;
			ile2 = 0;
			break;
		case GORUSH_SZAMAN:
			ile1 = 6;
			ile2 = 1;
			break;
		case GORUSH_LOWCA:
			ile1 = 6;
			ile2 = 2;
			break;
		case GORUSH_WOJ:
			ile1 = 7;
			ile2 = 3;
			break;
		}

		int ile = max(0, ile1-orkowie_gorush->CountItem(hp2));
		if(ile)
			orkowie_gorush->AddItem(hp2, ile, false);

		if(ile2)
		{
			ile = max(0, ile2-orkowie_gorush->CountItem(hp3));
			if(ile)
				orkowie_gorush->AddItem(hp3, ile, false);
		}

		orkowie_gorush->ai->have_potion = 2;
	}

	// kupowanie miksturek przez jozana
	if(zlo_stan == ZS_ZAMYKANIE_PORTALI || zlo_stan == ZS_ZABIJ_BOSSA)
	{
		Unit* u = NULL;
		UnitData* ud = FindUnitData("q_zlo_kaplan");
		for(vector<Unit*>::iterator it = team.begin(), end = team.end(); it != end; ++it)
		{
			if((*it)->data == ud)
			{
				u = *it;
				break;
			}
		}

		if(u)
		{
			int ile = max(0, 5-u->CountItem(hp2));
			if(ile)
			{
				u->AddItem(hp2, ile, false);
				u->ai->have_potion = 2;
			}
		}
	}
}

const Item* Game::GetBetterItem(const Item* item)
{
#define E(x) (strcmp(item->id, x) == 0)
	assert(item);
	switch(item->type)
	{
	case IT_WEAPON:
		if(E("dagger_short"))
			return FindItem("dagger_sword");
		else if(E("dagger_sword"))
			return FindItem("dagger_rapier");
		else if(E("dagger_rapier"))
			return FindItem("dagger_assassin");
		else if(E("dagger_assassin"))
			return FindItem("dagger_sword_a");
		else if(E("sword_long"))
			return FindItem("sword_scimitar");
		else if(E("sword_scimitar"))
			return FindItem("sword_orcish");
		else if(E("sword_orcish"))
			return FindItem("sword_serrated");
		else if(E("sword_serrated"))
			return FindItem("sword_adamantine");
		else if(E("axe_small"))
			return FindItem("axe_battle");
		else if(E("axe_battle"))
			return FindItem("axe_orcish");
		else if(E("axe_orcish"))
			return FindItem("axe_crystal");
		else if(E("axe_crystal"))
			return FindItem("axe_giant");
		else if(E("blunt_club"))
			return FindItem("blunt_mace");
		else if(E("blunt_mace"))
			return FindItem("blunt_orcish");
		else if(E("blunt_orcish"))
			return FindItem("blunt_morningstar");
		else if(E("blunt_orcish_shaman"))
			return FindItem("blunt_morningstar");
		else if(E("blunt_morningstar"))
			return FindItem("blunt_dwarven");
		else if(E("blunt_dwarven"))
			return FindItem("blunt_adamantine");
		else if(E("wand_1"))
			return FindItem("wand_2");
		else if(E("wand_2"))
			return FindItem("wand_3");
		else if(E("blunt_blacksmith"))
			return FindItem("blunt_mace");
		else
			return NULL;
	case IT_ARMOR:
		if(E("armor_leather"))
			return FindItem("armor_studded");
		else if(E("armor_studded"))
			return FindItem("armor_chain_shirt");
		else if(E("armor_chain_shirt"))
			return FindItem("armor_mithril_shirt");
		else if(E("armor_mithril_shirt"))
			return FindItem("armor_dragonskin");
		else if(E("armor_chainmail"))
			return FindItem("armor_breastplate");
		else if(E("armor_breastplate"))
			return FindItem("armor_plate");
		else if(E("armor_plate"))
			return FindItem("armor_crystal");
		else if(E("armor_crystal"))
			return FindItem("armor_adamantine");
		else if(E("armor_blacksmith"))
			return FindItem("armor_studded");
		else if(E("armor_merchant"))
			return FindItem("armor_leather");
		else if(E("armor_alchemist"))
			return FindItem("armor_leather");
		else if(E("armor_innkeeper"))
			return FindItem("armor_leather");
		else if(E("clothes"))
			return FindItem("armor_leather");
		else if(E("clothes2"))
			return FindItem("armor_leather");
		else if(E("clothes3"))
			return FindItem("armor_leather");
		else if(E("clothes4"))
			return FindItem("armor_leather");
		else if(E("clothes5"))
			return FindItem("armor_leather");
		else if(E("armor_mage_1"))
			return FindItem("armor_mage_2");
		else if(E("armor_mage_2"))
			return FindItem("armor_mage_3");
		else if(E("armor_necromancer"))
			return FindItem("armor_chain_shirt");
		else
			return NULL;
	case IT_BOW:
		if(E("bow_short"))
			return FindItem("bow_long");
		else if(E("bow_long"))
			return FindItem("bow_composite");
		else if(E("bow_composite"))
			return FindItem("bow_elven");
		else if(E("bow_elven"))
			return FindItem("bow_dragonbone");
		else
			return NULL;
	case IT_SHIELD:
		if(E("shield_wood"))
			return FindItem("shield_iron");
		else if(E("shield_iron"))
			return FindItem("shield_steel");
		else if(E("shield_steel"))
			return FindItem("shield_mithril");
		else if(E("shield_mithril"))
			return FindItem("shield_adamantine");
		else
			return NULL;
	default:
		assert(0);
		return NULL;
	}
#undef E
}

void Game::CheckIfLocationCleared()
{
	if(city_ctx)
		return;

	bool czysto = true;
	for(vector<Unit*>::iterator it = local_ctx.units->begin(), end = local_ctx.units->end(); it != end; ++it)
	{
		if((*it)->IsAlive() && IsEnemy(*pc->unit, **it, true))
		{
			czysto = false;
			break;
		}
	}

	if(czysto)
	{
		bool cleared = false;
		if(!location->outside)
		{
			InsideLocation* inside = (InsideLocation*)location;
			if(inside->IsMultilevel())
			{
				if(((MultiInsideLocation*)inside)->LevelCleared())
					cleared = true;
			}
			else
				cleared = true;
		}
		else
			cleared = true;

		if(cleared)
		{
			location->state = LS_CLEARED;
			if(location->spawn != SG_BRAK)
			{
				if(location->type == L_CAMP)
					AddNews(Format(txNewsCampCleared, locations[GetNearestCity(location->pos)]->name.c_str()));
				else
					AddNews(Format(txNewsLocCleared, location->name.c_str()));
			}
		}

		if(location_event_handler)
			location_event_handler->HandleLocationEvent(LocationEventHandler::CLEARED);
	}
}

void Game::SpawnArenaViewers(int count)
{
	assert(in_range(count, 1, 9));

	vector<Animesh::Point*> points;
	UnitData& ud = *FindUnitData("viewer");
	Animesh* ani = buildings[B_ARENA].inside_mesh;
	InsideBuilding* arena = GetArena();

	for(vector<Animesh::Point>::iterator it = ani->attach_points.begin(), end = ani->attach_points.end(); it != end; ++it)
	{
		if(strncmp(it->name.c_str(), "o_s_viewer_", 11) == 0)
			points.push_back(&*it);
	}

	while(count > 0)
	{
		int id = rand2()%points.size();
		Animesh::Point* pt = points[id];
		points.erase(points.begin()+id);
		VEC3 pos(pt->mat._41+arena->offset.x, pt->mat._42, pt->mat._43+arena->offset.y);
		VEC3 look_at(arena->offset.x, 0, arena->offset.y);
		Unit* u = SpawnUnitNearLocation(arena->ctx, pos, ud, &look_at, -1, 2.f);
		if(u && IsOnline())
			Net_SpawnUnit(u);
		--count;
		u->ai->loc_timer = random(6.f,12.f);
		arena_viewers.push_back(u);
	}
}

void Game::RemoveArenaViewers()
{
	UnitData* ud = FindUnitData("viewer");
	LevelContext& ctx = GetArena()->ctx;

	for(vector<Unit*>::iterator it = ctx.units->begin(), end = ctx.units->end(); it != end; ++it)
	{
		if((*it)->data == ud)
		{
			(*it)->to_remove = true;
			to_remove.push_back(*it);
			if(IsOnline())
				Net_RemoveUnit(*it);
		}
	}
}

float Game::PlayerAngleY()
{
	//const float c_cam_angle_min = PI+0.1f;
	//const float c_cam_angle_max = PI*1.8f-0.1f;
	const float pt0 = 4.6662526f;

	/*if(rotY < pt0)
	{
		if(rotY < pt0-0.6f)
			return -1.f;
		else
			return lerp(0.f, -1.f, (pt0 - rotY)/0.6f);
	}
	else
	{
		if(rotY > pt0+0.6f)
			return 1.f;
		else
			return lerp(0.f, 1.f, (rotY-pt0)/0.6f);
	}*/

	/*if(rotY < c_cam_angle_min+range/4)
		return -1.f;
	else if(rotY > c_cam_angle_max-range/4)
		return 1.f;
	else
		return lerp(-1.f, 1.f, (rotY - (c_cam_angle_min+range/4)) / (range/2));*/

// 	if(rotY < pt0)
// 	{
// 		return rotY - pt0;
// 	}
	return rotY - pt0;
}

VEC3 Game::GetExitPos(Unit& u)
{
	const VEC3& pos = u.pos;

	if(location->outside)
	{
		if(u.in_building != -1)
			return VEC3_x0y(city_ctx->inside_buildings[u.in_building]->exit_area.Midpoint());
		else if(city_ctx)
		{
			float best_dist, dist;
			int best_index = -1, index = 0;

			for(vector<EntryPoint>::const_iterator it = city_ctx->entry_points.begin(), end = city_ctx->entry_points.end(); it != end; ++it, ++index)
			{
				if(it->exit_area.IsInside(u.pos))
				{
					// unit is already inside exit area, goto outside exit
					best_index = -1;
					break;
				}
				else
				{
					dist = distance(VEC2(u.pos.x, u.pos.z), it->exit_area.Midpoint());
					if(best_index == -1 || dist < best_dist)
					{
						best_dist = dist;
						best_index = index;
					}
				}
			}

			if(best_index != -1)
				return VEC3_x0y(city_ctx->entry_points[best_index].exit_area.Midpoint());
		}

		int best = 0;
		float dist, best_dist;

		// lewa kraw�d�
		best_dist = abs(pos.x - 32.f);

		// prawa kraw�d�
		dist = abs(pos.x - (256.f-32.f));
		if(dist < best_dist)
		{
			best_dist = dist;
			best = 1;
		}

		// g�rna kraw�d�
		dist = abs(pos.z - 32.f);
		if(dist < best_dist)
		{
			best_dist = dist;
			best = 2;
		}

		// dolna kraw�d�
		dist = abs(pos.z - (256.f-32.f));
		if(dist < best_dist)
			best = 3;

		switch(best)
		{
		default:
			assert(0);
		case 0:
			return VEC3(32.f,0,pos.z);
		case 1:
			return VEC3(256.f-32.f,0,pos.z);
		case 2:
			return VEC3(pos.x,0,32.f);
		case 3:
			return VEC3(pos.x,0,256.f-32.f);
		}
	}
	else
	{
		InsideLocation* inside = (InsideLocation*)location;
		if(dungeon_level == 0 && inside->from_portal)
			return inside->portal->pos;
		INT2& pt = inside->GetLevelData().schody_gora;
		return VEC3(2.f*pt.x+1,0,2.f*pt.y+1);
	}
}

void Game::AttackReaction(Unit& attacked, Unit& attacker)
{
	if(attacker.IsPlayer() && attacked.IsAI() && attacked.in_arena == -1 && !attacked.attack_team)
	{
		if(attacked.data->grupa == G_MIESZKANCY)
		{
			if(!bandyta)
			{
				bandyta = true;
				if(IsOnline())
					PushNetChange(NetChange::CHANGE_FLAGS);
			}
		}
		else if(attacked.data->grupa == G_SZALENI)
		{
			if(!atak_szalencow)
			{
				atak_szalencow = true;
				if(IsOnline())
					PushNetChange(NetChange::CHANGE_FLAGS);
			}
		}
		if(attacked.dont_attack)
		{
			for(vector<Unit*>::iterator it = local_ctx.units->begin(), end = local_ctx.units->end(); it != end; ++it)
			{
				if((*it)->dont_attack)
				{
					(*it)->dont_attack = false;
					(*it)->ai->change_ai_mode = true;
				}
			}
		}
	}
}

int Game::CanLeaveLocation(Unit& unit)
{
	if(sekret_stan == SS2_WALKA)
		return false;

	if(city_ctx)
	{
		for(vector<Unit*>::iterator it = team.begin(), end = team.end(); it != end; ++it)
		{
			Unit& u = **it;
			if(u.busy != Unit::Busy_No && u.busy != Unit::Busy_Tournament)
				return 1;

			if(u.IsPlayer())
			{
				if(u.in_building != -1 || distance2d(unit.pos, u.pos) > 8.f)
					return 1;
			}

			for(vector<Unit*>::iterator it2 = local_ctx.units->begin(), end2 = local_ctx.units->end(); it2 != end2; ++it2)
			{
				Unit& u2 = **it2;
				if(&u != &u2 && u2.IsStanding() && IsEnemy(u, u2) && u2.IsAI() && u2.ai->in_combat && distance2d(u.pos, u2.pos) < alert_range.x && CanSee(u, u2))
					return 2;
			}
		}
	}
	else
	{
		for(vector<Unit*>::iterator it = team.begin(), end = team.end(); it != end; ++it)
		{
			Unit& u = **it;
			if(u.busy != Unit::Busy_No || distance2d(unit.pos, u.pos) > 8.f)
				return 1;

			for(vector<Unit*>::iterator it2 = local_ctx.units->begin(), end2 = local_ctx.units->end(); it2 != end2; ++it2)
			{
				Unit& u2 = **it2;
				if(&u != &u2 && u2.IsStanding() && IsEnemy(u, u2) && u2.IsAI() && u2.ai->in_combat && distance2d(u.pos, u2.pos) < alert_range.x && CanSee(u, u2))
					return 2;
			}
		}
	}

	return 0;
}

void Game::GenerateTraps()
{
	InsideLocation* inside = (InsideLocation*)location;
	BaseLocation& base = g_base_locations[inside->cel];

	if(!IS_SET(base.traps, TRAPS_NORMAL|TRAPS_MAGIC))
		return;

	InsideLocationLevel& lvl = inside->GetLevelData();

	int szansa;
	INT2 pt(-1000,-1000);
	if(IS_SET(base.traps, TRAPS_NEAR_ENTRANCE))
	{
		if(dungeon_level != 0)
			return;
		szansa = 10;
		pt = lvl.schody_gora;
	}
	else if(IS_SET(base.traps, TRAPS_NEAR_END))
	{
		if(inside->IsMultilevel())
		{
			MultiInsideLocation* multi = (MultiInsideLocation*)inside;
			int size = int(multi->levels.size());
			switch(size)
			{
			case 0:
				szansa = 25;
				break;
			case 1:
				if(dungeon_level == 1)
					szansa = 25;
				else
					szansa = 0;
				break;
			case 2:
				if(dungeon_level == 2)
					szansa = 25;
				else if(dungeon_level == 1)
					szansa = 15;
				else
					szansa = 0;
				break;
			case 3:
				if(dungeon_level == 3)
					szansa = 25;
				else if(dungeon_level == 2)
					szansa = 15;
				else if(dungeon_level == 1)
					szansa = 10;
				else
					szansa = 0;
				break;
			default:
				if(dungeon_level == size-1)
					szansa = 25;
				else if(dungeon_level == size-2)
					szansa = 15;
				else if(dungeon_level == size-3)
					szansa = 10;
				else
					szansa = 0;
				break;
			}

			if(szansa == 0)
				return;
		}
		else
			szansa = 20;
	}
	else
		szansa = 20;

	vector<TRAP_TYPE> traps;
	if(IS_SET(base.traps, TRAPS_NORMAL))
	{
		traps.push_back(TRAP_ARROW);
		traps.push_back(TRAP_POISON);
		traps.push_back(TRAP_SPEAR);
	}
	if(IS_SET(base.traps, TRAPS_MAGIC))
		traps.push_back(TRAP_FIREBALL);

	for(int y=1; y<lvl.h-1; ++y)
	{
		for(int x=1; x<lvl.w-1; ++x)
		{
			if(lvl.mapa[x+y*lvl.w].co == PUSTE 
				&& !OR2_EQ(lvl.mapa[x-1+y*lvl.w].co, SCHODY_DOL, SCHODY_GORA)
				&& !OR2_EQ(lvl.mapa[x+1+y*lvl.w].co, SCHODY_DOL, SCHODY_GORA)
				&& !OR2_EQ(lvl.mapa[x+(y-1)*lvl.w].co, SCHODY_DOL, SCHODY_GORA)
				&& !OR2_EQ(lvl.mapa[x+(y+1)*lvl.w].co, SCHODY_DOL, SCHODY_GORA))
			{
				if(rand2()%500 < szansa + max(0, 30-distance(pt, INT2(x,y))))
					CreateTrap(INT2(x,y), traps[rand2()%traps.size()]);
			}
		}
	}
}

void Game::RegenerateTraps()
{
	InsideLocation* inside = (InsideLocation*)location;
	BaseLocation& base = g_base_locations[inside->cel];

	if(!IS_SET(base.traps, TRAPS_MAGIC))
		return;

	InsideLocationLevel& lvl = inside->GetLevelData();

	int szansa;
	INT2 pt(-1000,-1000);
	if(IS_SET(base.traps, TRAPS_NEAR_ENTRANCE))
	{
		if(dungeon_level != 0)
			return;
		szansa = 0;
		pt = lvl.schody_gora;
	}
	else if(IS_SET(base.traps, TRAPS_NEAR_END))
	{
		if(inside->IsMultilevel())
		{
			MultiInsideLocation* multi = (MultiInsideLocation*)inside;
			int size = int(multi->levels.size());
			switch(size)
			{
			case 0:
				szansa = 25;
				break;
			case 1:
				if(dungeon_level == 1)
					szansa = 25;
				else
					szansa = 0;
				break;
			case 2:
				if(dungeon_level == 2)
					szansa = 25;
				else if(dungeon_level == 1)
					szansa = 15;
				else
					szansa = 0;
				break;
			case 3:
				if(dungeon_level == 3)
					szansa = 25;
				else if(dungeon_level == 2)
					szansa = 15;
				else if(dungeon_level == 1)
					szansa = 10;
				else
					szansa = 0;
				break;
			default:
				if(dungeon_level == size-1)
					szansa = 25;
				else if(dungeon_level == size-2)
					szansa = 15;
				else if(dungeon_level == size-3)
					szansa = 10;
				else
					szansa = 0;
				break;
			}

			if(szansa == 0)
				return;
		}
		else
			szansa = 20;
	}
	else
		szansa = 20;

	vector<Trap*>& traps = *local_ctx.traps;
	int id = 0, topid = traps.size();

	for(int y=1; y<lvl.h-1; ++y)
	{
		for(int x=1; x<lvl.w-1; ++x)
		{
			if(lvl.mapa[x+y*lvl.w].co == PUSTE 
				&& !OR2_EQ(lvl.mapa[x-1+y*lvl.w].co, SCHODY_DOL, SCHODY_GORA)
				&& !OR2_EQ(lvl.mapa[x+1+y*lvl.w].co, SCHODY_DOL, SCHODY_GORA)
				&& !OR2_EQ(lvl.mapa[x+(y-1)*lvl.w].co, SCHODY_DOL, SCHODY_GORA)
				&& !OR2_EQ(lvl.mapa[x+(y+1)*lvl.w].co, SCHODY_DOL, SCHODY_GORA))
			{
				int s = szansa + max(0, 30-distance(pt, INT2(x,y)));
				if(IS_SET(base.traps, TRAPS_NORMAL))
					s /= 4;
				if(rand2()%500 < s)
				{
					bool ok = false;
					if(id == -1)
						ok = true;
					else if(id == topid)
					{
						id = -1;
						ok = true;
					}
					else
					{
						while(id != topid)
						{
							if(traps[id]->tile.y > y || (traps[id]->tile.y == y && traps[id]->tile.x > x))
							{
								ok = true;
								break;
							}
							else if(traps[id]->tile.x == x && traps[id]->tile.y == y)
							{
								++id;
								break;
							}
							else
								++id;
						}
					}

					if(ok)
						CreateTrap(INT2(x,y), TRAP_FIREBALL);
				}
			}
		}
	}

#ifdef _DEBUG
	LOG(Format("Traps: %d", local_ctx.traps->size()));
#endif
}

bool SortItemsByValue(const ItemSlot& a, const ItemSlot& b)
{
	return a.item->GetWeightValue() < b.item->GetWeightValue();
}

void Game::SpawnHeroesInsideDungeon()
{
	InsideLocation* inside = (InsideLocation*)location;
	InsideLocationLevel& lvl = inside->GetLevelData();

	Pokoj* p = lvl.GetUpStairsRoom();
	int room_id = lvl.GetRoomId(p);
	int szansa = 23;

	vector<std::pair<Pokoj*, int> > sprawdzone;
	vector<int> ok_room;
	sprawdzone.push_back(std::make_pair(p, room_id));

	while(true)
	{
		p = sprawdzone.back().first;
		for(vector<int>::iterator it = p->polaczone.begin(), end = p->polaczone.end(); it != end; ++it)
		{
			room_id = *it;
			bool ok = true;
			for(vector<std::pair<Pokoj*, int> >::iterator it2 = sprawdzone.begin(), end2 = sprawdzone.end(); it2 != end2; ++it2)
			{
				if(room_id == it2->second)
				{
					ok = false;
					break;
				}
			}
			if(ok && (rand2()%20 < szansa || rand2()%3 == 0))
				ok_room.push_back(room_id);
		}

		if(ok_room.empty())
			break;
		else
		{
			room_id = ok_room[rand2()%ok_room.size()];
			ok_room.clear();
			sprawdzone.push_back(std::make_pair(&lvl.pokoje[room_id], room_id));
			--szansa;
		}
	}

	// cofnij ich z korytarza
	while(sprawdzone.back().first->korytarz)
		sprawdzone.pop_back();

	int gold = 0;
	vector<ItemSlot> items;
	LocalVector<Chest*> chests;

	// pozabijaj jednostki w pokojach, ograb skrzynie
	// troch� to nieefektywne :/
	vector<std::pair<Pokoj*, int> >::iterator end = sprawdzone.end();
	if(rand2()%2 == 0)
		--end;
	for(vector<Unit*>::iterator it2 = local_ctx.units->begin(), end2 = local_ctx.units->end(); it2 != end2; ++it2)
	{
		Unit& u = **it2;
		if(u.IsAlive() && IsEnemy(*pc->unit, u))
		{
			for(vector<std::pair<Pokoj*, int> >::iterator it = sprawdzone.begin(); it != end; ++it)
			{
				if(it->first->IsInside(u.pos))
				{
					gold += u.gold;
					for(int i=0; i<SLOT_MAX; ++i)
					{
						if(u.slots[i] && u.slots[i]->GetWeightValue() >= 5.f)
						{
							ItemSlot& slot = Add1(items);
							slot.item = u.slots[i];
							slot.count = slot.team_count = 1u;
							u.weight -= u.slots[i]->weight;
							u.slots[i] = NULL;
						}
					}
					for(vector<ItemSlot>::iterator it3 = u.items.begin(), end3 = u.items.end(); it3 != end3;)
					{
						if(it3->item->GetWeightValue() >= 5.f)
						{
							u.weight -= it3->item->weight * it3->count;
							items.push_back(*it3);
							it3 = u.items.erase(it3);
							end3 = u.items.end();
						}
						else
							++it3;
					}
					u.gold = 0;
					u.live_state = Unit::DEAD;
					u.animacja = u.animacja2 = ANI_UMIERA;
					u.ani->SetToEnd(NAMES::ani_die);
					u.hp = 0.f;
					CreateBlood(local_ctx, u, true);
					++total_kills;

					// przenie� fizyke
					btVector3 a_min, a_max;
					u.cobj->getWorldTransform().setOrigin(btVector3(1000,1000,1000));
					u.cobj->getCollisionShape()->getAabb(u.cobj->getWorldTransform(), a_min, a_max);
					phy_broadphase->setAabb(u.cobj->getBroadphaseHandle(), a_min, a_max, phy_dispatcher);

					if(u.event_handler)
						u.event_handler->HandleUnitEvent(UnitEventHandler::DIE, &u);
					break;
				}
			}
		}
	}
	for(vector<Chest*>::iterator it2 = local_ctx.chests->begin(), end2 = local_ctx.chests->end(); it2 != end2; ++it2)
	{
		for(vector<std::pair<Pokoj*, int> >::iterator it = sprawdzone.begin(); it != end; ++it)
		{
			if(it->first->IsInside((*it2)->pos))
			{
				for(vector<ItemSlot>::iterator it3 = (*it2)->items.begin(), end3 = (*it2)->items.end(); it3 != end3;)
				{
					if(it3->item->type == IT_GOLD)
					{
						gold += it3->count;
						it3 = (*it2)->items.erase(it3);
						end3 = (*it2)->items.end();
					}
					else if(it3->item->GetWeightValue() >= 5.f)
					{
						items.push_back(*it3);
						it3 = (*it2)->items.erase(it3);
						end3 = (*it2)->items.end();
					}
					else
						++it3;
				}
				chests->push_back(*it2);
				break;
			}
		}
	}

	// otw�rz drzwi pomi�dzy obszarami
	for(vector<std::pair<Pokoj*, int> >::iterator it2 = sprawdzone.begin(), end2 = sprawdzone.end(); it2 != end2; ++it2)
	{
		Pokoj& a = *it2->first,
			 & b = lvl.pokoje[it2->second];

		// wsp�lny obszar pomi�dzy pokojami
		int x1 = max(a.pos.x,b.pos.x),
			x2 = min(a.pos.x+a.size.x,b.pos.x+b.size.x),
			y1 = max(a.pos.y,b.pos.y),
			y2 = min(a.pos.y+a.size.y,b.pos.y+b.size.y);

		// szukaj drzwi
		for(int y=y1; y<y2; ++y)
		{
			for(int x=x1; x<x2; ++x)
			{
				Pole& po = lvl.mapa[x+y*lvl.w];
				if(po.co == DRZWI)
				{
					Door* door = lvl.FindDoor(INT2(x,y));					
					if(door && door->state == Door::Closed)
					{
						door->state = Door::Open;
						btVector3& pos = door->phy->getWorldTransform().getOrigin();
						pos.setY(pos.y() - 100.f);
						door->ani->SetToEnd(&door->ani->ani->anims[0]);
					}
				}
			}
		}
	}

	// stw�rz bohater�w
	int ile = random(2,4);
	LocalVector<Unit*> heroes;
	p = sprawdzone.back().first;
	for(int i=0; i<ile; ++i)
	{
		cstring name;
		switch(rand2()%7)
		{
		default:
		case 0:
			name = "hero_mage";
			break;
		case 1:
		case 2:
			name = "hero_warrior";
			break;
		case 3:
		case 4:
			name = "hero_hunter";
			break;
		case 5:
		case 6:
			name = "hero_rogue";
			break;
		}

		Unit* u = SpawnUnitInsideRoom(*p, *FindUnitData(name), random(2,15));
		if(u)
			heroes->push_back(u);
		else
			break;
	}

	// sortuj przedmioty wed�ug warto�ci
	std::sort(items.begin(), items.end(), SortItemsByValue);

	// rozdziel z�oto
	int gold_per_hero = gold/heroes->size();
	for(vector<Unit*>::iterator it = heroes->begin(), end = heroes->end(); it != end; ++it)
		(*it)->gold += gold_per_hero;
	gold -= gold_per_hero*heroes->size();
	if(gold)
		heroes->back()->gold += gold;

	// rozdziel przedmioty
	vector<Unit*>::iterator heroes_it = heroes->begin(), heroes_end = heroes->end();
	while(!items.empty())
	{
		ItemSlot& item = items.back();
		for(int i=0, ile=item.count; i<ile; ++i)
		{
			if((*heroes_it)->CanTake(item.item))
			{
				(*heroes_it)->AddItemAndEquipIfNone(item.item);
				--item.count;
				++heroes_it;
				if(heroes_it == heroes_end)
					heroes_it = heroes->begin();
			}
			else
			{
				// ten heros nie mo�e wzi��� tego przedmiotu, jest szansa �e wzi��by jaki� l�ejszy i ta�szy ale ma�a
				heroes_it = heroes->erase(heroes_it);
				if(heroes->empty())
					break;
				heroes_end = heroes->end();
				if(heroes_it == heroes_end)
					heroes_it = heroes->begin();
			}
		}
		if(heroes->empty())
			break;
		items.pop_back();
	}
	
	// pozosta�e przedmioty schowaj do skrzyni o ile jest co i gdzie
	if(!chests->empty() && !items.empty())
	{
		chests.Shuffle();
		vector<Chest*>::iterator chest_begin = chests->begin(), chest_end = chests->end(), chest_it = chest_begin;
		for(vector<ItemSlot>::iterator it = items.begin(), end = items.end(); it != end; ++it)
		{
			(*chest_it)->AddItem(it->item, it->count);
			++chest_it;
			if(chest_it == chest_end)
				chest_it = chest_begin;
		}
	}

	// sprawd� czy lokacja oczyszczona (raczej nie jest)
	CheckIfLocationCleared();
}

GroundItem* Game::SpawnGroundItemInsideRoom(Pokoj& pokoj, const Item* item)
{
	for(int i=0; i<50; ++i)
	{
		VEC3 pos = pokoj.GetRandomPos(0.5f);
		global_col.clear();
		GatherCollisionObjects(local_ctx, global_col, pos, 0.25f);
		if(!Collide(global_col, pos, 0.25f))
		{
			GroundItem* gi = new GroundItem;
			gi->count = 1;
			gi->team_count = 1;
			gi->rot = random(MAX_ANGLE);
			gi->pos = pos;
			gi->item = item;
			gi->netid = item_netid_counter++;
			local_ctx.items->push_back(gi);
			return gi;
		}
	}

	return NULL;
}

GroundItem* Game::SpawnGroundItemInsideRadius(const Item* item, const VEC2& pos, float radius, bool try_exact)
{
	VEC3 pt;
	for(int i=0; i<50; ++i)
	{
		if(!try_exact)
		{
			float a = random(), b = random();
			if(b < a)
				std::swap(a, b);
			pt = VEC3(pos.x+b*radius*cos(2*PI*a/b), 0, pos.y+b*radius*sin(2*PI*a/b));
		}
		else
		{
			try_exact = false;
			pt = VEC3(pos.x,0,pos.y);
		}
		global_col.clear();
		GatherCollisionObjects(local_ctx, global_col, pt, 0.25f);
		if(!Collide(global_col, pt, 0.25f))
		{
			GroundItem* gi = new GroundItem;
			gi->count = 1;
			gi->team_count = 1;
			gi->rot = random(MAX_ANGLE);
			gi->pos = pt;
			gi->item = item;
			gi->netid = item_netid_counter++;
			local_ctx.items->push_back(gi);
			return gi;
		}
	}

	return NULL;
}

int Game::GetRandomCityLocation(const vector<int>& used, int type) const
{
	int id = rand2()%cities;
	
loop:
	if(type != 0)
	{
		if(type == 1)
		{
			if(locations[id]->type == L_VILLAGE)
			{
				id = (id+1)%cities;
				goto loop;
			}
		}
		else
		{
			if(locations[id]->type == L_CITY)
			{
				id = (id+1)%cities;
				goto loop;
			}
		}
	}

	for(vector<int>::const_iterator it = used.begin(), end = used.end(); it != end; ++it)
	{
		if(*it == id)
		{
			id = (id+1)%cities;
			goto loop;
		}
	}
	
	return id;
}

void Game::InitQuests()
{
	vector<int> used;

	// gobliny
	{
		gobliny_miasto = GetRandomCityLocation(used, 1);
		Quest_Gobliny* q = new Quest_Gobliny;
		gobliny_refid = q->refid = quest_counter;
		++quest_counter;
		q->Start();
		q->start_loc = gobliny_miasto;
		unaccepted_quests.push_back(q);
		used.push_back(gobliny_miasto);
		gobliny_stan = GS_BRAK;
		gobliny_szlachcic = NULL;
		gobliny_poslaniec = NULL;
	}

	// bandyci
	{
		bandyci_miasto = GetRandomCityLocation(used, 1);
		Quest_Bandyci* q = new Quest_Bandyci;
		bandyci_refid = q->refid = quest_counter;
		++quest_counter;
		q->Start();
		q->start_loc = bandyci_miasto;
		unaccepted_quests.push_back(q);
		used.push_back(bandyci_miasto);
		bandyci_stan = BS_BRAK;
		bandyci_gdzie = -1;
		bandyci_czas = 0.f;
		bandyci_agent = NULL;
	}

	// tartak
	{
		tartak_miasto = GetRandomCityLocation(used);
		tartak_stan = 0;
		tartak_stan2 = 0;
		tartak_poslaniec = NULL;
		Quest_Tartak* q = new Quest_Tartak;
		tartak_refid = q->refid = quest_counter;
		++quest_counter;
		q->Start();
		q->start_loc = tartak_miasto;
		unaccepted_quests.push_back(q);
		used.push_back(tartak_miasto);
	}

	// kopalnia
	{
		kopalnia_miasto = GetRandomCityLocation(used);
		kopalnia_gdzie = GetClosestLocation(L_CAVE, locations[kopalnia_miasto]->pos);
		kopalnia_stan = KS_BRAK;
		kopalnia_stan2 = KS2_BRAK;
		kopalnia_stan3 = KS3_BRAK;
		kopalnia_poslaniec = NULL;
		Quest_Kopalnia* q = new Quest_Kopalnia;
		kopalnia_refid = q->refid = quest_counter;
		++quest_counter;
		q->Start();
		q->start_loc = kopalnia_miasto;
		q->target_loc = kopalnia_gdzie;
		unaccepted_quests.push_back(q);
		used.push_back(kopalnia_miasto);
	}

	// magowie
	{
		magowie_miasto = GetRandomCityLocation(used);
		Quest_Magowie* q = new Quest_Magowie;
		magowie_refid = q->refid = quest_counter;
		++quest_counter;
		q->Start();
		q->start_loc = magowie_miasto;
		unaccepted_quests.push_back(q);
		used.push_back(magowie_miasto);
		magowie_stan = MS_BRAK;
		magowie_uczony = NULL;
		magowie_refid2 = -1;
		magowie_zaplacono = false;
		plotka_questowa[P_MAGOWIE2] = true;
		--ile_plotek_questowych;
	}

	// orkowie
	{
		orkowie_miasto = GetRandomCityLocation(used);
		Quest_Orkowie* q = new Quest_Orkowie;
		orkowie_refid = q->refid = quest_counter;
		++quest_counter;
		q->Start();
		q->start_loc = orkowie_miasto;
		unaccepted_quests.push_back(q);
		used.push_back(orkowie_miasto);
		orkowie_stan = OS_BRAK;
		orkowie_straznik = NULL;
		orkowie_gorush = NULL;
		orkowie_klasa = GORUSH_BRAK;
		Quest_Orkowie2* q2 = new Quest_Orkowie2;
		orkowie_refid2 = q2->refid = quest_counter;
		++quest_counter;
		q2->Start();
		unaccepted_quests.push_back(q2);
	}

	// z�o
	{
		zlo_miasto = GetRandomCityLocation(used);
		Quest_Zlo* q = new Quest_Zlo;
		zlo_refid = q->refid = quest_counter;
		++quest_counter;
		q->Start();
		q->start_loc = zlo_miasto;
		unaccepted_quests.push_back(q);
		used.push_back(zlo_miasto);
		zlo_stan = ZS_BRAK;
		zlo_gdzie = -1;
		zlo_gdzie2 = -1;
		jozan = NULL;
	}

	// szaleni
	{
		Quest_Szaleni* q = new Quest_Szaleni;
		szaleni_refid = q->refid = quest_counter;
		q->Start();
		++quest_counter;
		unaccepted_quests.push_back(q);
		szaleni_stan = SS_BRAK;
		szaleni_sprawdz_kamien = false;
	}

	// sekret
	sekret_stan = (FindObject("tomashu_dom")->ani ? SS2_BRAK : SS2_WYLACZONY);
	GetSecretNote()->desc.clear();
	sekret_gdzie = -1;
	sekret_gdzie2 = -1;

	// zawody w piciu
	chlanie_stan = 0;
	chlanie_gdzie = GetRandomCityLocation();
	chlanie_ludzie.clear();
	chlanie_wygenerowano = false;
	chlanie_zwyciesca = NULL;

	// zawody
	zawody_rok = 0;
	zawody_rok_miasta = year;
	zawody_miasto = GetRandomCity();
	zawody_stan = IS_BRAK;
	zawody_ludzie.clear();
	zawody_zwyciezca = NULL;
	zawody_wygenerowano = false;

#ifdef _DEBUG
	LOG(Format("Quest 'Tartak' - %s.", locations[tartak_miasto]->name.c_str()));
	LOG(Format("Quest 'Kopalnia' - %s, %s.", locations[kopalnia_miasto]->name.c_str(), locations[kopalnia_gdzie]->name.c_str()));
	LOG(Format("Quest 'Bandyci' - %s.", locations[bandyci_miasto]->name.c_str()));
	LOG(Format("Quest 'Magowie' - %s.", locations[magowie_miasto]->name.c_str()));
	LOG(Format("Quest 'Orkowie' - %s.", locations[orkowie_miasto]->name.c_str()));
	LOG(Format("Quest 'Gobliny' - %s.", locations[gobliny_miasto]->name.c_str()));
	LOG(Format("Quest 'Z�o' - %s.", locations[zlo_miasto]->name.c_str()));
	LOG(Format("Zawody - %s.", locations[zawody_miasto]->name.c_str()));
	LOG(Format("Zawody w piciu - %s.", locations[chlanie_gdzie]->name.c_str()));
#endif
}

void Game::GenerateQuestUnits()
{
	if(current_location == tartak_miasto && tartak_stan == 0)
	{
		Unit* u = SpawnUnitInsideInn(*FindUnitData("artur_drwal"), -2);
		assert(u);
		if(u)
		{
			u->rot = random(MAX_ANGLE);
			u->hero->name = txArthur;
			tartak_stan = 1;
			hd_artur_drwal.Get(*u->human_data);
			DEBUG_LOG(Format("Generated quest unit '%s'.", txArthur));
		}
	}

	if(current_location == kopalnia_miasto && kopalnia_stan == KS_BRAK)
	{
		Unit* u = SpawnUnitInsideInn(*FindUnitData("inwestor"), -2);
		assert(u);
		if(u)
		{
			u->rot = random(MAX_ANGLE);
			u->hero->name = "Marco Bartoli";
			kopalnia_stan = KS_WYGENEROWANO_INWESTORA;
			DEBUG_LOG("Generated quest unit 'Marco Bartoli'.");
		}
	}

	if(current_location == bandyci_miasto && bandyci_stan == BS_BRAK)
	{
		Unit* u = SpawnUnitInsideInn(*FindUnitData("mistrz_agentow"), -2);
		assert(u);
		if(u)
		{
			u->rot = random(MAX_ANGLE);
			u->hero->name = "Arto";
			bandyci_stan = BS_WYGENEROWANO_ARTO;
			DEBUG_LOG("Wygenerowano questow� jednostk� 'Mistrz agent�w'.");
		}
	}

	if(current_location == magowie_miasto && magowie_stan == MS_BRAK)
	{
		Unit* u = SpawnUnitInsideInn(*FindUnitData("q_magowie_uczony"), -2);
		assert(u);
		if(u)
		{
			u->rot = random(MAX_ANGLE);
			magowie_stan = MS_WYGENEROWANO_UCZONEGO;
			DEBUG_LOG("Wygenerowano questow� jednostk� 'Uczony'.");
		}
	}

	if(current_location == magowie_gdzie)
	{
		if(magowie_stan == MS_POROZMAWIANO_Z_KAPITANEM)
		{
			Unit* u = SpawnUnitInsideInn(*FindUnitData("q_magowie_stary"), 15);
			assert(u);
			if(u)
			{
				u->rot = random(MAX_ANGLE);
				magowie_stan = MS_WYGENEROWANO_STAREGO_MAGA;
				DEBUG_LOG("Wygenerowano questow� jednostk� 'Stary mag'.");
			}
		}
		else if(magowie_stan == MS_MAG_POSZEDL)
		{
			Unit* u = SpawnUnitInsideInn(*FindUnitData("q_magowie_stary"), 15);
			assert(u);
			if(u)
			{
				magowie_uczony = u;
				u->rot = random(MAX_ANGLE);
				u->hero->know_name = true;
				u->hero->name = magowie_imie_dobry;
				magowie_imie_dobry.clear();
				magowie_hd.Set(*u->human_data);
				magowie_stan = MS_MAG_WYGENEROWANY_W_MIESCIE;
				DEBUG_LOG("Wygenerowano questow� jednostk� 'Stary mag'.");
			}
		}
	}

	if(current_location == orkowie_miasto && orkowie_stan == OS_BRAK)
	{
		Unit* u = SpawnUnitInsideInn(*FindUnitData("q_orkowie_straznik"));
		assert(u);
		if(u)
		{
			u->rot = random(MAX_ANGLE);
			u->auto_talk = 1;
			orkowie_stan = OS_WYGENEROWANO_STRAZNIKA;
			orkowie_straznik = u;
			DEBUG_LOG("Wygenerowano questow� jednostk� 'Stra�nik'.");
		}
	}

	if(current_location == gobliny_miasto && gobliny_stan == GS_BRAK)
	{
		Unit* u = SpawnUnitInsideInn(*FindUnitData("q_gobliny_szlachcic"));
		assert(u);
		if(u)
		{
			gobliny_szlachcic = u;
			gobliny_hd.Get(*gobliny_szlachcic->human_data);
			u->rot = random(MAX_ANGLE);
			u->hero->name = "Sir Foltest Denhoff";
			gobliny_stan = GS_WYGENEROWANO_SZLACHCICA;
			DEBUG_LOG("Wygenerowano questow� jednostk� 'Szlachcic'.");
		}
	}

	if(current_location == zlo_miasto && zlo_stan == ZS_BRAK)
	{
		CityBuilding* b = city_ctx->FindBuilding(BG_INN);
		Unit* u = SpawnUnitNearLocation(local_ctx, b->walk_pt, *FindUnitData("q_zlo_kaplan"), NULL, 10);
		assert(u);
		if(u)
		{
			u->rot = random(MAX_ANGLE);
			u->hero->name = "Jozan";
			u->auto_talk = 1;
			jozan = u;
			zlo_stan = ZS_WYGENEROWANO_KAPLANA;
			DEBUG_LOG("Wygenerowano questow� jednostk� 'Kap�an'.");
		}
	}

	if(bandyta)
		return;

	if(tartak_stan == 2)
	{
		if(tartak_dni >= 30 && city_ctx)
		{
			tartak_dni = 0;
			Unit* u = SpawnUnitNearLocation(GetContext(*leader), leader->pos, *FindUnitData("poslaniec_tartak"), &leader->pos, -2, 2.f);
			if(u)
			{
				tartak_poslaniec = u;
				StartDialog2(leader->player, u);
			}
		}
	}
	else if(tartak_stan == 3)
	{
		int ile = tartak_dni/30;
		if(ile)
		{
			tartak_dni -= ile*30;
			AddGold(ile*400, NULL, true, "Z�oto +%d");
		}
	}

	if(kopalnia_dni >= kopalnia_ile_dni &&
		((kopalnia_stan2 == KS2_TRWA_BUDOWA && kopalnia_stan == KS_MASZ_UDZIALY) || // poinformuj gracza o wybudowaniu kopalni i daj z�oto
		kopalnia_stan2 == KS2_WYBUDOWANO || // poinformuj gracza o mo�liwej inwestycji
		kopalnia_stan2 == KS2_TRWA_ROZBUDOWA || // poinformuj gracza o uko�czeniu rozbudowy i daj z�oto
		kopalnia_stan2 == KS2_ROZBUDOWANO)) // poinformuj gracza o znalezieniu portalu
	{
		Unit* u = SpawnUnitNearLocation(GetContext(*leader), leader->pos, *FindUnitData("poslaniec_kopalnia"), &leader->pos, -2, 2.f);
		if(u)
		{
			kopalnia_poslaniec = u;
			StartDialog2(leader->player, u);
		}
	}

	GenerateQuestUnits2(true);

	if(zlo_stan == ZS_GENERUJ_MAGA && current_location == zlo_gdzie2)
	{
		Unit* u = SpawnUnitInsideInn(*FindUnitData("q_zlo_mag"), -2);
		assert(u);
		if(u)
		{
			u->rot = random(MAX_ANGLE);
			zlo_stan = ZS_WYGENEROWANO_MAGA;
			DEBUG_LOG("Wygenerowano questow� jednostk� 'Mag'.");
		}
	}
}

void Game::GenerateQuestUnits2(bool on_enter)
{
	if(gobliny_stan == GS_ODLICZANIE && gobliny_dni <= 0)
	{
		Unit* u = SpawnUnitNearLocation(GetContext(*leader), leader->pos, *FindUnitData("q_gobliny_poslaniec"), &leader->pos, -2, 2.f);
		if(u)
		{
			if(IsOnline() && !on_enter)
				Net_SpawnUnit(u);
			gobliny_poslaniec = u;
			StartDialog2(leader->player, u);
		}
	}

	if(gobliny_stan == GS_SZLACHCIC_ZNIKNAL && gobliny_dni <= 0)
	{
		Unit* u = SpawnUnitNearLocation(GetContext(*leader), leader->pos, *FindUnitData("q_gobliny_mag"), &leader->pos, 5, 2.f);
		if(u)
		{
			if(IsOnline() && !on_enter)
				Net_SpawnUnit(u);
			gobliny_poslaniec = u;
			gobliny_stan = GS_WYGENEROWANO_MAGA;
			StartDialog2(leader->player, u);
		}
	}
}

void Game::UpdateQuests(int days)
{
	if(bandyta)
		return;

	RemoveQuestUnits(false);

	int zysk = 0;

	// tartak
	if(tartak_stan == 2)
	{
		tartak_dni += days;
		if(tartak_dni >= 30 && city_ctx && game_state == GS_LEVEL)
		{
			Unit* u = SpawnUnitNearLocation(GetContext(*leader), leader->pos, *FindUnitData("poslaniec_tartak"), &leader->pos, -2, 2.f);
			if(u)
			{
				if(IsOnline())
					Net_SpawnUnit(u);
				tartak_poslaniec = u;
				StartDialog2(leader->player, u);
			}
		}
	}
	else if(tartak_stan == 3)
	{
		tartak_dni += days;
		int ile = tartak_dni/30;
		if(ile)
		{
			tartak_dni -= ile*30;
			zysk += ile*400;
		}
	}

	// kopalnia
	if(kopalnia_stan2 == KS2_TRWA_BUDOWA)
	{
		kopalnia_dni += days;
		if(kopalnia_stan == KS_MASZ_UDZIALY)
		{
			// gracz zainwestowa� w kopalnie, poinformuj go o wybudowaniu
			if(kopalnia_dni >= kopalnia_ile_dni && city_ctx && game_state == GS_LEVEL)
			{
				Unit* u = SpawnUnitNearLocation(GetContext(*leader), leader->pos, *FindUnitData("poslaniec_kopalnia"), &leader->pos, -2, 2.f);
				if(u)
				{
					if(IsOnline())
						Net_SpawnUnit(u);
					AddNews(Format(txMineBuilt, locations[kopalnia_gdzie]->name.c_str()));
					kopalnia_poslaniec = u;
					StartDialog2(leader->player, u);
				}
			}
		}
		else
		{
			// gracz wybra� z�oto, nie informuj
			if(kopalnia_dni >= kopalnia_ile_dni)
			{
				AddNews(Format(txMineBuilt, locations[kopalnia_gdzie]->name.c_str()));
				kopalnia_stan2 = KS2_WYBUDOWANO;
				kopalnia_dni = 0;
				kopalnia_ile_dni = random(60,90);
			}
		}
	}
	else if(kopalnia_stan2 == KS2_WYBUDOWANO || kopalnia_stan2 == KS2_TRWA_ROZBUDOWA || kopalnia_stan2 == KS2_ROZBUDOWANO)
	{
		// kopalnia jest wybudowana/rozbudowywana/rozbudowana
		// odliczaj czas do informacji o rozbudowie/o uko�czeniu rozbudowy/o znalezieniu portalu
		kopalnia_dni += days;
		if(kopalnia_dni >= kopalnia_ile_dni && city_ctx && game_state == GS_LEVEL)
		{
			Unit* u = SpawnUnitNearLocation(GetContext(*leader), leader->pos, *FindUnitData("poslaniec_kopalnia"), &leader->pos, -2, 2.f);
			if(u)
			{
				if(IsOnline())
					Net_SpawnUnit(u);
				kopalnia_poslaniec = u;
				StartDialog2(leader->player, u);
			}
		}
	}

	// daj z�oto za kopalnie
	if(kopalnia_stan == KS_MASZ_UDZIALY && kopalnia_stan2 >= KS2_WYBUDOWANO)
	{
		kopalnia_dni_zloto += days;
		int ile = kopalnia_dni_zloto/30;
		if(ile)
		{
			kopalnia_dni_zloto -= ile*30;
			zysk += ile*500;
		}
	}
	else if(kopalnia_stan == KS_MASZ_DUZE_UDZIALY && kopalnia_stan2 >= KS2_ROZBUDOWANO)
	{
		kopalnia_dni_zloto += days;
		int ile = kopalnia_dni_zloto/30;
		if(ile)
		{
			kopalnia_dni_zloto -= ile*30;
			zysk += ile*1000;
		}
	}

	if(zysk != 0)
		AddGold(zysk, NULL, true);

	int stan; // 0 - przed zawodami, 1 - czas na zawody, 2 - po zawodach

	if(month < 8)
		stan = 0;
	else if(month == 8)
	{
		if(day < 20)
			stan = 0;
		else if(day == 20)
			stan = 1;
		else
			stan = 2;
	}
	else
		stan = 2;
	
	switch(stan)
	{
	case 0:
		if(chlanie_stan != 0)
		{
			chlanie_stan = 0;
			chlanie_gdzie = GetRandomCityLocation(chlanie_gdzie);
		}
		chlanie_wygenerowano = false;
		chlanie_ludzie.clear();
		break;
	case 1:
		chlanie_stan = 2;
		if(!chlanie_wygenerowano && game_state == GS_LEVEL && current_location == chlanie_gdzie)
			SpawnDrunkmans();
		break;
	case 2:
		chlanie_stan = 1;
		chlanie_wygenerowano = false;
		chlanie_ludzie.clear();
		break;
	}

	//----------------------------
	// magowie
	if(magowie_stan == MS_ODLICZANIE)
	{
		magowie_dni -= days;
		if(magowie_dni <= 0)
		{
			magowie_stan = MS_SPOTKANIE;
			// od teraz mo�na spotyka� golemy
			// zahardcodowane :/
		}
	}

	// orkowie
	if(orkowie_stan == OS_UKONCZONO_DOLACZYL || orkowie_stan == OS_OCZYSZCZONO || orkowie_stan == OS_WYBRAL_KLASE)
	{
		orkowie_dni -= days;
		if(orkowie_dni <= 0)
			orkowie_gorush->auto_talk = 1;
	}

	// gobliny
	if(gobliny_stan == GS_ODLICZANIE || gobliny_stan == GS_SZLACHCIC_ZNIKNAL)
		gobliny_dni -= days;

	// szaleni
	if(szaleni_stan == SS_WZIETO_KAMIEN)
		szaleni_dni -= days;

	// zawody
	if(year != zawody_rok_miasta)
	{
		zawody_rok_miasta = year;
		zawody_miasto = GetRandomCity(zawody_miasto);
		zawody_mistrz = NULL;
	}
	else if(day == 6 && month == 2 && city_ctx && city_ctx->type == L_CITY)
		GenerateTournamentUnits();
	if(month > 2 || (month == 2 && day > 6))
		zawody_rok = year;

	if(city_ctx)
		GenerateQuestUnits2(false);
}

void Game::RemoveQuestUnit(UnitData* ud, bool on_leave)
{
	assert(ud);

	for(vector<Unit*>::iterator it = local_ctx.units->begin(), end = local_ctx.units->end(); it != end; ++it)
	{
		if((*it)->data == ud && (*it)->IsAlive())
		{
			(*it)->to_remove = true;
			to_remove.push_back(*it);
			if(!on_leave && IsOnline())
				Net_RemoveUnit(*it);
			return;
		}
	}

	for(vector<InsideBuilding*>::iterator it2 = city_ctx->inside_buildings.begin(), end2 = city_ctx->inside_buildings.end(); it2 != end2; ++it2)
	{
		for(vector<Unit*>::iterator it = (*it2)->units.begin(), end = (*it2)->units.end(); it != end; ++it)
		{
			if((*it)->data == ud && (*it)->IsAlive())
			{
				(*it)->to_remove = true;
				to_remove.push_back(*it);
				if(!on_leave && IsOnline())
					Net_RemoveUnit(*it);
				return;
			}
		}
	}
}

void Game::RemoveQuestUnits(bool on_leave)
{
	if(city_ctx)
	{
		if(tartak_poslaniec)
		{
			RemoveQuestUnit(FindUnitData("poslaniec_tartak"), on_leave);
			tartak_poslaniec = NULL;
		}

		if(kopalnia_poslaniec)
		{
			RemoveQuestUnit(FindUnitData("poslaniec_kopalnia"), on_leave);
			kopalnia_poslaniec = NULL;
		}

		if(open_location == tartak_miasto && tartak_stan == 2 && tartak_stan2 == 0)
		{
			UnitData* ud = FindUnitData("artur_drwal");
			int id;
			InsideBuilding* inn = city_ctx->FindInn(id);
			for(vector<Unit*>::iterator it = inn->units.begin(), end = inn->units.end(); it != end; ++it)
			{
				if((*it)->data == ud && (*it)->IsAlive())
				{
					(*it)->to_remove = true;
					to_remove.push_back(*it);
					tartak_stan2 = 1;
					if(!on_leave && IsOnline())
						Net_RemoveUnit(*it);
					break;
				}
			}
		}

		if(magowie_uczony && magowie_stan == MS_UCZONY_CZEKA)
		{
			RemoveQuestUnit(FindUnitData("q_magowie_uczony"), on_leave);
			magowie_uczony = NULL;
			magowie_stan = MS_ODLICZANIE;
			magowie_dni = random(15, 30);
		}

		if(orkowie_straznik && orkowie_stan >= OS_STRAZNIK_POGADAL)
		{
			RemoveQuestUnit(FindUnitData("q_orkowie_straznik"), on_leave);
			orkowie_straznik = NULL;
		}
	}

	if(bandyci_stan == BS_AGENT_POGADAL)
	{
		bandyci_stan = BS_AGENT_POSZEDL;
		for(vector<Unit*>::iterator it = local_ctx.units->begin(), end = local_ctx.units->end(); it != end; ++it)
		{
			if(*it == bandyci_agent && (*it)->IsAlive())
			{
				(*it)->to_remove = true;
				to_remove.push_back(*it);
				if(!on_leave && IsOnline())
					Net_RemoveUnit(*it);
				break;
			}
		}
		bandyci_agent = NULL;
	}

	if(magowie_stan == MS_MAG_IDZIE)
	{
		magowie_imie_dobry = magowie_uczony->hero->name;
		magowie_hd.Set(*magowie_uczony->human_data);
		magowie_uczony = NULL;
		RemoveQuestUnit(FindUnitData("q_magowie_stary"), on_leave);
		magowie_stan = MS_MAG_POSZEDL;
	}

	if(gobliny_stan == GS_POSLANIEC_POGADAL && gobliny_poslaniec)
	{
		RemoveQuestUnit(FindUnitData("q_gobliny_poslaniec"), on_leave);
		gobliny_poslaniec = NULL;
	}

	if(gobliny_stan == GS_ODDANO_LUK && gobliny_szlachcic)
	{
		RemoveQuestUnit(FindUnitData("q_gobliny_szlachcic"), on_leave);
		gobliny_szlachcic = NULL;
		gobliny_stan = GS_SZLACHCIC_ZNIKNAL;
		gobliny_dni = random(15,30);
	}

	if(gobliny_stan == GS_MAG_POGADAL && gobliny_poslaniec)
	{
		RemoveQuestUnit(FindUnitData("q_gobliny_mag"), on_leave);
		gobliny_poslaniec = NULL;
		gobliny_stan = GS_MAG_POSZEDL;
	}

	if(zlo_stan == ZS_JOZAN_IDZIE)
	{
		jozan->to_remove = true;
		to_remove.push_back(jozan);
		if(!on_leave && IsOnline())
			Net_RemoveUnit(jozan);
		jozan = NULL;
		zlo_stan = ZS_JOZAN_POSZEDL;
	}
}

cstring tartak_objs[] = {
	"barrel",
	"barrels",
	"box",
	"boxes",
	"torch",
	"torch_off",
	"firewood"
};
const uint n_tartak_objs = countof(tartak_objs);
Obj* tartak_objs_ptrs[n_tartak_objs];

void Game::GenerujTartak(bool w_budowie)
{
	for(vector<Unit*>::iterator it = local_ctx.units->begin(), end = local_ctx.units->end(); it != end; ++it)
		delete *it;
	local_ctx.units->clear();
	local_ctx.bloods->clear();

	// wyr�wnaj teren
	OutsideLocation* outside = (OutsideLocation*)location;
	float* h = outside->h;
	const int _s = outside->size+1;
	vector<INT2> tiles;
	float wys = 0.f;
	for(int y=64-6; y<64+6; ++y)
	{
		for(int x=64-6; x<64+6; ++x)
		{
			if(distance(VEC2(2.f*x+1.f, 2.f*y+1.f), VEC2(128,128)) < 8.f)
			{
				wys += h[x+y*_s];
				tiles.push_back(INT2(x,y));
			}
		}
	}
	wys /= tiles.size();
	for(vector<INT2>::iterator it = tiles.begin(), end = tiles.end(); it != end; ++it)
		h[it->x+it->y*_s] = wys;
	terrain->Rebuild(true);

	// usu� obiekty
	for(vector<Object>::iterator it = local_ctx.objects->begin(), end = local_ctx.objects->end(); it != end;)
	{
		if(distance2d(it->pos, VEC3(128,0,128)) < 16.f)
		{
			if(it+1 == end)
			{
				local_ctx.objects->pop_back();
				break;
			}
			else
			{
				it->Swap(*(end-1));
				local_ctx.objects->pop_back();
				end = local_ctx.objects->end();
			}
		}
		else
			++it;
	}

	if(!tartak_objs_ptrs[0])
	{
		for(uint i=0; i<n_tartak_objs; ++i)
			tartak_objs_ptrs[i] = FindObject(tartak_objs[i]);
	}

	UnitData& ud = *FindUnitData("artur_drwal");
	UnitData& ud2 = *FindUnitData("drwal");
	
	if(w_budowie)
	{
		// artur drwal
		Unit* u = SpawnUnitNearLocation(local_ctx, VEC3(128,0,128), ud, NULL, -2);
		assert(u);
		u->rot = random(MAX_ANGLE);
		u->hero->name = txArthur;
		u->hero->know_name = true;
		hd_artur_drwal.Set(*u->human_data);

		// generuj obiekty
		for(int i=0; i<25; ++i)
		{
			VEC2 pt = random(VEC2(128-16,128-16), VEC2(128+16,128+16));
			Obj* obj = tartak_objs_ptrs[rand2()%n_tartak_objs];
			SpawnObjectNearLocation(local_ctx, obj, pt, random(MAX_ANGLE), 2.f);
		}

		// generuj innych drwali
		int ile = random(5,10);
		for(int i=0; i<ile; ++i)
		{
			Unit* u = SpawnUnitNearLocation(local_ctx, random(VEC3(128-16,0,128-16), VEC3(128+16,0,128+16)), ud2, NULL, -2);
			if(u)
				u->rot = random(MAX_ANGLE);
		}

		tartak_stan2 = 2;
	}
	else
	{
		// budynek
		VEC3 spawn_pt;
		float rot = PI/2*(rand2()%4);
		SpawnObject(local_ctx, FindObject("tartak"), VEC3(128,wys,128), rot, 1.f, &spawn_pt);

		// artur drwal
		Unit* u = SpawnUnitNearLocation(local_ctx, spawn_pt, ud, NULL, -2);
		assert(u);
		u->rot = rot;
		u->hero->name = txArthur;
		u->hero->know_name = true;
		hd_artur_drwal.Set(*u->human_data);

		// obiekty
		for(int i=0; i<25; ++i)
		{
			VEC2 pt = random(VEC2(128-16,128-16), VEC2(128+16,128+16));
			Obj* obj = tartak_objs_ptrs[rand2()%n_tartak_objs];
			SpawnObjectNearLocation(local_ctx, obj, pt, random(MAX_ANGLE), 2.f);
		}
		
		// inni drwale
		int ile = random(5,10);
		for(int i=0; i<ile; ++i)
		{
			Unit* u = SpawnUnitNearLocation(local_ctx, random(VEC3(128-16,0,128-16), VEC3(128+16,0,128+16)), ud2, NULL, -2);
			if(u)
				u->rot = random(MAX_ANGLE);
		}

		tartak_stan2 = 3;
	}
}

void Object::Swap(Object& o)
{
	VEC3 tv;

	tv = pos;
	pos = o.pos;
	o.pos = tv;

	tv = rot;
	rot = o.rot;
	o.rot = tv;

	tv.x = scale;
	scale = o.scale;
	o.scale = tv.x;

	void* p;

	p = mesh;
	mesh = o.mesh;
	o.mesh = (Animesh*)p;

	p = base;
	base = o.base;
	o.base = (Obj*)p;

// 	p = ptr;
// 	ptr = o.ptr;
// 	o.ptr = p;
}

bool Game::GenerujKopalnie()
{
	switch(kopalnia_stan3)
	{
	case KS3_BRAK:
		break;
	case KS3_WYGENEROWANO:
		if(kopalnia_stan2 == KS2_BRAK)
			return true;
		break;
	case KS3_WYGENEROWANO_W_BUDOWIE:
		if(kopalnia_stan2 <= KS2_TRWA_BUDOWA)
			return true;
		break;
	case KS3_WYGENEROWANO_WYBUDOWANY:
		if(kopalnia_stan2 <= KS2_WYBUDOWANO)
			return true;
		break;
	case KS3_WYGENEROWANO_ROZBUDOWANY:
		if(kopalnia_stan2 <= KS2_ROZBUDOWANO)
			return true;
		break;
	case KS3_WYGENEROWANO_PORTAL:
		if(kopalnia_stan2 <= KS2_ZNALEZIONO_PORTAL)
			return true;
		break;
	default:
		assert(0);
		return true;
	}

	CaveLocation* cave = (CaveLocation*)location;
	InsideLocationLevel& lvl = cave->GetLevelData();

	bool respawn_units = true;

	// usu� stare jednostki i krew
	if((kopalnia_stan3 == KS3_BRAK || kopalnia_stan3 == KS3_WYGENEROWANO) && kopalnia_stan2 >= KS2_TRWA_BUDOWA)
	{
		DeleteElements(local_ctx.units);
		DeleteElements(ais);
		local_ctx.units->clear();
		local_ctx.bloods->clear();
		respawn_units = false;
	}

	bool generuj_rude = false;
	int zloto_szansa, powieksz = 0;
	bool rysuj_m = false;

	if(kopalnia_stan3 == KS3_BRAK)
	{
		generuj_rude = true;
		zloto_szansa = 0;
	}

	// pog��b jaskinie
	if(kopalnia_stan2 >= KS2_TRWA_BUDOWA && kopalnia_stan3 < KS3_WYGENEROWANO_W_BUDOWIE)
	{
		generuj_rude = true;
		zloto_szansa = 0;
		++powieksz;
		rysuj_m = true;
	}

	// bardziej pog��b jaskinie
	if(kopalnia_stan2 >= KS2_ROZBUDOWANO && kopalnia_stan3 < KS3_WYGENEROWANO_ROZBUDOWANY)
	{
		generuj_rude = true;
		zloto_szansa = 4;
		++powieksz;
		rysuj_m = true;
	}

	vector<INT2> nowe;

	for(int i=0; i<powieksz; ++i)
	{
		for(int y=1; y<lvl.h-1; ++y)
		{
			for(int x=1; x<lvl.w-1; ++x)
			{
				if(lvl.mapa[x+y*lvl.w].co == SCIANA)
				{
#define A(xx,yy) lvl.mapa[x+(xx)+(y+(yy))*lvl.w].co
					if(rand2()%2 == 0 && (!czy_blokuje21(A(-1,0)) || !czy_blokuje21(A(1,0)) || !czy_blokuje21(A(0,-1)) || !czy_blokuje21(A(0,1))) &&
						(A(-1,-1) != SCHODY_GORA && A(-1,1) != SCHODY_GORA && A(1,-1) != SCHODY_GORA && A(1,1) != SCHODY_GORA))
					{
						Pole& p = lvl.mapa[x+y*lvl.w];
						//p.co = PUSTE;
						CLEAR_BIT(p.flagi, Pole::F_ODKRYTE);
						nowe.push_back(INT2(x,y));
					}
#undef A
				}
			}
		}

		// nie potrzebnie dwa razy to robi je�li powi�ksz = 2
		for(vector<INT2>::iterator it = nowe.begin(), end = nowe.end(); it != end; ++it)
			lvl.mapa[it->x+it->y*lvl.w].co = PUSTE;
	}

	// generuj portal
	if(kopalnia_stan2 >= KS2_ZNALEZIONO_PORTAL && kopalnia_stan3 < KS3_WYGENEROWANO_PORTAL)
	{
		generuj_rude = true;
		zloto_szansa = 7;
		rysuj_m = true;

		// szukaj dobrego miejsca
		vector<INT2> good_pts;
		for(int y=1; y<lvl.h-5; ++y)
		{
			for(int x=1; x<lvl.w-5; ++x)
			{
				for(int h=0; h<5; ++h)
				{
					for(int w=0; w<5; ++w)
					{
						if(lvl.mapa[x+w+(y+h)*lvl.w].co != SCIANA)
							goto dalej;
					}
				}

				// jest dobre miejsce
				good_pts.push_back(INT2(x,y));
dalej:
				;
			}
		}

		if(good_pts.empty())
		{
			if(lvl.schody_gora.x / 26 == 1)
			{
				if(lvl.schody_gora.y / 26 == 1)
					good_pts.push_back(INT2(1,1));
				else
					good_pts.push_back(INT2(1,lvl.h-6));
			}
			else
			{
				if(lvl.schody_gora.y / 26 == 1)
					good_pts.push_back(INT2(lvl.w-6,1));
				else
					good_pts.push_back(INT2(lvl.w-6,lvl.h-6));
			}
		}

		INT2 pt = good_pts[rand2()%good_pts.size()];

		// przygotuj pok�j
		// BBZBB
		// B___B
		// Z___Z
		// B___B
		// BBZBB
		const INT2 p_blokady[] = {
			INT2(0,0),
			INT2(1,0),
			INT2(3,0),
			INT2(4,0),
			INT2(0,1),
			INT2(4,1),
			INT2(0,3),
			INT2(4,3),
			INT2(0,4),
			INT2(1,4),
			INT2(3,4),
			INT2(4,4)
		};
		const INT2 p_zajete[] = {
			INT2(2,0),
			INT2(0,2),
			INT2(4,2),
			INT2(2,4)
		};
		for(int i=0; i<countof(p_blokady); ++i)
		{
			Pole& p = lvl.mapa[(pt+p_blokady[i])(lvl.w)];
			p.co = BLOKADA;
			p.flagi = 0;
		}
		for(int i=0; i<countof(p_zajete); ++i)
		{
			Pole& p = lvl.mapa[(pt+p_zajete[i])(lvl.w)];
			p.co = ZAJETE;
			p.flagi = 0;
		}

		// dor�b wej�cie
		// znajd� najbli�szy pokoju wolny punkt
		const INT2 center(pt.x+2, pt.y+2);
		INT2 closest;
		int best_dist = 999;
		for(int y=1; y<lvl.h-1; ++y)
		{
			for(int x=1; x<lvl.w-1; ++x)
			{
				if(lvl.mapa[x+y*lvl.w].co == PUSTE)
				{
					int dist = distance(INT2(x,y), center);
					if(dist < best_dist && dist > 2)
					{
						best_dist = dist;
						closest = INT2(x,y);
					}
				}
			}
		}

		// prowad� drog� do �rodka
		// tu mo�e by� niesko�czona p�tla ale nie powinno jej by� chyba �e b�d� jakie� blokady na mapie :3
		INT2 end_pt;
		while(true)
		{
			if(abs(closest.x-center.x) > abs(closest.y-center.y))
			{
po_x:
				if(closest.x > center.x)
				{
					--closest.x;
					Pole& p = lvl.mapa[closest.x+closest.y*lvl.w];
					if(p.co == ZAJETE)
					{
						end_pt = closest;
						break;
					}
					else if(p.co == BLOKADA)
					{
						++closest.x;
						goto po_y;
					}
					else
					{
						p.co = PUSTE;
						p.flagi = 0;
						nowe.push_back(closest);
					}
				}
				else
				{
					++closest.x;
					Pole& p = lvl.mapa[closest.x+closest.y*lvl.w];
					if(p.co == ZAJETE)
					{
						end_pt = closest;
						break;
					}
					else if(p.co == BLOKADA)
					{
						--closest.x;
						goto po_y;
					}
					else
					{
						p.co = PUSTE;
						p.flagi = 0;
						nowe.push_back(closest);
					}
				}
			}
			else
			{
po_y:
				if(closest.y > center.y)
				{
					--closest.y;
					Pole& p = lvl.mapa[closest.x+closest.y*lvl.w];
					if(p.co == ZAJETE)
					{
						end_pt = closest;
						break;
					}
					else if(p.co == BLOKADA)
					{
						++closest.y;
						goto po_x;
					}
					else
					{
						p.co = PUSTE;
						p.flagi = 0;
						nowe.push_back(closest);
					}
				}
				else
				{
					++closest.y;
					Pole& p = lvl.mapa[closest.x+closest.y*lvl.w];
					if(p.co == ZAJETE)
					{
						end_pt = closest;
						break;
					}
					else if(p.co == BLOKADA)
					{
						--closest.y;
						goto po_x;
					}
					else
					{
						p.co = PUSTE;
						p.flagi = 0;
						nowe.push_back(closest);
					}
				}
			}
		}
		
		// ustaw �ciany
		for(int i=0; i<countof(p_blokady); ++i)
		{
			Pole& p = lvl.mapa[(pt+p_blokady[i])(lvl.w)];
			p.co = SCIANA;
		}
		for(int i=0; i<countof(p_zajete); ++i)
		{
			Pole& p = lvl.mapa[(pt+p_zajete[i])(lvl.w)];
			p.co = SCIANA;
		}
		for(int y=1; y<4; ++y)
		{
			for(int x=1; x<4; ++x)
			{
				Pole& p = lvl.mapa[pt.x+x+(pt.y+y)*lvl.w];
				p.co = PUSTE;
				p.flagi = Pole::F_DRUGA_TEKSTURA;
			}
		}
		Pole& p = lvl.mapa[end_pt(lvl.w)];
		p.co = DRZWI;
		p.flagi = 0;

		// ustaw pok�j
		Pokoj& pok = Add1(lvl.pokoje);
		pok.cel = POKOJ_CEL_PORTAL;
		pok.korytarz = false;
		pok.pos = pt;
		pok.size = INT2(5,5);

		// dodaj drzwi, portal, pochodnie
		Obj* portal = FindObject("portal"),
		   * pochodnia = FindObject("torch");

		// drzwi
		{
			Object& o = Add1(local_ctx.objects);
			o.mesh = aNaDrzwi;
			o.pos = VEC3(float(end_pt.x*2)+1,0,float(end_pt.y*2)+1);
			o.scale = 1;
			o.base = NULL;

			// hack :3
			Pokoj& p2 = Add1(lvl.pokoje);
			p2.korytarz = true;

			if(czy_blokuje2(lvl.mapa[end_pt.x-1+end_pt.y*lvl.w].co))
			{
				o.rot = VEC3(0,0,0);
				if(end_pt.y > center.y)
				{
					o.pos.z -= 0.8229f;
					lvl.At(end_pt+INT2(0,1)).pokoj = 1;
				}
				else
				{
					o.pos.z += 0.8229f;
					lvl.At(end_pt+INT2(0,-1)).pokoj = 1;
				}
			}
			else
			{
				o.rot = VEC3(0,PI/2,0);
				if(end_pt.x > center.x)
				{
					o.pos.x -= 0.8229f;
					lvl.At(end_pt+INT2(1,0)).pokoj = 1;
				}
				else
				{
					o.pos.x += 0.8229f;
					lvl.At(end_pt+INT2(-1,0)).pokoj = 1;
				}
			}

			Door* door = new Door;
			local_ctx.doors->push_back(door);
			door->pt = end_pt;
			door->pos = o.pos;
			door->rot = o.rot.y;
			door->state = Door::Closed;
			door->ani = new AnimeshInstance(aDrzwi);
			door->ani->groups[0].speed = 2.f;
			door->phy = new btCollisionObject;
			door->phy->setCollisionShape(shape_door);
			door->locked = LOCK_KOPALNIA;
			door->netid = door_netid_counter++;
			btTransform& tr = door->phy->getWorldTransform();
			VEC3 pos = door->pos;
			pos.y += 1.319f;
			tr.setOrigin(ToVector3(pos));
			tr.setRotation(btQuaternion(door->rot, 0, 0));
			phy_world->addCollisionObject(door->phy);
		}

		// pochodnia
		{
			VEC3 pos(2.f*(pt.x+1), 0, 2.f*(pt.y+1));

			switch(rand2()%4)
			{
			case 0:
				pos.x += pochodnia->r*2;
				pos.z += pochodnia->r*2;
				break;
			case 1:
				pos.x += 6.f-pochodnia->r*2;
				pos.z += pochodnia->r*2;
				break;
			case 2:
				pos.x += pochodnia->r*2;
				pos.z += 6.f-pochodnia->r*2;
				break;
			case 3:
				pos.x += 6.f-pochodnia->r*2;
				pos.z += 6.f-pochodnia->r*2;
				break;
			}

			SpawnObject(local_ctx, pochodnia, pos, random(MAX_ANGLE));
		}

		// portal
		{
			float rot;
			if(end_pt.y == center.y)
			{
				if(end_pt.x > center.x)
					rot = PI*3/2;
				else
					rot = PI/2;
			}
			else
			{
				if(end_pt.y > center.y)
					rot = 0;
				else
					rot = PI;
			}

			rot = clip(rot+PI);

			// obiekt
			const VEC3 pos(2.f*pt.x+5,0,2.f*pt.y+5);
			SpawnObject(local_ctx, portal, pos, rot);

			// lokacja
			SingleInsideLocation* loc = new SingleInsideLocation;
			loc->active_quest = (Quest_Dungeon*)FindQuest(kopalnia_refid);
			loc->cel = KOPALNIA_POZIOM;
			loc->from_portal = true;
			loc->name = txAncientArmory;
			loc->pos = VEC2(-999,-999);
			loc->spawn = SG_GOLEMY;
			loc->st = 14;
			loc->type = L_DUNGEON;
			int loc_id = AddLocation(loc);
			Quest_Kopalnia* q = (Quest_Kopalnia*)FindQuest(kopalnia_refid);
			q->sub.target_loc = q->dungeon_loc = loc_id;

			// funkcjonalno�� portalu
			cave->portal = new Portal;
			cave->portal->at_level = 0;
			cave->portal->target = 0;
			cave->portal->target_loc = loc_id;
			cave->portal->rot = rot;
			cave->portal->next_portal = NULL;
			cave->portal->pos = pos;

			// info dla portalu po drugiej stronie
			loc->portal = new Portal;
			loc->portal->at_level = 0;
			loc->portal->target = 0;
			loc->portal->target_loc = current_location;
			loc->portal->next_portal = NULL;
		}
	}

	if(!nowe.empty())
		regenerate_cave_flags(lvl.mapa, lvl.w);

#ifdef _DEBUG
	if(rysuj_m)
		rysuj_mape_konsola(lvl.mapa, lvl.w, lvl.h);
#endif

	// generuj rud�
	if(generuj_rude)
	{
		Obj* iron_ore = FindObject("iron_ore");

		// usu� star� rud�
		if(kopalnia_stan3 != KS3_BRAK)
			DeleteElements(local_ctx.useables);

		// dodaj now�
		for(int y=1; y<lvl.h-1; ++y)
		{
			for(int x=1; x<lvl.w-1; ++x)
			{
				if(rand2()%3 == 0)
					continue;

#define P(xx,yy) !czy_blokuje21(lvl.mapa[x-(xx)+(y+(yy))*lvl.w])
#undef S
#define S(xx,yy) lvl.mapa[x-(xx)+(y+(yy))*lvl.w].co == SCIANA

				// ruda jest generowana dla takich przypadk�w, w tym obr�conych
				//  ### ### ###
				//  _?_ #?_ #?#
				//  ___ #__ #_#
				if(lvl.mapa[x+y*lvl.w].co == PUSTE && rand2()%3 != 0 && !IS_SET(lvl.mapa[x+y*lvl.w].flagi, Pole::F_DRUGA_TEKSTURA))
				{
					int dir = -1;

					// ma by� �ciana i wolne z ty�u oraz wolne na lewo lub prawo lub zaj�te z obu stron
					// __#
					// _?#
					// __#
					if(P(-1,0) && S(1,0) && S(1,-1) && S(1,1) && ((P(-1,-1) && P(0,-1)) || (P(-1,1) && P(0,1)) || (S(-1,-1) && S(0,-1) && S(-1,1) && S(0,1))))
					{
						dir = 1;
					}
					// #__
					// #?_
					// #__
					else if(P(1,0) && S(-1,0) && S(-1,1) && S(-1,-1) && ((P(0,-1) && P(1,-1)) || (P(0,1) && P(1,1)) || (S(0,-1) && S(1,-1) && S(0,1) && S(1,1))))
					{
						dir = 3;
					}
					// ### 
					// _?_
					// ___
					else if(P(0,1) && S(0,-1) && S(-1,-1) && S(1,-1) && ((P(-1,0) && P(-1,1)) || (P(1,0) && P(1,1)) || (S(-1,0) && S(-1,1) && S(1,0) && S(1,1))))
					{
						dir = 0;
					}
					// ___
					// _?_
					// ###
					else if(P(0,-1) && S(0,1) && S(-1,1) && S(1,1) && ((P(-1,0) && P(-1,-1)) || (P(1,0) && P(1,-1)) || (S(-1,0) && S(-1,-1) && S(1,0) && S(1,-1))))
					{
						dir = 2;
					}

					if(dir != -1)
					{
						VEC3 pos(2.f*x+1,0,2.f*y+1);

						switch(dir)
						{
						case 0:
							pos.z -= 1.f;
							break;
						case 1:
							pos.x -= 1.f;
							break;
						case 2:
							pos.z += 1.f;
							break;
						case 3:
							pos.x += 1.f;
							break;
						}

						float rot = clip(dir_to_rot(dir)+PI);
						static float radius = max(iron_ore->size.x, iron_ore->size.y) * SQRT_2;

						IgnoreObjects ignore = {0};
						ignore.ignore_blocks = true;
						global_col.clear();
						GatherCollisionObjects(local_ctx, global_col, pos, radius, &ignore);

						BOX2D box(pos.x-iron_ore->size.x, pos.z-iron_ore->size.y, pos.x+iron_ore->size.x, pos.z+iron_ore->size.y);

						if(!Collide(global_col, box, 0.f, rot))
						{
							Useable* u = new Useable;
							u->pos = pos;
							u->rot = rot;
							u->type = (rand2()%10 < zloto_szansa ? U_ZYLA_ZLOTA : U_ZYLA_ZELAZA);
							u->user = NULL;
							u->netid = useable_netid_counter++;
							local_ctx.useables->push_back(u);

							CollisionObject& c = Add1(local_ctx.colliders);
							btCollisionObject* cobj = new btCollisionObject;
							cobj->setCollisionShape(iron_ore->shape);

							btTransform& tr = cobj->getWorldTransform();
							VEC3 zero(0,0,0), pos2;
							D3DXVec3TransformCoord(&pos2, &zero, iron_ore->matrix);
							pos2 += pos;
							tr.setOrigin(ToVector3(pos2));
							tr.setRotation(btQuaternion(rot, 0, 0));

							c.pt = VEC2(pos2.x, pos2.z);
							c.w = iron_ore->size.x;
							c.h = iron_ore->size.y;
							if(not_zero(rot))
							{
								c.type = CollisionObject::RECTANGLE_ROT;
								c.rot = rot;
								c.radius = radius;
							}
							else
								c.type = CollisionObject::RECTANGLE;

							phy_world->addCollisionObject(cobj, CG_WALL);
						}
					}
#undef P
#undef S
				}
			}
		}
	}

	// generuj nowe obiekty
	if(!nowe.empty())
	{
		Obj* kamien = FindObject("rock"),
		   * krzak = FindObject("plant2"),
		   * grzyb = FindObject("mushrooms");

		for(vector<INT2>::iterator it = nowe.begin(), end = nowe.end(); it != end; ++it)
		{
			if(rand2()%10 == 0)
			{
				Obj* obj;
				switch(rand2()%3)
				{
				case 0:
					obj = kamien;
					break;
				case 1:
					obj = krzak;
					break;
				case 2:
					obj = grzyb;
					break;
				}

				SpawnObject(local_ctx, obj, VEC3(2.f*it->x+random(0.1f,1.9f), 0.f, 2.f*it->y+random(0.1f,1.9f)), random(MAX_ANGLE));
			}
		}
	}

	// generuj jednostki
	bool ustaw = true;

	if(kopalnia_stan3 < KS3_WYGENEROWANO_W_BUDOWIE && kopalnia_stan2 >= KS2_TRWA_BUDOWA)
	{
		ustaw = false;

		// szef g�rnik�w na wprost wej�cia
		INT2 pt = lvl.schody_gora + g_kierunek2[lvl.schody_gora_dir];
		int odl = 1;
		while(lvl.mapa[pt(lvl.w)].co == PUSTE && odl < 5)
		{
			pt += g_kierunek2[lvl.schody_gora_dir];
			++odl;
		}
		pt -= g_kierunek2[lvl.schody_gora_dir];

		SpawnUnitNearLocation(local_ctx, VEC3(2.f*pt.x+1,0,2.f*pt.y+1), *FindUnitData("gornik_szef"), &VEC3(2.f*lvl.schody_gora.x+1,0,2.f*lvl.schody_gora.y+1), -2);

		// g�rnicy
		UnitData& gornik = *FindUnitData("gornik");
		for(int i=0; i<10; ++i)
		{
			for(int j=0; j<15; ++j)
			{
				INT2 tile = cave->GetRandomTile();
				const Pole& p = lvl.At(tile);
				if(p.co == PUSTE && !IS_SET(p.flagi, Pole::F_DRUGA_TEKSTURA))
				{
					SpawnUnitNearLocation(local_ctx, VEC3(2.f*tile.x+random(0.4f,1.6f),0,2.f*tile.y+random(0.4f,1.6f)), gornik, NULL, -2);
					break;
				}
			}
		}
	}

	// ustaw jednostki
	if(!ustaw && kopalnia_stan3 >= KS3_WYGENEROWANO_W_BUDOWIE)
	{
		UnitData* gornik = FindUnitData("gornik"),
			* szef_gornikow = FindUnitData("gornik_szef");
		for(vector<Unit*>::iterator it = local_ctx.units->begin(), end = local_ctx.units->end(); it != end; ++it)
		{
			Unit* u = *it;
			if(u->IsAlive())
			{
				if(u->data == gornik)
				{
					for(int i=0; i<10; ++i)
					{
						INT2 tile = cave->GetRandomTile();
						const Pole& p = lvl.At(tile);
						if(p.co == PUSTE && !IS_SET(p.flagi, Pole::F_DRUGA_TEKSTURA))
						{
							WarpUnit(*u, VEC3(2.f*tile.x+random(0.4f,1.6f),0,2.f*tile.y+random(0.4f,1.6f)));
							break;
						}
					}
				}
				else if(u->data == szef_gornikow)
				{
					INT2 pt = lvl.schody_gora + g_kierunek2[lvl.schody_gora_dir];
					int odl = 1;
					while(lvl.mapa[pt(lvl.w)].co == PUSTE && odl < 5)
					{
						pt += g_kierunek2[lvl.schody_gora_dir];
						++odl;
					}
					pt -= g_kierunek2[lvl.schody_gora_dir];

					WarpUnit(*u, VEC3(2.f*pt.x+1,0,2.f*pt.y+1));
				}
			}
		}
	}

	switch(kopalnia_stan2)
	{
	case KS2_BRAK:
		kopalnia_stan3 = KS3_WYGENEROWANO;
		break;
	case KS2_TRWA_BUDOWA:
		kopalnia_stan3 = KS3_WYGENEROWANO_W_BUDOWIE;
		break;
	case KS2_WYBUDOWANO:
	case KS2_MOZLIWA_ROZBUDOWA:
	case KS2_TRWA_ROZBUDOWA:
		kopalnia_stan3 = KS3_WYGENEROWANO_WYBUDOWANY;
		break;
	case KS2_ROZBUDOWANO:
		kopalnia_stan3 = KS3_WYGENEROWANO_ROZBUDOWANY;
		break;
	case KS2_ZNALEZIONO_PORTAL:
		kopalnia_stan3 = KS3_WYGENEROWANO_PORTAL;
		break;
	default:
		assert(0);
		break;
	}

	return respawn_units;
}

void Game::HandleUnitEvent(UnitEventHandler::TYPE event, Unit* unit)
{
	assert(unit);

	if(event == UnitEventHandler::FALL)
	{
		// jednostka poleg�a w zawodach w piciu :3
		unit->look_target = NULL;
		unit->busy = Unit::Busy_No;
		unit->event_handler = NULL;
		RemoveElement(chlanie_ludzie, unit);

		if(IsOnline() && unit->IsPlayer() && unit->player != pc)
		{
			NetChangePlayer& c = Add1(net_changes_player);
			c.type = NetChangePlayer::LOOK_AT;
			c.pc = unit->player;
			c.id = -1;
			GetPlayerInfo(c.pc).NeedUpdate();
		}
	}
}

int Game::GetUnitEventHandlerQuestRefid()
{
	// specjalna warto�� u�ywana dla wska�nika na Game
	return -2;
}

bool Game::IsTeamNotBusy()
{
	for(vector<Unit*>::iterator it = team.begin(), end = team.end(); it != end; ++it)
	{
		if((*it)->busy)
			return false;
	}

	return true;
}

// czy ktokolwiek w dru�ynie rozmawia
// b�dzie u�ywane w multiplayer
bool Game::IsAnyoneTalking() const
{
	if(IsLocal())
	{
		if(IsOnline())
		{
			for(vector<Unit*>::const_iterator it = active_team.begin(), end = active_team.end(); it != end; ++it)
			{
				if((*it)->IsPlayer() && (*it)->player->dialog_ctx->dialog_mode)
					return true;
			}
			return false;
		}
		else
			return dialog_context.dialog_mode;
	}
	else
		return anyone_talking;
}

bool Game::FindItemInTeam(const Item* item, int refid, Unit** unit, int* i_index, bool check_npc)
{
	assert(item);

	for(vector<Unit*>::iterator it = team.begin(), end = team.end(); it != end; ++it)
	{
		if((*it)->IsPlayer() || check_npc)
		{
			int index = (*it)->FindItem(item, refid);
			if(index != INVALID_IINDEX)
			{
				if(unit)
					*unit = *it;
				if(i_index)
					*i_index = index;
				return true;
			}
		}
	}

	return false;
}

bool Game::RemoveQuestItem(const Item* item, int refid)
{
	Unit* unit;
	int slot_id;

	if(FindItemInTeam(item, refid, &unit, &slot_id))
	{
		RemoveItem(*unit, slot_id, 1);
		return true;
	}
	else
		return false;
}

void Game::EndUniqueQuest()
{
	++unique_quests_completed;
}

Pokoj& Game::GetRoom(InsideLocationLevel& lvl, int cel, bool schody_dol)
{
	if(cel == POKOJ_CEL_BRAK)
		return lvl.GetFarRoom(schody_dol);
	else
	{
		int id = lvl.FindRoomId(cel);
		if(id == -1)
		{
			assert(0);
			id = 0;
		}

		return lvl.pokoje[id];
	}
}

void Game::AddNews(cstring text)
{
	assert(text);

	News* n = new News;
	n->text = text;
	n->add_time = worldtime;

	news.push_back(n);
}

void Game::SetMusic(MUSIC_TYPE type)
{
	if(nomusic || type == music_type)
		return;

	music_type = type;
	SetupTracks();
}

void Game::SetupTracks()
{
	tracks.clear();

	for(uint i=0; i<n_musics; ++i)
	{
		if(g_musics[i].type == music_type)
			tracks.push_back(&g_musics[i]);
	}

	if(tracks.empty())
	{
		PlayMusic(NULL);
		return;
	}

	if(tracks.size() != 1)
	{
		std::random_shuffle(tracks.begin(), tracks.end(), myrand);

		if(tracks.front()->snd == last_music)
			std::iter_swap(tracks.begin(), tracks.end()-1);
	}

	track_id = 0;
	last_music = tracks.front()->snd;
	PlayMusic(last_music);
	music_ended = false;
}

void Game::UpdateMusic()
{
	if(nomusic || music_type == MUSIC_MISSING || tracks.empty())
		return;

	if(music_ended)
	{
		if(track_id == tracks.size()-1)
			SetupTracks();
		else
		{
			++track_id;
			last_music = tracks[track_id]->snd;
			PlayMusic(last_music);
			music_ended = false;
		}
	}
}

void Game::SetMusic()
{
	if(nomusic)
		return;

	if(!IsLocal() && boss_level_mp)
	{
		SetMusic(MUSIC_BOSS);
		return;
	}

	for(vector<INT2>::iterator it = boss_levels.begin(), end = boss_levels.end(); it != end; ++it)
	{
		if(current_location == it->x && dungeon_level == it->y)
		{
			SetMusic(MUSIC_BOSS);
			return;
		}
	}

	MUSIC_TYPE type;

	switch(location->type)
	{
	case L_CITY:
	case L_VILLAGE:
		type = MUSIC_CITY;
		break;
	case L_CRYPT:
		type = MUSIC_CRYPT;
		break;
	case L_DUNGEON:
	case L_CAVE:
		type = MUSIC_DUNGEON;
		break;
	case L_FOREST:
	case L_CAMP:
		if(current_location == sekret_gdzie2)
			type = MUSIC_MOONWELL;
		else
			type = MUSIC_FOREST;
		break;
	case L_ENCOUNTER:
		type = MUSIC_TRAVEL;
		break;
	case L_MOONWELL:
		type = MUSIC_MOONWELL;
		break;
	default:
		assert(0);
		type = MUSIC_DUNGEON;
		break;
	}

	SetMusic(type);
}

void Game::UpdateGame2(float dt)
{
	// arena
	if(arena_tryb != Arena_Brak)
	{
		if(arena_etap == Arena_OdliczanieDoPrzeniesienia)
		{
			arena_t += dt*2;
			if(arena_t >= 1.f)
			{
				if(arena_tryb == Arena_Walka)
				{
					for(vector<Unit*>::iterator it = at_arena.begin(), end = at_arena.end(); it != end; ++it)
					{
						if((*it)->in_arena == 0)
						{
							UnitWarpData& uwd = Add1(unit_warp_data);
							uwd.unit = *it;
							uwd.where = -2;
						}
					}
				}
				else
				{
					for(vector<Unit*>::iterator it = at_arena.begin(), end = at_arena.end(); it != end; ++it)
					{
						UnitWarpData& uwd = Add1(unit_warp_data);
						uwd.unit = *it;
						uwd.where = -2;
					}

					at_arena[0]->in_arena = 0;
					at_arena[1]->in_arena = 1;

					if(IsOnline())
					{
						{
							NetChange& c = Add1(net_changes);
							c.type = NetChange::CHANGE_ARENA_STATE;
							c.unit = at_arena[0];
						}
						{
							NetChange& c = Add1(net_changes);
							c.type = NetChange::CHANGE_ARENA_STATE;
							c.unit = at_arena[1];
						}
					}
				}

				arena_etap = Arena_OdliczanieDoStartu;
				arena_t = 0.f;
			}
		}
		else if(arena_etap == Arena_OdliczanieDoStartu)
		{
			arena_t += dt;
			if(arena_t >= 2.f)
			{
				if(sound_volume)
					PlaySound2d(sArenaFight);
				if(IsOnline())
				{
					NetChange& c = Add1(net_changes);
					c.type = NetChange::ARENA_SOUND;
					c.id = 0;
				}
				arena_etap = Arena_TrwaWalka;
				for(vector<Unit*>::iterator it = at_arena.begin(), end = at_arena.end(); it != end; ++it)
				{
					(*it)->frozen = 0;
					if((*it)->IsPlayer() && (*it)->player != pc)
					{
						NetChangePlayer& c = Add1(net_changes_player);
						c.type = NetChangePlayer::START_ARENA_COMBAT;
						c.pc = (*it)->player;
						GetPlayerInfo(c.pc).NeedUpdate();
					}
				}
			}
		}
		else if(arena_etap == Arena_TrwaWalka)
		{
			// gadanie przez obserwator�w
			for(vector<Unit*>::iterator it = arena_viewers.begin(), end = arena_viewers.end(); it != end; ++it)
			{
				Unit& u = **it;
				u.ai->loc_timer -= dt;
				if(u.ai->loc_timer <= 0.f)
				{
					u.ai->loc_timer = random(6.f,12.f);

					cstring text;
					if(rand2()%2 == 0)
						text = random_string(txArenaText);
					else
						text = Format(random_string(txArenaTextU), GetRandomArenaHero()->GetRealName());

					UnitTalk(u, text);
				}
			}

			// sprawd� ile os�b jeszcze �yje
			int ile[2] = {0};
			for(vector<Unit*>::iterator it = at_arena.begin(), end = at_arena.end(); it != end; ++it)
			{
				if((*it)->live_state != Unit::DEAD)
					++ile[(*it)->in_arena];
			}

			if(ile[0] == 0 || ile[1] == 0)
			{
				arena_etap = Arena_OdliczanieDoKonca;
				arena_t = 0.f;
				bool victory_sound;
				if(ile[0] == 0)
				{
					arena_wynik = 1;
					if(arena_tryb == Arena_Walka)
						victory_sound = false;
					else
						victory_sound = true;
				}
				else
				{
					arena_wynik = 0;
					victory_sound = true;
				}

				if(sound_volume)
					PlaySound2d(victory_sound ? sArenaWygrana : sArenaPrzegrana);
				if(IsOnline())
				{
					NetChange& c = Add1(net_changes);
					c.type = NetChange::ARENA_SOUND;
					c.id = victory_sound ? 1 : 2;
				}
			}
		}
		else if(arena_etap == Arena_OdliczanieDoKonca)
		{
			arena_t += dt;
			if(arena_t >= 2.f)
			{
				for(vector<Unit*>::iterator it = at_arena.begin(), end = at_arena.end(); it != end; ++it)
				{
					(*it)->frozen = 2;
					if((*it)->IsPlayer())
					{
						if((*it)->player == pc)
						{
							fallback_co = FALLBACK_ARENA_EXIT;
							fallback_t = -1.f;
						}
						else
						{
							NetChangePlayer& c = Add1(net_changes_player);
							c.type = NetChangePlayer::EXIT_ARENA;
							c.pc = (*it)->player;
							GetPlayerInfo(c.pc).NeedUpdate();
						}
					}
				}

				arena_etap = Arena_OdliczanieDoWyjscia;
				arena_t = 0.f;
			}
		}
		else
		{
			arena_t += dt*2;
			if(arena_t >= 1.f)
			{
				if(arena_tryb == Arena_Walka)
				{
					if(arena_wynik == 0)
					{
						int ile;
						switch(arena_poziom)
						{
						case 1:
							ile = 150;
							break;
						case 2:
							ile = 350;
							break;
						case 3:
							ile = 750;
							break;
						}

						AddGoldArena(ile);
					}

					for(vector<Unit*>::iterator it = at_arena.begin(), end = at_arena.end(); it != end; ++it)
					{
						if((*it)->in_arena == 0)
						{
							(*it)->frozen = 0;
							(*it)->in_arena = -1;
							if((*it)->hp <= 0.f)
							{
								(*it)->HealPoison();
								(*it)->live_state = Unit::ALIVE;
								(*it)->ani->Play("wstaje2", PLAY_ONCE|PLAY_PRIO3, 0);
								(*it)->action = A_ANIMATION;
								if((*it)->IsAI())
									(*it)->ai->Reset();
								if(IsOnline())
								{
									NetChange& c = Add1(net_changes);
									c.type = NetChange::STAND_UP;
									c.unit = *it;
								}
							}

							UnitWarpData& warp_data = Add1(unit_warp_data);
							warp_data.unit = *it;
							warp_data.where = -1;

							if(IsOnline())
							{
								NetChange& c = Add1(net_changes);
								c.type = NetChange::CHANGE_ARENA_STATE;
								c.unit = *it;
							}
						}
						else
						{
							to_remove.push_back(*it);
							(*it)->to_remove = true;
							if(IsOnline())
								Net_RemoveUnit(*it);
						}
					}
				}
				else
				{
					for(vector<Unit*>::iterator it = at_arena.begin(), end = at_arena.end(); it != end; ++it)
					{
						(*it)->frozen = 0;
						(*it)->in_arena = -1;
						if((*it)->hp <= 0.f)
						{
							(*it)->HealPoison();
							(*it)->live_state = Unit::ALIVE;
							(*it)->ani->Play("wstaje2", PLAY_ONCE|PLAY_PRIO3, 0);
							(*it)->action = A_ANIMATION;
							if((*it)->IsAI())
								(*it)->ai->Reset();
							if(IsOnline())
							{
								NetChange& c = Add1(net_changes);
								c.type = NetChange::STAND_UP;
								c.unit = *it;
							}
						}

						UnitWarpData& warp_data = Add1(unit_warp_data);
						warp_data.unit = *it;
						warp_data.where = -1;

						if(IsOnline())
						{
							NetChange& c = Add1(net_changes);
							c.type = NetChange::CHANGE_ARENA_STATE;
							c.unit = *it;
						}
					}

					if(arena_tryb == Arena_Pvp && arena_fighter->IsHero())
					{
						arena_fighter->hero->lost_pvp = (arena_wynik == 0);
						StartDialog2(pvp_player, arena_fighter, IS_SET(arena_fighter->data->flagi, F_SZALONY) ? dialog_szalony_pvp : dialog_hero_pvp);
					}
				}

				if(arena_tryb != Arena_Zawody)
				{
					RemoveArenaViewers();
					arena_viewers.clear();
					at_arena.clear();
				}
				else
					zawody_stan3 = 5;
				arena_tryb = Arena_Brak;
				arena_free = true;
			}
		}
	}

	// zawody
	if(zawody_stan != IS_BRAK)
		UpdateTournament(dt);

	// wymiana sprz�tu dru�yny
	if(fallback_co == -1 && team_share_id != -1)
	{
		TeamShareItem& tsi = team_shares[team_share_id];
		int state = 1; // 0-ta wymiana nie jest ju� potrzebna, 1-poinformuj o wymianie, 2-czekaj na rozmowe
		DialogEntry* dialog = NULL;
		if(tsi.to->busy != Unit::Busy_No)
			state = 2;
		else if(!CheckTeamShareItem(tsi))
			state = 0;
		else
		{
			ItemSlot& slot = tsi.from->items[tsi.index];
			if(!IsBetterItem(*tsi.to, tsi.item))
				state = 0;
			else
			{
				int prev_item_weight;
				ITEM_SLOT slot_type = ItemTypeToSlot(slot.item->type);
				if(tsi.to->slots[slot_type])
					prev_item_weight = tsi.to->slots[slot_type]->weight;
				else
					prev_item_weight = 0;

				if(tsi.to->weight + slot.item->weight - prev_item_weight > tsi.to->weight_max)
				{
					// nie bierz przedmiotu bo nie masz miejsca
					state = 0;
				}
				else if(slot.team_count == 0)
				{
					if(tsi.from->IsHero())
					{
						// jeden NPC sprzedaje drugiemu przedmiot
						state = 0;
						tsi.to->AddItem(tsi.item, 1, false);
						int value = tsi.item->value/2;
						tsi.to->gold -= value;
						tsi.from->gold += value;
						tsi.from->items.erase(tsi.from->items.begin()+tsi.index);
						UpdateUnitInventory(*tsi.to);
					}
					else
					{
						// NPC chce kupi� przedmiot od PC
						if(tsi.to->gold >= tsi.item->value/2)
						{
							if(distance2d(tsi.from->pos, tsi.to->pos) > 8.f)
								state = 0;
							else if(tsi.from->busy == Unit::Busy_No && tsi.from->player->action == PlayerController::Action_None)
							{
								if(IS_SET(tsi.to->data->flagi, F_SZALONY))
									dialog = dialog_szalony_przedmiot_kup;
								else
									dialog = dialog_hero_przedmiot_kup;
							}
							else
								state = 2;
						}
						else
							state = 0;
					}
				}
				else
				{
					if(tsi.from->IsHero())
					{
						// NPC chce wzi��� przedmiot na kredyt od NPC, pyta przyw�dcy
						if(distance2d(tsi.to->pos, leader->pos) > 8.f)
							state = 0;
						else if(leader->busy == Unit::Busy_No && leader->player->action == PlayerController::Action_None)
						{
							if(IS_SET(tsi.to->data->flagi, F_SZALONY))
								dialog = dialog_szalony_przedmiot;
							else
								dialog = dialog_hero_przedmiot;
						}
						else
							state = 2;
					}
					else
					{
						// NPC chce wzi��� przedmiot na kredyt od PC
						if(distance2d(tsi.from->pos, tsi.to->pos) > 8.f)
							state = 0;
						else if(tsi.from->busy == Unit::Busy_No && tsi.from->player->action == PlayerController::Action_None)
						{
							if(IS_SET(tsi.to->data->flagi, F_SZALONY))
								dialog = dialog_szalony_przedmiot;
							else
								dialog = dialog_hero_przedmiot;
						}
						else
							state = 2;
					}
				}
			}
		}

		if(state < 2)
		{
			if(state == 1)
			{
				if(tsi.from->IsPlayer())
				{
					DialogContext& ctx = *tsi.from->player->dialog_ctx;
					ctx.team_share_id = team_share_id;
					ctx.team_share_item = tsi.from->items[tsi.index].item;
					StartDialog2(tsi.from->player, tsi.to, dialog);
				}
				else
				{
					DialogContext& ctx = *leader->player->dialog_ctx;
					ctx.team_share_id = team_share_id;
					ctx.team_share_item = tsi.from->items[tsi.index].item;
					StartDialog2(leader->player, tsi.to, dialog);
				}
			}

			++team_share_id;
			if(team_share_id == (int)team_shares.size())
				team_share_id = -1;
		}
	}

	// zawody w piciu
	UpdateContest(dt);

	// quest bandyci
	if(bandyci_stan == BS_ODLICZANIE)
	{
		bandyci_czas -= dt;
		if(bandyci_czas <= 0.f)
		{
			// wygeneruj agenta
			Unit* u = SpawnUnitNearLocation(GetContext(*leader), leader->pos, *FindUnitData("agent"), &leader->pos, -2, 2.f);
			if(u)
			{
				if(IsOnline())
					Net_SpawnUnit(u);
				bandyci_stan = BS_AGENT_PRZYSZEDL;
				bandyci_agent = u;
				StartDialog2(leader->player, u);
			}
		}
	}

	// quest magowie
	if(magowie_stan == MS_STARY_MAG_DOLACZYL && current_location == magowie_gdzie)
	{
		magowie_czas += dt;
		if(magowie_czas >= 30.f && magowie_uczony->auto_talk == 0)
			magowie_uczony->auto_talk = 1;
	}

	// quest z�o
	if(zlo_stan == ZS_OLTARZ_STWORZONY && current_location == zlo_gdzie)
	{
		for(vector<Unit*>::iterator it = team.begin(), end = team.end(); it != end; ++it)
		{
			Unit& u = **it;
			if(u.IsStanding() && u.IsPlayer() && distance(u.pos, zlo_pos) < 5.f && CanSee(u.pos, zlo_pos))
			{
				zlo_stan = ZS_PRZYWOLANIE;
				if(sound_volume)
					PlaySound2d(sEvil);
				FindQuest(zlo_refid)->SetProgress(4);
				// stw�rz nieumar�ych
				InsideLocation* inside = (InsideLocation*)location;
				inside->spawn = SG_NIEUMARLI;
				uint offset = local_ctx.units->size();
				GenerateDungeonUnits();
				if(IsOnline())
				{
					PushNetChange(NetChange::EVIL_SOUND);
					for(uint i=offset, ile=local_ctx.units->size(); i<ile; ++i)
						Net_SpawnUnit(local_ctx.units->at(i));
				}
				break;
			}
		}
	}
	else if(zlo_stan == ZS_ZAMYKANIE_PORTALI && !location->outside && location->GetLastLevel() == dungeon_level)
	{
		Quest_Zlo* q = (Quest_Zlo*)FindQuest(zlo_refid);
		int d = q->GetLocId(current_location);
		if(d != -1)
		{
			Quest_Zlo::Loc& loc = q->loc[d];
			if(loc.state != 3)
			{
				Unit* u = jozan;

				if(u->ai->state == AIController::Idle)
				{
					// sprawd� czy podszed� do portalu
					float dist = distance2d(u->pos, loc.pos);
					if(dist < 5.f)
					{
						// podejd�
						u->ai->idle_action = AIController::Idle_Move;
						u->ai->idle_data.pos.Build(loc.pos);
						u->ai->timer = 1.f;
						u->hero->mode = HeroData::Wait;

						// zamknij
						if(dist < 2.f)
						{
							zlo_czas -= dt;
							if(zlo_czas <= 0.f)
							{
								loc.state = 3;
								u->hero->mode = HeroData::Follow;
								u->ai->idle_action = AIController::Idle_None;
								q->msgs.push_back(Format(txPortalClosed, location->name.c_str()));
								journal->NeedUpdate(Journal::Quests, GetQuestIndex(q));
								AddGameMsg3(GMS_JOURNAL_UPDATED);
								u->auto_talk = 1;
								q->changed = true;
								++q->closed;
								delete location->portal;
								location->portal = NULL;
								AddNews(Format(txPortalClosedNews, location->name.c_str()));
								if(IsOnline())
									PushNetChange(NetChange::CLOSE_PORTAL);
							}
						}
						else
							zlo_czas = 1.5f;
					}
					else
						u->hero->mode = HeroData::Follow;
				}
			}
		}
	}

	// sekret
	if(sekret_stan == SS2_WALKA)
	{
		int ile[2] = {0};

		for(vector<Unit*>::iterator it = at_arena.begin(), end = at_arena.end(); it != end; ++it)
		{
			if((*it)->IsStanding())
				ile[(*it)->in_arena]++;
		}

		if(at_arena[0]->hp < 10.f)
			ile[1] = 0;

		if(ile[0] == 0 || ile[1] == 0)
		{
			// o�yw wszystkich
			for(vector<Unit*>::iterator it = at_arena.begin(), end = at_arena.end(); it != end; ++it)
			{
				(*it)->in_arena = -1;
				if((*it)->hp <= 0.f)
				{
					(*it)->HealPoison();
					(*it)->live_state = Unit::ALIVE;
					(*it)->ani->Play("wstaje2", PLAY_ONCE|PLAY_PRIO3, 0);
					(*it)->action = A_ANIMATION;
					if((*it)->IsAI())
						(*it)->ai->Reset();
					if(IsOnline())
					{
						NetChange& c = Add1(net_changes);
						c.type = NetChange::STAND_UP;
						c.unit = *it;
					}
				}

				if(IsOnline())
				{
					NetChange& c = Add1(net_changes);
					c.type = NetChange::CHANGE_ARENA_STATE;
					c.unit = *it;
				}
			}

			at_arena[0]->hp = at_arena[0]->hpmax;
			if(IsOnline())
			{
				NetChange& c = Add1(net_changes);
				c.type = NetChange::UPDATE_HP;
				c.unit = at_arena[0];
			}

			if(ile[0])
			{
				// gracz wygra�
				sekret_stan = SS2_WYGRANO;
				at_arena[0]->auto_talk = 1;
			}
			else
			{
				// gracz przegra�
				sekret_stan = SS2_PRZEGRANO;
			}

			at_arena.clear();
		}
	}
}

void Game::UpdateContest(float dt)
{
	if(chlanie_stan == 3)
	{
		int id;
		InsideBuilding* inn = city_ctx->FindInn(id);
		Unit& innkeeper = *inn->FindUnit(FindUnitData("innkeeper"));

		if(innkeeper.busy == Unit::Busy_No)
		{
			float prev = chlanie_czas;
			chlanie_czas += dt;
			if(prev < 5.f && chlanie_czas >= 5.f)
				UnitTalk(innkeeper, txContestStart);
		}

		if(chlanie_czas >= 15.f && innkeeper.busy != Unit::Busy_Talking)
		{
			chlanie_stan = 4;
			// zbierz jednostki
			for(vector<Unit*>::iterator it = inn->ctx.units->begin(), end = inn->ctx.units->end(); it != end; ++it)
			{
				Unit& u = **it;
				if(u.IsStanding() && u.IsAI() && !u.event_handler && u.frozen == 0 && u.busy == Unit::Busy_No)
				{
					bool ok = false;
					if(IS_SET(u.data->flagi2, F2_ZAWODY_W_PICIU))
						ok = true;
					else if(IS_SET(u.data->flagi2, F2_ZAWODY_W_PICIU_50))
					{
						if(rand2()%2 == 0)
							ok = true;
					}
					else if(IS_SET(u.data->flagi3, F3_ZAWODY_W_PICIU_25))
					{
						if(rand2()%4 == 0)
							ok = true;
					}
					else if(IS_SET(u.data->flagi3, F3_MAG_PIJAK))
					{
						if(magowie_stan < MS_MAG_WYLECZONY)
							ok = true;
					}

					if(ok)
						chlanie_ludzie.push_back(*it);
				}
			}
			chlanie_stan2 = 0;

			// patrzenie si� postaci i usuni�cie zaj�tych
			bool removed = false;
			for(vector<Unit*>::iterator it = chlanie_ludzie.begin(), end = chlanie_ludzie.end(); it != end; ++it)
			{
				Unit& u = **it;
				if(u.in_building != id || u.frozen != 0 || !u.IsStanding())
				{
					*it = NULL;
					removed = true;
				}
				else
				{
					u.busy = Unit::Busy_Yes;
					u.look_target = &innkeeper;
					u.event_handler = this;
					if(u.IsPlayer())
					{
						BreakPlayerAction(u.player);
						if(u.player != pc)
						{
							NetChangePlayer& c = Add1(net_changes_player);
							c.type = NetChangePlayer::LOOK_AT;
							c.pc = u.player;
							c.id = innkeeper.netid;
							GetPlayerInfo(c.pc).NeedUpdate();
						}
					}
				}
			}
			if(removed)
				RemoveNullElements(chlanie_ludzie);

			// je�li jest za ma�o ludzi
			if(chlanie_ludzie.size() <= 1u)
			{
				chlanie_stan = 5;
				chlanie_stan2 = 3;
				innkeeper.ai->idle_action = AIController::Idle_Rot;
				innkeeper.ai->idle_data.rot = innkeeper.ai->start_rot;
				innkeeper.ai->timer = 3.f;
				innkeeper.busy = Unit::Busy_Yes;
				UnitTalk(innkeeper, txContestNoPeople);
			}
			else
			{
				innkeeper.ai->idle_action = AIController::Idle_Rot;
				innkeeper.ai->idle_data.rot = innkeeper.ai->start_rot;
				innkeeper.ai->timer = 3.f;
				innkeeper.busy = Unit::Busy_Yes;
				UnitTalk(innkeeper, txContestTalk[0]);
			}
		}
	}
	else if(chlanie_stan == 4)
	{
		InsideBuilding* inn = city_ctx->FindInn();
		Unit& innkeeper = *inn->FindUnit(FindUnitData("innkeeper"));
		bool talking = true;
		cstring next_text = NULL, next_drink = NULL;

		switch(chlanie_stan2)
		{
		case 0:
			next_text = txContestTalk[1];
			break;
		case 1:
			next_text = txContestTalk[2];
			break;
		case 2:
			next_text = txContestTalk[3];
			break;
		case 3:
			next_drink = "potion_beer";
			break;
		case 4:
			talking = false;
			next_text = txContestTalk[4];
			break;
		case 5:
			next_drink = "potion_beer";
			break;
		case 6:
			talking = false;
			next_text = txContestTalk[5];
			break;
		case 7:
			next_drink = "potion_beer";
			break;
		case 8:
			talking = false;
			next_text = txContestTalk[6];
			break;
		case 9:
			next_drink = "potion_vodka";
			break;
		case 10:
			talking = false;
			next_text = txContestTalk[7];
			break;
		case 11:
			next_drink = "potion_vodka";
			break;
		case 12:
			talking = false;
			next_text = txContestTalk[8];
			break;
		case 13:
			next_drink = "potion_vodka";
			break;
		case 14:
			talking = false;
			next_text = txContestTalk[9];
			break;
		case 15:
			next_text = txContestTalk[10];
			break;
		case 16:
			next_drink = "potion_spirit";
			break;
		case 17:
			talking = false;
			next_text = txContestTalk[11];
			break;
		case 18:
			next_drink = "potion_spirit";
			break;
		case 19:
			talking = false;
			next_text = txContestTalk[12];
			break;
		default:
			if((chlanie_stan2-20)%2 == 0)
			{
				if(chlanie_stan2 != 20)
					talking = false;
				next_text = txContestTalk[13];
			}
			else
				next_drink = "potion_spirit";
			break;
		}

		if(talking)
		{
			if(!innkeeper.bubble)
			{
				if(next_text)
					UnitTalk(innkeeper, next_text);
				else
				{
					assert(next_drink);
					chlanie_czas = 0.f;
					const Consumeable& drink = FindItem(next_drink)->ToConsumeable();
					for(vector<Unit*>::iterator it = chlanie_ludzie.begin(), end = chlanie_ludzie.end(); it != end; ++it)
						(*it)->ConsumeItem(drink, true);
				}

				++chlanie_stan2;
			}
		}
		else
		{
			chlanie_czas += dt;
			if(chlanie_czas >= 5.f)
			{
				if(chlanie_ludzie.size() >= 2)
				{
					assert(next_text);
					UnitTalk(innkeeper, next_text);
					++chlanie_stan2;
				}
				else if(chlanie_ludzie.size() == 1)
				{
					chlanie_stan = 5;
					chlanie_stan2 = 0;
					innkeeper.look_target = chlanie_ludzie.back();
					AddNews(Format(txContestWinNews, chlanie_ludzie.back()->GetName()));
					UnitTalk(innkeeper, txContestWin);
				}
				else
				{
					chlanie_stan = 5;
					chlanie_stan2 = 1;
					AddNews(txContestNoWinner);
					UnitTalk(innkeeper, txContestNoWinner);
				}
			}
		}
	}
	else if(chlanie_stan == 5)
	{
		InsideBuilding* inn = city_ctx->FindInn();
		Unit& innkeeper = *inn->FindUnit(FindUnitData("innkeeper"));

		if(!innkeeper.bubble)
		{
			switch(chlanie_stan2)
			{
			case 0: // wygrana
				chlanie_stan2 = 2;
				UnitTalk(innkeeper, txContestPrize);
				break;
			case 1: // remis
				innkeeper.busy = Unit::Busy_No;
				innkeeper.look_target = NULL;
				chlanie_stan = 1;
				chlanie_wygenerowano = false;
				chlanie_zwyciesca = NULL;
				break;
			case 2: // wygrana2
				innkeeper.busy = Unit::Busy_No;
				innkeeper.look_target = NULL;
				chlanie_zwyciesca = chlanie_ludzie.back();
				chlanie_ludzie.clear();
				chlanie_stan = 1;
				chlanie_wygenerowano = false;
				chlanie_zwyciesca->look_target = NULL;
				chlanie_zwyciesca->busy = Unit::Busy_No;
				chlanie_zwyciesca->event_handler = NULL;
				break;
			case 3: // brak ludzi
				for(vector<Unit*>::iterator it = chlanie_ludzie.begin(), end = chlanie_ludzie.end(); it != end; ++it)
				{
					Unit& u = **it;
					u.busy = Unit::Busy_No;
					u.look_target = NULL;
					u.event_handler = NULL;
					if(u.IsPlayer())
					{
						BreakPlayerAction(u.player);
						if(u.player != pc)
						{
							NetChangePlayer& c = Add1(net_changes_player);
							c.type = NetChangePlayer::LOOK_AT;
							c.pc = u.player;
							c.id = -1;
							GetPlayerInfo(c.pc).NeedUpdate();
						}
					}
				}
				chlanie_stan = 1;
				innkeeper.busy = Unit::Busy_No;
				break;
			}
		}
	}
}

void Game::SetUnitWeaponState(Unit& u, bool wyjmuje, BRON co)
{
	if(wyjmuje)
	{
		switch(u.stan_broni)
		{
		case BRON_SCHOWANA:
			// wyjmij bron
			u.ani->Play(u.GetTakeWeaponAnimation(co == B_JEDNORECZNA), PLAY_ONCE|PLAY_PRIO1, 1);
			u.action = A_TAKE_WEAPON;
			u.wyjeta = co;
			u.stan_broni = BRON_WYJMUJE;
			u.etap_animacji = 0;
			break;
		case BRON_CHOWA:
			if(u.chowana == co)
			{
				if(u.etap_animacji == 0)
				{
					// jeszcze nie schowa� tej broni, wy��cz grup�
					u.action = A_NONE;
					u.wyjeta = u.chowana;
					u.chowana = B_BRAK;
					u.stan_broni = BRON_WYJETA;
					u.ani->Deactivate(1);
				}
				else
				{
					// schowa� bro�, zacznij wyci�ga�
					u.wyjeta = u.chowana;
					u.chowana = B_BRAK;
					u.stan_broni = BRON_WYJMUJE;
					CLEAR_BIT(u.ani->groups[1].state, AnimeshInstance::FLAG_BACK);
				}
			}
			else
			{
				// chowa bro�, zacznij wyci�ga�
				u.ani->Play(u.GetTakeWeaponAnimation(co == B_JEDNORECZNA), PLAY_ONCE|PLAY_PRIO1, 1);
				u.action = A_TAKE_WEAPON;
				u.wyjeta = co;
				u.chowana = B_BRAK;
				u.stan_broni = BRON_WYJMUJE;
				u.etap_animacji = 0;
			}
			break;
		case BRON_WYJMUJE:
		case BRON_WYJETA:
			if(u.wyjeta != co)
			{
				// wyjmuje z�� bro�, zacznij wyjmowa� dobr�
				// lub
				// powinien mie� wyj�t� bro�, ale nie t�!
				u.ani->Play(u.GetTakeWeaponAnimation(co == B_JEDNORECZNA), PLAY_ONCE|PLAY_PRIO1, 1);
				u.action = A_TAKE_WEAPON;
				u.wyjeta = co;
				u.chowana = B_BRAK;
				u.stan_broni = BRON_WYJMUJE;
				u.etap_animacji = 0;
			}
			break;
		}
	}
	else // chowa
	{
		switch(u.stan_broni)
		{
		case BRON_SCHOWANA:
			// schowana to schowana, nie ma co sprawdza� czy to ta
			break;
		case BRON_CHOWA:
			if(u.chowana != co)
			{
				// chowa z�� bro�, zamie�
				u.chowana = co;
			}
			break;
		case BRON_WYJMUJE:
			if(u.etap_animacji == 0)
			{
				// jeszcze nie wyj�� broni z pasa, po prostu wy��cz t� grupe
				u.action = A_NONE;
				u.wyjeta = B_BRAK;
				u.stan_broni = BRON_SCHOWANA;
				u.ani->Deactivate(1);
			}
			else
			{
				// wyj�� bro� z pasa, zacznij chowa�
				u.chowana = u.wyjeta;
				u.wyjeta = B_BRAK;
				u.stan_broni = BRON_CHOWA;
				u.etap_animacji = 0;
				SET_BIT(u.ani->groups[1].state, AnimeshInstance::FLAG_BACK);
			}
			break;
		case BRON_WYJETA:
			// zacznij chowa�
			u.ani->Play(u.GetTakeWeaponAnimation(co == B_JEDNORECZNA), PLAY_ONCE|PLAY_BACK|PLAY_PRIO1, 1);
			u.chowana = co;
			u.wyjeta = B_BRAK;
			u.stan_broni = BRON_CHOWA;
			u.action = A_TAKE_WEAPON;
			u.etap_animacji = 0;
			break;
		}
	}
}

void Game::AddGameMsg3(GMS id)
{
	cstring text;
	float time = 3.f;
	bool repeat = false;

	switch(id)
	{
	case GMS_IS_LOOTED:
		text = txGmsLooted;
		break;
	case GMS_ADDED_RUMOR:
		repeat = true;
		text = txGmsRumor;
		break;
	case GMS_JOURNAL_UPDATED:
		repeat = true;
		text = txGmsJournalUpdated;
		break;
	case GMS_USED:
		text = txGmsUsed;
		time = 2.f;
		break;
	case GMS_UNIT_BUSY:
		text = txGmsUnitBusy;
		break;
	case GMS_GATHER_TEAM:
		text = txGmsGatherTeam;
		break;
	case GMS_NOT_LEADER:
		text = txGmsNotLeader;
		break;
	case GMS_NOT_IN_COMBAT:
		text = txGmsNotInCombat;
		break;
	case GMS_ADDED_ITEM:
		text = txGmsAddedItem;
		repeat = true;
		break;
	default:
		assert(0);
		return;
	}

	if(repeat)
		AddGameMsg(text, time);
	else
		AddGameMsg2(text, time, id);
}

void Game::UpdatePlayerView()
{
	LevelContext& ctx = GetContext(*pc->unit);
	Unit& u = *pc->unit;

	for(vector<Unit*>::iterator it = ctx.units->begin(), end = ctx.units->end(); it != end; ++it)
	{
		Unit& u2 = **it;
		if(&u == &u2 || u2.to_remove)
			continue;

		bool mark = false;
		if(IsEnemy(u, u2))
		{
			if(u2.IsAlive())
				mark = true;
		}
		else if(IsFriend(u, u2))
			mark = true;

		// oznaczanie pobliskich postaci
		if(mark)
		{
			float dist = distance(u.visual_pos, u2.visual_pos);

			if(dist < alert_range.x && camera_frustum.SphereToFrustum(u2.visual_pos, u2.GetSphereRadius()) && CanSee(u, u2))
			{
				// dodaj do pobliskich jednostek
				bool jest = false;
				for(vector<UnitView>::iterator it2 = unit_views.begin(), end2 = unit_views.end(); it2 != end2; ++it2)
				{
					if(it2->unit == *it)
					{
						jest = true;
						it2->valid = true;
						it2->last_pos = u2.GetUnitTextPos();
						break;
					}
				}
				if(!jest)
				{
					UnitView& uv = Add1(unit_views);
					uv.valid = true;
					uv.unit = *it;
					uv.time = 0.f;
					uv.last_pos = u2.GetUnitTextPos();
				}
			}
		}
	}
}

void Game::OnCloseInventory()
{
	if(inventory_mode == I_TRADE)
	{
		if(IsLocal())
		{
			pc->action_unit->busy = Unit::Busy_No;
			pc->action_unit->look_target = NULL;
		}
		else
			PushNetChange(NetChange::STOP_TRADE);
	}
	else if(inventory_mode == I_SHARE || inventory_mode == I_GIVE)
	{
		if(IsLocal())
		{
			pc->action_unit->busy = Unit::Busy_No;
			pc->action_unit->look_target = NULL;
		}
		else
			PushNetChange(NetChange::STOP_TRADE);
	}
	else if(inventory_mode == I_LOOT_CHEST && IsLocal())
	{
		pc->action_chest->looted = false;
		pc->action_chest->ani->Play(&pc->action_chest->ani->ani->anims[0], PLAY_PRIO1|PLAY_ONCE|PLAY_STOP_AT_END|PLAY_BACK, 0);
		if(sound_volume)
		{
			VEC3 pos = pc->action_chest->pos;
			pos.y += 0.5f;
			PlaySound3d(sChestClose, pos, 2.f, 5.f);
		}
	}

	if(IsOnline() && (inventory_mode == I_LOOT_BODY || inventory_mode == I_LOOT_CHEST))
	{
		if(IsClient())
			PushNetChange(NetChange::STOP_TRADE);
		else if(inventory_mode == I_LOOT_BODY)
			pc->action_unit->busy = Unit::Busy_No;
		else
		{
			NetChange& c = Add1(net_changes);
			c.type = NetChange::CHEST_CLOSE;
			c.id = pc->action_chest->netid;
		}
	}

	pc->action = PlayerController::Action_None;

	if(IsLocal() && dialog_context.next_talker)
	{
		Unit* t = dialog_context.next_talker;
		dialog_context.next_talker = NULL;
		StartDialog(dialog_context, t, dialog_context.next_dialog);
	}

	inventory_mode = I_NONE;
}

// zamyka ekwipunek i wszystkie okna kt�re on m�g�by utworzy�
void Game::CloseInventory(bool do_close)
{
	if(do_close)
		OnCloseInventory();
	inventory_mode = I_NONE;
}

void Game::CloseAllPanels()
{
	if(inventory)
	{
		if(stats->visible)
			stats->Hide();
		if(inventory->visible)
			inventory->Hide();
		if(team_panel->visible)
			team_panel->Hide();
		else if(gp_trade->visible)
			gp_trade->Hide();
		else if(journal->visible)
			journal->visible = false;
		else if(minimap->visible)
			minimap->visible = false;
	}
}

bool Game::CanShowEndScreen()
{
	if(IsLocal())
		return !unique_completed_show && unique_quests_completed == UNIQUE_QUESTS && city_ctx && !dialog_context.dialog_mode && pc->unit->IsStanding();
	else
		return unique_completed_show && city_ctx && !dialog_context.dialog_mode && pc->unit->IsStanding();
}

void Game::UpdateGameDialogClient()
{
	if(dialog_context.show_choices)
	{
		if(game_gui->UpdateChoice(dialog_context, dialog_choices.size()))
		{
			dialog_context.show_choices = false;
			dialog_context.dialog_text = "";

			NetChange& c = Add1(net_changes);
			c.type = NetChange::CHOICE;
			c.id = dialog_context.choice_selected;
		}
	}
	else if(dialog_context.dialog_wait > 0.f && dialog_context.skip_id != -1)
	{
		if(KeyPressedReleaseAllowed(GK_SKIP_DIALOG) || KeyPressedReleaseAllowed(GK_SELECT_DIALOG) || KeyPressedReleaseAllowed(GK_ATTACK_USE) ||
			(AllowKeyboard() && Key.PressedRelease(VK_ESCAPE)))
		{
			NetChange& c = Add1(net_changes);
			c.type = NetChange::SKIP_DIALOG;
			c.id = dialog_context.skip_id;
			dialog_context.skip_id = -1;
		}
	}
}

LevelContext& Game::GetContextFromInBuilding(int in_building)
{
	if(in_building == -1)
		return local_ctx;
	assert(city_ctx);
	return city_ctx->inside_buildings[in_building]->ctx;
}

bool Game::FindQuestItem2(Unit* unit, cstring id, Quest** out_quest, int* i_index)
{
	assert(unit && id);

	if(id[1] == '$')
	{
		// szukaj w za�o�onych przedmiotach
		for(int i=0; i<SLOT_MAX; ++i)
		{
			if(unit->slots[i] && IS_SET(unit->slots[i]->flags, ITEM_QUEST))
			{
				Quest* quest = FindQuest(unit->slots[i]->refid);
				if(quest && quest->IsActive() && quest->IfHaveQuestItem2(id))
				{
					if(i_index)
						*i_index = SlotToIIndex(ITEM_SLOT(i));
					if(out_quest)
						*out_quest = quest;
					return true;
				}
			}
		}

		// szukaj w nie za�o�onych
		int index = 0;
		for(vector<ItemSlot>::iterator it2 = unit->items.begin(), end2 = unit->items.end(); it2 != end2; ++it2, ++index)
		{
			if(it2->item && IS_SET(it2->item->flags, ITEM_QUEST))
			{
				Quest* quest = FindQuest(it2->item->refid);
				if(quest && quest->IsActive() && quest->IfHaveQuestItem2(id))
				{
					if(i_index)
						*i_index = index;
					if(out_quest)
						*out_quest = quest;
					return true;
				}
			}
		}
	}
	else
	{
		// szukaj w za�o�onych przedmiotach
		for(int i=0; i<SLOT_MAX; ++i)
		{
			if(unit->slots[i] && IS_SET(unit->slots[i]->flags, ITEM_QUEST) && strcmp(unit->slots[i]->id, id) == 0)
			{
				Quest* quest = FindQuest(unit->slots[i]->refid);
				if(quest && quest->IsActive() && quest->IfHaveQuestItem())
				{
					if(i_index)
						*i_index = SlotToIIndex(ITEM_SLOT(i));
					if(out_quest)
						*out_quest = quest;
					return true;
				}
			}
		}

		// szukaj w nie za�o�onych
		int index = 0;
		for(vector<ItemSlot>::iterator it2 = unit->items.begin(), end2 = unit->items.end(); it2 != end2; ++it2, ++index)
		{
			if(it2->item && IS_SET(it2->item->flags, ITEM_QUEST) && strcmp(it2->item->id, id) == 0)
			{
				Quest* quest = FindQuest(it2->item->refid);
				if(quest && quest->IsActive() && quest->IfHaveQuestItem())
				{
					if(i_index)
						*i_index = index;
					if(out_quest)
						*out_quest = quest;
					return true;
				}
			}
		}
	}

	return false;
}

bool Game::Cheat_KillAll(int typ, Unit& unit, Unit* ignore)
{
	if(!in_range(typ, 0, 3))
		return false;

	if(!IsLocal())
	{
		NetChange& c = Add1(net_changes);
		c.type = NetChange::CHEAT_KILL_ALL;
		c.id = typ;
		c.unit = ignore;
		return true;
	}

	switch(typ)
	{
	case 0:
		{
			LevelContext& ctx = GetContext(unit);
			for(vector<Unit*>::iterator it = ctx.units->begin(), end = ctx.units->end(); it != end; ++it)
			{
				if((*it)->IsAlive() && IsEnemy(**it, unit))
					GiveDmg(ctx, NULL, (*it)->hp, **it, NULL);
			}
		}
		break;
	case 1:
		{
			LevelContext& ctx = GetContext(unit);
			for(vector<Unit*>::iterator it = ctx.units->begin(), end = ctx.units->end(); it != end; ++it)
			{
				if((*it)->IsAlive() && !(*it)->IsPlayer())
					GiveDmg(ctx, NULL, (*it)->hp, **it, NULL);
			}
		}
		break;
	case 2:
		for(vector<Unit*>::iterator it = local_ctx.units->begin(), end = local_ctx.units->end(); it != end; ++it)
		{
			if((*it)->IsAlive() && !(*it)->IsPlayer())
				GiveDmg(local_ctx, NULL, (*it)->hp, **it, NULL);
		}
		if(city_ctx)
		{
			for(vector<InsideBuilding*>::iterator it2 = city_ctx->inside_buildings.begin(), end2 = city_ctx->inside_buildings.end(); it2 != end2; ++it2)
			{
				for(vector<Unit*>::iterator it = (*it2)->units.begin(), end = (*it2)->units.end(); it != end; ++it)
				{
					if((*it)->IsAlive() && !(*it)->IsPlayer())
						GiveDmg((*it2)->ctx, NULL, (*it)->hp, **it, NULL);
				}
			}
		}
		break;
	case 3:
		for(vector<Unit*>::iterator it = local_ctx.units->begin(), end = local_ctx.units->end(); it != end; ++it)
		{
			if((*it)->IsAlive() && !(*it)->IsPlayer() && *it != ignore)
				GiveDmg(local_ctx, NULL, (*it)->hp, **it, NULL);
		}
		break;
	}
	
	return true;
}

void Game::Event_Pvp(int id)
{
	dialog_pvp = NULL;
	if(!pvp_response.ok)
		return;

	if(IsServer())
	{
		if(id == BUTTON_YES)
		{
			// zaakceptuj pvp
			StartPvp(pvp_response.from->player, pvp_response.to);
		}
		else
		{
			// nie akceptuj pvp
			NetChangePlayer& c = Add1(net_changes_player);
			c.type = NetChangePlayer::NO_PVP;
			c.pc = pvp_response.from->player;
			c.id = pvp_response.to->player->id;
			GetPlayerInfo(c.pc).NeedUpdate();
		}
	}
	else
	{
		NetChange& c = Add1(net_changes);
		c.type = NetChange::PVP;
		c.unit = pvp_unit;
		if(id == BUTTON_YES)
			c.id = 1;
		else
			c.id = 0;
	}

	pvp_response.ok = false;
}

void Game::Cheat_Reveal()
{
	int index = 0;
	for(vector<Location*>::iterator it = locations.begin(), end = locations.end(); it != end; ++it, ++index)
	{
		if(*it && (*it)->state == LS_UNKNOWN)
		{
			(*it)->state = LS_KNOWN;
			if(IsOnline())
				Net_ChangeLocationState(index, false);
		}
	}
}

void Game::Cheat_ShowMinimap()
{
	if(!location->outside)
	{
		InsideLocationLevel& lvl = ((InsideLocation*)location)->GetLevelData();

		for(int y=0; y<lvl.h; ++y)
		{
			for(int x=0; x<lvl.w; ++x)
				minimap_reveal.push_back(INT2(x, y));
		}

		UpdateDungeonMinimap(false);

		if(IsLocal() && IsOnline())
			PushNetChange(NetChange::CHEAT_SHOW_MINIMAP);
	}
}

void Game::StartPvp(PlayerController* player, Unit* unit)
{
	arena_free = false;
	arena_tryb = Arena_Pvp;
	arena_etap = Arena_OdliczanieDoPrzeniesienia;
	arena_t = 0.f;
	at_arena.clear();

	// fallback gracza
	if(player == pc)
	{
		fallback_co = FALLBACK_ARENA;
		fallback_t = -1.f;
	}
	else
	{
		NetChangePlayer& c = Add1(net_changes_player);
		c.type = NetChangePlayer::ENTER_ARENA;
		c.pc = player;
		GetPlayerInfo(player).NeedUpdate();
	}

	// fallback postaci
	if(unit->IsPlayer())
	{
		if(unit->player == pc)
		{
			fallback_co = FALLBACK_ARENA;
			fallback_t = -1.f;
		}
		else
		{
			NetChangePlayer& c = Add1(net_changes_player);
			c.type = NetChangePlayer::ENTER_ARENA;
			c.pc = unit->player;
			GetPlayerInfo(c.pc).NeedUpdate();
		}
	}

	// dodaj do areny
	player->unit->frozen = 2;
	player->arena_fights++;
	if(IsOnline())
		player->stat_flags |= STAT_ARENA_FIGHTS;
	at_arena.push_back(player->unit);
	unit->frozen = 2;
	at_arena.push_back(unit);
	if(unit->IsHero())
		unit->hero->following = player->unit;
	else if(unit->IsPlayer())
	{
		unit->player->arena_fights++;
		if(IsOnline())
			unit->player->stat_flags |= STAT_ARENA_FIGHTS;
	}
	pvp_player = player;
	arena_fighter = unit;

	// stw�rz obserwator�w na arenie na podstawie poziomu postaci
	player->unit->level = player->unit->CalculateLevel();
	if(unit->IsPlayer())
		unit->level = unit->CalculateLevel();
	int level = max(player->unit->level, unit->level);

	if(level < 7)
		SpawnArenaViewers(1);
	else if(level < 14)
		SpawnArenaViewers(2);
	else
		SpawnArenaViewers(3);
}

void Game::UpdateGameNet(float dt)
{
	if(IsServer())
		UpdateServer(dt);
	else
		UpdateClient(dt);
}

void Game::CheckCredit(bool require_update, bool ignore)
{
	if(active_team.size() > 1)
	{
		int ile = GetCredit(*active_team.front());

		for(vector<Unit*>::iterator it = active_team.begin()+1, end = active_team.end(); it != end; ++it)
		{
			int kredyt = GetCredit(**it);
			if(kredyt < ile)
				ile = kredyt;
		}

		if(ile > 0)
		{
			require_update = true;
			for(vector<Unit*>::iterator it = active_team.begin(), end = active_team.end(); it != end; ++it)
				GetCredit(**it) -= ile;
		}
	}
	else
	{
		pc->unit->gold += team_gold;
		team_gold = 0;
		pc->kredyt = 0;
	}

	if(!ignore && require_update && IsOnline())
		PushNetChange(NetChange::UPDATE_CREDIT);
}

void Game::StartDialog2(PlayerController* player, Unit* talker, DialogEntry* dialog)
{
	assert(player && talker);

	DialogContext& ctx = *player->dialog_ctx;

	if(ctx.dialog_mode)
	{
		ctx.next_talker = talker;
		ctx.next_dialog = dialog;
	}
	else
	{
		if(player != pc)
			Net_StartDialog(player, talker);

		StartDialog(ctx, talker, dialog);
	}
}

void Game::UpdateUnitPhysics(Unit* unit, const VEC3& pos)
{
	assert(unit);

	btVector3 a_min, a_max, bpos(ToVector3(pos));
	bpos.setY(pos.y + max(MIN_H, unit->GetUnitHeight())/2);
	unit->cobj->getWorldTransform().setOrigin(bpos);
	unit->cobj->getCollisionShape()->getAabb(unit->cobj->getWorldTransform(), a_min, a_max);
	phy_broadphase->setAabb(unit->cobj->getBroadphaseHandle(), a_min, a_max, phy_dispatcher);
}

void Game::WarpNearLocation(LevelContext& ctx, Unit& unit, const VEC3& pos, float extra_radius, bool allow_exact, int tries)
{
	const float radius = unit.GetUnitRadius();

	global_col.clear();
	IgnoreObjects ignore = {0};
	const Unit* ignore_units[2] = {&unit, NULL};
	ignore.ignored_units = ignore_units;
	GatherCollisionObjects(ctx, global_col, pos, extra_radius+radius, &ignore);

	VEC3 tmp_pos = pos;
	if(!allow_exact)
	{
		int j = rand2()%poisson_disc_count;
		tmp_pos.x += POISSON_DISC_2D[j].x*extra_radius;
		tmp_pos.z += POISSON_DISC_2D[j].y*extra_radius;
	}

	for(int i=0; i<tries; ++i)
	{
		if(!Collide(global_col, tmp_pos, radius))
			break;

		int j = rand2()%poisson_disc_count;
		tmp_pos = pos;
		tmp_pos.x += POISSON_DISC_2D[j].x*extra_radius;
		tmp_pos.z += POISSON_DISC_2D[j].y*extra_radius;
	}

	unit.pos = tmp_pos;
	MoveUnit(unit, true);
	unit.visual_pos = unit.pos;

	if(unit.cobj)
		UpdateUnitPhysics(&unit, unit.pos);
}

void Game::ProcessRemoveUnits()
{
	for(vector<Unit*>::iterator it = to_remove.begin(), end = to_remove.end(); it != end; ++it)
	{
		if((*it)->interp)
			interpolators.Free((*it)->interp);
		DeleteUnit(*it);
	}
	to_remove.clear();
}

void Game::Train(Unit& unit, bool is_skill, int co, bool add_one)
{
	int* value, *train_points, *train_next;
	if(is_skill)
	{
		value = &unit.skill[co];
		train_points = &unit.player->sp[co];
		train_next = &unit.player->sn[co];
	}
	else
	{
		value = &unit.attrib[co];
		train_points = &unit.player->ap[co];
		train_next = &unit.player->an[co];
	}

	if(*value == 100)
		return;

	int ile = (add_one ? 1 : 10-(*value)/10);
	*value += ile;
	*train_points /= 2;

	if(is_skill)
		*train_next = unit.player->GetRequiredSkillPoints(*value);
	else
	{
		*train_next = unit.player->GetRequiredAttributePoints(*value);
		if(co == (int)Attribute::STR || co == (int)Attribute::CON)
		{
			unit.RecalculateHp();
			if(co == (int)Attribute::STR)
				unit.GetLoad();

			if(IsOnline())
			{
				NetChange& c = Add1(net_changes);
				c.type = NetChange::UPDATE_HP;
				c.unit = &unit;
			}
		}
	}

	if(SHOW_HERO_GAIN)
	{
		int SkillToGain(Skill);
		int AttributeToGain(Attribute);
		int gain = is_skill ? SkillToGain((Skill)co) : AttributeToGain((Attribute)co);
		if(unit.player == pc)
			ShowStatGain(gain, ile);
		else
		{
			PlayerInfo& info = GetPlayerInfo(unit.player);
			NetChangePlayer& c = Add1(net_changes_player);
			c.type = NetChangePlayer::GAIN_STAT;
			c.id = gain;
			c.ile = ile;
			c.pc = unit.player;
			info.NeedUpdate();
			info.update_flags |= (is_skill ? PlayerInfo::UF_SKILLS : PlayerInfo::UF_ATTRIB);
		}
	}
	else if(IsOnline() && unit.player != pc)
		GetPlayerInfo(unit.player).update_flags |= (is_skill ? PlayerInfo::UF_SKILLS : PlayerInfo::UF_ATTRIB);
}

void Game::ShowStatGain(int co, int ile)
{
	cstring s;
	switch(co)
	{
	case GAIN_STAT_STR:
		s = txGainStr;
		break;
	case GAIN_STAT_END:
		s = txGainEnd;
		break;
	case GAIN_STAT_DEX:
		s = txGainDex;
		break;
	case GAIN_STAT_WEP:
		s = txGainOneHanded;
		break;
	case GAIN_STAT_SHI:
		s = txGainShield;
		break;
	case GAIN_STAT_BOW:
		s = txGainBow;
		break;
	case GAIN_STAT_LAR:
		s = txGainLightArmor;
		break;
	case GAIN_STAT_HAR:
		s = txGainHeavyArmor;
		break;
	default:
		assert(0);
		s = "[ERROR]";
		break;
	}

	AddGameMsg(Format(txGainText, s, ile), 3.f);
}

void Game::BreakPlayerAction(PlayerController* player)
{
	assert(player);

	switch(player->action)
	{
	case PlayerController::Action_LootUnit:
		player->action_unit->busy = Unit::Busy_No;
		CloseInventory(false);
		break;
	case PlayerController::Action_LootChest:
		player->action_chest->looted = false;
		// brak animacji zamykania! p�ki co nie ma problemu - nie ma skrzyni niedaleko areny, w karczmie
		CloseInventory(false);
		break;
	case PlayerController::Action_Talk:
		player->action_unit->busy = Unit::Busy_No;
		player->action_unit->look_target = NULL;
		if(IsLocal())
		{
			player->dialog_ctx->dialog_mode = false;
			player->dialog_ctx->next_talker = NULL;
		}
		else
			dialog_context.dialog_mode = false;
		break;
	case PlayerController::Action_Trade:
		player->action_unit->busy = Unit::Busy_No;
		player->action_unit->look_target = NULL;
		CloseInventory(false);
		break;
	case PlayerController::Action_ShareItems:
	case PlayerController::Action_GiveItems:
		player->action_unit->busy = Unit::Busy_No;
		player->action_unit->look_target = NULL;
		CloseInventory(false);
		break;
	}

	BreakAction(*player->unit, false);

	if(player != pc)
	{
		NetChangePlayer& c = Add1(net_changes_player);
		c.type = NetChangePlayer::BREAK_ACTION;
		c.pc = player;
		GetPlayerInfo(player).NeedUpdate();
	}

	player->action = PlayerController::Action_None;
}

void Game::ActivateChangeLeaderButton(bool activate)
{
	team_panel->bt[2].state = (activate ? Button::NONE : Button::DISABLED);
}

void Game::RespawnTraps()
{
	for(vector<Trap*>::iterator it = local_ctx.traps->begin(), end = local_ctx.traps->end(); it != end; ++it)
	{
		Trap& trap = **it;

		trap.state = 0;
		if(trap.base->type == TRAP_SPEAR)
		{
			if(trap.hitted)
				trap.hitted->clear();
			else
				trap.hitted = new vector<Unit*>;
		}
	}
}

// zmienia tylko pozycj� bo ta funkcja jest wywo�ywana przy opuszczaniu miasta
void Game::WarpToInn(Unit& unit)
{
	assert(city_ctx);

	int id;
	InsideBuilding* inn = city_ctx->FindInn(id);

	WarpToArea(inn->ctx, (rand2()%5 == 0 ? inn->arena2 : inn->arena1), unit.GetUnitRadius(), unit.pos, 20);
	unit.visual_pos = unit.pos;
	unit.in_building = id;
}

void Game::PayCredit(PlayerController* player, int ile)
{
	LocalVector<Unit*> _units;
	vector<Unit*>& units = _units;

	for(vector<Unit*>::iterator it = active_team.begin(), end = active_team.end(); it != end; ++it)
	{
		if(*it != player->unit)
			units.push_back(*it);
	}

	AddGold(ile, &units, false);

	player->kredyt -= ile;
	if(player->kredyt < 0)
	{
		WARN(Format("Player '%s' paid %d credit and now have %d!", player->name.c_str(), ile, player->kredyt));
		player->kredyt = 0;
#ifdef IS_DEV
		AddGameMsg("Player has invalid credit!", 5.f);
#endif
	}

	if(IsOnline())
		PushNetChange(NetChange::UPDATE_CREDIT);
}

InsideBuilding* Game::GetArena()
{
	assert(city_ctx);
	for(vector<InsideBuilding*>::iterator it = city_ctx->inside_buildings.begin(), end = city_ctx->inside_buildings.end(); it != end; ++it)
	{
		if((*it)->type == B_ARENA)
			return *it;
	}
	assert(0);
	return NULL;
}

void Game::CreateSaveImage(cstring filename)
{
	assert(filename);

	SURFACE surf;
	V( tSave->GetSurfaceLevel(0, &surf) );

	// ustaw render target
	if(sSave)
		sCustom = sSave;
	else
		sCustom = surf;

	int old_flags = draw_flags;
	if(game_state == GS_LEVEL)
		draw_flags = (0xFFFFFFFF & ~DF_GUI & ~DF_MENU);
	else
		draw_flags = (0xFFFFFFFF & ~DF_MENU);
	OnDraw(false);
	draw_flags = old_flags;
	sCustom = NULL;

	// kopiuj tekstur�
	if(sSave)
	{
		V( tSave->GetSurfaceLevel(0, &surf) );
		V( device->StretchRect(sSave, NULL, surf, NULL, D3DTEXF_NONE) );
		surf->Release();
	}

	// zapisz obrazek
	V( D3DXSaveSurfaceToFile(filename, D3DXIFF_JPG, surf, NULL, NULL) );
	surf->Release();

	// przywr�c render target
	V( device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &surf) );
	V( device->SetRenderTarget(0, surf) );
	surf->Release();
}

void Game::PlayerUseUseable(Useable* useable, bool after_action)
{
	Unit& u = *pc->unit;
	Useable& use = *useable;
	BaseUsable& bu = g_base_usables[use.type];

	bool ok = true;
	if(bu.item)
	{
		if(!u.HaveItem(bu.item) && u.slots[SLOT_WEAPON] != bu.item)
		{
			if(use.type == U_KOCIOLEK)
				AddGameMsg2(txNeedLadle, 2.f, GMS_NEED_LADLE);
			else if(use.type == U_KOWADLO)
				AddGameMsg2(txNeedHammer, 2.f, GMS_NEED_HAMMER);
			else if(use.type == U_ZYLA_ZELAZA || use.type == U_ZYLA_ZLOTA)
				AddGameMsg2(txNeedPickaxe, 2.f, GMS_NEED_PICKAXE);
			else
				AddGameMsg2(txNeedUnk, 2.f, GMS_NEED_PICKAXE);
			ok = false;
		}
		else if(pc->unit->stan_broni != BRON_SCHOWANA && (bu.item != &pc->unit->GetWeapon() || pc->unit->HaveShield()))
		{
			if(after_action)
				return;
			u.HideWeapon();
			pc->po_akcja = PO_UZYJ;
			pc->po_akcja_useable = &use;
			ok = false;
		}
		else
			u.used_item = bu.item;
	}

	if(ok)
	{
		if(IsLocal())
		{
			u.action = A_ANIMATION2;
			u.animacja = ANI_ODTWORZ;
			u.ani->Play(bu.anim, PLAY_PRIO1, 0);
			u.useable = &use;
			u.useable->user = &u;
			u.target_pos = u.pos;
			u.target_pos2 = use.pos;
			if(g_base_usables[use.type].limit_rot == 4)
				u.target_pos2 -= VEC3(sin(use.rot)*1.5f,0,cos(use.rot)*1.5f);
			u.timer = 0.f;
			u.etap_animacji = 0;
			u.use_rot = lookat_angle(u.pos, u.useable->pos);
			before_player = BP_NONE;

			if(IsOnline())
			{
				NetChange& c = Add1(net_changes);
				c.type = NetChange::USE_USEABLE;
				c.unit = &u;
				c.id = u.useable->netid;
				c.ile = 1;
			}
		}
		else
		{
			NetChange& c = Add1(net_changes);
			c.type = NetChange::USE_USEABLE;
			c.id = before_player_ptr.useable->netid;
			c.ile = 1;
		}
	}
}

SOUND Game::GetTalkSound(Unit& u)
{
	if(IS_SET(u.data->flagi2, F2_XAR))
		return sXarTalk;
	else if(u.type == Unit::HUMAN)
		return sTalk[rand2()%4];
	else if(IS_SET(u.data->flagi2, F2_DZWIEK_ORK))
		return sOrcTalk;
	else if(IS_SET(u.data->flagi2, F2_DZWIEK_GOBLIN))
		return sGoblinTalk;
	else if(IS_SET(u.data->flagi2, F2_DZWIEK_GOLEM))
		return sGolemTalk;
	else
		return NULL;
}

void Game::UnitTalk(Unit& u, cstring text)
{
	assert(text && IsLocal());

	game_gui->AddSpeechBubble(&u, text);

	int ani = 0;
	if(u.type == Unit::HUMAN && rand2()%3 != 0)
	{
		ani = rand2()%2+1;
		u.ani->Play(ani == 1 ? "i_co" : "pokazuje", PLAY_ONCE|PLAY_PRIO2, 0);
		u.animacja = ANI_ODTWORZ;
		u.action = A_ANIMATION;
	}

	if(IsOnline())
	{
		NetChange& c = Add1(net_changes);
		c.type = NetChange::TALK;
		c.unit = &u;
		c.str = StringPool.Get();
		*c.str = text;
		c.id = ani;
		c.ile = -1;
		net_talk.push_back(c.str);
	}
}

void Game::OnEnterLocation()
{
	Unit* talker = NULL;
	cstring text = NULL;

	// gadanie gorusha po wej�ciu do lokacji
	if(orkowie_stan == OS_POWIEDZIAL_O_OBOZIE)
	{
		Quest_Orkowie2* q = (Quest_Orkowie2*)FindQuest(orkowie_refid2);
		if(current_location == q->target_loc && q->talked == 0)
		{
			q->talked = 1;
			talker = orkowie_gorush;
			text = txOrcCamp;
		}
	}

	if(!talker)
	{
		TeamInfo info;
		GetTeamInfo(info);
		bool pewna_szansa = false;

		if(info.sane_heroes > 0)
		{
			switch(location->type)
			{
			case L_CITY:
				text = random_string(txAiCity);
				break;
			case L_VILLAGE:
				text = random_string(txAiVillage);
				break;
			case L_MOONWELL:
				text = txAiMoonwell;
				break;
			case L_FOREST:
				text = txAiForest;
				break;
			case L_CAMP:
				if(location->state != LS_CLEARED)
				{
					pewna_szansa = true;
					cstring co;

					switch(location->spawn)
					{
					case SG_GOBLINY:
						co = txSGOGoblins;
						break;
					case SG_BANDYCI:
						co = txSGOBandits;
						break;
					case SG_ORKOWIE:
						co = txSGOOrcs;
						break;
					default:
						pewna_szansa = false;
						co = NULL;
						break;
					}

					if(pewna_szansa)
						text = Format(txAiCampFull, co);
					else
						text = NULL;
				}
				else
					text = txAiCampEmpty;
				break;
			}

			if(text && (pewna_szansa || rand2()%2 == 0))
				talker = GetRandomSaneHero();
		}
	}

	if(talker)
		UnitTalk(*talker, text);
}

void Game::OnEnterLevel()
{
	Unit* talker = NULL;
	cstring text = NULL;

	// gadanie jozana po wej�ciu do lokacji
	if(zlo_stan == ZS_ZAMYKANIE_PORTALI || zlo_stan == ZS_ZABIJ_BOSSA)
	{
		Quest_Zlo* q = (Quest_Zlo*)FindQuest(zlo_refid);
		if(zlo_stan == ZS_ZAMYKANIE_PORTALI)
		{
			int d = q->GetLocId(current_location);
			if(d != -1)
			{
				Quest_Zlo::Loc& loc = q->loc[d];

				if(dungeon_level == location->GetLastLevel())
				{
					if(loc.state < 2)
					{
						talker = jozan;
						text = txPortalCloseLevel;
						loc.state = 2;
					}
				}
				else if(dungeon_level == 0 && loc.state == 0)
				{
					talker = jozan;
					text = txPortalClose;
					loc.state = 1;
				}
			}
		}
		else if(current_location == q->target_loc && !q->told_about_boss)
		{
			q->told_about_boss = true;
			talker = jozan;
			text = txXarDanger;
		}
	}

	// gadanie gorusha po wej�ciu do lokacji
	if(!talker && (orkowie_stan == OS_GENERUJ_ORKI || orkowie_stan == OS_WYGENEROWANO_ORKI))
	{
		Quest_Orkowie2* q = (Quest_Orkowie2*)FindQuest(orkowie_refid2);
		if(current_location == q->target_loc)
		{
			if(dungeon_level == 0)
			{
				if(q->talked < 2)
				{
					q->talked = 2;
					talker = orkowie_gorush;
					text = txGorushDanger;
				}
			}
			else if(dungeon_level == location->GetLastLevel())
			{
				if(q->talked < 3)
				{
					q->talked = 3;
					talker = orkowie_gorush;
					text = txGorushCombat;
				}
			}
		}
	}

	// gadanie starego maga po wej�ciu do lokacji
	if(!talker && (magowie_stan == MS_STARY_MAG_DOLACZYL || magowie_stan == MS_MAG_ZREKRUTOWANY))
	{
		Quest_Magowie2* q = (Quest_Magowie2*)FindQuest(magowie_refid2);
		if(q->target_loc == current_location)
		{
			if(magowie_stan == MS_STARY_MAG_DOLACZYL)
			{
				if(dungeon_level == 0 && q->talked == 0)
				{
					q->talked = 1;
					text = txMageHere;
				}
			}
			else
			{
				if(dungeon_level == 0)
				{
					if(q->talked < 2)
					{
						q->talked = 2;
						text = Format(txMageEnter, magowie_imie.c_str());
					}
				}
				else if(dungeon_level == location->GetLastLevel() && q->talked < 3)
				{
					q->talked = 3;
					text = txMageFinal;
				}
			}
		}

		if(text)
			talker = FindTeamMemberById("q_magowie_stary");
	}

	if(!talker && dungeon_level == 0 && (enter_from == ENTER_FROM_OUTSIDE || enter_from >= ENTER_FROM_PORTAL))
	{
		TeamInfo info;
		GetTeamInfo(info);

		if(info.sane_heroes > 0)
		{
			LocalString s;

			switch(location->type)
			{
			case L_DUNGEON:
			case L_CRYPT:
				{
					InsideLocation* inside = (InsideLocation*)location;
					switch(inside->cel)
					{
					case HUMAN_FORT:
					case THRONE_FORT:
					case TUTORIAL_FORT:
						s = txAiFort;
						break;
					case DWARF_FORT:
						s = txAiDwarfFort;
						break;
					case MAGE_TOWER:
						s = txAiTower;
						break;
					case KOPALNIA_POZIOM:
						s = txAiArmory;
						break;
					case BANDITS_HIDEOUT:
						s = txAiHideout;
						break;
					case VAULT:
					case THRONE_VAULT:
						s = txAiVault;
						break;
					case HERO_CRYPT:
					case MONSTER_CRYPT:
						s = txAiCrypt;
						break;
					case OLD_TEMPLE:
						s = txAiTemple;
						break;
					case NECROMANCER_BASE:
						s = txAiNecromancerBase;
						break;
					case LABIRYNTH:
						s = txAiLabirynth;
						break;
					}

					if(inside->spawn == SG_BRAK)
						s += txAiNoEnemies;
					else
					{
						cstring co;

						switch(inside->spawn)
						{
						case SG_GOBLINY:
							co = txSGOGoblins;
							break;
						case SG_ORKOWIE:
							co = txSGOOrcs;
							break;
						case SG_BANDYCI:
							co = txSGOBandits;
							break;
						case SG_NIEUMARLI:
						case SG_NEKRO:
						case SG_ZLO:
							co = txSGOUndeads;
							break;
						case SG_MAGOWIE:
							co = txSGOMages;
							break;
						case SG_GOLEMY:
							co = txSGOGolems;
							break;
						case SG_MAGOWIE_I_GOLEMY:
							co = txSGOMagesAndGolems;
							break;
						case SG_UNK:
							co = txSGOUnk;
							break;
						case SG_WYZWANIE:
							co = txSGOPowerfull;
							break;
						default:
							co = txSGOEnemies;
							break;
						}

						s += Format(txAiNearEnemies, co);
					}
				}
				break;
			case L_CAVE:
				s = txAiCave;
				break;
			}

			UnitTalk(*GetRandomSaneHero(), s->c_str());
			return;
		}
	}

	if(talker)
		UnitTalk(*talker, text);
}

Unit* Game::FindTeamMemberById(cstring id)
{
	UnitData* ud = FindUnitData(id);
	assert(ud);

	for(vector<Unit*>::iterator it = team.begin(), end = team.end(); it != end; ++it)
	{
		if((*it)->data == ud)
			return *it;
	}

	assert(0);
	return NULL;
}

Unit* Game::GetRandomArenaHero()
{
	LocalVector<Unit*> v;

	for(vector<Unit*>::iterator it = at_arena.begin(), end = at_arena.end(); it != end; ++it)
	{
		if((*it)->IsPlayer() || (*it)->IsHero())
			v->push_back(*it);
	}

	return v->at(rand2()%v->size());
}

cstring Game::GetRandomIdleText(Unit& u)
{
	if(IS_SET(u.data->flagi3, F3_MAG_PIJAK) && magowie_stan < MS_MAG_WYLECZONY)
		return random_string(txAiDrunkMageText);

	int n = rand2()%100;
	if(n == 0)
		return random_string(txAiSecretText);

	int type = 1; // 0 - tekst hero, 1 - normalny tekst

	switch(u.data->grupa)
	{
	case G_SZALENI:
		if(u.IsTeamMember())
		{
			if(n < 33)
				return random_string(txAiInsaneText);
			else if(n < 66)
				type = 0;
			else
				type = 1;
		}
		else
		{
			if(n < 50)
				return random_string(txAiInsaneText);
			else
				type = 1;
		}
		break;
	case G_MIESZKANCY:
		if(u.IsTeamMember())
		{
			if(u.in_building >= 0 && (IS_SET(u.data->flagi, F_AI_PIJAK) || IS_SET(u.data->flagi3, F3_PIJAK_PO_ZAWODACH)))
			{
				int id;
				if(city_ctx->FindInn(id) && id == u.in_building)
				{
					if(IS_SET(u.data->flagi, F_AI_PIJAK) || zawody_stan != 1)
					{
						if(rand2()%3 == 0)
							return random_string(txAiDrunkText);
					}
					else
						return random_string(txAiDrunkmanText);
				}
			}
			if(n < 10)
				return random_string(txAiHumanText);
			else if(n < 55)
				type = 0;
			else
				type = 1;
		}
		else
			type = 1;
		break;
	case G_BANDYCI:
		if(n < 50)
			return random_string(txAiBanditText);
		else
			type = 1;
		break;
	case G_MAGOWIE:
		if(IS_SET(u.data->flagi, F_MAG) && n < 50)
			return random_string(txAiMageText);
		else
			type = 1;
		break;
	case G_GOBLINY:
		if(n < 50 && !IS_SET(u.data->flagi2, F2_NIE_GOBLIN))
			return random_string(txAiGoblinText);
		else
			type = 1;
		break;
	case G_ORKOWIE:
		if(n < 50)
			return random_string(txAiOrcText);
		else
			type = 1;
		break;
	}

	if(type == 0)
	{
		if(location->type == L_CITY || location->type == L_VILLAGE)
			return random_string(txAiHeroCityText);
		else if(location->outside)
			return random_string(txAiHeroOutsideText);
		else
			return random_string(txAiHeroDungeonText);
	}
	else
	{
		n = rand2()%100;
		if(n < 60)
			return random_string(txAiDefaultText);
		else if(location->outside)
			return random_string(txAiOutsideText);
		else
			return random_string(txAiInsideText);
	}
}

void Game::GetTeamInfo(TeamInfo& info)
{
	info.players = 0;
	info.npcs = 0;
	info.heroes = 0;
	info.sane_heroes = 0;
	info.insane_heroes = 0;
	info.free_members = 0;

	for(vector<Unit*>::iterator it = team.begin(), end = team.end(); it != end; ++it)
	{
		if((*it)->IsPlayer())
			++info.players;
		else
		{
			++info.npcs;
			if((*it)->IsHero())
			{
				if((*it)->hero->free)
					++info.free_members;
				else
				{
					++info.heroes;
					if(IS_SET((*it)->data->flagi, F_SZALONY))
						++info.insane_heroes;
					else
						++info.sane_heroes;
				}
			}
			else
				++info.free_members;
		}
	}
}

Unit* Game::GetRandomSaneHero()
{
	LocalVector<Unit*> v;

	for(vector<Unit*>::iterator it = active_team.begin(), end = active_team.end(); it != end; ++it)
	{
		if((*it)->IsHero() && !IS_SET((*it)->data->flagi, F_SZALONY))
			v->push_back(*it);
	}

	return v->at(rand2()%v->size());
}

UnitData* Game::GetRandomHeroData()
{
	cstring id;

	switch(rand2()%8)
	{
	case 0:
	case 1:
	case 2:
		id = "hero_warrior";
		break;
	case 3:
	case 4:
		id = "hero_hunter";
		break;
	case 5:
	case 6:
		id = "hero_rogue";
		break;
	case 7:
	default:
		id = "hero_mage";
		break;
	}

	return FindUnitData(id);
}

void Game::SzaleniSprawdzKamien()
{
	szaleni_sprawdz_kamien = false;

	const Item* kamien = FindItem("q_szaleni_kamien");
	if(!FindItemInTeam(kamien, -1, NULL, NULL, false))
	{
		// usu� kamie� z gry o ile to nie encounter bo i tak jest resetowany
		if(location->type != L_ENCOUNTER)
		{
			Quest_Szaleni* q = (Quest_Szaleni*)FindQuest(szaleni_refid);
			if(q && q->target_loc == current_location)
			{
				// jest w dobrym miejscu, sprawd� czy w�o�y� kamie� do skrzyni
				if(local_ctx.chests && local_ctx.chests->size() > 0)
				{
					Chest* chest;
					int slot;
					if(local_ctx.FindItemInChest(kamien, &chest, &slot))
					{
						// w�o�y� kamie�, koniec questa
						chest->items.erase(chest->items.begin()+slot);
						q->SetProgress(2);
						return;
					}
				}
			}
			
			RemoveItemFromWorld(kamien);
		}

		// dodaj kamie� przyw�dcy
		leader->AddItem(kamien, 1, false);
	}

	if(szaleni_stan == SS_POGADANO_Z_SZALONYM)
	{
		szaleni_stan = SS_WZIETO_KAMIEN;
		szaleni_dni = 13;
	}
}

// usuwa podany przedmiot ze �wiata
// u�ywane w que�cie z kamieniem szale�c�w
bool Game::RemoveItemFromWorld(const Item* item)
{
	assert(item);

	if(local_ctx.RemoveItemFromWorld(item))
		return true;

	if(city_ctx)
	{
		for(vector<InsideBuilding*>::iterator it = city_ctx->inside_buildings.begin(), end = city_ctx->inside_buildings.end(); it != end; ++it)
		{
			if((*it)->ctx.RemoveItemFromWorld(item))
				return true;
		}
	}

	return false;
}

const Item* Game::GetRandomItem(int max_value)
{
	int type = rand2()%6;

	LocalVector<const Item*> items;

	switch(type)
	{
	case 0:
		for(uint i=0; i<n_weapons; ++i)
		{
			if(g_weapons[i].value <= max_value && g_weapons[i].CanBeGenerated())
				items->push_back(&g_weapons[i]);
		}
		break;
	case 1:
		for(uint i=0; i<n_bows; ++i)
		{
			if(g_bows[i].value <= max_value && g_bows[i].CanBeGenerated())
				items->push_back(&g_bows[i]);
		}
		break;
	case 2:
		for(uint i=0; i<n_armors; ++i)
		{
			if(g_armors[i].value <= max_value && g_armors[i].CanBeGenerated())
				items->push_back(&g_armors[i]);
		}
		break;
	case 3:
		for(uint i=0; i<n_shields; ++i)
		{
			if(g_shields[i].value <= max_value && g_shields[i].CanBeGenerated())
				items->push_back(&g_shields[i]);
		}
		break;
	case 4:
		for(uint i=0; i<n_consumeables; ++i)
		{
			if(g_consumeables[i].value <= max_value && g_consumeables[i].CanBeGenerated())
				items->push_back(&g_consumeables[i]);
		}
		break;
	case 5:
		for(uint i=0; i<n_others; ++i)
		{
			if(g_others[i].value <= max_value && g_consumeables[i].CanBeGenerated())
				items->push_back(&g_others[i]);
		}
		break;
	}

	return items->at(rand2()%items->size());
}

bool Game::CheckMoonStone(GroundItem* item, Unit* unit)
{
	assert(item && unit);

	if(sekret_stan == SS2_BRAK && location->type == L_MOONWELL && strcmp(item->item->id, "vs_krystal") == 0 && distance2d(item->pos, VEC3(128.f,0,128.f)) < 1.2f)
	{
		AddGameMsg(txSecretAppear, 3.f);
		sekret_stan = SS2_WRZUCONO_KAMIEN;
		int loc = CreateLocation(L_DUNGEON, VEC2(0,0), -128.f, DWARF_FORT, SG_WYZWANIE, false, 3);
		Location& l = *locations[loc];
		l.st = 18;
		l.active_quest = (Quest_Dungeon*)ACTIVE_QUEST_HOLDER;
		l.state = LS_UNKNOWN;
		sekret_gdzie = loc;
		VEC2& cpos = location->pos;
		Item* note = GetSecretNote();
		note->desc = Format("\"%c %d km, %c %d km\"", cpos.y>l.pos.y ? 'S' : 'N', (int)abs((cpos.y-l.pos.y)/3), cpos.x>l.pos.x ? 'W' : 'E', (int)abs((cpos.x-l.pos.x)/3));
		unit->AddItem(note);
		delete item;
		if(IsOnline())
			PushNetChange(NetChange::SECRET_TEXT);
		return true;
	}

	return false;
}

UnitData* Game::GetUnitDataFromClass(Class clas, bool crazy)
{
	cstring id = NULL;

	switch(clas)
	{
	case Class::WARRIOR:
		id = (crazy ? "crazy_warrior" : "hero_warrior");
		break;
	case Class::HUNTER:
		id = (crazy ? "crazy_hunter" : "hero_hunter");
		break;
	case Class::ROGUE:
		id = (crazy ? "crazy_rogue" : "hero_rogue");
		break;
	case Class::MAGE:
		id = (crazy ? "crazy_mage" : "hero_mage");
		break;
	}

	if(id)
		return FindUnitData(id, false);
	else
		return NULL;
}

void Game::AddTimedUnit(Unit* unit, int location, int days)
{
	assert(unit && location >= 0 && days > 0);

	TimedUnit& tu = Add1(timed_units);
	tu.unit = unit;
	tu.location = location;
	tu.days = days;
}

void Game::RemoveTimedUnit(Unit* unit)
{
	assert(unit);

	for(vector<TimedUnit>::iterator it = timed_units.begin(), end = timed_units.end(); it != end; ++it)
	{
		if(it->unit == unit)
		{
			timed_units.erase(it);
			return;
		}
	}

	assert(0);
}

void Game::RemoveUnitFromLocation(Unit* unit, int location)
{
	if(game_state == GS_LEVEL && location == current_location)
	{
		unit->to_remove = true;
		to_remove.push_back(unit);
	}
	else
	{
		Location* loc = locations[location];

		if(loc->type == L_CITY || loc->type == L_VILLAGE)
		{
			City* city = (City*)loc;
			for(vector<Unit*>::iterator it = city->units.begin(), end = city->units.end(); it != end; ++it)
			{
				if(*it == unit)
				{
					city->units.erase(it);
					delete unit;
					return;
				}
			}

			for(vector<InsideBuilding*>::iterator it = city->inside_buildings.begin(), end = city->inside_buildings.end(); it != end; ++it)
			{
				for(vector<Unit*>::iterator it2 = (*it)->units.begin(), end2 = (*it)->units.end(); it2 != end2; ++it2)
				{
					if(*it2 == unit)
					{
						(*it)->units.erase(it2);
						delete unit;
						return;
					}
				}
			}
		}
		else
		{
			// dzia�a tylko w mie�cie/wiosce
			assert(0);
		}
	}
}

int xdif(int a, int b)
{
	if(a == b)
		return 0;
	else if(a < b)
	{
		a += 10;
		if(a >= b)
			return 1;
		a += 25;
		if(a >= b)
			return 2;
		a += 50;
		if(a >= b)
			return 3;
		a += 100;
		if(a >= b)
			return 4;
		a += 150;
		if(a >= b)
			return 5;
		return 6;
	}
	else
	{
		b += 10;
		if(b >= a)
			return -1;
		b += 25;
		if(b >= a)
			return -2;
		b += 50;
		if(b >= a)
			return -3;
		b += 100;
		if(b >= a)
			return -4;
		b += 150;
		if(b >= a)
			return -5;
		return -6;
	}
}

Game::BLOCK_RESULT Game::CheckBlock(Unit& hitted, float angle_dif, float attack_power, float skill, float str)
{
	int k_block;
// 	if(hitted.HaveShield())
// 	{

	// blokowanie tarcz�
	k_block = xdif((int)hitted.CalculateBlock(&hitted.GetShield()), (int)attack_power);

	k_block += xdif(hitted.skill[(int)Skill::SHIELD], (int)skill);

// 	k_block += hitted.attrib[A_STR]/2;
// 	k_block += hitted.attrib[A_DEX]/4;
// 	k_block += hitted.skill[S_SHIELD];

		// premia za bro�
// 	}
// 	else
// 	{
// 		// blokowanie broni�
// 		k_block = 0;
// 	}

	if(str > 0.f)
		k_block += xdif(hitted.attrib[(int)Attribute::STR], (int)str);

	if(angle_dif > PI/4)
	{
		if(angle_dif > PI/4+PI/8)
			k_block -= 2;
		else
			--k_block;
	}

	k_block += random(-5,5);

	LOG(Format("k_block: %d", k_block));

	/*return BLOCK_PERFECT;*/

	if(k_block > 8)
	{
		LOG("BLOCK_BREAK");
		return BLOCK_BREAK;
	}
	else if(k_block > 4)
	{
		LOG("BLOCK_POOR");
		return BLOCK_POOR;
	}
	else if(k_block > 0)
	{
		LOG("BLOCK_MEDIUM");
		return BLOCK_MEDIUM;
	}
	else if(k_block > -4)
	{
		LOG("BLOCK_GOOD");
		return BLOCK_GOOD;
	}
	else
	{
		LOG("BLOCK_PERFECT");
		return BLOCK_PERFECT;
	}
}

void Game::AddTeamMember(Unit* unit, bool active)
{
	assert(unit);

	// dodaj
	if(active)
	{
		if(active_team.size() == 1u)
			active_team[0]->MakeItemsTeam(false);
		active_team.push_back(unit);
	}
	team.push_back(unit);

	// ustaw przedmioty jako nie dru�ynowe
	unit->MakeItemsTeam(false);

	// aktualizuj TeamPanel o ile otwarty
	if(team_panel->visible)
		team_panel->Changed();
}

void Game::RemoveTeamMember(Unit* unit)
{
	// usu�
	RemoveElementOrder(team, unit);
	RemoveElementTry(active_team, unit);

	// ustaw przedmioty jako dru�ynowe
	unit->MakeItemsTeam(true);

	// aktualizuj TeamPanel o ile otwarty
	if(team_panel->visible)
		team_panel->Changed();
}

void Game::DropGold(int ile)
{
	pc->unit->gold -= ile;
	if(sound_volume)
		PlaySound2d(sMoneta);

	// animacja wyrzucania
	pc->unit->action = A_ANIMATION;
	pc->unit->ani->Play("wyrzuca", PLAY_ONCE|PLAY_PRIO2, 0);
	pc->unit->ani->frame_end_info = false;

	if(IsLocal())
	{
		// stw�rz przedmiot
		GroundItem* item = new GroundItem;
		item->item = &gold_item;
		item->count = ile;
		item->team_count = 0;
		item->pos = pc->unit->pos;
		item->pos.x -= sin(pc->unit->rot)*0.25f;
		item->pos.z -= cos(pc->unit->rot)*0.25f;
		item->rot = random(MAX_ANGLE);
		AddGroundItem(GetContext(*pc->unit), item);

		// wy�lij info o animacji
		if(IsServer())
		{
			NetChange& c = Add1(net_changes);
			c.type = NetChange::DROP_ITEM;
			c.unit = pc->unit;
		}
	}
	else
	{
		// wy�lij komunikat o wyrzucaniu z�ota
		NetChange& c = Add1(net_changes);
		c.type = NetChange::DROP_GOLD;
		c.id = ile;
	}
}

void Game::AddItem(Unit& unit, const Item* item, uint count, uint team_count, bool send_msg)
{
	assert(item && count && team_count <= count);

	// ustal docelowy ekwipunek (o ile jest)
	Inventory* inv = NULL;
	ItemSlot slot;
	switch(Inventory::lock_id)
	{
	case LOCK_NO:
		break;
	case LOCK_MY:
		if(inventory->unit == &unit)
			inv = inventory;
		break;
	case LOCK_TRADE_MY:
		if(inv_trade_mine->unit == &unit)
			inv = inv_trade_mine;
		break;
	case LOCK_TRADE_OTHER:
		if(pc->action == PlayerController::Action_LootUnit && inv_trade_other->unit == &unit)
			inv = inv_trade_other;
		break;
	}
	if(inv && Inventory::lock_index >= 0)
		slot = inv->items->at(Inventory::lock_index);

	// dodaj przedmiot
	unit.AddItem(item, count, team_count);

	// komunikat
	if(send_msg && IsServer())
	{
		if(unit.IsPlayer())
		{
			if(unit.player != pc)
			{
				// dodaj komunikat o dodaniu przedmiotu
				NetChangePlayer& c = Add1(net_changes_player);
				c.type = NetChangePlayer::ADD_ITEMS;
				c.pc = unit.player;
				c.item = item;
				c.ile = count;
				c.id = team_count;
				GetPlayerInfo(c.pc).NeedUpdate();
			}
		}
		else
		{
			// sprawd� czy ta jednostka nie wymienia si� przedmiotami z graczem
			Unit* u = NULL;
			for(vector<Unit*>::iterator it = active_team.begin(), end = active_team.end(); it != end; ++it)
			{
				if((*it)->IsPlayer() && (*it)->player->IsTradingWith(&unit))
				{
					u = *it;
					break;
				}
			}

			if(u && u->player != pc)
			{
				// wy�lij komunikat do gracza z aktualizacj� ekwipunku
				NetChangePlayer& c = Add1(net_changes_player);
				c.type = NetChangePlayer::ADD_ITEMS_TRADER;
				c.pc = u->player;
				c.item = item;
				c.id = unit.netid;
				c.ile = count;
				c.a = team_count;
				GetPlayerInfo(c.pc).NeedUpdate();
			}
		}
	}

	// czy trzeba budowa� tymczasowy ekwipunek
	int rebuild_id = -1;
	if(&unit == pc->unit)
	{
		if(inventory->visible || gp_trade->visible)
			rebuild_id = 0;
	}
	else if(gp_trade->visible && inv_trade_other->unit == &unit)
		rebuild_id = 1;
	if(rebuild_id != -1)
		BuildTmpInventory(rebuild_id);

	// aktualizuj zlockowany przedmiot
	if(inv)
	{		
		while(1)
		{
			ItemSlot& s = inv->items->at(Inventory::lock_index);
			if(s.item == slot.item && s.count >= slot.count)
				break;
			++Inventory::lock_index;
		}
	}
}

void Game::AddItem(Chest& chest, const Item* item, uint count, uint team_count, bool send_msg)
{
	assert(item && count && team_count <= count);

	// ustal docelowy ekwipunek (o ile jest)
	Inventory* inv = NULL;
	ItemSlot slot;
	if(Inventory::lock_id == LOCK_TRADE_OTHER && pc->action == PlayerController::Action_LootChest && pc->action_chest == &chest && Inventory::lock_index >= 0)
	{
		inv = inv_trade_other;
		slot = inv->items->at(Inventory::lock_index);
	}

	// dodaj przedmiot
	chest.AddItem(item, count, team_count);

	// komunikat
	if(send_msg && chest.looted)
	{
		Unit* u = FindChestUserIfPlayer(&chest);
		if(u)
		{
			// dodaj komunikat o dodaniu przedmiotu do skrzyni
			NetChangePlayer& c = Add1(net_changes_player);
			c.type = NetChangePlayer::ADD_ITEMS_CHEST;
			c.pc = u->player;
			c.item = item;
			c.id = chest.netid;
			c.ile = count;
			c.a = team_count;
			GetPlayerInfo(c.pc).NeedUpdate();
		}
	}

	// czy trzeba przebudowa� tymczasowy ekwipunek
	if(gp_trade->visible && pc->action == PlayerController::Action_LootChest && pc->action_chest == &chest)
		BuildTmpInventory(1);

	// aktualizuj zlockowany przedmiot
	if(inv)
	{
		while(1)
		{
			ItemSlot& s = inv->items->at(Inventory::lock_index);
			if(s.item == slot.item && s.count >= slot.count)
				break;
			++Inventory::lock_index;
		}
	}
}

// zbuduj tymczasowy ekwipunek kt�ry ��czy slots i items postaci
void Game::BuildTmpInventory(int index)
{
	assert(index == 0 || index == 1);

	vector<int>& ids = tmp_inventory[index];
	const Item** slots;
	vector<ItemSlot>* items;
	int& shift = tmp_inventory_shift[index];
	shift = 0;

	if(index == 0)
	{
		// przedmioty gracza
		slots = pc->unit->slots;
		items = &pc->unit->items;
	}
	else
	{
		// przedmioty innej postaci, w skrzyni
		if(pc->action == PlayerController::Action_LootChest || pc->action == PlayerController::Action_Trade)
			slots = NULL;
		else
			slots = pc->action_unit->slots;
		items = pc->chest_trade;
	}

	ids.clear();

	// je�li to posta� to dodaj za�o�one przedmioty
	if(slots)
	{
		for(int i=0; i<SLOT_MAX; ++i)
		{
			if(slots[i])
			{
				ids.push_back(-i-1);
				++shift;
			}
		}
	}

	// nie za�o�one przedmioty
	for(int i=0, ile = (int)items->size(); i<ile; ++i)
		ids.push_back(i);
}

int Game::GetQuestIndex(Quest* quest)
{
	assert(quest);

	int index = 0;
	for(vector<Quest*>::iterator it = quests.begin(), end = quests.end(); it != end; ++it, ++index)
	{
		if(*it == quest)
			return index;
	}

	return -1;
}

Unit* Game::FindChestUserIfPlayer(Chest* chest)
{
	assert(chest && chest->looted);

	for(vector<Unit*>::iterator it = active_team.begin(), end = active_team.end(); it != end; ++it)
	{
		if((*it)->IsPlayer() && (*it)->player->action == PlayerController::Action_LootChest && (*it)->player->action_chest == chest)
			return *it;
	}

	return NULL;
}

void Game::RemoveItem(Unit& unit, int i_index, uint count)
{
	// ustal docelowy ekwipunek (o ile jest)
	Inventory* inv = NULL;
	ItemSlot slot;
	switch(Inventory::lock_id)
	{
	default:
		assert(0);
	case LOCK_NO:
		break;
	case LOCK_MY:
		if(inventory->unit == &unit)
			inv = inventory;
		break;
	case LOCK_TRADE_MY:
		if(inv_trade_mine->unit == &unit)
			inv = inv_trade_mine;
		break;
	case LOCK_TRADE_OTHER:
		if(pc->action == PlayerController::Action_LootUnit && inv_trade_other->unit == &unit)
			inv = inv_trade_other;
		break;
	}
	if(inv && Inventory::lock_index >= 0)
		slot = inv->items->at(Inventory::lock_index);

	// usu� przedmiot
	bool removed = false;
	if(i_index >= 0)
	{
		ItemSlot& s = unit.items[i_index];
		uint ile = (count == 0 ? s.count : min(s.count, count));
		s.count -= ile;
		if(s.count == 0)
		{
			removed = true;
			unit.items.erase(unit.items.begin()+i_index);
		}
		else if(s.team_count > 0)
			s.team_count -= min(s.team_count, ile);
		unit.weight -= s.item->weight*ile;
	}
	else
	{
		ITEM_SLOT type = IIndexToSlot(i_index);
		unit.weight -= unit.slots[type]->weight;
		unit.slots[type] = NULL;
		removed = true;

		if(IsServer() && players > 1)
		{
			NetChange& c = Add1(net_changes);
			c.type = NetChange::CHANGE_EQUIPMENT;
			c.unit = &unit;
			c.id = type;
		}
	}

	// komunikat
	if(IsServer())
	{
		if(unit.IsPlayer())
		{
			// dodaj komunikat o dodaniu przedmiotu
			NetChangePlayer& c = Add1(net_changes_player);
			c.type = NetChangePlayer::REMOVE_ITEMS;
			c.pc = unit.player;
			c.id = i_index;
			c.ile = count;
			GetPlayerInfo(c.pc).NeedUpdate();
		}
		else
		{
			Unit* t = NULL;

			// szukaj gracza kt�ry handluje z t� postaci�
			for(vector<Unit*>::iterator it = active_team.begin(), end = active_team.end(); it != end; ++it)
			{
				if((*it)->IsPlayer() && (*it)->player->IsTradingWith(&unit))
				{
					t = *it;
					break;
				}
			}

			if(t && t->player != pc)
			{
				// dodaj komunikat o dodaniu przedmiotu
				NetChangePlayer& c = Add1(net_changes_player);
				c.type = NetChangePlayer::REMOVE_ITEMS_TRADER;
				c.pc = t->player;
				c.id = unit.netid;
				c.ile = count;
				c.a = i_index;
				GetPlayerInfo(c.pc).NeedUpdate();
			}
		}
	}

	// aktualizuj tymczasowy ekwipunek
	if(pc->unit == &unit)
	{
		if(inventory->visible || gp_trade->visible)
			BuildTmpInventory(0);
	}
	else if(gp_trade->visible && inv_trade_other->unit == &unit)
		BuildTmpInventory(1);

	// aktualizuj zlockowany przedmiot
	if(inv && removed)
	{
		if(i_index == Inventory::lock_index)
			Inventory::lock_index = LOCK_REMOVED;
		else if(i_index < Inventory::lock_index)
		{
			while(1)
			{
				ItemSlot& s = inv->items->at(Inventory::lock_index);
				if(s.item == slot.item && s.count == slot.count)
					break;
				--Inventory::lock_index;
			}
		}
	}
}

bool Game::RemoveItem(Unit& unit, const Item* item, uint count)
{
	int i_index = unit.FindItem(item);
	if(i_index == INVALID_IINDEX)
		return false;
	RemoveItem(unit, i_index, count);
	return true;
}

INT2 Game::GetSpawnPoint()
{
	InsideLocation* inside = (InsideLocation*)location;
	InsideLocationLevel& lvl = inside->GetLevelData();

	if(enter_from >= ENTER_FROM_PORTAL)
		return pos_to_pt(inside->GetPortal(enter_from)->GetSpawnPos());
	else if(enter_from == ENTER_FROM_DOWN_LEVEL)
		return lvl.schody_gora+g_kierunek2[lvl.schody_gora_dir];
	else
		return lvl.schody_dol+g_kierunek2[lvl.schody_dol_dir];
}

InsideLocationLevel* Game::TryGetLevelData()
{
	if(location->type == L_DUNGEON || location->type == L_CRYPT || location->type == L_CAVE)
		return &((InsideLocation*)location)->GetLevelData();
	else
		return NULL;
}

GroundItem* Game::SpawnGroundItemInsideAnyRoom(InsideLocationLevel& lvl, const Item* item)
{
	assert(item);
	while(true)
	{
		int id = rand2()%lvl.pokoje.size();
		if(!lvl.pokoje[id].korytarz)
		{
			GroundItem* item2 = SpawnGroundItemInsideRoom(lvl.pokoje[id], item);
			if(item2)
				return item2;
		}
	}
}

Unit* Game::SpawnUnitInsideRoomOrNear(InsideLocationLevel& lvl, Pokoj& p, UnitData& ud, int level, const INT2& pt, const INT2& pt2)
{
	Unit* u = SpawnUnitInsideRoom(p, ud, level, pt, pt2);
	if(u)
		return u;

	LocalVector<int> connected(p.polaczone);
	connected.Shuffle();

	for(vector<int>::iterator it = connected->begin(), end = connected->end(); it != end; ++it)
	{
		u = SpawnUnitInsideRoom(lvl.pokoje[*it], ud, level, pt, pt2);
		if(u)
			return u;
	}

	return NULL;
}

void Game::ValidateTeamItems()
{
	if(!IsLocal())
		return;

	struct IVector
	{
		void* _Alval;
		void* _Myfirst;	// pointer to beginning of array
		void* _Mylast;	// pointer to current end of sequence
		void* _Myend;
	};

	int errors = 0;
	for(vector<Unit*>::iterator it = active_team.begin(), end = active_team.end(); it != end; ++it)
	{
		if((*it)->items.empty())
			continue;

		IVector* iv = (IVector*)&(*it)->items;
		if(!iv->_Myfirst)
		{
			ERROR(Format("Hero '%s' items._Myfirst = NULL!", (*it)->GetName()));
			++errors;
			continue;
		}

		int index = 0;
		for(vector<ItemSlot>::iterator it2 = (*it)->items.begin(), end2 = (*it)->items.end(); it2 != end2; ++it2, ++index)
		{
			if(!it2->item)
			{
				ERROR(Format("Hero '%s' has NULL item at index %d.", (*it)->GetName(), index));
				++errors;
			}
			else if(it2->item->IsStackable())
			{
				int index2 = index+1;
				for(vector<ItemSlot>::iterator it3 = it2+1; it3 != end2; ++it3, ++index2)
				{
					if(it2->item == it3->item)
					{
						ERROR(Format("Hero '%s' has multiple stacks of %s, index %d and %d.", (*it)->GetName(), it2->item->id, index, index2));
						++errors;
						break;
					}
				}
			}
		}
	}

	if(errors)
		AddGameMsg(Format("%d hero inventory errors!", errors), 10.f);
}

Unit* Game::SpawnUnitInsideInn(UnitData& ud, int level, InsideBuilding* inn)
{
	if(!inn)
		inn = city_ctx->FindInn();

	VEC3 pos;
	bool ok = false;
	if(rand2()%5 != 0)
	{
		if(WarpToArea(inn->ctx, inn->arena1, ud.GetRadius(), pos, 20) || 
			WarpToArea(inn->ctx, inn->arena2, ud.GetRadius(), pos, 10))
			ok = true;
	}
	else
	{
		if(WarpToArea(inn->ctx, inn->arena2, ud.GetRadius(), pos, 10) || 
			WarpToArea(inn->ctx, inn->arena1, ud.GetRadius(), pos, 20))
			ok = true;
	}

	if(ok)
		return CreateUnitWithAI(inn->ctx, ud, level, NULL, &pos);
	else
		return NULL;
}

void Game::SpawnDrunkmans()
{
	InsideBuilding* inn = city_ctx->FindInn();
	chlanie_wygenerowano = true;
	UnitData& pijak = *FindUnitData("pijak");
	int ile = random(4,6);
	for(int i=0; i<ile; ++i)
	{
		Unit* u = SpawnUnitInsideInn(pijak, random(2,15), inn);
		if(u)
		{
			u->rot = random(MAX_ANGLE);
			u->temporary = true;
			if(IsOnline())
				Net_SpawnUnit(u);
		}
	}
}

void Game::SetOutsideParams()
{
	draw_range = 80.f;
	clear_color2 = WHITE;
	fog_params = VEC4(40, 80, 40, 0);
	fog_color = VEC4(0.9f, 0.85f, 0.8f, 1);
	ambient_color = VEC4(0.5f, 0.5f, 0.5f, 1);
}

void Game::OnEnterLevelOrLocation()
{
	ClearGui();
}