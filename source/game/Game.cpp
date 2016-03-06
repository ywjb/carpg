// game
#include "Pch.h"
#include "Base.h"
#include "Game.h"
#include "Terrain.h"
#include "EnemyGroup.h"
#include "ParticleSystem.h"
#include "Language.h"
#include "Version.h"
#include "CityGenerator.h"
#include "Quest_Mages.h"

// limit fps
#define LIMIT_DT 0.3f

// symulacja lag�w
#define TESTUJ_LAG
#ifndef _DEBUG
#undef TESTUJ_LAG
#endif
#ifdef TESTUJ_LAG
#define MY_PRIORITY HIGH_PRIORITY
#else
#define MY_PRIORITY IMMEDIATE_PRIORITY
#endif

const float bazowa_wysokosc = 1.74f;
Game* Game::_game;
cstring Game::txGoldPlus, Game::txQuestCompletedGold;
GameKeys GKey;
extern string g_system_dir;
extern cstring RESTART_MUTEX_NAME;

Game::Game() : have_console(false), vbParticle(nullptr), peer(nullptr), quickstart(QUICKSTART_NONE), inactive_update(false), last_screenshot(0), console_open(false),
cl_fog(true), cl_lighting(true), draw_particle_sphere(false), draw_unit_radius(false), draw_hitbox(false), noai(false), testing(0), speed(1.f), cheats(false),
used_cheats(false), draw_phy(false), draw_col(false), force_seed(0), next_seed(0), force_seed_all(false), obj_alpha("tmp_alpha", 0, 0, "tmp_alpha", nullptr, 1), alpha_test_state(-1),
debug_info(false), dont_wander(false), exit_mode(false), local_ctx_valid(false), city_ctx(nullptr), check_updates(true), skip_version(-1), skip_tutorial(false), sv_online(false), portal_anim(0),
nosound(false), nomusic(false), debug_info2(false), music_type(MUSIC_MISSING), contest_state(CONTEST_NOT_DONE), koniec_gry(false), net_stream(64*1024),
net_stream2(64*1024), exit_to_menu(false), mp_interp(0.05f), mp_use_interp(true), mp_port(PORT), paused(false), pick_autojoin(false), draw_flags(0xFFFFFFFF), tMiniSave(nullptr),
prev_game_state(GS_LOAD), clearup_shutdown(false), tSave(nullptr), sItemRegion(nullptr), sChar(nullptr), sSave(nullptr), in_tutorial(false), cursor_allow_move(true), mp_load(false), was_client(false),
sCustom(nullptr), cl_postfx(true), mp_timeout(10.f), sshader_pool(nullptr), cl_normalmap(true), cl_specularmap(true), dungeon_tex_wrap(true), mutex(nullptr), profiler_mode(0), grass_range(40.f),
vbInstancing(nullptr), vb_instancing_max(0), screenshot_format(D3DXIFF_JPG), next_seed_extra(false), quickstart_class(Class::RANDOM), autopick_class(Class::INVALID), gold_item(IT_GOLD),
current_packet(nullptr), game_state(GS_LOAD)
{
#ifdef _DEBUG
	cheats = true;
	used_cheats = true;
#endif

	_game = this;
	Quest::game = this;

	dialog_context.is_local = true;

	ClearPointers();
	NullGui();

	light_angle = 0.f;
	uv_mod = Terrain::DEFAULT_UV_MOD;
	cam.draw_range = 80.f;

	gen = new CityGenerator;
}

Game::~Game()
{
	delete gen;
}

Texture Game::LoadTex2(cstring name)
{
	assert(name);

	return Texture(LoadTexResource(name));
}

#ifdef CHECK_OOBOX_COL
VEC3 pos1, pos2, rot1, rot2, hitpoint;
bool contact;
#endif

//=================================================================================================
// Rysowanie gry
//=================================================================================================
void Game::OnDraw()
{
	OnDraw(true);
}

void Game::OnDraw(bool normal)
{
	if(normal)
	{
		if(profiler_mode == 2)
			g_profiler.Start();
		else if(profiler_mode == 0)
			g_profiler.Clear();
	}

	if(post_effects.empty() || !ePostFx)
	{
		if(sCustom)
			V( device->SetRenderTarget(0, sCustom) );

		V( device->Clear(0, nullptr, D3DCLEAR_ZBUFFER | D3DCLEAR_TARGET | D3DCLEAR_STENCIL, clear_color, 1.f, 0) );
		V( device->BeginScene() );

		if(game_state == GS_LEVEL)
		{
			// draw level
			Draw();

			// draw glow
			if(before_player != BP_NONE && !draw_batch.glow_nodes.empty())
			{
				V( device->EndScene() );
				DrawGlowingNodes(false);
				V( device->BeginScene() );
			}
		}

		// draw gui
		GUI.mViewProj = cam.matViewProj;
		GUI.Draw(wnd_size);

		V( device->EndScene() );
	}
	else
	{
		// renderuj scen� do tekstury
		SURFACE sPost;
		if(!IsMultisamplingEnabled())
			V( tPostEffect[2]->GetSurfaceLevel(0, &sPost) );
		else
			sPost = sPostEffect[2];
		
		V( device->SetRenderTarget(0, sPost) );
		V( device->Clear(0, nullptr, D3DCLEAR_ZBUFFER | D3DCLEAR_TARGET | D3DCLEAR_STENCIL, clear_color, 1.f, 0) );

		if(game_state == GS_LEVEL)
		{
			V( device->BeginScene() );
			Draw();
			V( device->EndScene() );
			if(before_player != BP_NONE && !draw_batch.glow_nodes.empty())
				DrawGlowingNodes(true);
		}

		PROFILER_BLOCK("PostEffects");

		TEX t;
		if(!IsMultisamplingEnabled())
		{
			sPost->Release();
			t = tPostEffect[2];
		}
		else
		{
			SURFACE surf2;
			V( tPostEffect[0]->GetSurfaceLevel(0, &surf2) );
			V( device->StretchRect(sPost, nullptr, surf2, nullptr, D3DTEXF_NONE) );
			surf2->Release();
			t = tPostEffect[0];
		}

		// post effects
		V( device->SetVertexDeclaration(vertex_decl[VDI_TEX]) );
		V( device->SetStreamSource(0, vbFullscreen, 0, sizeof(VTex)) );
		SetAlphaTest(false);
		SetAlphaBlend(false);
		SetNoCulling(false);
		SetNoZWrite(true);

		UINT passes;
		int index_surf = 1;
		for(vector<PostEffect>::iterator it = post_effects.begin(), end = post_effects.end(); it != end; ++it)
		{
			SURFACE surf;
			if(it+1 == end)
			{
				// ostatni pass
				if(sCustom)
					surf = sCustom;
				else
					V( device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &surf) );
			}
			else
			{
				// jest nast�pny
				if(!IsMultisamplingEnabled())
					V( tPostEffect[index_surf]->GetSurfaceLevel(0, &surf) );
				else
					surf = sPostEffect[index_surf];
			}

			V( device->SetRenderTarget(0, surf) );
			V( device->BeginScene() );

			V( ePostFx->SetTechnique(it->tech) );
			V( ePostFx->SetTexture(hPostTex, t) );
			V( ePostFx->SetFloat(hPostPower, it->power) );
			V( ePostFx->SetVector(hPostSkill, &it->skill) );

			V( ePostFx->Begin(&passes, 0) );
			V( ePostFx->BeginPass(0) );
			V( device->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 2) );
			V( ePostFx->EndPass() );
			V( ePostFx->End() );

			if(it+1 == end)
			{
				GUI.mViewProj = cam.matViewProj;
				GUI.Draw(wnd_size);
			}

			V( device->EndScene() );

			if(it+1 == end)
			{
				if(!sCustom)
					surf->Release();
			}
			else if(!IsMultisamplingEnabled())
			{
				surf->Release();
				t = tPostEffect[index_surf];
			}
			else
			{
				SURFACE surf2;
				V( tPostEffect[0]->GetSurfaceLevel(0, &surf2) );
				V( device->StretchRect(surf, nullptr, surf2, nullptr, D3DTEXF_NONE) );
				surf2->Release();
				t = tPostEffect[0];
			}
			
			index_surf = (index_surf + 1) % 3;
		}
	}

	g_profiler.End();
}

void Game::LoadData()
{
	LOG("Creating list of files.");
	AddDir("data");

	LOG("Preloading files.");
	CreateTextures();
	PreloadData();

	//-----------------------------------------------------------
	//-------------------- SHADERY ------------------------------
	load_tasks.push_back(LoadTask("mesh.fx", &eMesh));
	load_tasks.push_back(LoadTask("particle.fx", &eParticle));
	load_tasks.push_back(LoadTask("skybox.fx", &eSkybox));
	load_tasks.push_back(LoadTask("terrain.fx", &eTerrain));
	load_tasks.push_back(LoadTask("area.fx", &eArea));
	load_tasks.push_back(LoadTask("post.fx", &ePostFx));
	load_tasks.push_back(LoadTask("glow.fx", &eGlow));
	load_tasks.push_back(LoadTask("grass.fx", &eGrass));
	load_tasks.push_back(LoadTask(LoadTask::SetupShaders));

	//-----------------------------------------------------------
	//------------------ TEKSTURY -------------------------------
	// GUI
	load_tasks.push_back(LoadTask("klasa_cecha.png", &tKlasaCecha));
	load_tasks.push_back(LoadTask("celownik.png", &tCelownik));
	load_tasks.push_back(LoadTask("bubble.png", &tBubble));
	load_tasks.push_back(LoadTask("gotowy.png", &tGotowy));
	load_tasks.push_back(LoadTask("niegotowy.png", &tNieGotowy));
	load_tasks.push_back(LoadTask("save-16.png", &tIcoZapis));
	load_tasks.push_back(LoadTask("padlock-16.png", &tIcoHaslo));
	// GAME
	load_tasks.push_back(LoadTask("emerytura.jpg", &tEmerytura));
	load_tasks.push_back(LoadTask("equipped.png", &tEquipped));
	load_tasks.push_back(LoadTask("czern.bmp", &tCzern));
	load_tasks.push_back(LoadTask("rip.jpg", &tRip));
	load_tasks.push_back(LoadTask("dialog_up.png", &tDialogUp));
	load_tasks.push_back(LoadTask("dialog_down.png", &tDialogDown));
	load_tasks.push_back(LoadTask("mini_unit.png", &tMiniunit));
	load_tasks.push_back(LoadTask("mini_unit2.png", &tMiniunit2));
	load_tasks.push_back(LoadTask("schody_dol.png", &tSchodyDol));
	load_tasks.push_back(LoadTask("schody_gora.png", &tSchodyGora));
	load_tasks.push_back(LoadTask("czerwono.png", &tObwodkaBolu));
	load_tasks.push_back(LoadTask("dark_portal.png", &tPortal));
	load_tasks.push_back(LoadTask("mini_unit3.png", &tMiniunit3));
	load_tasks.push_back(LoadTask("mini_unit4.png", &tMiniunit4));
	load_tasks.push_back(LoadTask("mini_unit5.png", &tMiniunit5));
	load_tasks.push_back(LoadTask("mini_bag.png", &tMinibag));
	load_tasks.push_back(LoadTask("mini_bag2.png", &tMinibag2));
	load_tasks.push_back(LoadTask("mini_portal.png", &tMiniportal));
	for(ClassInfo& ci : g_classes)
		load_tasks.push_back(LoadTask(ci.icon_file, &ci.icon));
	// TERRAIN
	load_tasks.push_back(LoadTask("trawa.jpg", &tTrawa));
	load_tasks.push_back(LoadTask("droga.jpg", &tDroga));
	load_tasks.push_back(LoadTask("ziemia.jpg", &tZiemia));
	load_tasks.push_back(LoadTask("Grass0157_5_S.jpg", &tTrawa2));
	load_tasks.push_back(LoadTask("LeavesDead0045_1_S.jpg", &tTrawa3));
	load_tasks.push_back(LoadTask("pole.jpg", &tPole));
	// KREW
	load_tasks.push_back(LoadTask("krew.png", &tKrew[BLOOD_RED]));
	load_tasks.push_back(LoadTask("krew2.png", &tKrew[BLOOD_GREEN]));
	load_tasks.push_back(LoadTask("krew3.png", &tKrew[BLOOD_BLACK]));
	load_tasks.push_back(LoadTask("iskra.png", &tKrew[BLOOD_BONE]));
	load_tasks.push_back(LoadTask("kamien.png", &tKrew[BLOOD_ROCK]));
	load_tasks.push_back(LoadTask("iskra.png", &tKrew[BLOOD_IRON]));
	load_tasks.push_back(LoadTask("krew_slad.png", &tKrewSlad[BLOOD_RED]));
	load_tasks.push_back(LoadTask("krew_slad2.png", &tKrewSlad[BLOOD_GREEN]));
	load_tasks.push_back(LoadTask("krew_slad3.png", &tKrewSlad[BLOOD_BLACK]));
	tKrewSlad[BLOOD_BONE].res = nullptr;
	tKrewSlad[BLOOD_ROCK].res = nullptr;
	tKrewSlad[BLOOD_IRON].res = nullptr;
	load_tasks.push_back(LoadTask("iskra.png", &tIskra));
	load_tasks.push_back(LoadTask("water.png", &tWoda));
	// PODZIEMIA
	load_tasks.push_back(LoadTask(/*"dir2.png"*/"droga.jpg", &tFloorBase.diffuse));
	tFloorBase.normal = nullptr;
	tFloorBase.specular = nullptr;
	load_tasks.push_back(LoadTask(/*"dir2.png"*/"sciana.jpg", &tWallBase.diffuse));
	load_tasks.push_back(LoadTask("sciana_nrm.jpg", &tWallBase.normal));
	load_tasks.push_back(LoadTask("sciana_spec.jpg", &tWallBase.specular));
	load_tasks.push_back(LoadTask(/*"dir2.png"*/"sufit.jpg", &tCeilBase.diffuse));
	tCeilBase.normal = nullptr;
	tCeilBase.specular = nullptr;
	// CZ�STECZKI
	load_tasks.push_back(LoadTask("flare.png", &tFlare));
	load_tasks.push_back(LoadTask("flare2.png", &tFlare2));
	load_tasks.push_back(LoadTask("lighting_line.png", &tLightingLine));
	// PRZEDMIOTY QUESTOWE
	load_tasks.push_back(LoadTask("list.png", &tList));
	load_tasks.push_back(LoadTask("paczka.png", &tPaczka));
	load_tasks.push_back(LoadTask("wanted.png", &tListGonczy));

	//-----------------------------------------------------------
	//-------------------- MODELE -------------------------------
	// CZ�OWIEK
	load_tasks.push_back(LoadTask("czlowiek.qmsh", &aHumanBase));
	load_tasks.push_back(LoadTask("hair1.qmsh", &aHair[0]));
	load_tasks.push_back(LoadTask("hair2.qmsh", &aHair[1]));
	load_tasks.push_back(LoadTask("hair3.qmsh", &aHair[2]));
	load_tasks.push_back(LoadTask("hair4.qmsh", &aHair[3]));
	load_tasks.push_back(LoadTask("hair5.qmsh", &aHair[4]));
	load_tasks.push_back(LoadTask("eyebrows.qmsh", &aEyebrows));
	load_tasks.push_back(LoadTask("mustache1.qmsh", &aMustache[0]));
	load_tasks.push_back(LoadTask("mustache2.qmsh", &aMustache[1]));
	load_tasks.push_back(LoadTask("beard1.qmsh", &aBeard[0]));
	load_tasks.push_back(LoadTask("beard2.qmsh", &aBeard[1]));
	load_tasks.push_back(LoadTask("beard3.qmsh", &aBeard[2]));
	load_tasks.push_back(LoadTask("beard4.qmsh", &aBeard[3]));
	load_tasks.push_back(LoadTask("beardm1.qmsh", &aBeard[4]));
	// OBIEKTY PROSTE
	load_tasks.push_back(LoadTask("box.qmsh", &aBox));
	load_tasks.push_back(LoadTask("cylinder.qmsh", &aCylinder));
	load_tasks.push_back(LoadTask("sphere.qmsh", &aSphere));
	load_tasks.push_back(LoadTask("capsule.qmsh", &aCapsule));
	// MODELE
	load_tasks.push_back(LoadTask("strzala.qmsh", &aArrow));
	load_tasks.push_back(LoadTask("skybox.qmsh", &aSkybox));
	load_tasks.push_back(LoadTask("worek.qmsh", &aWorek));
	load_tasks.push_back(LoadTask("skrzynia.qmsh", &aSkrzynia));
	load_tasks.push_back(LoadTask("kratka.qmsh", &aKratka));
	load_tasks.push_back(LoadTask("nadrzwi.qmsh", &aNaDrzwi));
	load_tasks.push_back(LoadTask("nadrzwi2.qmsh", &aNaDrzwi2));
	load_tasks.push_back(LoadTask("schody_dol.qmsh", &aSchodyDol));
	load_tasks.push_back(LoadTask("schody_dol2.qmsh", &aSchodyDol2));
	load_tasks.push_back(LoadTask("schody_gora.qmsh", &aSchodyGora));
	load_tasks.push_back(LoadTask("beczka.qmsh", &aBeczka));
	load_tasks.push_back(LoadTask("spellball.qmsh", &aSpellball));
	load_tasks.push_back(LoadTask("drzwi.qmsh", &aDrzwi));
	load_tasks.push_back(LoadTask("drzwi2.qmsh", &aDrzwi2));
	// MODELE BUDYNK�W
	for(int i=0; i<B_MAX; ++i)
	{
		Building& b = buildings[i];
		if(b.mesh_id)
			load_tasks.push_back(LoadTask(b.mesh_id, &b.mesh));
		if(b.inside_mesh_id)
			load_tasks.push_back(LoadTask(b.inside_mesh_id, &b.inside_mesh));
	}

	//-----------------------------------------------------------
	//-------------------- FIZYKA -------------------------------
	load_tasks.push_back(LoadTask("schody_gora_phy.qmsh", &vdSchodyGora));
	load_tasks.push_back(LoadTask("schody_dol_phy.qmsh", &vdSchodyDol));
	load_tasks.push_back(LoadTask("nadrzwi_phy.qmsh", &vdNaDrzwi));

	//-----------------------------------------------------------
	//------ MIESZANE (MODELE, D�WI�KI, TEKSTURY & INNE) --------
	// PU�APKI
	for(uint i=0; i<n_traps; ++i)
	{
		BaseTrap& t = g_traps[i];
		if(t.mesh_id)
			load_tasks.push_back(LoadTask(t.mesh_id, &t));
		if(t.mesh_id2)
			load_tasks.push_back(LoadTask(t.mesh_id2, &t.mesh2));
		if(!nosound)
		{
			if(t.sound_id)
				load_tasks.push_back(LoadTask(t.sound_id, &t.sound));
			if(t.sound_id2)
				load_tasks.push_back(LoadTask(t.sound_id2, &t.sound2));
			if(t.sound_id3)
				load_tasks.push_back(LoadTask(t.sound_id3, &t.sound3));
		}
	}
	// CZARY
	for(uint i=0; i<n_spells; ++i)
	{
		Spell& s = g_spells[i];

		if(!nosound)
		{
			if(!s.sound_cast_id.empty())
				load_tasks.push_back(LoadTask(s.sound_cast_id.c_str(), &s.sound_cast));
			if(!s.sound_hit_id.empty())
				load_tasks.push_back(LoadTask(s.sound_hit_id.c_str(), &s.sound_hit));
		}
		if(!s.tex_id.empty())
			load_tasks.push_back(LoadTask(s.tex_id.c_str(), &s.tex));
		if(!s.tex_particle_id.empty())
			load_tasks.push_back(LoadTask(s.tex_particle_id.c_str(), &s.tex_particle));
		if(!s.tex_explode_id.empty())
			load_tasks.push_back(LoadTask(s.tex_explode_id.c_str(), &s.tex_explode));
		if(!s.mesh_id.empty())
			load_tasks.push_back(LoadTask(s.mesh_id.c_str(), &s.mesh));

		if(s.type == Spell::Ball || s.type == Spell::Point)
			s.shape = new btSphereShape(s.size);
	}
	// OBIEKTY
	for(uint i=0; i<n_objs; ++i)
	{
		Obj& o = g_objs[i];
		if(IS_SET(o.flags2, OBJ2_VARIANT))
			load_tasks.push_back(LoadTask(o.mesh, &o));
		else if(o.mesh)
		{
			if(IS_SET(o.flags, OBJ_SCALEABLE))
			{
				load_tasks.push_back(LoadTask(o.mesh, &o.ani));
				o.matrix = nullptr;
			}
			else
			{
				if(o.type == OBJ_CYLINDER)
				{
					load_tasks.push_back(LoadTask(o.mesh, &o.ani));
					if(!IS_SET(o.flags, OBJ_NO_PHYSICS))
					{
						btCylinderShape* shape = new btCylinderShape(btVector3(o.r, o.h, o.r));
						o.shape = shape;
					}
					o.matrix = nullptr;
				}
				else
					load_tasks.push_back(LoadTask(o.mesh, &o));
			}
		}
		else
		{
			o.ani = nullptr;
			o.matrix = nullptr;
		}
	}
	// POSTACIE
	for(uint i=0; i<n_base_units; ++i)
	{
		// model
		if(!g_base_units[i].mesh.empty())
			load_tasks.push_back(LoadTask(g_base_units[i].mesh.c_str(), &g_base_units[i].ani));

		// d�wi�ki
		SoundPack& sounds = *g_base_units[i].sounds;
		if(!nosound && !sounds.inited)
		{
			sounds.inited = true;
			for(int i=0; i<SOUND_MAX; ++i)
			{
				if(!sounds.filename[i].empty())
					load_tasks.push_back(LoadTask(sounds.filename[i].c_str(), &sounds.sound[i]));
			}
		}

		// tekstury
		if(g_base_units[i].tex)
		{
			for(TexId& ti : *g_base_units[i].tex)
			{
				if(!ti.id.empty())
					load_tasks.push_back(LoadTask(ti.id.c_str(), &ti.res));
			}
		}
	}
	// PRZEDMIOTY
	for(Armor* armor : g_armors)
	{
		Armor& a = *armor;
		if(!a.tex_override.empty())
		{
			for(TexId& ti : a.tex_override)
			{
				if(!ti.id.empty())
					load_tasks.push_back(LoadTask(ti.id.c_str(), &ti.res));
			}
		}
	}
	LoadItemsData();
	// U�YWALNE OBIEKTY
	for(uint i=0; i<n_base_usables; ++i)
	{
		BaseUsable& bu = g_base_usables[i];
		bu.obj = FindObject(bu.obj_name);
		if(!nosound && bu.sound_id)
			load_tasks.push_back(LoadTask(bu.sound_id, &bu.sound));
		if(bu.item_id)
			bu.item = FindItem(bu.item_id);
	}

	//-----------------------------------------------------------
	//-------------------- D�WI�KI ------------------------------
	if(!nosound)
	{
		load_tasks.push_back(LoadTask("gulp.mp3", &sGulp));
		load_tasks.push_back(LoadTask("moneta2.mp3", &sMoneta));
		load_tasks.push_back(LoadTask("bow1.mp3", &sBow[0]));
		load_tasks.push_back(LoadTask("bow2.mp3", &sBow[1]));
		load_tasks.push_back(LoadTask("drzwi-02.mp3", &sDoor[0]));
		load_tasks.push_back(LoadTask("drzwi-03.mp3", &sDoor[1]));
		load_tasks.push_back(LoadTask("drzwi-04.mp3", &sDoor[2]));
		load_tasks.push_back(LoadTask("104528__skyumori__door-close-sqeuak-02.mp3", &sDoorClose));
		load_tasks.push_back(LoadTask("wont_budge.ogg", &sDoorClosed[0]));
		load_tasks.push_back(LoadTask("wont_budge2.ogg", &sDoorClosed[1]));
		load_tasks.push_back(LoadTask("bottle.wav", &sItem[0])); // miksturka
		load_tasks.push_back(LoadTask("armor-light.wav", &sItem[1])); // lekki pancerz
		load_tasks.push_back(LoadTask("chainmail1.wav", &sItem[2])); // ci�ki pancerz
		load_tasks.push_back(LoadTask("metal-ringing.wav", &sItem[3])); // kryszta�
		load_tasks.push_back(LoadTask("wood-small.wav", &sItem[4])); // �uk
		load_tasks.push_back(LoadTask("cloth-heavy.wav", &sItem[5])); // tarcza
		load_tasks.push_back(LoadTask("sword-unsheathe.wav", &sItem[6])); // bro�
		load_tasks.push_back(LoadTask("interface3.wav", &sItem[7]));
		load_tasks.push_back(LoadTask("hello-3.mp3", &sTalk[0]));
		load_tasks.push_back(LoadTask("hello-4.mp3", &sTalk[1]));
		load_tasks.push_back(LoadTask("hmph.wav", &sTalk[2]));
		load_tasks.push_back(LoadTask("huh-2.mp3", &sTalk[3]));
		load_tasks.push_back(LoadTask("chest_open.mp3", &sChestOpen));
		load_tasks.push_back(LoadTask("chest_close.mp3", &sChestClose));
		load_tasks.push_back(LoadTask("door_budge.mp3", &sDoorBudge));
		load_tasks.push_back(LoadTask("atak_kamien.mp3", &sRock));
		load_tasks.push_back(LoadTask("atak_drewno.mp3", &sWood));
		load_tasks.push_back(LoadTask("atak_krysztal.mp3", &sCrystal));
		load_tasks.push_back(LoadTask("atak_metal.mp3", &sMetal));
		load_tasks.push_back(LoadTask("atak_cialo.mp3", &sBody[0]));
		load_tasks.push_back(LoadTask("atak_cialo2.mp3", &sBody[1]));
		load_tasks.push_back(LoadTask("atak_cialo3.mp3", &sBody[2]));
		load_tasks.push_back(LoadTask("atak_cialo4.mp3", &sBody[3]));
		load_tasks.push_back(LoadTask("atak_cialo5.mp3", &sBody[4]));
		load_tasks.push_back(LoadTask("atak_kosci.mp3", &sBone));
		load_tasks.push_back(LoadTask("atak_skora.mp3", &sSkin));
		load_tasks.push_back(LoadTask("arena_fight.mp3", &sArenaFight));
		load_tasks.push_back(LoadTask("arena_wygrana.mp3", &sArenaWygrana));
		load_tasks.push_back(LoadTask("arena_porazka.mp3", &sArenaPrzegrana));
		load_tasks.push_back(LoadTask("unlock.mp3", &sUnlock));
		load_tasks.push_back(LoadTask("TouchofDeath.ogg", &sEvil));
		load_tasks.push_back(LoadTask("shade8.wav", &sXarTalk));
		load_tasks.push_back(LoadTask("ogre1.wav", &sOrcTalk));
		load_tasks.push_back(LoadTask("goblin-7.wav", &sGoblinTalk));
		load_tasks.push_back(LoadTask("golem_alert.mp3", &sGolemTalk));
		load_tasks.push_back(LoadTask("eat.mp3", &sEat));
	}

	//-----------------------------------------------------------
	//--------------------- MUZYKA ------------------------------
	if(!nomusic)
	{
		// skip intro
		for(uint i=1; i<n_musics; ++i)
			load_tasks.push_back(LoadTask(&g_musics[i]));
	}
}

extern Item* gold_item_ptr;

void PostacPredraw(void* ptr, MATRIX* mat, int n)
{
	if(n != 0)
		return;

	Unit* u = (Unit*)ptr;

	if(u->type == Unit::HUMAN)
	{
		int bone = u->ani->ani->GetBone("usta")->id;
		static MATRIX mat2;
		float val = u->talking ? sin(u->talk_timer*6) : 0.f;
		D3DXMatrixRotationX(&mat2, val/5);
		mat[bone] = mat2 * mat[bone];
	}
}

//=================================================================================================
// Inicjalizacja gry
//=================================================================================================
void Game::InitGame()
{
	V( device->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD) );
	V( device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA) );
	V( device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA) );
	V( device->SetRenderState(D3DRS_ALPHAREF, 200) );
	V( device->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL) );

	r_alphablend = false;
	r_alphatest = false;
	r_nocull = false;
	r_nozwrite = false;

	if(!disabled_sound)
	{
		group_default->setVolume(float(sound_volume)/100);
		group_music->setVolume(float(music_volume)/100);
	}

	InitScene();
	InitSuperShader();
	AddCommands();
	InitUnits();
	LoadDatafiles();
	SetItemsMap();
	SetBetterItemMap();
	cursor_pos.x = float(wnd_size.x/2);
	cursor_pos.y = float(wnd_size.y/2);

	LoadLanguageFile("menu.txt");
	LoadLanguageFile("stats.txt");
	LoadLanguageFile("dialogs.txt");
	LoadLanguageFiles();

	AnimeshInstance::Predraw = PostacPredraw;

	LoadGameCommonText();
	LoadItemStatsText();
	LoadLocationNames();
	LoadNames();
	InitGameText();
	LoadData();
	LoadSaveSlots();
	LoadStatsText();
	InitGui();
	ResetGameKeys();
	LoadGameKeys();
	SaveCfg();

	//ExportDialogs();

	LoadGuiData();
	DoLoading();

	PostInitGui();

	if(music_type != MUSIC_INTRO)
		SetMusic(MUSIC_TITLE);
	SetMeshSpecular();

	clear_color = BLACK;
	game_state = GS_MAIN_MENU;
	load_screen->visible = false;

	create_character->Init();
	terrain = new Terrain;
	TerrainOptions terrain_options;
	terrain_options.n_parts = 8;
	terrain_options.tex_size = 256;
	terrain_options.tile_size = 2.f;
	terrain_options.tiles_per_part = 16;
	terrain->Init(device, terrain_options);

	TEX tex[5] = {tTrawa, tTrawa2, tTrawa3, tZiemia, tDroga};
	terrain->SetTextures(tex);
	terrain->Build();
	terrain->RemoveHeightMap(true);

	gold_item.id = "gold";
	gold_item.name = Str("gold");
	gold_item.tex = LoadTex("goldstack.png");
	gold_item.value = 1;
	gold_item.weight = 0;
	gold_item_ptr = &gold_item;

	tFloor[1] = tFloorBase;
	tCeil[1] = tCeilBase;
	tWall[1] = tWallBase;

	CreateCollisionShapes();
	SetUnitPointers();
	SetRoomPointers();

	for(int i=0; i<SG_MAX; ++i)
	{
		if(g_spawn_groups[i].id_name[0] == 0)
			g_spawn_groups[i].id = -1;
		else
			g_spawn_groups[i].id = FindEnemyGroupId(g_spawn_groups[i].id_name);
	}

	for(ClassInfo& ci : g_classes)
		ci.unit_data = FindUnitData(ci.unit_data_id, false);

	// test & validate game data (in debug always check some things)
	if(testing)
	{
		TestGameData(true);
		ValidateGameData(true);
	}
#ifdef _DEBUG
	else
	{
		TestGameData(false);
		ValidateGameData(false);
	}
#endif

	// save config
	cfg.Add("adapter", Format("%d", used_adapter));
	cfg.Add("resolution", Format("%dx%d", wnd_size.x, wnd_size.y));
	cfg.Add("refresh", Format("%d", wnd_hz));
	SaveCfg();

	main_menu->visible = true;

	switch(quickstart)
	{
	case QUICKSTART_NONE:
		break;
	case QUICKSTART_SINGLE:
		StartQuickGame();
		break;
	case QUICKSTART_HOST:
		if(!player_name.empty())
		{
			if(!server_name.empty())
			{
				try
				{
					InitServer();
				}
				catch(cstring err)
				{
					GUI.SimpleDialog(err, nullptr);
					break;
				}

				server_panel->Show();
				Net_OnNewGameServer();
				UpdateServerInfo();

				if(change_title_a)
					ChangeTitle();
			}
			else
				WARN("Quickstart: Can't create server, no server name.");
		}
		else
			WARN("Quickstart: Can't create server, no player nick.");
		break;
	case QUICKSTART_JOIN_LAN:
		if(!player_name.empty())
		{
			pick_autojoin = true;
			pick_server_panel->Show();
		}
		else
			WARN("Quickstart: Can't join server, no player nick.");
		break;
	case QUICKSTART_JOIN_IP:
		if(!player_name.empty())
		{
			if(!server_ip.empty())
				QuickJoinIp();
			else
				WARN("Quickstart: Can't join server, no server ip.");
		}
		else
			WARN("Quickstart: Can't join server, no player nick.");
		break;
	default:
		assert(0);
		break;
	}
}

void Game::LoadDatafiles()
{
	LoadItems(crc_items);
	LOG(Format("Loaded items: %d (crc %p).", g_items.size(), crc_items));
	/*LoadUnits(crc_units);
	LOG(Format("Loaded units: %d (crc %p).", unit_datas.size(), crc_units));
	TestUnits();
	LoadDialogs(crc_dialogs);
	LoadDialogTexts();
	LOG(Format("Loaded dialogs: %d (crc %p).", dialogs.size(), crc_dialogs));
	LoadSpells(crc_spells);
	LOG(Format("Loaded spells: %d (crc %p).", spells.size(), crc_spells));*/
}

inline cstring GameStateToString(GAME_STATE state)
{
	switch(state)
	{
	case GS_MAIN_MENU:
		return "GS_MAIN_MENU";
	case GS_LEVEL:
		return "GS_LEVEL";
	case GS_WORLDMAP:
		return "GS_WORLDMAP";
	case GS_LOAD:
		return "GS_LOAD";
	default:
		return "Unknown";
	}
}

#ifdef CHECK_OOBOX_COL
bool wybor_k;
#endif

//=================================================================================================
// Aktualizacja gry, g��wnie multiplayer
//=================================================================================================
void Game::OnTick(float dt)
{
	// sprawdzanie pami�ci
	assert(_CrtCheckMemory());

	// limit czasu ramki
	if(dt > LIMIT_DT)
		dt = LIMIT_DT;

	if(profiler_mode == 1)
		g_profiler.Start();
	else if(profiler_mode == 0)
		g_profiler.Clear();

	UpdateMusic();

	if(!IsOnline() || !paused)
	{
		// aktualizacja czasu sp�dzonego w grze
		if(game_state != GS_MAIN_MENU && game_state != GS_LOAD)
		{
			gt_tick += dt;
			if(gt_tick >= 1.f)
			{
				gt_tick -= 1.f;
				++gt_second;
				if(gt_second == 60)
				{
					++gt_minute;
					gt_second = 0;
					if(gt_minute == 60)
					{
						++gt_hour;
						gt_minute = 0;
					}
				}
			}
		}
	}

	allow_input = ALLOW_INPUT;

	// utracono urz�dzenie directx lub okno nie aktywne
	if(IsLostDevice() || !active || !locked_cursor)
	{
		Key.SetFocus(false);
		if(!IsOnline() && !inactive_update)
			return;
	}
	else
		Key.SetFocus(true);

	if(cheats)
	{
		if(Key.PressedRelease(VK_F3))
			debug_info = !debug_info;
	}
	if(Key.PressedRelease(VK_F2))
		debug_info2 = !debug_info2;

	// szybkie wyj�cie z gry (alt+f4)
	if(Key.Focus() && Key.Down(VK_MENU) && Key.Down(VK_F4) && !GUI.HaveTopDialog("dialog_alt_f4"))
		ShowQuitDialog();

	if(koniec_gry)
	{
		console_open = false;
		death_fade += dt;
		if(death_fade >= 1.f && AllowKeyboard() && Key.PressedRelease(VK_ESCAPE))
		{
			ExitToMenu();
			koniec_gry = false;
		}
		allow_input = ALLOW_NONE;
	}

	// globalna obs�uga klawiszy
	if(allow_input == ALLOW_INPUT)
	{
		// konsola
		if(!GUI.HaveTopDialog("dialog_alt_f4") && !GUI.HaveDialog("console") && KeyDownUpAllowed(GK_CONSOLE))
			GUI.ShowDialog(console);

		// uwolnienie myszki
		if(!fullscreen && active && locked_cursor && Key.Shortcut(VK_CONTROL,'U'))
			UnlockCursor();

		// zmiana trybu okna
		if(Key.Shortcut(VK_MENU, VK_RETURN))
			ChangeMode(!fullscreen);

		// screenshot
		if(Key.PressedRelease(VK_SNAPSHOT))
			TakeScreenshot(false, Key.Down(VK_SHIFT));

		// zatrzymywanie/wznawianie gry
		if(KeyPressedReleaseAllowed(GK_PAUSE))
		{
			if(!IsOnline())
				paused = !paused;
			else if(IsServer())
			{
				paused = !paused;
				AddMultiMsg(paused ? txGamePaused : txGameResumed);
				NetChange& c = Add1(net_changes);
				c.type = NetChange::PAUSED;
				c.id = (paused ? 1 : 0);
				if(paused && game_state == GS_WORLDMAP && world_state == WS_TRAVEL)
					PushNetChange(NetChange::UPDATE_MAP_POS);
			}
		}
	}

	// obs�uga paneli
	if(GUI.HaveDialog() || (game_gui->mp_box->visible && game_gui->mp_box->itb.focus))
		allow_input = ALLOW_NONE;
	else if(AllowKeyboard() && game_state == GS_LEVEL && death_screen == 0 && !dialog_context.dialog_mode)
	{
		OpenPanel open = game_gui->GetOpenPanel(),
			to_open = OpenPanel::None;		
		
		if(GKey.PressedRelease(GK_STATS))
			to_open = OpenPanel::Stats;
		else if(GKey.PressedRelease(GK_INVENTORY))
			to_open = OpenPanel::Inventory;
		else if(GKey.PressedRelease(GK_TEAM_PANEL))
			to_open = OpenPanel::Team;
		else if(GKey.PressedRelease(GK_JOURNAL))
			to_open = OpenPanel::Journal;
		else if(GKey.PressedRelease(GK_MINIMAP))
			to_open = OpenPanel::Minimap;
		else if(open == OpenPanel::Trade && Key.PressedRelease(VK_ESCAPE))
			to_open = OpenPanel::Trade;

		if(to_open != OpenPanel::None)
			game_gui->ShowPanel(to_open, open);

		switch(open)
		{
		case OpenPanel::None:
		case OpenPanel::Minimap:
		default:
			if(game_gui->use_cursor)
				allow_input = ALLOW_KEYBOARD;
			break;
		case OpenPanel::Stats:
		case OpenPanel::Inventory:
		case OpenPanel::Team:
		case OpenPanel::Trade:
			allow_input = ALLOW_KEYBOARD;
			break;
		case OpenPanel::Journal:
			allow_input = ALLOW_NONE;
			break;
		}
	}

	// szybkie zapisywanie
	if(KeyPressedReleaseAllowed(GK_QUICKSAVE))
		Quicksave(false);

	// szybkie wczytywanie
	if(KeyPressedReleaseAllowed(GK_QUICKLOAD))
		Quickload(false);

	// mp box
	if((game_state == GS_LEVEL || game_state == GS_WORLDMAP) && KeyPressedReleaseAllowed(GK_TALK_BOX))
		game_gui->mp_box->visible = !game_gui->mp_box->visible;

	// aktualizuj gui
	UpdateGui(dt);
	if(exit_mode)
		return;

	// otw�rz menu
	if(AllowKeyboard() && CanShowMenu() && Key.PressedRelease(VK_ESCAPE))
		ShowMenu();

	if(game_menu->visible)
		game_menu->Set(CanSaveGame(), CanLoadGame(), hardcore_mode);

	if(!paused)
	{
		// pytanie o pvp
		if(game_state == GS_LEVEL && IsOnline())
		{
			if(pvp_response.ok)
			{
				pvp_response.timer += dt;
				if(pvp_response.timer >= 5.f)
				{
					pvp_response.ok = false;
					if(pvp_response.to == pc->unit)
					{
						dialog_pvp->CloseDialog();
						RemoveElement(GUI.created_dialogs, dialog_pvp);
						delete dialog_pvp;
						dialog_pvp = nullptr;
					}
					if(IsServer())
					{
						if(pvp_response.from == pc->unit)
							AddMsg(Format(txPvpRefuse, pvp_response.to->player->name.c_str()));
						else
						{
							NetChangePlayer& c = Add1(net_changes_player);
							c.type = NetChangePlayer::NO_PVP;
							c.pc = pvp_response.from->player;
							c.id = pvp_response.to->player->id;
							GetPlayerInfo(c.pc).NeedUpdate();
						}
					}
				}
			}
		}
		else
			pvp_response.ok = false;
	}

	// aktualizacja gry
	if(!koniec_gry)
	{
		if(game_state == GS_LEVEL)
		{
			if(paused)
			{
				if(IsOnline())
					UpdateGameNet(dt);
			}
			else if(GUI.HavePauseDialog())
			{
				if(IsOnline())
					UpdateGame(dt);
			}
			else
				UpdateGame(dt);
		}
		else if(game_state == GS_WORLDMAP)
		{
			if(IsOnline())
				UpdateGameNet(dt);
		}

		if(!IsOnline() && game_state != GS_MAIN_MENU)
		{
			assert(net_changes.empty());
			assert(net_changes_player.empty());
		}
	}
	else if(IsOnline())
		UpdateGameNet(dt);

	// aktywacja mp_box
	if(AllowKeyboard() && game_state == GS_LEVEL && game_gui->mp_box->visible && !game_gui->mp_box->itb.focus && Key.PressedRelease(VK_RETURN))
	{
		game_gui->mp_box->itb.focus = true;
		game_gui->mp_box->Event(GuiEvent_GainFocus);
		game_gui->mp_box->itb.Event(GuiEvent_GainFocus);
	}

	g_profiler.End();
}

void Game::GetTitle(LocalString& s)
{
	s = "CaRpg " VERSION_STR;
	bool none = true;

#ifdef IS_DEBUG
	none = false;
	s += " -  DEBUG";
#endif

	if(change_title_a && ((game_state != GS_MAIN_MENU && game_state != GS_LOAD) || (server_panel && server_panel->visible)))
	{
		if(sv_online)
		{
			if(sv_server)
			{
				if(none)
					s += " - SERVER";
				else
					s += ", SERVER";
			}
			else
			{
				if(none)
					s += " - CLIENT";
				else
					s += ", CLIENT";
			}
		}
		else
		{
			if(none)
				s += " - SINGLE";
			else
				s += ", SINGLE";
		}
	}

	s += Format(" [%d]", GetCurrentProcessId());
}

void Game::ChangeTitle()
{
	LocalString s;
	GetTitle(s);
	SetConsoleTitle(s->c_str());
	SetTitle(s->c_str());
}

bool Game::Start0(bool _fullscreen, int w, int h)
{
	LocalString s;
	GetTitle(s);
	return Start(s->c_str(), _fullscreen, w, h);
}

struct Point
{
	INT2 pt, prev;
};

struct AStarSort
{
	AStarSort(vector<APoint>& a_map, int rozmiar) : a_map(a_map), rozmiar(rozmiar)
	{
	}

	bool operator() (const Point& pt1, const Point& pt2) const
	{
		return a_map[pt1.pt.x+pt1.pt.y*rozmiar].suma > a_map[pt2.pt.x+pt2.pt.y*rozmiar].suma;
	}

	vector<APoint>& a_map;
	int rozmiar;
};

void Game::OnReload()
{
	GUI.OnReload();

	V( eMesh->OnResetDevice() );
	V( eParticle->OnResetDevice() );
	V( eTerrain->OnResetDevice() );
	V( eSkybox->OnResetDevice() );
	V( eArea->OnResetDevice() );
	V( eGui->OnResetDevice() );
	V( ePostFx->OnResetDevice() );
	V( eGlow->OnResetDevice() );
	V( eGrass->OnResetDevice() );

	for(vector<SuperShader>::iterator it = sshaders.begin(), end = sshaders.end(); it != end; ++it)
		V( it->e->OnResetDevice() );

	V( device->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD) );
	V( device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA) );
	V( device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA) );
	V( device->SetRenderState(D3DRS_ALPHAREF, 200) );
	V( device->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL) );

	CreateTextures();
	BuildDungeon();
	RebuildMinimap();

	r_alphatest = false;
	r_alphablend = false;
	r_nocull = false;
	r_nozwrite = false;
}

void Game::OnReset()
{
	GUI.OnReset();

	V( eMesh->OnLostDevice() );
	V( eParticle->OnLostDevice() );
	V( eTerrain->OnLostDevice() );
	V( eSkybox->OnLostDevice() );
	V( eArea->OnLostDevice() );
	V( eGui->OnLostDevice() );
	V( ePostFx->OnLostDevice() );
	V( eGlow->OnLostDevice() );
	V( eGrass->OnLostDevice() );

	for(vector<SuperShader>::iterator it = sshaders.begin(), end = sshaders.end(); it != end; ++it)
		V( it->e->OnLostDevice() );

	SafeRelease(tItemRegion);
	SafeRelease(tMinimap);
	SafeRelease(tChar);
	SafeRelease(tSave);
	SafeRelease(sItemRegion);
	SafeRelease(sChar);
	SafeRelease(sSave);
	for(int i=0; i<3; ++i)
	{
		SafeRelease(sPostEffect[i]);
		SafeRelease(tPostEffect[i]);
	}
	SafeRelease(vbFullscreen);
	SafeRelease(vbParticle);
	SafeRelease(vbDungeon);
	SafeRelease(ibDungeon);
	SafeRelease(vbInstancing);
	vb_instancing_max = 0;
}

void Game::OnChar(char c)
{
	if((c != 0x08 && c != 0x0D && byte(c) < 0x20) || c == '`')
		return;

	GUI.OnChar(c);
}

void Game::TakeScreenshot(bool text, bool no_gui)
{
	if(no_gui)
	{
		int old_flags = draw_flags;
		draw_flags = (0xFFFF & ~DF_GUI);
		Render(true);
		draw_flags = old_flags;
	}
	else
		Render(true);

	SURFACE back_buffer;
	HRESULT hr = device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &back_buffer);
	if(FAILED(hr))
	{
		if(text)
			AddConsoleMsg(txSsFailed);
		ERROR(Format("Failed to get front buffer data to save screenshot (%d)!", hr));
	}
	else
	{
		CreateDirectory("screenshots", nullptr);

		time_t t = ::time(0);
		tm lt;
		localtime_s(&lt, &t);

		if(t == last_screenshot)
			++screenshot_count;
		else
		{
			last_screenshot = t;
			screenshot_count = 1;
		}

		cstring ext;
		switch(screenshot_format)
		{
		case D3DXIFF_BMP:
			ext = "bmp";
			break;
		default:
		case D3DXIFF_JPG:
			ext = "jpg";
			break;
		case D3DXIFF_TGA:
			ext = "tga";
			break;
		case D3DXIFF_PNG:
			ext = "png";
			break;
		}

		cstring path = Format("screenshots\\%04d%02d%02d_%02d%02d%02d_%02d.%s", lt.tm_year+1900, lt.tm_mon+1,
			lt.tm_mday, lt.tm_hour, lt.tm_min, lt.tm_sec, screenshot_count, ext);

		D3DXSaveSurfaceToFileA(path, screenshot_format, back_buffer, nullptr, nullptr);

		if(text)
			AddConsoleMsg(Format(txSsDone, path));
		LOG(Format("Screenshot saved to '%s'.", path));

		back_buffer->Release();
	}
}

void Game::ExitToMenu()
{
	if(sv_online)
		CloseConnection(VoidF(this, &Game::DoExitToMenu));
	else
		DoExitToMenu();
}

void Game::DoExitToMenu()
{
	exit_mode = true;

	StopSounds();
	attached_sounds.clear();
	ClearGame();
	game_state = GS_MAIN_MENU;

	exit_mode = false;
	paused = false;
	mp_load = false;
	was_client = false;

	SetMusic(MUSIC_TITLE);
	contest_state = CONTEST_NOT_DONE;
	koniec_gry = false;
	exit_to_menu = true;

	CloseAllPanels();
	GUI.CloseDialogs();
	game_menu->visible = false;
	game_gui->visible = false;
	world_map->visible = false;
	main_menu->visible = true;

	if(change_title_a)
		ChangeTitle();
}

// szuka �cie�ki u�ywaj�c algorytmu A-Star
// zwraca true je�li znaleziono i w wektorze jest ta �cie�ka, w �cie�ce nie ma pocz�tkowego kafelka
bool Game::FindPath(LevelContext& ctx, const INT2& _start_tile, const INT2& _target_tile, vector<INT2>& _path, bool can_open_doors, bool wedrowanie, vector<INT2>* blocked)
{
	if(_start_tile == _target_tile || ctx.type == LevelContext::Building)
	{
		// jest w tym samym miejscu
		_path.clear();
		_path.push_back(_target_tile);
		return true;
	}

	static vector<Point> do_sprawdzenia;
	do_sprawdzenia.clear();

	if(ctx.type == LevelContext::Outside)
	{
		// zewn�trze
		OutsideLocation* outside = (OutsideLocation*)location;
		const TerrainTile* m = outside->tiles;
		const int w = OutsideLocation::size;

		// czy poza map�
		if(!outside->IsInside(_start_tile) || !outside->IsInside(_target_tile))
			return false;

		// czy na blokuj�cym miejscu
		if(m[_start_tile(w)].IsBlocking() || m[_target_tile(w)].IsBlocking())
			return false;

		// powi�ksz map�
		const uint size = uint(w*w);
		if(size > a_map.size())
			a_map.resize(size);

		// wyzeruj
		memset(&a_map[0], 0, sizeof(APoint)*size);
		_path.clear();

		// apply blocked tiles
		if(blocked)
		{
			for(vector<INT2>::iterator it = blocked->begin(), end = blocked->end(); it != end; ++it)
				a_map[(*it)(w)].stan = 1;
			if(a_map[_start_tile(w)].stan == 1 || a_map[_target_tile(w)].stan == 1)
				return false;
		}

		// dodaj pierwszy kafelek do sprawdzenia
		APoint apt, prev_apt;
		apt.suma = apt.odleglosc = 10 * (abs(_target_tile.x - _start_tile.x) + abs(_target_tile.y - _start_tile.y));
		apt.stan = 1;
		apt.koszt = 0;
		a_map[_start_tile(w)] = apt;

		Point pt, new_pt;
		pt.pt = pt.prev = _start_tile;
		do_sprawdzenia.push_back(pt);

		AStarSort sortownik(a_map, w);

		// szukaj drogi
		while(!do_sprawdzenia.empty())
		{
			pt = do_sprawdzenia.back();
			do_sprawdzenia.pop_back();

			apt = a_map[pt.pt(w)];
			prev_apt = a_map[pt.prev(w)];

			if(pt.pt == _target_tile)
			{
				APoint& ept = a_map[pt.pt(w)];
				ept.stan = 1;
				ept.prev = pt.prev;
				break;
			}

			const INT2 kierunek[4] = {
				INT2(0,1),
				INT2(1,0),
				INT2(0,-1),
				INT2(-1,0)
			};

			const INT2 kierunek2[4] = {
				INT2(1,1),
				INT2(1,-1),
				INT2(-1,1),
				INT2(-1,-1)
			};

			for(int i=0; i<4; ++i)
			{
				const INT2& pt1 = kierunek[i] + pt.pt;
				const INT2& pt2 = kierunek2[i] + pt.pt;

				if(pt1.x>=0 && pt1.y>=0 && pt1.x<w-1 && pt1.y<w-1 && a_map[pt1(w)].stan == 0 && !m[pt1(w)].IsBlocking())
				{
					apt.prev = pt.pt;
					apt.koszt = prev_apt.koszt + 10;
					if(wedrowanie)
					{
						TERRAIN_TILE type = m[pt1(w)].t;
						if(type == TT_SAND)
							apt.koszt += 10;
						else if(type != TT_ROAD)
							apt.koszt += 30;
					}
					apt.odleglosc = (abs(pt1.x - _target_tile.x)+abs(pt1.y-_target_tile.y))*10;
					apt.suma = apt.odleglosc + apt.koszt;
					apt.stan = 1;
					a_map[pt1(w)] = apt;

					new_pt.pt = pt1;
					new_pt.prev = pt.pt;
					do_sprawdzenia.push_back(new_pt);
				}

				if(pt2.x>=0 && pt2.y>=0 && pt2.x<w-1 && pt2.y<w-1 && a_map[pt2(w)].stan == 0 &&
					!m[pt2(w)].IsBlocking() &&
					!m[kierunek2[i].x+pt.pt.x+pt.pt.y*w].IsBlocking() &&
					!m[pt.pt.x+(kierunek2[i].y+pt.pt.y)*w].IsBlocking())
				{
					apt.prev = pt.pt;
					apt.koszt = prev_apt.koszt + 15;
					if(wedrowanie)
					{
						TERRAIN_TILE type = m[pt2(w)].t;
						if(type == TT_SAND)
							apt.koszt += 10;
						else if(type != TT_ROAD)
							apt.koszt += 30;
					}
					apt.odleglosc = (abs(pt2.x - _target_tile.x)+abs(pt2.y-_target_tile.y))*10;
					apt.suma = apt.odleglosc + apt.koszt;
					apt.stan = 1;
					a_map[pt2(w)] = apt;

					new_pt.pt = pt2;
					new_pt.prev = pt.pt;
					do_sprawdzenia.push_back(new_pt);
				}
			}

			std::sort(do_sprawdzenia.begin(), do_sprawdzenia.end(), sortownik);
		}

		// je�li cel jest niezbadany to nie znaleziono �cie�ki
		if(a_map[_target_tile(w)].stan == 0)
			return false;

		// wype�nij elementami �cie�k�
		_path.push_back(_target_tile);

		INT2 p;

		while((p = _path.back()) != _start_tile)
			_path.push_back(a_map[p(w)].prev);
	}
	else
	{
		// wn�trze
		InsideLocation* inside = (InsideLocation*)location;
		InsideLocationLevel& lvl = inside->GetLevelData();
		const Pole* m = lvl.map;
		const int w = lvl.w, h = lvl.h;

		// czy poza map�
		if(!lvl.IsInside(_start_tile) || !lvl.IsInside(_target_tile))
			return false;

		// czy na blokuj�cym miejscu
		if(czy_blokuje2(m[_start_tile(w)]) || czy_blokuje2(m[_target_tile(w)]))
			return false;

		// powi�ksz map�
		const uint size = uint(w*h);
		if(size > a_map.size())
			a_map.resize(size);

		// wyzeruj
		memset(&a_map[0], 0, sizeof(APoint)*size);
		_path.clear();

		// apply blocked tiles
		if(blocked)
		{
			for(vector<INT2>::iterator it = blocked->begin(), end = blocked->end(); it != end; ++it)
				a_map[(*it)(w)].stan = 1;
			if(a_map[_start_tile(w)].stan == 1 || a_map[_target_tile(w)].stan == 1)
				return false;
		}

		// dodaj pierwszy kafelek do sprawdzenia
		APoint apt, prev_apt;
		apt.suma = apt.odleglosc = 10 * (abs(_target_tile.x - _start_tile.x) + abs(_target_tile.y - _start_tile.y));
		apt.stan = 1;
		apt.koszt = 0;
		a_map[_start_tile(w)] = apt;

		Point pt, new_pt;
		pt.pt = pt.prev = _start_tile;
		do_sprawdzenia.push_back(pt);

		AStarSort sortownik(a_map, w);

		// szukaj drogi
		while(!do_sprawdzenia.empty())
		{
			pt = do_sprawdzenia.back();
			do_sprawdzenia.pop_back();

			prev_apt = a_map[pt.pt(w)];

			//LOG(Format("(%d,%d)->(%d,%d), K:%d D:%d S:%d", pt.prev.x, pt.prev.y, pt.pt.x, pt.pt.y, prev_apt.koszt, prev_apt.odleglosc, prev_apt.suma));

			if(pt.pt == _target_tile)
			{
				APoint& ept = a_map[pt.pt(w)];
				ept.stan = 1;
				ept.prev = pt.prev;
				break;
			}

			const INT2 kierunek[4] = {
				INT2(0,1),
				INT2(1,0),
				INT2(0,-1),
				INT2(-1,0)
			};

			const INT2 kierunek2[4] = {
				INT2(1,1),
				INT2(1,-1),
				INT2(-1,1),
				INT2(-1,-1)
			};

			if(can_open_doors)
			{
				for(int i=0; i<4; ++i)
				{
					const INT2& pt1 = kierunek[i] + pt.pt;
					const INT2& pt2 = kierunek2[i] + pt.pt;

					if(pt1.x>=0 && pt1.y>=0 && pt1.x<w-1 && pt1.y<h-1 && !czy_blokuje2(m[pt1(w)]))
					{
						apt.prev = pt.pt;
						apt.koszt = prev_apt.koszt + 10;
						apt.odleglosc = (abs(pt1.x - _target_tile.x)+abs(pt1.y - _target_tile.y))*10;
						apt.suma = apt.odleglosc + apt.koszt;

						if(a_map[pt1(w)].IsLower(apt.suma))
						{
							apt.stan = 1;
							a_map[pt1(w)] = apt;

							new_pt.pt = pt1;
							new_pt.prev = pt.pt;
							do_sprawdzenia.push_back(new_pt);
						}
					}

					if(pt2.x>=0 && pt2.y>=0 && pt2.x<w-1 && pt2.y<h-1 &&
						!czy_blokuje2(m[pt2(w)]) &&
						!czy_blokuje2(m[kierunek2[i].x+pt.pt.x+pt.pt.y*w]) &&
						!czy_blokuje2(m[pt.pt.x+(kierunek2[i].y+pt.pt.y)*w]))
					{
						apt.prev = pt.pt;
						apt.koszt = prev_apt.koszt + 15;
						apt.odleglosc = (abs(pt2.x - _target_tile.x)+abs(pt2.y - _target_tile.y))*10;
						apt.suma = apt.odleglosc + apt.koszt;

						if(a_map[pt2(w)].IsLower(apt.suma))
						{
							apt.stan = 1;
							a_map[pt2(w)] = apt;

							new_pt.pt = pt2;
							new_pt.prev = pt.pt;
							do_sprawdzenia.push_back(new_pt);
						}
					}
				}
			}
			else
			{
				for(int i=0; i<4; ++i)
				{
					const INT2& pt1 = kierunek[i] + pt.pt;
					const INT2& pt2 = kierunek2[i] + pt.pt;

					if(pt1.x>=0 && pt1.y>=0 && pt1.x<w-1 && pt1.y<h-1 && !czy_blokuje2(m[pt1(w)]))
					{
						int ok = 2; // 2-ok i dodaj, 1-ok, 0-nie

						if(m[pt1(w)].type == DRZWI)
						{
							Door* door = FindDoor(ctx, pt1);
							if(door && door->IsBlocking())
							{
								// ustal gdzie s� drzwi na polu i czy z tej strony mo�na na nie wej��
								if(czy_blokuje2(lvl.map[pt1.x - 1 + pt1.y*lvl.w].type))
								{
									// #   #
									// #---#
									// #   #
									int mov = 0;
									if(lvl.rooms[lvl.map[pt1.x + (pt1.y - 1)*lvl.w].room].corridor)
										++mov;
									if(lvl.rooms[lvl.map[pt1.x + (pt1.y + 1)*lvl.w].room].corridor)
										--mov;
									if(mov == 1)
									{
										// #---#
										// #   #
										// #   #
										if(i != 0)
											ok = 0;
									}
									else if(mov == -1)
									{
										// #   #
										// #   #
										// #---#
										if(i != 2)
											ok = 0;
									}
									else
										ok = 1;
								}
								else
								{
									// ###
									//  | 
									//  | 
									// ###
									int mov = 0;
									if(lvl.rooms[lvl.map[pt1.x - 1 + pt1.y*lvl.w].room].corridor)
										++mov;
									if(lvl.rooms[lvl.map[pt1.x + 1 + pt1.y*lvl.w].room].corridor)
										--mov;
									if(mov == 1)
									{
										// ###
										//   |
										//   |
										// ###
										if(i != 1)
											ok = 0;
									}
									else if(mov == -1)
									{
										// ###
										// |
										// |
										// ###
										if(i != 3)
											ok = 0;
									}
									else
										ok = 1;
								}
							}
						}

						if(ok)
						{
							apt.prev = pt.pt;
							apt.koszt = prev_apt.koszt + 10;
							apt.odleglosc = (abs(pt1.x - _target_tile.x)+abs(pt1.y - _target_tile.y))*10;
							apt.suma = apt.odleglosc + apt.koszt;

							if(a_map[pt1(w)].IsLower(apt.suma))
							{
								apt.stan = 1;
								a_map[pt1(w)] = apt;

								if(ok == 2)
								{
									new_pt.pt = pt1;
									new_pt.prev = pt.pt;
									do_sprawdzenia.push_back(new_pt);
								}
							}
						}
					}

					if(pt2.x>=0 && pt2.y>=0 && pt2.x<w-1 && pt2.y<h-1 &&
						!czy_blokuje2(m[pt2(w)]) &&
						!czy_blokuje2(m[kierunek2[i].x+pt.pt.x+pt.pt.y*w]) &&
						!czy_blokuje2(m[pt.pt.x+(kierunek2[i].y+pt.pt.y)*w]))
					{
						bool ok = true;

						if(m[pt2(w)].type == DRZWI)
						{
							Door* door = FindDoor(ctx, pt2);
							if(door && door->IsBlocking())
								ok = false;
						}

						if(ok && m[kierunek2[i].x + pt.pt.x + pt.pt.y*w].type == DRZWI)
						{
							Door* door = FindDoor(ctx, INT2(kierunek2[i].x+pt.pt.x, pt.pt.y));
							if(door && door->IsBlocking())
								ok = false;
						}

						if(ok && m[pt.pt.x + (kierunek2[i].y + pt.pt.y)*w].type == DRZWI)
						{
							Door* door = FindDoor(ctx, INT2(pt.pt.x, kierunek2[i].y+pt.pt.y));
							if(door && door->IsBlocking())
								ok = false;
						}

						if(ok)
						{
							apt.prev = pt.pt;
							apt.koszt = prev_apt.koszt + 15;
							apt.odleglosc = (abs(pt2.x - _target_tile.x)+abs(pt2.y - _target_tile.y))*10;
							apt.suma = apt.odleglosc + apt.koszt;

							if(a_map[pt2(w)].IsLower(apt.suma))
							{
								apt.stan = 1;
								a_map[pt2(w)] = apt;

								new_pt.pt = pt2;
								new_pt.prev = pt.pt;
								do_sprawdzenia.push_back(new_pt);
							}
						}
					}
				}
			}

			std::sort(do_sprawdzenia.begin(), do_sprawdzenia.end(), sortownik);
		}

		// je�li cel jest niezbadany to nie znaleziono �cie�ki
		if(a_map[_target_tile(w)].stan == 0)
			return false;

		// wype�nij elementami �cie�k�
		_path.push_back(_target_tile);

		INT2 p;

		while((p = _path.back()) != _start_tile)
			_path.push_back(a_map[p(w)].prev);
	}

	// usu� ostatni element �cie�ki
	_path.pop_back();

	return true;
}

INT2 Game::RandomNearTile(const INT2& _tile)
{
	struct DoSprawdzenia
	{
		int ile;
		INT2 tile[3];
	};
	static vector<INT2> tiles;

	const DoSprawdzenia allowed[] = {
		{2, INT2(-2,0), INT2(-1,0), INT2(0,0)},
		{1, INT2(-1,0), INT2(0,0), INT2(0,0)},
		{3, INT2(-1,1), INT2(-1,0), INT2(0,1)},
		{3, INT2(-1,-1), INT2(-1,0), INT2(0,-1)},
		{2, INT2(2,0), INT2(1,0), INT2(0,0)},
		{1, INT2(1,0), INT2(0,0), INT2(0,0)},
		{3, INT2(1,1), INT2(1,0), INT2(0,1)},
		{3, INT2(1,-1), INT2(1,0), INT2(0,-1)},
		{2, INT2(0,2), INT2(0,1), INT2(0,0)},
		{1, INT2(0,1), INT2(0,0), INT2(0,0)},
		{2, INT2(0,-2), INT2(0,0), INT2(0,0)},
		{1, INT2(0,-1),  INT2(0,0), INT2(0,0)}
	};

	bool blokuje = true;

	if(location->outside)
	{
		OutsideLocation* outside = (OutsideLocation*)location;
		const TerrainTile* m = outside->tiles;
		const int w = OutsideLocation::size;

		for(uint i=0; i<12; ++i)
		{
			const int x = _tile.x + allowed[i].tile[0].x,
				y = _tile.y + allowed[i].tile[0].y;
			if(!outside->IsInside(x, y))
				continue;
			if(m[x+y*w].IsBlocking())
				continue;
			blokuje = false;
			for(int j=1; j<allowed[i].ile; ++j)
			{
				if(m[_tile.x+allowed[i].tile[j].x+(_tile.y+allowed[i].tile[j].y)*w].IsBlocking())
				{
					blokuje = true;
					break;
				}
			}
			if(!blokuje)
				tiles.push_back(allowed[i].tile[0]);
		}
	}
	else
	{
		InsideLocation* inside = (InsideLocation*)location;
		InsideLocationLevel& lvl = inside->GetLevelData();
		const Pole* m = lvl.map;
		const int w = lvl.w;

		for(uint i=0; i<12; ++i)
		{
			const int x = _tile.x + allowed[i].tile[0].x,
				      y = _tile.y + allowed[i].tile[0].y;
			if(!lvl.IsInside(x, y))
				continue;
			if(czy_blokuje2(m[x+y*w]))
				continue;
			blokuje = false;
			for(int j=1; j<allowed[i].ile; ++j)
			{
				if(czy_blokuje2(m[_tile.x+allowed[i].tile[j].x+(_tile.y+allowed[i].tile[j].y)*w]))
				{
					blokuje = true;
					break;
				}
			}
			if(!blokuje)
				tiles.push_back(allowed[i].tile[0]);
		}
	}

	if(tiles.empty())
		return _tile;
	else
		return tiles[rand2()%tiles.size()] + _tile;
}

// 0 - path found
// 1 - start pos and target pos too far
// 2 - start location is blocked
// 3 - start tile and target tile is equal
// 4 - target tile is blocked
// 5 - path not found
int Game::FindLocalPath(LevelContext& ctx, vector<INT2>& _path, const INT2& my_tile, const INT2& target_tile, const Unit* _me, const Unit* _other, const void* useable, bool is_end_point)
{
	assert(_me);

	_path.clear();
#ifdef DRAW_LOCAL_PATH
	if(marked == _me)
		test_pf.clear();
#endif

	if(my_tile == target_tile)
		return 3;

	int dist = distance(my_tile, target_tile);

	if(dist >= 32)
		return 1;

	if(dist <= 0 || my_tile.x < 0 || my_tile.y < 0)
	{
		ERROR(Format("Invalid FindLocalPath, ctx type %d, ctx building %d, my tile %d %d, target tile %d %d, me %s (%p; %g %g %g; %d), useable %p, is end point %d.",
			ctx.type, ctx.building_id, my_tile.x, my_tile.y, target_tile.x, target_tile.y, _me->data->id.c_str(), _me, _me->pos.x, _me->pos.y, _me->pos.z, _me->in_building,
			useable, is_end_point ? 1 : 0));
		if(_other)
		{
			ERROR(Format("Other unit %s (%p; %g, %g, %g, %d).", _other->data->id.c_str(), _other, _other->pos.x, _other->pos.y, _other->pos.z, _other->in_building));
		}
	}
	
	// �rodek
	const INT2 center_tile((my_tile+target_tile)/2);

	int minx = max(ctx.mine.x*8, center_tile.x-15),
		miny = max(ctx.mine.y*8, center_tile.y-15),
		maxx = min(ctx.maxe.x*8-1, center_tile.x+16),
		maxy = min(ctx.maxe.y*8-1, center_tile.y+16);

	int w = maxx-minx,
		h = maxy-miny;

	uint size = (w+1)*(h+1);

	if(a_map.size() < size)
		a_map.resize(size);
	memset(&a_map[0], 0, sizeof(APoint)*size);
	if(local_pfmap.size() < size)
		local_pfmap.resize(size);

	const Unit* ignored_units[3] = {_me, 0};
	if(_other)
		ignored_units[1] = _other;
	IgnoreObjects ignore = {0};
	ignore.ignored_units = (const Unit**)ignored_units;
	const void* ignored_objects[2] = {0};
	if(useable)
	{
		ignored_objects[0] = useable;
		ignore.ignored_objects = ignored_objects;
	}

	global_col.clear();
	GatherCollisionObjects(ctx, global_col, BOX2D(float(minx)/4-0.25f, float(miny)/4-0.25f, float(maxx)/4+0.25f, float(maxy)/4+0.25f), &ignore);

	const float r = _me->GetUnitRadius()-0.25f/2;

	for(int y=miny, y2 = 0; y<maxy; ++y, ++y2)
	{
		for(int x=minx, x2 = 0; x<maxx; ++x, ++x2)
		{
			if(!Collide(global_col, BOX2D(0.25f*x, 0.25f*y, 0.25f*(x+1), 0.25f*(y+1)), r))
			{
#ifdef DRAW_LOCAL_PATH
				if(marked == _me)
					test_pf.push_back(std::pair<VEC2, int>(VEC2(0.25f*x, 0.25f*y), 0));
#endif
				local_pfmap[x2+y2*w] = false;
			}
			else
				local_pfmap[x2+y2*w] = true;
		}
	}

	const INT2 target_rel(target_tile.x - minx, target_tile.y - miny),
		       my_rel(my_tile.x - minx, my_tile.y - miny);

	// target tile is blocked
	if(!is_end_point && local_pfmap[target_rel(w)])
		return 4;

	// oznacz moje pole jako wolne
	local_pfmap[my_rel(w)] = false;
	const INT2 neis[8] = {
		INT2(-1,-1),
		INT2(0,-1),
		INT2(1,-1),
		INT2(-1,0),
		INT2(1,0),
		INT2(-1,1),
		INT2(0,1),
		INT2(1,1)
	};

	bool jest = false;
	for(int i=0; i<8; ++i)
	{
		INT2 pt = my_rel + neis[i];
		if(pt.x < 0 || pt.y < 0 || pt.x > 32 || pt.y > 32)
			continue;
		if(!local_pfmap[pt(w)])
		{
			jest = true;
			break;
		}
	}

	if(!jest)
		return 2;

	// dodaj pierwszy punkt do sprawdzenia
	static vector<Point> do_sprawdzenia;
	do_sprawdzenia.clear();
	{
		Point& pt = Add1(do_sprawdzenia);
		pt.pt = target_rel;
		pt.prev = INT2(0,0);
	}

	APoint apt, prev_apt;
	apt.odleglosc = apt.suma = distance(my_rel, target_rel)*10;
	apt.koszt = 0;
	apt.stan = 1;
	apt.prev = INT2(0,0);
	a_map[target_rel(w)] = apt;

	AStarSort sortownik(a_map, w);
	Point new_pt;

	const int MAX_STEPS = 100;
	int steps = 0;

	// rozpocznij szukanie �cie�ki
	while(!do_sprawdzenia.empty())
	{
		Point pt = do_sprawdzenia.back();
		do_sprawdzenia.pop_back();
		prev_apt = a_map[pt.pt(w)];

		if(pt.pt == my_rel)
		{
			APoint& ept = a_map[pt.pt(w)];
			ept.stan = 1;
			ept.prev = pt.prev;
			break;
		}

		const INT2 kierunek[4] = {
			INT2(0,1),
			INT2(1,0),
			INT2(0,-1),
			INT2(-1,0)
		};

		const INT2 kierunek2[4] = {
			INT2(1,1),
			INT2(1,-1),
			INT2(-1,1),
			INT2(-1,-1)
		};

		for(int i=0; i<4; ++i)
		{
			const INT2& pt1 = kierunek[i] + pt.pt;
			const INT2& pt2 = kierunek2[i] + pt.pt;

			if(pt1.x>=0 && pt1.y>=0 && pt1.x<w-1 && pt1.y<h-1 && !local_pfmap[pt1(w)])
			{
				apt.prev = pt.pt;
				apt.koszt = prev_apt.koszt + 10;
				apt.odleglosc = distance(pt1, my_rel)*10;
				apt.suma = apt.odleglosc + apt.koszt;

				if(a_map[pt1(w)].IsLower(apt.suma))
				{
					apt.stan = 1;
					a_map[pt1(w)] = apt;

					new_pt.pt = pt1;
					new_pt.prev = pt.pt;
					do_sprawdzenia.push_back(new_pt);
				}
			}

			if(pt2.x>=0 && pt2.y>=0 && pt2.x<w-1 && pt2.y<h-1 && !local_pfmap[pt2(w)] /*&&
				!local_pfmap[kierunek2[i].x+pt.pt.x+pt.pt.y*w] && !local_pfmap[pt.pt.x+(kierunek2[i].y+pt.pt.y)*w]*/)
			{
				apt.prev = pt.pt;
				apt.koszt = prev_apt.koszt + 15;
				apt.odleglosc = distance(pt2, my_rel)*10;
				apt.suma = apt.odleglosc + apt.koszt;

				if(a_map[pt2(w)].IsLower(apt.suma))
				{
					apt.stan = 1;
					a_map[pt2(w)] = apt;

					new_pt.pt = pt2;
					new_pt.prev = pt.pt;
					do_sprawdzenia.push_back(new_pt);
				}
			}
		}

		++steps;
		if(steps > MAX_STEPS)
			break;

		std::sort(do_sprawdzenia.begin(), do_sprawdzenia.end(), sortownik);
	}

	// set debug path drawing
#ifdef DRAW_LOCAL_PATH
	if(marked == _me)
		test_pf_outside = (location->outside && _me->in_building == -1);
#endif

	if(a_map[my_rel(w)].stan == 0)
	{
		// path not found
#ifdef DRAW_LOCAL_PATH
		if(marked == _me)
		{
			test_pf.push_back(std::pair<VEC2, int>(VEC2(0.25f*my_tile.x, 0.25f*my_tile.y), 1));
			test_pf.push_back(std::pair<VEC2, int>(VEC2(0.25f*target_tile.x, 0.25f*target_tile.y), 1));
		}
#endif
		return 5;
	}

#ifdef DRAW_LOCAL_PATH
	if(marked == _me)
	{
		test_pf.push_back(std::pair<VEC2, int>(VEC2(0.25f*my_tile.x, 0.25f*my_tile.y), 1));
		test_pf.push_back(std::pair<VEC2, int>(VEC2(0.25f*target_tile.x, 0.25f*target_tile.y), 1));
	}
#endif

	// populate path vector (and debug drawing)
	INT2 p = my_rel;

	do 
	{
#ifdef DRAW_LOCAL_PATH
		if(marked == _me)
			test_pf.push_back(std::pair<VEC2, int>(VEC2(0.25f*(p.x+minx), 0.25f*(p.y+miny)), 1));
#endif
		_path.push_back(INT2(p.x+minx, p.y+miny));
		p = a_map[p(w)].prev;
	}
	while (p != target_rel);

	_path.push_back(target_tile);
	std::reverse(_path.begin(), _path.end());
	_path.pop_back();

	return 0;
}

void Game::DoLoading()
{
	Timer t;
	float dt = 0.f;
	uint index = 0;

	clear_color = BLACK;
	game_state = GS_LOAD;
	load_game_progress = 0.f;
	load_game_text = txLoadingResources;
	DoPseudotick();
	t.Tick();

	for(vector<LoadTask>::iterator load_task = load_tasks.begin(), end = load_tasks.end(); load_task != end; ++load_task, ++index)
	{
		switch(load_task->type)
		{
		case LoadTask::LoadShader:
			*load_task->effect = CompileShader(load_task->filename);
			break;
		case LoadTask::SetupShaders:
			SetupShaders();
			break;
		case LoadTask::LoadTex:
			*load_task->tex = LoadTex(load_task->filename);
			break;
		case LoadTask::LoadTex2:
			*load_task->tex2 = LoadTex2(load_task->filename);
			break;
		case LoadTask::LoadMesh:
			*load_task->mesh = LoadMesh(load_task->filename);
			break;
		case LoadTask::LoadVertexData:
			*load_task->vd = LoadMeshVertexData(load_task->filename);
			break;
		case LoadTask::LoadTrap:
			{
				BaseTrap& t = *load_task->trap;
				t.mesh = LoadMesh(t.mesh_id);
				Animesh::Point* pt = t.mesh->FindPoint("hitbox");
				assert(pt);
				if(pt->type == Animesh::Point::BOX)
				{
					t.rw = pt->size.x;
					t.h = pt->size.z;
				}
				else
					t.h = t.rw = pt->size.x;
			}
			break;
		case LoadTask::LoadSound:
			if(!nosound)
				*load_task->sound = LoadSound(load_task->filename);
			break;
		case LoadTask::LoadObject:
			{
				Obj& o = *load_task->obj;

				if(IS_SET(o.flags2, OBJ2_VARIANT))
				{
					VariantObj& vo = *o.variant;
					if(!vo.loaded)
					{
						for(uint i=0; i<vo.count; ++i)
							vo.entries[i].mesh = LoadMesh(vo.entries[i].mesh_name);
						vo.loaded = true;
					}
				}
				else
					o.ani = LoadMesh(o.mesh);

				if(!IS_SET(o.flags, OBJ_BUILDING))
				{
					Animesh::Point* point;
					if(!IS_SET(o.flags2, OBJ2_VARIANT))
						point = o.ani->FindPoint("hit");
					else
						point = o.variant->entries[0].mesh->FindPoint("hit");

					if(point && point->IsBox())
					{
						assert(point->size.x >= 0 && point->size.y >= 0 && point->size.z >= 0);
						if(!IS_SET(o.flags, OBJ_NO_PHYSICS))
						{
							btBoxShape* shape = new btBoxShape(ToVector3(point->size));
							o.shape = shape;
						}
						else
							o.shape = nullptr;
						o.matrix = &point->mat;
						o.size = ToVEC2(point->size);

						if(IS_SET(o.flags, OBJ_PHY_ROT))
							o.type = OBJ_HITBOX_ROT;

						if(IS_SET(o.flags2, OBJ_MULTI_PHYSICS))
						{
							LocalVector2<Animesh::Point*> points;
							Animesh::Point* prev_point = point;
							
							while(true)
							{
								Animesh::Point* new_point = o.ani->FindNextPoint("hit", prev_point);
								if(new_point)
								{
									assert(new_point->IsBox() && new_point->size.x >= 0 && new_point->size.y >= 0 && new_point->size.z >= 0);
									points.push_back(new_point);
									prev_point = new_point;
								}
								else
									break;
							}

							assert(points.size() > 1u);
							o.next_obj = new Obj[points.size()+1];
							for(uint i=0, size = points.size(); i < size; ++i)
							{
								Obj& o2 = o.next_obj[i];
								o2.shape = new btBoxShape(ToVector3(points[i]->size));
								if(IS_SET(o.flags, OBJ_PHY_BLOCKS_CAM))
									o2.flags = OBJ_PHY_BLOCKS_CAM;
								o2.matrix = &points[i]->mat;
								o2.size = ToVEC2(points[i]->size);
								o2.type = o.type;
							}
							o.next_obj[points.size()].shape = nullptr;
						}
						else if(IS_SET(o.flags, OBJ_DOUBLE_PHYSICS))
						{
							Animesh::Point* point2 = o.ani->FindNextPoint("hit", point);
							if(point2 && point2->IsBox())
							{
								assert(point2->size.x >= 0 && point2->size.y >= 0 && point2->size.z >= 0);
								o.next_obj = new Obj("",0,0,"","");
								if(!IS_SET(o.flags, OBJ_NO_PHYSICS))
								{
									btBoxShape* shape = new btBoxShape(ToVector3(point2->size));
									o.next_obj->shape = shape;
									if(IS_SET(o.flags, OBJ_PHY_BLOCKS_CAM))
										o.next_obj->flags = OBJ_PHY_BLOCKS_CAM;
								}
								else
									o.next_obj->shape = nullptr;
								o.next_obj->matrix = &point2->mat;
								o.next_obj->size = ToVEC2(point2->size);
								o.next_obj->type = o.type;
							}
						}
					}
					else
					{
						o.shape = nullptr;
						o.matrix = nullptr;
					}
				}
			}
			break;
		case LoadTask::LoadTexResource:
			*load_task->tex_res = LoadTexResource(load_task->filename);
			break;
		case LoadTask::LoadItem:
			load_task->item->ani = LoadMesh(load_task->filename);
			GenerateImage(load_task->item);
			break;
		case LoadTask::LoadMusic:
			if(!nomusic)
			{
				load_task->music->snd = LoadMusic(load_task->filename);
				if(!load_task->music->snd)
				{
					WARN(Format("Failed to load music '%s'.", load_task->filename));
					load_task->music->type = MUSIC_MISSING;
				}
			}
			break;
		default:
			assert(0);
			break;
		}

		dt += t.Tick();
		if(dt >= 1.f/20)
		{
			dt = 0.f;
			load_game_progress = float(index) / load_tasks.size();

			switch(load_task->type)
			{
			case LoadTask::LoadShader:
				load_game_text = Format(txLoadingShader, load_task->filename);
				break;
			case LoadTask::SetupShaders:
				load_game_text = txConfiguringShaders;
				break;
			case LoadTask::LoadTex:
			case LoadTask::LoadTex2:
			case LoadTask::LoadTexResource:
				load_game_text = Format(txLoadingTexture, load_task->filename);
				break;
			case LoadTask::LoadMesh:
			case LoadTask::LoadTrap:
			case LoadTask::LoadItem:
				load_game_text = Format(txLoadingMesh, load_task->filename);
				break;
			case LoadTask::LoadVertexData:
				load_game_text = Format(txLoadingMeshVertex, load_task->filename);
				break;
			case LoadTask::LoadSound:
				load_game_text = Format(txLoadingSound, load_task->filename);
				break;
			case LoadTask::LoadMusic:
				load_game_text = Format(txLoadingMusic, load_task->filename);
				break;
			case LoadTask::LoadObject:
				if(IS_SET(load_task->obj->flags2, OBJ2_VARIANT))
					load_game_text = Format(txLoadingMesh, load_task->obj->variant->entries[0].mesh_name);
				else
					load_game_text = Format(txLoadingMesh, load_task->filename);
				break;
			default:
				assert(0);
				break;
			}

			DoPseudotick();
			t.Tick();
		}

		if(mutex && load_game_progress >= 0.5f)
		{
			ReleaseMutex(mutex);
			CloseHandle(mutex);
			mutex = nullptr;
		}
	}

	load_game_progress = 1.f;
	load_game_text = txLoadingComplete;
	DoPseudotick();

	if(pak1)
	{
		PakClose(pak1);
		pak1 = nullptr;
	}
}

void Game::SaveCfg()
{
	if(cfg.Save(cfg_file.c_str()) == Config::CANT_SAVE)
		ERROR(Format("Failed to save configuration file '%s'!", cfg_file.c_str()));
}

// to mog�o by by� w konstruktorze ale za du�o tego
void Game::ClearPointers()
{
	// shadery
	eMesh = nullptr;
	eParticle = nullptr;
	eTerrain = nullptr;
	eSkybox = nullptr;
	eArea = nullptr;
	eGui = nullptr;
	ePostFx = nullptr;
	eGlow = nullptr;
	eGrass = nullptr;

	// bufory wierzcho�k�w i indeksy
	vbParticle = nullptr;
	vbDungeon = nullptr;
	ibDungeon = nullptr;
	vbFullscreen = nullptr;

	// tekstury render target, powierzchnie
	tItemRegion = nullptr;
	tMinimap = nullptr;
	tChar = nullptr;
	for(int i=0; i<3; ++i)
	{
		sPostEffect[i] = nullptr;
		tPostEffect[i] = nullptr;
	}

	// vertex data
	vdSchodyGora = nullptr;
	vdSchodyDol = nullptr;
	vdNaDrzwi = nullptr;

	// teren
	terrain = nullptr;
	terrain_shape = nullptr;

	// fizyka
	shape_wall = nullptr;
	shape_low_celling = nullptr;
	shape_celling = nullptr;
	shape_floor = nullptr;
	shape_door = nullptr;
	shape_block = nullptr;
	shape_schody_c[0] = nullptr;
	shape_schody_c[1] = nullptr;
	shape_schody = nullptr;
	obj_arrow = nullptr;
	obj_spell = nullptr;

	// vertex declarations
	for(int i=0; i<VDI_MAX; ++i)
		vertex_decl[i] = nullptr;
}

void Game::OnCleanup()
{
	if(!clearup_shutdown)
		ClearGame();

	if(pak1)
	{
		PakClose(pak1);
		pak1 = nullptr;
	}

	RemoveGui();
	GUI.OnClean();
	CleanScene();
	DeleteElements(bow_instances);
	ClearQuadtree();
	ClearLanguages();

	// shadery
	ReleaseShaders();

	// bufory wierzcho�k�w i indeksy
	SafeRelease(vbParticle);
	SafeRelease(vbDungeon);
	SafeRelease(ibDungeon);
	SafeRelease(vbFullscreen);
	SafeRelease(vbInstancing);

	// tekstury render target, powierzchnie
	SafeRelease(tItemRegion);
	SafeRelease(tMinimap);
	SafeRelease(tChar);
	SafeRelease(tSave);
	SafeRelease(sItemRegion);
	SafeRelease(sChar);
	SafeRelease(sSave);
	for(int i=0; i<3; ++i)
	{
		SafeRelease(sPostEffect[i]);
		SafeRelease(tPostEffect[i]);
	}

	// item icons
	for(auto it : g_items)
	{
		Item& item = *it.second;
		if(!IS_SET(item.flags, ITEM_TEX_ONLY))
			SafeRelease(item.tex);
	}

	ClearItems();

	// vertex data
	delete vdSchodyGora;
	delete vdSchodyDol;
	delete vdNaDrzwi;

	// teren
	delete terrain;
	delete terrain_shape;

	// fizyka
	delete shape_wall;
	delete shape_low_celling;
	delete shape_celling;
	delete shape_floor;
	delete shape_door;
	delete shape_block;
	delete shape_schody_c[0];
	delete shape_schody_c[1];
	delete shape_schody;
	if(obj_arrow)
		delete obj_arrow->getCollisionShape();
	delete obj_arrow;
	delete obj_spell;

	// fizyka czar�w
	for(uint i=0; i<n_spells; ++i)
		delete g_spells[i].shape;

	// kszta�ty obiekt�w
	for(uint i=0; i<n_objs; ++i)
	{
		delete g_objs[i].shape;
		if(IS_SET(g_objs[i].flags, OBJ_DOUBLE_PHYSICS) && g_objs[i].next_obj)
		{
			delete g_objs[i].next_obj->shape;
			delete g_objs[i].next_obj;
		}
		else if(IS_SET(g_objs[i].flags2, OBJ_MULTI_PHYSICS) && g_objs[i].next_obj)
		{
			for(int j=0;;++j)
			{
				bool have_next = (g_objs[i].next_obj[j].shape != nullptr);
				delete g_objs[i].next_obj[j].shape;
				if(!have_next)
					break;
			}
			delete[] g_objs[i].next_obj;
		}
	}

	draw_batch.Clear();
	free_cave_data();

	if(peer)
		RakNet::RakPeerInterface::DestroyInstance(peer);
}

void Game::CreateTextures()
{
	V( device->CreateTexture(64, 64, 0, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &tItemRegion, nullptr) );
	V( device->CreateTexture(128, 128, 0, D3DUSAGE_DYNAMIC, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &tMinimap, nullptr) );
	V( device->CreateTexture(128, 256, 0, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &tChar, nullptr) );
	V( device->CreateTexture(256, 256, 0, D3DUSAGE_RENDERTARGET, D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &tSave, nullptr) );

	int ms, msq;
	GetMultisampling(ms, msq);
	D3DMULTISAMPLE_TYPE type = (D3DMULTISAMPLE_TYPE)ms;
	if(ms != D3DMULTISAMPLE_NONE)
	{
		V( device->CreateRenderTarget(64, 64, D3DFMT_A8R8G8B8, type, msq, FALSE, &sItemRegion, nullptr) );
		V( device->CreateRenderTarget(128, 256, D3DFMT_A8R8G8B8, type, msq, FALSE, &sChar, nullptr) );
		V( device->CreateRenderTarget(256, 256, D3DFMT_X8R8G8B8, type, msq, FALSE, &sSave, nullptr) );
		for(int i=0; i<3; ++i)
		{
			V( device->CreateRenderTarget(wnd_size.x, wnd_size.y, D3DFMT_X8R8G8B8, type, msq, FALSE, &sPostEffect[i], nullptr) );
			tPostEffect[i] = nullptr;
		}
		V( device->CreateTexture(wnd_size.x, wnd_size.y, 1, D3DUSAGE_RENDERTARGET, D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &tPostEffect[0], nullptr) );
	}
	else
	{
		for(int i=0; i<3; ++i)
		{
			V( device->CreateTexture(wnd_size.x, wnd_size.y, 1, D3DUSAGE_RENDERTARGET, D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &tPostEffect[i], nullptr) );
			sPostEffect[i] = nullptr;
		}
	}

	// fullscreen vertexbuffer
	VTex* v;
	V( device->CreateVertexBuffer(sizeof(VTex)*6, D3DUSAGE_WRITEONLY, 0, D3DPOOL_DEFAULT, &vbFullscreen, nullptr) );
	V( vbFullscreen->Lock(0, sizeof(VTex)*6, (void**)&v, 0) );

	// co� mi si� obi�o o uszy z tym p� teksela przy renderowaniu
	// ale szczeg��w nie znam
	const float u_start = 0.5f / wnd_size.x;
	const float u_end = 1.f + 0.5f / wnd_size.x;
	const float v_start = 0.5f / wnd_size.y;
	const float v_end = 1.f + 0.5f / wnd_size.y;

	v[0] = VTex(-1.f,1.f,0.f,u_start,v_start);
	v[1] = VTex(1.f,1.f,0.f,u_end,v_start);
	v[2] = VTex(1.f,-1.f,0.f,u_end,v_end);
	v[3] = VTex(1.f,-1.f,0.f,u_end,v_end);
	v[4] = VTex(-1.f,-1.f,0.f,u_start,v_end);
	v[5] = VTex(-1.f,1.f,0.f,u_start,v_start);

	V( vbFullscreen->Unlock() );
}

void Game::InitGameKeys()
{
	GKey[GK_MOVE_FORWARD].id = "keyMoveForward";
	GKey[GK_MOVE_BACK].id = "keyMoveBack";
	GKey[GK_MOVE_LEFT].id = "keyMoveLeft";
	GKey[GK_MOVE_RIGHT].id = "keyMoveRight";
	GKey[GK_WALK].id = "keyWalk";
	GKey[GK_ROTATE_LEFT].id = "keyRotateLeft";
	GKey[GK_ROTATE_RIGHT].id = "keyRotateRight";
	GKey[GK_TAKE_WEAPON].id = "keyTakeWeapon";
	GKey[GK_ATTACK_USE].id = "keyAttackUse";
	GKey[GK_USE].id = "keyUse";
	GKey[GK_BLOCK].id = "keyBlock";
	GKey[GK_STATS].id = "keyStats";
	GKey[GK_INVENTORY].id = "keyInventory";
	GKey[GK_TEAM_PANEL].id = "keyTeam";
	GKey[GK_ACTION_PANEL].id = "keyActions";
	GKey[GK_JOURNAL].id = "keyGameJournal";
	GKey[GK_MINIMAP].id = "keyMinimap";
	GKey[GK_QUICKSAVE].id = "keyQuicksave";
	GKey[GK_QUICKLOAD].id = "keyQuickload";
	GKey[GK_POTION].id = "keyPotion";
	GKey[GK_MELEE_WEAPON].id = "keyMeleeWeapon";
	GKey[GK_RANGED_WEAPON].id = "keyRangedWeapon";
	GKey[GK_TAKE_ALL].id = "keyTakeAll";
	GKey[GK_SELECT_DIALOG].id = "keySelectDialog";
	GKey[GK_SKIP_DIALOG].id = "keySkipDialog";
	GKey[GK_TALK_BOX].id = "keyTalkBox";
	GKey[GK_PAUSE].id = "keyPause";
	GKey[GK_YELL].id = "keyYell";
	GKey[GK_CONSOLE].id = "keyConsole";
	GKey[GK_ROTATE_CAMERA].id = "keyRotateCamera";

	for(int i=0; i<GK_MAX; ++i)
		GKey[i].text = Str(GKey[i].id);
}

void Game::ResetGameKeys()
{
	GKey[GK_MOVE_FORWARD].Set('W', VK_UP);
	GKey[GK_MOVE_BACK].Set('S', VK_DOWN);
	GKey[GK_MOVE_LEFT].Set('A', VK_LEFT);
	GKey[GK_MOVE_RIGHT].Set('D', VK_RIGHT);
	GKey[GK_WALK].Set(VK_SHIFT);
	GKey[GK_ROTATE_LEFT].Set('Q');
	GKey[GK_ROTATE_RIGHT].Set('E');
	GKey[GK_TAKE_WEAPON].Set(VK_SPACE);
	GKey[GK_ATTACK_USE].Set(VK_LBUTTON, 'Z');
	GKey[GK_USE].Set('R');
	GKey[GK_BLOCK].Set(VK_RBUTTON, 'X');
	GKey[GK_STATS].Set('C');
	GKey[GK_INVENTORY].Set('I');
	GKey[GK_TEAM_PANEL].Set('T');
	GKey[GK_ACTION_PANEL].Set('K');
	GKey[GK_JOURNAL].Set('J');
	GKey[GK_MINIMAP].Set('M');
	GKey[GK_QUICKSAVE].Set(VK_F5);
	GKey[GK_QUICKLOAD].Set(VK_F9);
	GKey[GK_POTION].Set('H');
	GKey[GK_MELEE_WEAPON].Set('1');
	GKey[GK_RANGED_WEAPON].Set('2');
	GKey[GK_TAKE_ALL].Set(VK_RETURN);
	GKey[GK_SELECT_DIALOG].Set(VK_RETURN);
	GKey[GK_SKIP_DIALOG].Set(VK_SPACE);
	GKey[GK_TALK_BOX].Set('N');
	GKey[GK_PAUSE].Set(VK_PAUSE);
	GKey[GK_YELL].Set('Y');
	GKey[GK_CONSOLE].Set(VK_OEM_3);
	GKey[GK_ROTATE_CAMERA].Set('V');
}

void Game::SaveGameKeys()
{
	for(int i=0; i<GK_MAX; ++i)
	{
		GameKey& k = GKey[i];
		for(int j=0; j<2; ++j)
			cfg.Add(Format("%s%d", k.id, j), Format("%d", k[j]));
	}

	SaveCfg();
}

void Game::LoadGameKeys()
{
	for(int i=0; i<GK_MAX; ++i)
	{
		GameKey& k = GKey[i];
		for(int j=0; j<2; ++j)
		{
			cstring s = Format("%s%d", k.id, j);
			int w = cfg.GetInt(s);
			if(w == VK_ESCAPE || w < -1 || w > 255)
			{
				WARN(Format("Config: Invalid value for %s: %d.", s, w));
				w = -1;
				cfg.Add(s, Format("%d", k[j]));
			}
			if(w != -1)
				k[j] = (byte)w;
		}
	}
}

void Game::PreloadData()
{
	// tekstury dla ekranu wczytywania
	tWczytywanie[0] = LoadTex("wczytywanie.png");
	tWczytywanie[1] = LoadTex("wczytywanie2.png");
	LoadScreen::tBackground = LoadTex("load_bg.jpg");

	// shader dla gui
	eGui = CompileShader("gui.fx");
	GUI.SetShader(eGui);

	// pak
	try
	{
		LOG("Opening file 'data.pak'.");
		pak1 = PakOpen("data/data.pak", "KrystaliceFire");
	}
	catch(cstring err)
	{
		ERROR(Format("Failed to read 'data.pak': %s", err));
	}

	// czcionka z pliku
	if(AddFontResourceExA("data/fonts/Florence-Regular.otf", FR_PRIVATE, nullptr) != 1)
		throw Format("Failed to load font 'Florence-Regular.otf' (%d)!", GetLastError());

	// intro music
	Music& m = g_musics[0];
	m.snd = LoadMusic(m.file);
	SetMusic(MUSIC_INTRO);
}

void Game::RestartGame()
{
	// stw�rz mutex
	HANDLE mutex = CreateMutex(nullptr, TRUE, RESTART_MUTEX_NAME);
	DWORD dwLastError = GetLastError();
	bool AlreadyRunning = (dwLastError == ERROR_ALREADY_EXISTS || dwLastError == ERROR_ACCESS_DENIED);
	if(AlreadyRunning)
	{
		WaitForSingleObject(mutex, INFINITE);
		CloseHandle(mutex);
		return;
	}

	STARTUPINFO si = {0};
	si.cb = sizeof(STARTUPINFO);
	PROCESS_INFORMATION pi = {0};

	// drugi parametr tak na prawd� nie jest modyfikowany o ile nie u�ywa si� unicode (tak jest napisane w doku)
	// z ka�dym restartem dodaje prze��cznik, mam nadzieje �e nikt nie b�dzie restartowa� 100 razy pod rz�d bo mo�e sko�czy� si� miejsce w cmdline albo co
	CreateProcess(nullptr, (char*)Format("%s -restart", GetCommandLine()), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi);

	Quit();
}

void escape(string& s, cstring str)
{
	s.clear();
	do 
	{
		char c = *str;
		if(c == 0)
			break;
		else if(c == '"')
		{
			s += '\\';
			s += '"';
		}
		else
			s += c;
		++str;
	}
	while(true);
}

void Game::LoadStatsText()
{
	// typ broni
	weapon_type_info[WT_SHORT].name = Str("wt_shortBlade");
	weapon_type_info[WT_LONG].name = Str("wt_longBlade");
	weapon_type_info[WT_MACE].name = Str("wt_blunt");
	weapon_type_info[WT_AXE].name = Str("wt_axe");
}

void Game::InitGameText()
{
#define LOAD_ARRAY(var, str) for(int i=0; i<countof(var); ++i) var[i] = Str(Format(str "%d", i))

	txGoldPlus = Str("goldPlus");
	txQuestCompletedGold = Str("questCompletedGold");

	// ai
	LOAD_ARRAY(txAiNoHpPot, "aiNoHpPot");
	LOAD_ARRAY(txAiJoinTour, "aiJoinTour");
	LOAD_ARRAY(txAiCity, "aiCity");
	LOAD_ARRAY(txAiVillage, "aiVillage");
	txAiMoonwell = Str("aiMoonwell");
	txAiForest = Str("aiForest");
	txAiCampEmpty = Str("aiCampEmpty");
	txAiCampFull = Str("aiCampFull");
	txAiFort = Str("aiFort");
	txAiDwarfFort = Str("aiDwarfFort");
	txAiTower = Str("aiTower");
	txAiArmory = Str("aiArmory");
	txAiHideout = Str("aiHideout");
	txAiVault = Str("aiVault");
	txAiCrypt = Str("aiCrypt");
	txAiTemple = Str("aiTemple");
	txAiNecromancerBase = Str("aiNecromancerBase");
	txAiLabirynth = Str("aiLabirynth");
	txAiNoEnemies = Str("aiNoEnemies");
	txAiNearEnemies = Str("aiNearEnemies");
	txAiCave = Str("aiCave");
	LOAD_ARRAY(txAiInsaneText, "aiInsaneText");
	LOAD_ARRAY(txAiDefaultText, "aiDefaultText");
	LOAD_ARRAY(txAiOutsideText, "aiOutsideText");
	LOAD_ARRAY(txAiInsideText, "aiInsideText");
	LOAD_ARRAY(txAiHumanText, "aiHumanText");
	LOAD_ARRAY(txAiOrcText, "aiOrcText");
	LOAD_ARRAY(txAiGoblinText, "aiGoblinText");
	LOAD_ARRAY(txAiMageText, "aiMageText");
	LOAD_ARRAY(txAiSecretText, "aiSecretText");
	LOAD_ARRAY(txAiHeroDungeonText, "aiHeroDungeonText");
	LOAD_ARRAY(txAiHeroCityText, "aiHeroCityText");
	LOAD_ARRAY(txAiBanditText, "aiBanditText");
	LOAD_ARRAY(txAiHeroOutsideText, "aiHeroOutsideText");
	LOAD_ARRAY(txAiDrunkMageText, "aiDrunkMageText");
	LOAD_ARRAY(txAiDrunkText, "aiDrunkText");
	LOAD_ARRAY(txAiDrunkmanText, "aiDrunkmanText");

	// nazwy lokacji
	txRandomEncounter = Str("randomEncounter");
	txCamp = Str("camp");

	// mapa
	txEnteringLocation = Str("enteringLocation");
	txGeneratingMap = Str("generatingMap");
	txGeneratingBuildings = Str("generatingBuildings");
	txGeneratingObjects = Str("generatingObjects");
	txGeneratingUnits = Str("generatingUnits");
	txGeneratingItems = Str("generatingItems");
	txGeneratingPhysics = Str("generatingPhysics");
	txRecreatingObjects = Str("recreatingObjects");
	txGeneratingMinimap = Str("generatingMinimap");
	txLoadingComplete = Str("loadingComplete");
	txWaitingForPlayers = Str("waitingForPlayers");
	txGeneratingTerrain = Str("generatingTerrain");

	// zawody w piciu
	txContestNoWinner = Str("contestNoWinner");
	txContestStart = Str("contestStart");
	LOAD_ARRAY(txContestTalk, "contestTalk");
	txContestWin = Str("contestWin");
	txContestWinNews = Str("contestWinNews");
	txContestDraw = Str("contestDraw");
	txContestPrize = Str("contestPrize");
	txContestNoPeople = Str("contestNoPeople");

	// samouczek
	LOAD_ARRAY(txTut, "tut");
	txTutNote = Str("tutNote");
	txTutLoc = Str("tutLoc");
	txTutPlay = Str("tutPlay");
	txTutTick = Str("tutTick");

	// zawody
	LOAD_ARRAY(txTour, "tour");

	txCantSaveGame = Str("cantSaveGame");
	txSaveFailed = Str("saveFailed");
	txSavedGameN = Str("savedGameN");
	txLoadFailed = Str("loadFailed");
	txQuickSave = Str("quickSave");
	txGameSaved = Str("gameSaved");
	txLoadingLocations = Str("loadingLocations");
	txLoadingData = Str("loadingData");
	txLoadingQuests = Str("loadingQuests");
	txEndOfLoading = Str("endOfLoading");
	txCantSaveNow = Str("cantSaveNow");
	txCantLoadGame = Str("cantLoadGame");
	txLoadSignature = Str("loadSignature");
	txLoadVersion = Str("loadVersion");
	txLoadSaveVersionNew = Str("loadSaveVersionNew");
	txLoadSaveVersionOld = Str("loadSaveVersionOld");
	txLoadMP = Str("loadMP");
	txLoadSP = Str("loadSP");
	txLoadError = Str("loadError");
	txLoadOpenError = Str("loadOpenError");

	txPvpRefuse = Str("pvpRefuse");
	txSsFailed = Str("ssFailed");
	txSsDone = Str("ssDone");
	txLoadingResources = Str("loadingResources");
	txLoadingShader = Str("loadingShader");
	txConfiguringShaders = Str("configuringShaders");
	txLoadingTexture = Str("loadingTexture");
	txLoadingMesh = Str("loadingMesh");
	txLoadingMeshVertex = Str("loadingMeshVertex");
	txLoadingSound = Str("loadingSound");
	txLoadingMusic = Str("loadingMusic");
	txWin = Str("win");
	txWinMp = Str("winMp");
	txINeedWeapon = Str("iNeedWeapon");
	txNoHpp = Str("noHpp");
	txCantDo = Str("cantDo");
	txDontLootFollower = Str("dontLootFollower");
	txDontLootArena = Str("dontLootArena");
	txUnlockedDoor = Str("unlockedDoor");
	txNeedKey = Str("needKey");
	txLevelUp = Str("levelUp");
	txLevelDown = Str("levelDown");
	txLocationText = Str("locationText");
	txLocationTextMap = Str("locationTextMap");
	txRegeneratingLevel = Str("regeneratingLevel");
	txGmsLooted = Str("gmsLooted");
	txGmsRumor = Str("gmsRumor");
	txGmsJournalUpdated = Str("gmsJournalUpdated");
	txGmsUsed = Str("gmsUsed");
	txGmsUnitBusy = Str("gmsUnitBusy");
	txGmsGatherTeam = Str("gmsGatherTeam");
	txGmsNotLeader = Str("gmsNotLeader");
	txGmsNotInCombat = Str("gmsNotInCombat");
	txGainTextAttrib = Str("gainTextAttrib");
	txGainTextSkill = Str("gainTextSkill");
	txNeedLadle = Str("needLadle");
	txNeedPickaxe = Str("needPickaxe");
	txNeedHammer = Str("needHammer");
	txNeedUnk = Str("needUnk");
	txReallyQuit = Str("reallyQuit");
	txSecretAppear = Str("secretAppear");
	txGmsAddedItem = Str("gmsAddedItem");
	txGmsAddedItems = Str("gmsAddedItems");

	// plotki
	LOAD_ARRAY(txRumor, "rumor_");
	LOAD_ARRAY(txRumorD, "rumor_d_");

	// dialogi 1
	LOAD_ARRAY(txMayorQFailed, "mayorQFailed");
	LOAD_ARRAY(txQuestAlreadyGiven, "questAlreadyGiven");
	LOAD_ARRAY(txMayorNoQ, "mayorNoQ");
	LOAD_ARRAY(txCaptainQFailed, "captainQFailed");
	LOAD_ARRAY(txCaptainNoQ, "captainNoQ");
	LOAD_ARRAY(txLocationDiscovered, "locationDiscovered");
	LOAD_ARRAY(txAllDiscovered, "allDiscovered");
	LOAD_ARRAY(txCampDiscovered, "campDiscovered");
	LOAD_ARRAY(txAllCampDiscovered, "allCampDiscovered");
	LOAD_ARRAY(txNoQRumors, "noQRumors");
	LOAD_ARRAY(txRumorQ, "rumorQ");
	txNeedMoreGold = Str("needMoreGold");
	txNoNearLoc = Str("noNearLoc");
	txNearLoc = Str("nearLoc");
	LOAD_ARRAY(txNearLocEmpty, "nearLocEmpty");
	txNearLocCleared = Str("nearLocCleared");
	LOAD_ARRAY(txNearLocEnemy, "nearLocEnemy");
	LOAD_ARRAY(txNoNews, "noNews");
	LOAD_ARRAY(txAllNews, "allNews");
	txPvpTooFar = Str("pvpTooFar");
	txPvp = Str("pvp");
	txPvpWith = Str("pvpWith");
	txNewsCampCleared = Str("newsCampCleared");
	txNewsLocCleared = Str("newsLocCleared");
	LOAD_ARRAY(txArenaText, "arenaText");
	LOAD_ARRAY(txArenaTextU, "arenaTextU");
	txAllNearLoc = Str("allNearLoc");

	// dystans / si�a
	txNear = Str("near");
	txFar = Str("far");
	txVeryFar = Str("veryFar");
	LOAD_ARRAY(txELvlVeryWeak, "eLvlVeryWeak");
	LOAD_ARRAY(txELvlWeak, "eLvlWeak");
	LOAD_ARRAY(txELvlAverage, "eLvlAverage");
	LOAD_ARRAY(txELvlQuiteStrong, "eLvlQuiteStrong");
	LOAD_ARRAY(txELvlStrong, "eLvlStrong");

	// questy
	txArthur = Str("arthur");
	txMineBuilt = Str("mineBuilt");
	txAncientArmory = Str("ancientArmory");
	txPortalClosed = Str("portalClosed");
	txPortalClosedNews = Str("portalClosedNews");
	txHiddenPlace = Str("hiddenPlace");
	txOrcCamp = Str("orcCamp");
	txPortalClose = Str("portalClose");
	txPortalCloseLevel = Str("portalCloseLevel");
	txXarDanger = Str("xarDanger");
	txGorushDanger = Str("gorushDanger");
	txGorushCombat = Str("gorushCombat");
	txMageHere = Str("mageHere");
	txMageEnter = Str("mageEnter");
	txMageFinal = Str("mageFinal");
	LOAD_ARRAY(txQuest, "quest");
	txForMayor = Str("forMayor");
	txForSoltys = Str("forSoltys");

	// menu net
	txEnterIp = Str("enterIp");
	txConnecting = Str("connecting");
	txInvalidIp = Str("invalidIp");
	txWaitingForPswd = Str("waitingForPswd");
	txEnterPswd = Str("enterPswd");
	txConnectingTo = Str("connectingTo");
	txConnectTimeout = Str("connectTimeout");
	txConnectInvalid = Str("connectInvalid");
	txConnectVersion = Str("connectVersion");
	txConnectRaknet = Str("connectRaknet");
	txCantJoin = Str("cantJoin");
	txLostConnection = Str("lostConnection");
	txInvalidPswd = Str("invalidPswd");
	txCantJoin2 = Str("cantJoin2");
	txServerFull = Str("serverFull");
	txInvalidData = Str("invalidData");
	txNickUsed = Str("nickUsed");
	txInvalidVersion = Str("invalidVersion");
	txInvalidVersion2 = Str("invalidVersion2");
	txInvalidNick = Str("invalidNick");
	txGeneratingWorld = Str("generatingWorld");
	txLoadedWorld = Str("loadedWorld");
	txWorldDataError = Str("worldDataError");
	txLoadedPlayer = Str("loadedPlayer");
	txPlayerDataError = Str("playerDataError");
	txGeneratingLocation = Str("generatingLocation");
	txLoadingLocation = Str("loadingLocation");
	txLoadingLocationError = Str("loadingLocationError");
	txLoadingChars = Str("loadingChars");
	txLoadingCharsError = Str("loadingCharsError");
	txSendingWorld = Str("sendingWorld");
	txMpNPCLeft = Str("mpNPCLeft");
	txLoadingLevel = Str("loadingLevel");
	txDisconnecting = Str("disconnecting");
	txDisconnected = Str("disconnected");
	txLost = Str("lost");
	txLeft = Str("left");
	txLost2 = Str("left2");
	txUnconnected = Str("unconnected");
	txClosing = Str("closing");
	txKicked = Str("kicked");
	txUnknown = Str("unknown");
	txUnknown2 = Str("unknown2");
	txWaitingForServer = Str("waitingForServer");
	txStartingGame = Str("startingGame");
	txPreparingWorld = Str("preparingWorld");
	txInvalidCrc = Str("invalidCrc");
	txInvalidCrc2 = Str("invalidCrc2");

	// net
	txCreateServerFailed = Str("createServerFailed");
	txInitConnectionFailed = Str("initConnectionFailed");
	txServer = Str("server");
	txPlayerKicked = Str("playerKicked");
	txYouAreLeader = Str("youAreLeader");
	txRolledNumber = Str("rolledNumber");
	txPcIsLeader = Str("pcIsLeader");
	txReceivedGold = Str("receivedGold");
	txYouDisconnected = Str("youDisconnected");
	txYouKicked = Str("youKicked");
	txPcWasKicked = Str("pcWasKicked");
	txPcLeftGame = Str("pcLeftGame");
	txGamePaused = Str("gamePaused");
	txGameResumed = Str("gameResumed");
	txCanUseCheats = Str("canUseCheats");
	txCantUseCheats = Str("cantUseCheats");
	txPlayerLeft = Str("playerLeft");

	// ob�z wrog�w
	txSGOOrcs = Str("sgo_orcs");
	txSGOGoblins = Str("sgo_goblins");
	txSGOBandits = Str("sgo_bandits");
	txSGOEnemies = Str("sgo_enemies");
	txSGOUndead = Str("sgo_undead");
	txSGOMages = Str("sgo_mages");
	txSGOGolems = Str("sgo_golems");
	txSGOMagesAndGolems = Str("sgo_magesAndGolems");
	txSGOUnk = Str("sgo_unk");
	txSGOPowerfull = Str("sgo_powerfull");

	// nazwy u�ywalnych obiekt�w
	for(int i=0; i<U_MAX; ++i)
	{
		BaseUsable& u = g_base_usables[i];
		u.name = Str(u.id);
	}

	// nazwy budynk�w
	for(int i=0; i<B_MAX; ++i)
	{
		Building& b = buildings[i];
		if(IS_SET(b.flags, BF_LOAD_NAME))
			b.name = Str(Format("b_%s", b.id));
	}
	
	// rodzaje wrog�w
	for(int i=0; i<SG_MAX; ++i)
	{
		SpawnGroup& sg = g_spawn_groups[i];
		if(!sg.co)
			sg.co = Str(Format("sg_%s", sg.id_name));
	}

	// dialogi
	LOAD_ARRAY(txDialog, "d");
	LOAD_ARRAY(txYell, "yell");

	TakenPerk::LoadText();
}

Unit* Game::FindPlayerTradingWithUnit(Unit& u)
{
	for(vector<Unit*>::iterator it = active_team.begin(), end = active_team.end(); it != end; ++it)
	{
		if((*it)->IsPlayer() && (*it)->player->IsTradingWith(&u))
			return *it;
	}
	return nullptr;
}

bool Game::ValidateTarget(Unit& u, Unit* target)
{
	assert(target);

	LevelContext& ctx = GetContext(u);

	for(vector<Unit*>::iterator it = ctx.units->begin(), end = ctx.units->end(); it != end; ++it)
	{
		if(*it == target)
			return true;
	}

	return false;
}

void Game::UpdateLights(vector<Light>& lights)
{
	for(vector<Light>::iterator it = lights.begin(), end = lights.end(); it != end; ++it)
	{
		Light& s = *it;
		s.t_pos = s.pos + random(VEC3(-0.05f, -0.05f, -0.05f), VEC3(0.05f, 0.05f, 0.05f));
		s.t_color = clamp(s.color + random(VEC3(-0.1f, -0.1f, -0.1f), VEC3(0.1f, 0.1f, 0.1f)));
	}
}

bool Game::IsDrunkman(Unit& u)
{
	if(IS_SET(u.data->flags, F_AI_DRUNKMAN))
		return true;
	else if(IS_SET(u.data->flags3, F3_DRUNK_MAGE))
		return quest_mages2->mages_state < Quest_Mages2::State::MageCured;
	else if(IS_SET(u.data->flags3, F3_DRUNKMAN_AFTER_CONTEST))
		return contest_state == CONTEST_DONE;
	else
		return false;
}

void Game::PlayUnitSound(Unit& u, SOUND snd, float range)
{
	if(&u == pc->unit)
		PlaySound2d(snd);
	else
		PlaySound3d(snd, u.GetHeadSoundPos(), range);
}

void Game::UnitFall(Unit& u)
{
	ACTION prev_action = u.action;
	u.live_state = Unit::FALLING;

	if(IsLocal())
	{
		// przerwij akcj�
		BreakAction(u, true);

		// wstawanie
		u.raise_timer = random(5.f,7.f);

		// event
		if(u.event_handler)
			u.event_handler->HandleUnitEvent(UnitEventHandler::FALL, &u);

		// komunikat
		if(IsOnline())
		{
			NetChange& c = Add1(net_changes);
			c.type = NetChange::FALL;
			c.unit = &u;
		}

		if(u.player == pc)
			before_player = BP_NONE;
	}
	else
	{
		// przerwij akcj�
		BreakAction(u, true);

		// komunikat
		if(&u == pc->unit)
		{
			u.raise_timer = random(5.f,7.f);
			before_player = BP_NONE;
		}
	}

	if(prev_action == A_ANIMATION)
	{
		u.action = A_NONE;
		u.current_animation = ANI_STAND;
	}
	u.animation = ANI_DIE;
	u.talking = false;
	u.ani->need_update = true;
}

void Game::UnitDie(Unit& u, LevelContext* ctx, Unit* killer)
{
	ACTION prev_action = u.action;

	if(u.live_state == Unit::FALL)
	{
		// posta� ju� le�y na ziemi, dodaj krew
		CreateBlood(GetContext(u), u);
		u.live_state = Unit::DEAD;
	}
	else
		u.live_state = Unit::DYING;

	if(IsLocal())
	{
		// przerwij akcj�
		BreakAction(u, true);

		// dodaj z�oto do ekwipunku
		if(u.gold && !(u.IsPlayer() || u.IsFollower()))
		{
			u.AddItem(&gold_item, (uint)u.gold);
			u.gold = 0;
		}

		// og�o� �mier�
		for(vector<Unit*>::iterator it = ctx->units->begin(), end = ctx->units->end(); it != end; ++it)
		{
			if((*it)->IsPlayer() || !(*it)->IsStanding() || !IsFriend(u, **it))
				continue;

			if(distance(u.pos, (*it)->pos) <= 20.f && CanSee(u, **it))
				(*it)->ai->morale -= 2.f;
		}

		// o�ywianie / sprawd� czy lokacja oczyszczona
		if(u.IsTeamMember())
			u.raise_timer = random(5.f,7.f);
		else
			CheckIfLocationCleared();

		// event
		if(u.event_handler)
			u.event_handler->HandleUnitEvent(UnitEventHandler::DIE, &u);

		// muzyka bossa
		if(IS_SET(u.data->flags2, F2_BOSS))
		{
			if(RemoveElementTry(boss_levels, INT2(current_location, dungeon_level)))
				SetMusic();
		}

		// komunikat
		if(IsOnline())
		{
			NetChange& c2 = Add1(net_changes);
			c2.type = NetChange::DIE;
			c2.unit = &u;
		}

		// statystyki
		++total_kills;
		if(killer && killer->IsPlayer())
		{
			++killer->player->kills;
			if(IsOnline())
				killer->player->stat_flags |= STAT_KILLS;
		}
		if(u.IsPlayer())
		{
			++u.player->knocks;
			if(IsOnline())
				u.player->stat_flags |= STAT_KNOCKS;
			if(u.player == pc)
				before_player = BP_NONE;
		}
	}
	else
	{
		u.hp = 0.f;

		// przerwij akcj�
		BreakAction(u, true);

		// o�ywianie
		if(&u == pc->unit)
		{
			u.raise_timer = random(5.f,7.f);
			before_player = BP_NONE;
		}

		// muzyka bossa
		if(IS_SET(u.data->flags2, F2_BOSS) && boss_level_mp)
		{
			boss_level_mp = false;
			SetMusic();
		}
	}

	if(prev_action == A_ANIMATION)
	{
		u.action = A_NONE;
		u.current_animation = ANI_STAND;
	}
	u.animation = ANI_DIE;
	u.talking = false;
	u.ani->need_update = true;

	// d�wi�k
	if(sound_volume)
	{
		SOUND snd = u.data->sounds->sound[SOUND_DEATH];
		if(!snd)
			snd = u.data->sounds->sound[SOUND_PAIN];
		if(snd)
			PlayUnitSound(u, snd, 2.f);
	}

	// przenie� fizyke
	btVector3 a_min, a_max;
	u.cobj->getWorldTransform().setOrigin(btVector3(1000,1000,1000));
	u.cobj->getCollisionShape()->getAabb(u.cobj->getWorldTransform(), a_min, a_max);
	phy_broadphase->setAabb(u.cobj->getBroadphaseHandle(), a_min, a_max, phy_dispatcher);
}

void Game::UnitTryStandup(Unit& u, float dt)
{
	if(u.in_arena != -1 || death_screen != 0)
		return;

	u.raise_timer -= dt;
	bool ok = false;

	if(u.live_state == Unit::DEAD)
	{
		if(u.IsTeamMember())
		{
			if(u.hp > 0.f && u.raise_timer > 0.1f)
				u.raise_timer = 0.1f;
			
			if(u.raise_timer <= 0.f)
			{
				u.RemovePoison();

				if(u.alcohol > u.hpmax)
				{
					// m�g�by wsta� ale jest zbyt pijany
					u.live_state = Unit::FALL;
					UpdateUnitPhysics(u, u.pos);
				}
				else
				{
					// sprawd� czy nie ma wrog�w
					LevelContext& ctx = GetContext(u);
					ok = true;
					for(vector<Unit*>::iterator it = ctx.units->begin(), end = ctx.units->end(); it != end; ++it)
					{
						if((*it)->IsStanding() && IsEnemy(u, **it) && distance(u.pos, (*it)->pos) <= 20.f && CanSee(u, **it))
						{
							ok = false;
							break;
						}
					}
				}

				if(!ok)
				{
					if(u.hp > 0.f)
						u.raise_timer = 0.1f;
					else
						u.raise_timer = random(1.f,2.f);
				}
			}
		}
	}
	else if(u.live_state == Unit::FALL)
	{
		if(u.raise_timer <= 0.f)
		{
			if(u.alcohol < u.hpmax)
				ok = true;
			else
				u.raise_timer = random(1.f,2.f);
		}
	}

	if(ok)
	{
		UnitStandup(u);

		if(IsOnline())
		{
			NetChange& c = Add1(net_changes);
			c.type = NetChange::STAND_UP;
			c.unit = &u;
		}
	}
}

void Game::UnitStandup(Unit& u)
{
	u.HealPoison();
	u.live_state = Unit::ALIVE;
	Animesh::Animation* anim = u.ani->ani->GetAnimation("wstaje2");
	if(anim)
	{
		u.ani->Play("wstaje2", PLAY_ONCE|PLAY_PRIO3, 0);
		u.action = A_ANIMATION;
	}
	else
		u.action = A_NONE;
	u.used_item = nullptr;

	if(IsLocal() && u.IsAI())
	{
		if(u.ai->state != AIController::Idle)
		{
			u.ai->state = AIController::Idle;
			u.ai->change_ai_mode = true;
		}
		u.ai->alert_target = nullptr;
		u.ai->idle_action = AIController::Idle_None;
		u.ai->target = nullptr;
		u.ai->timer = random(2.f,5.f);
	}

	WarpUnit(u, u.pos);
}

void Game::UpdatePostEffects(float dt)
{
	post_effects.clear();
	if(!cl_postfx || game_state != GS_LEVEL)
		return;

	// szarzenie
	if(pc->unit->IsAlive())
		grayout = max(grayout-dt, 0.f);
	else
		grayout = min(grayout+dt, 1.f);
	if(grayout > 0.f)
	{
		PostEffect& e = Add1(post_effects);
		e.tech = ePostFx->GetTechniqueByName("Monochrome");
		e.power = grayout;
	}

	// upicie
	float drunk = pc->unit->alcohol/pc->unit->hpmax;
	if(drunk > 0.1f)
	{
		PostEffect* e = nullptr, *e2;
		post_effects.resize(post_effects.size()+2);
		e = &*(post_effects.end()-2);
		e2 = &*(post_effects.end()-1);

		e->id = e2->id = 0;
		e->tech = ePostFx->GetTechniqueByName("BlurX");
		e2->tech = ePostFx->GetTechniqueByName("BlurY");
		// 0.1-0.5 - 1
		// 1 - 2
		float mod;
		if(drunk < 0.5f)
			mod = 1.f;
		else
			mod = 1.f+(drunk-0.5f)*2;
		e->skill = e2->skill = VEC4(1.f/wnd_size.x*mod, 1.f/wnd_size.y*mod, 0, 0);
		// 0.1-0
		// 1-1
		e->power = e2->power = (drunk-0.1f)/0.9f;
	}
}

void Game::PlayerYell(Unit& u)
{
	UnitTalk(u, random_string(txYell));

	LevelContext& ctx = GetContext(u);
	for(vector<Unit*>::iterator it = ctx.units->begin(), end = ctx.units->end(); it != end; ++it)
	{
		Unit& u2 = **it;
		if(u2.IsAI() && u2.IsStanding() && !IsEnemy(u, u2) && !IsFriend(u, u2) && u2.busy == Unit::Busy_No && u2.frozen == 0 && !u2.useable && u2.ai->state == AIController::Idle &&
			!IS_SET(u2.data->flags, F_AI_STAY) &&
			(u2.ai->idle_action == AIController::Idle_None || u2.ai->idle_action == AIController::Idle_Animation || u2.ai->idle_action == AIController::Idle_Rot ||
			u2.ai->idle_action == AIController::Idle_Look))
		{
			u2.ai->idle_action = AIController::Idle_MoveAway;
			u2.ai->idle_data.unit = &u;
			u2.ai->timer = random(3.f,5.f);
		}
	}
}

bool Game::CanBuySell(const Item* item)
{
	assert(item);
	if(!trader_buy[item->type])
		return false;
	if(item->type == IT_CONSUMEABLE)
	{
		if(pc->action_unit->data->id == "alchemist")
		{
			if(item->ToConsumeable().cons_type != Potion)
				return false;
		}
		else if(pc->action_unit->data->id == "food_seller")
		{
			if(item->ToConsumeable().cons_type == Potion)
				return false;
		}
	}
	return true;
}

void Game::ResetCollisionPointers()
{
	for(vector<Object>::iterator it = local_ctx.objects->begin(), end = local_ctx.objects->end(); it != end; ++it)
	{
		if(it->base && IS_SET(it->base->flags, OBJ_PHYSICS_PTR))
		{
			btCollisionObject* cobj = (btCollisionObject*)it->ptr;
			if(cobj->getUserPointer() != (void*)&*it)
				cobj->setUserPointer(&*it);
		}
	}
}

void Game::InitSuperShader()
{
	V( D3DXCreateEffectPool(&sshader_pool) );

	FileReader f(Format("%s/shaders/super.fx", g_system_dir.c_str()));
	FILETIME file_time;
	GetFileTime(f.file, nullptr, nullptr, &file_time);
	if(CompareFileTime(&file_time, &sshader_edit_time) != 0)
	{
		f.ReadToString(sshader_code);
		GetFileTime(f.file, nullptr, nullptr, &sshader_edit_time);
	}

	GetSuperShader(0);

	SetupSuperShader();
}

SuperShader* Game::GetSuperShader(uint id)
{
	for(vector<SuperShader>::iterator it = sshaders.begin(), end = sshaders.end(); it != end; ++it)
	{
		if(it->id == id)
			return &*it;
	}

	return CompileSuperShader(id);
}

SuperShader* Game::CompileSuperShader(uint id)
{
	D3DXMACRO macros[10] = {0};
	uint i = 0;

	if(IS_SET(id, 1<<SSS_ANIMATED))
	{
		macros[i].Name = "ANIMATED";
		macros[i].Definition = "1";
		++i;
	}
	if(IS_SET(id, 1<<SSS_HAVE_BINORMALS))
	{
		macros[i].Name = "HAVE_BINORMALS";
		macros[i].Definition = "1";
		++i;
	}
	if(IS_SET(id, 1<<SSS_FOG))
	{
		macros[i].Name = "FOG";
		macros[i].Definition = "1";
		++i;
	}
	if(IS_SET(id, 1<<SSS_SPECULAR))
	{
		macros[i].Name = "SPECULAR_MAP";
		macros[i].Definition = "1";
		++i;
	}
	if(IS_SET(id, 1<<SSS_NORMAL))
	{
		macros[i].Name = "NORMAL_MAP";
		macros[i].Definition = "1";
		++i;
	}
	if(IS_SET(id, 1<<SSS_POINT_LIGHT))
	{
		macros[i].Name = "POINT_LIGHT";
		macros[i].Definition = "1";
		++i;

		macros[i].Name = "LIGHTS";
		macros[i].Definition = (shader_version == 2 ? "2" : "3");
		++i;
	}
	else if(IS_SET(id, 1<<SSS_DIR_LIGHT))
	{
		macros[i].Name = "DIR_LIGHT";
		macros[i].Definition = "1";
		++i;
	}

	macros[i].Name = "VS_VERSION";
	macros[i].Definition = (shader_version == 3 ? "vs_3_0" : "vs_2_0");
	++i;

	macros[i].Name = "PS_VERSION";
	macros[i].Definition = (shader_version == 3 ? "ps_3_0" : "ps_2_0");
	++i;

	LOG(Format("Compiling super shader: %u", id));

	CompileShaderParams params = {"super.fx"};
	params.cache_name = Format("%d_super%u.fcx", shader_version, id);
	params.file_time = sshader_edit_time;
	params.input = &sshader_code;
	params.macros = macros;
	params.pool = sshader_pool;
	
	SuperShader& s = Add1(sshaders);
	s.e = CompileShader(params);
	s.id = id;

	return &s;
}

void Game::SetupSuperShader()
{
	ID3DXEffect* e = sshaders[0].e;

	LOG("Setting up super shader parameters.");
	hSMatCombined = e->GetParameterByName(nullptr, "matCombined");
	hSMatWorld = e->GetParameterByName(nullptr, "matWorld");
	hSMatBones = e->GetParameterByName(nullptr, "matBones");
	hSTint = e->GetParameterByName(nullptr, "tint");
	hSAmbientColor = e->GetParameterByName(nullptr, "ambientColor");
	hSFogColor = e->GetParameterByName(nullptr, "fogColor");
	hSFogParams = e->GetParameterByName(nullptr, "fogParams");
	hSLightDir = e->GetParameterByName(nullptr, "lightDir");
	hSLightColor = e->GetParameterByName(nullptr, "lightColor");
	hSLights = e->GetParameterByName(nullptr, "lights");
	hSSpecularColor = e->GetParameterByName(nullptr, "specularColor");
	hSSpecularIntensity = e->GetParameterByName(nullptr, "specularIntensity");
	hSSpecularHardness = e->GetParameterByName(nullptr, "specularHardness");
	hSCameraPos = e->GetParameterByName(nullptr, "cameraPos");
	hSTexDiffuse = e->GetParameterByName(nullptr, "texDiffuse");
	hSTexNormal = e->GetParameterByName(nullptr, "texNormal");
	hSTexSpecular = e->GetParameterByName(nullptr, "texSpecular");
	assert(hSMatCombined && hSMatWorld && hSMatBones && hSTint && hSAmbientColor && hSFogColor && hSFogParams && hSLightDir && hSLightColor && hSLights && hSSpecularColor && hSSpecularIntensity
		&& hSSpecularHardness && hSCameraPos && hSTexDiffuse && hSTexNormal && hSTexSpecular);
}

void Game::ReloadShaders()
{
	LOG("Reloading shaders...");

	ReleaseShaders();

	InitSuperShader();

	try
	{
		eMesh = CompileShader("mesh.fx");
		eParticle = CompileShader("particle.fx");
		eSkybox = CompileShader("skybox.fx");
		eTerrain = CompileShader("terrain.fx");
		eArea = CompileShader("area.fx");
		eGui = CompileShader("gui.fx");
		ePostFx = CompileShader("post.fx");
		eGlow = CompileShader("glow.fx");
		eGrass = CompileShader("grass.fx");
	}
	catch(cstring err)
	{
		FatalError(Format("Failed to reload shaders.\n%s", err));
		return;
	}

	SetupShaders();
	GUI.SetShader(eGui);
}

void Game::ReleaseShaders()
{
	SafeRelease(eMesh);
	SafeRelease(eParticle);
	SafeRelease(eTerrain);
	SafeRelease(eSkybox);
	SafeRelease(eArea);
	SafeRelease(eGui);
	SafeRelease(ePostFx);
	SafeRelease(eGlow);
	SafeRelease(eGrass);

	SafeRelease(sshader_pool);
	for(vector<SuperShader>::iterator it = sshaders.begin(), end = sshaders.end(); it != end; ++it)
		SafeRelease(it->e);
	sshaders.clear();
}

void Game::SetMeshSpecular()
{
	for(Weapon* weapon : g_weapons)
	{
		Weapon& w = *weapon;
		if(w.ani && w.ani->head.version < 18)
		{
			const MaterialInfo& mat = g_materials[w.material];
			for(int i = 0; i < w.ani->head.n_subs; ++i)
			{
				w.ani->subs[i].specular_intensity = mat.intensity;
				w.ani->subs[i].specular_hardness = mat.hardness;
			}
		}
	}

	for(Shield* shield : g_shields)
	{
		Shield& s = *shield;
		if(s.ani && s.ani->head.version < 18)
		{
			const MaterialInfo& mat = g_materials[s.material];
			for(int i = 0; i < s.ani->head.n_subs; ++i)
			{
				s.ani->subs[i].specular_intensity = mat.intensity;
				s.ani->subs[i].specular_hardness = mat.hardness;
			}
		}
	}

	for(Armor* armor : g_armors)
	{
		Armor& a = *armor;
		if(a.ani && a.ani->head.version < 18)
		{
			const MaterialInfo& mat = g_materials[a.material];
			for(int i = 0; i < a.ani->head.n_subs; ++i)
			{
				a.ani->subs[i].specular_intensity = mat.intensity;
				a.ani->subs[i].specular_hardness = mat.hardness;
			}
		}
	}

	for(uint i = 0; i < n_base_units; ++i)
	{
		UnitData& ud = g_base_units[i];
		if(ud.ani && ud.ani->head.version < 18)
		{
			const MaterialInfo& mat = g_materials[ud.mat];
			for(int i = 0; i < ud.ani->head.n_subs; ++i)
			{
				ud.ani->subs[i].specular_intensity = mat.intensity;
				ud.ani->subs[i].specular_hardness = mat.hardness;
			}
		}
	}
}

void Game::ValidateGameData(bool popup)
{
	LOG("Validating game data...");

	int err = 0;

	AttributeInfo::Validate(err);
	SkillInfo::Validate(err);
	ClassInfo::Validate(err);
	Item::Validate(err);
	PerkInfo::Validate(err);

	if(err == 0)
		LOG("Validation succeeded.");
	else
	{
		cstring msg = Format("Validation failed, %d errors found.", err);
		if(popup)
			ShowError(msg);
		else
			ERROR(msg);
	}
}

AnimeshInstance* Game::GetBowInstance(Animesh* mesh)
{
	if(bow_instances.empty())
		return new AnimeshInstance(mesh);
	else
	{
		AnimeshInstance* instance = bow_instances.back();
		bow_instances.pop_back();
		instance->ani = mesh;
		return instance;
	}
}
