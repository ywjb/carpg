#include "Pch.h"
#include "Base.h"
#include "CreateCharacterPanel.h"
#include "Game.h"
#include "Language.h"
#include "GetTextDialog.h"
#include "Language.h"

//-----------------------------------------------------------------------------
const int SECTION_H = 40;
const int VALUE_H = 20;

//-----------------------------------------------------------------------------
enum ButtonId
{
	IdCancel = GuiEvent_Custom,
	IdNext,
	IdBack,
	IdCreate,
	IdHardcore,
	IdHair,
	IdMustache,
	IdBeard,
	IdColor,
	IdSize
};

//=================================================================================================
CreateCharacterPanel::CreateCharacterPanel(DialogInfo& info) : Dialog(info), unit(NULL)
{
	txNext = Str("next");
	txGoBack = Str("goBack");
	txCreate = Str("create");
	txHardcoreMode = Str("hardcoreMode");
	txHair = Str("hair");
	txMustache = Str("mustache");
	txBeard = Str("beard");
	txHairColor = Str("hairColor");
	txSize = Str("size");
	txCharacterCreation = Str("characterCreation");
	txName = Str("name");

	size = INT2(600,500);
	unit = new Unit;
	unit->type = Unit::HUMAN;
	unit->human_data = new Human;
	unit->player = NULL;
	unit->ai = NULL;
	unit->hero = NULL;
	unit->used_item = NULL;
	unit->stan_broni = BRON_SCHOWANA;
	unit->pos = unit->visual_pos = VEC3(0,0,0);
	unit->rot = 0.f;

	bts.resize(2);

	bts[0].id = IdCancel;
	bts[0].text = GUI.txCancel;
	bts[0].size = INT2(100,44);
	bts[0].parent = this;
	bts[0].pos = INT2(16,size.y-60);
	
	bts[1].id = IdNext;
	bts[1].text = txNext;
	bts[1].size = INT2(100,44);
	bts[1].parent = this;
	bts[1].pos = INT2(size.x-100-16,size.y-60);
	
	bts2[0].id = IdBack;
	bts2[0].text = txGoBack;
	bts2[0].size = INT2(100,44);
	bts2[0].parent = this;
	bts2[0].pos = INT2(16,size.y-60);

	bts2[1].id = IdCreate;
	bts2[1].text = txCreate;
	bts2[1].size = INT2(100,44);
	bts2[1].parent = this;
	bts2[1].pos = INT2(size.x-100-16,size.y-60);

	checkbox.bt_size = INT2(32,32);
	checkbox.checked = false;
	checkbox.id = IdHardcore;
	checkbox.parent = this;
	checkbox.text = txHardcoreMode;
	checkbox.pos = INT2(20, 350);
	checkbox.size = INT2(200,32);

	{
		Slider2& s = slider[0];
		s.id = IdHair;
		s.minv = 0;
		s.maxv = MAX_HAIR-1;
		s.val = 0;
		s.text = txHair;
		s.pos = INT2(20,100);
		s.parent = this;
	}

	{
		Slider2& s = slider[1];
		s.id = IdMustache;
		s.minv = 0;
		s.maxv = MAX_MUSTACHE-1;
		s.val = 0;
		s.text = txMustache;
		s.pos = INT2(20,150);
		s.parent = this;
	}

	{
		Slider2& s = slider[2];
		s.id = IdBeard;
		s.minv = 0;
		s.maxv = MAX_BEARD-1;
		s.val = 0;
		s.text = txBeard;
		s.pos = INT2(20,200);
		s.parent = this;
	}

	{
		Slider2& s = slider[3];
		s.id = IdColor;
		s.minv = 0;
		s.maxv = n_hair_colors-1;
		s.val = 0;
		s.text = txHairColor;
		s.pos = INT2(20,250);
		s.parent = this;
	}

	{
		Slider2& s = slider[4];
		s.id = IdSize;
		s.minv = 0;
		s.maxv = 100;
		s.val = 50;
		s.text = txSize;
		s.pos = INT2(20,300);
		s.parent = this;
		s.SetHold(true);
		s.hold_val = 25.f;
	}

	lbClasses.pos = INT2(16, 73);
	lbClasses.size = INT2(198, 235);
	lbClasses.SetForceImageSize(INT2(20, 20));
	lbClasses.SetItemHeight(24);
	lbClasses.event_handler = DialogEvent(this, &CreateCharacterPanel::OnChangeClass);
	lbClasses.parent = this;

	tbClassDesc.pos = INT2(130, 335);
	tbClassDesc.size = INT2(341, 93);
	tbClassDesc.readonly = true;
	tbClassDesc.AddScrollbar();

	flow_pos = INT2(368, 73);
	flow_size = INT2(198, 235);
	flow_scroll.pos = INT2(flow_pos.x+flow_size.x+2, flow_pos.y);
	flow_scroll.size = INT2(16, flow_size.y);
	flow_scroll.total = 100;
	flow_scroll.part = 10;
}

//=================================================================================================
CreateCharacterPanel::~CreateCharacterPanel()
{
	delete unit;
}

//=================================================================================================
void CreateCharacterPanel::Draw(ControlDrawData*)
{
	// t�o
	GUI.DrawSpriteFull(tBackground, COLOR_RGBA(255,255,255,128));

	// panel
	GUI.DrawItem(tDialog, global_pos, size, COLOR_RGBA(255,255,255,222), 16);

	// napis u g�ry
	RECT rect0 = {12+pos.x, 12+pos.y, pos.x+size.x-12, 12+pos.y+72};
	GUI.DrawText(GUI.fBig, txCharacterCreation, DT_NOCLIP|DT_CENTER|DT_VCENTER, BLACK, rect0);

	// posta�
	GUI.DrawSprite(game->tChar, INT2(pos.x+228,pos.y+64));

	RECT rect;
	MATRIX mat;

	if(mode == PickClass)
	{
		lbClasses.Draw();

		// przyciski
		for(int i=0; i<2; ++i)
			bts[i].Draw();

		/*int index = 0;

		// paski atrybut�w
		for(int i=0; i<(int)Attribute::MAX; ++i)
		{
			D3DXMatrixTransformation2D(&mat, NULL, 0.f, &VEC2(180.f/256,17.f/32), NULL, 0.f, &VEC2(float(388+pos.x),float(97+pos.y+19*index)));
			RECT r = {0,0,int(float(unit->attrib[i]-35)/35*256),32};
			GUI.DrawSpriteTransformPart(game->tKlasaCecha, mat, r);
			++index;
		}

		++index;

		// paski umiej�tno�ci
		for(int i=0; i<(int)Skill::MAX; ++i)
		{
			if(unit->skill[i] > 0)
			{
				D3DXMatrixTransformation2D(&mat, NULL, 0.f, &VEC2(180.f/256, 17.f/32), NULL, 0.f, &VEC2(float(388+pos.x), float(97+pos.y+19*index)));
				RECT r = { 0, 0, int(float(unit->skill[i])/25*256), 32 };
				GUI.DrawSpriteTransformPart(game->tKlasaCecha, mat, r);
			}
			++index;
		}*/

		// warto�ci atrybut�w / umiej�tno�ci
		/*rect.left = 390+int(pos.x);
		rect.right = size.x-12+int(pos.x);
		rect.top = 96+int(pos.y);
		rect.bottom = size.y-112+int(pos.y);
		UnitData& ud = *g_classes[(int)clas].unit_data;
		LocalString s;
		for(int i=0; i<(int)Attribute::MAX; ++i)
			s += Format("%s: %d\n", g_attributes[i].name.c_str(), ud.attrib[i].x);
		s += "\n";
		SkillProfile& sp = g_skill_profiles[(int)ud.skill_profile];
		for(int i=0; i<(int)Skill::MAX; ++i)
			s += Format("%s: %d\n", g_skills[i].name.c_str(), sp.skill[i]);
		GUI.DrawText(GUI.default_font, s, 0, BLACK, rect);*/

		// opis klasy
		tbClassDesc.Draw();

		// attribute/skill flow panel
		INT2 fpos = flow_pos + global_pos;
		GUI.DrawItem(GUI.tBox, fpos, flow_size, WHITE, 8, 32);
		flow_scroll.Draw();

		rect.left = fpos.x + 2;
		rect.right = rect.left + flow_size.x - 4;
		rect.top = fpos.y + 2;
		rect.bottom = rect.top + flow_size.y - 4;

		RECT r = { rect.left, rect.top, rect.right, rect.top+20 },
			part = { 0, 0, 256, 32 };

		for(FlowItem& fi : flow_items)
		{
			r.top = fpos.y + fi.y - (int)flow_scroll.offset;
			cstring text = GetText(fi.type, fi.id);
			if(fi.type == FlowItem::Type::Section)
			{				
				r.bottom = r.top + SECTION_H;
				if(!GUI.DrawText(GUI.fBig, text, DT_SINGLELINE, BLACK, r, &rect))
					break;
			}
			else
			{
				if(fi.part > 0)
				{
					D3DXMatrixTransformation2D(&mat, NULL, 0.f, &VEC2(float(flow_size.x-4)/256, 17.f/32), NULL, 0.f, &VEC2(float(r.left), float(r.top)));
					part.right = int(fi.part*256);
					GUI.DrawSprite2(game->tKlasaCecha, &mat, &part, &rect, WHITE);
				}
				r.bottom = r.top + VALUE_H;
				if(!GUI.DrawText(GUI.default_font, text, DT_SINGLELINE, BLACK, r, &rect))
					break;
			}
		}
	}
	else
	{
		for(int i=0; i<2; ++i)
			bts2[i].Draw();

		if(!game->IsOnline())
			checkbox.Draw();

		for(int i=0; i<5; ++i)
			slider[i].Draw();
	}
}

//=================================================================================================
void CreateCharacterPanel::Update(float dt)
{
	RenderUnit();
	UpdateUnit(dt);

	// obracanie
	if(PointInRect(GUI.cursor_pos, INT2(pos.x+228,pos.y+94), INT2(128,256)) && Key.Focus() && focus)
	{
		if(Key.Down(VK_LBUTTON))
			unit->rot = clip(unit->rot - float(GUI.cursor_pos.x - pos.x - 228 - 64)/16*dt);
		//dist = clamp(dist + GUI.mouse_wheel, -3.f, -1.f);
	}

	if(mode == PickClass)
	{
		lbClasses.mouse_focus = focus;
		lbClasses.Update(dt);

		for(int i=0; i<2; ++i)
		{
			bts[i].mouse_focus = focus;
			bts[i].Update(dt);
		}

		tbClassDesc.mouse_focus = focus;
		tbClassDesc.Update(dt);

		flow_scroll.mouse_focus = focus;
		flow_scroll.Update(dt);
	}
	else
	{
		for(int i=0; i<2; ++i)
		{
			bts2[i].mouse_focus = focus;
			bts2[i].Update(dt);
		}

		if(!game->IsOnline())
		{
			checkbox.mouse_focus = focus;
			checkbox.Update(dt);
		}

		for(int i=0; i<5; ++i)
		{
			slider[i].mouse_focus = focus;
			slider[i].Update(dt);
		}
	}

	if(focus && Key.Focus() && Key.PressedRelease(VK_ESCAPE))
		Event((GuiEvent)IdCancel);
}

//=================================================================================================
void CreateCharacterPanel::Event(GuiEvent e)
{
	if(e == GuiEvent_Show || e == GuiEvent_WindowResize)
	{
		if(e == GuiEvent_Show)
		{
			visible = true;
			unit->rot = 0;
			dist = -2.5f;
		}
		pos = global_pos = (GUI.wnd_size - size)/2;
		for(int i = 0; i < 2; ++i)
		{
			bts[i].global_pos = global_pos + bts[i].pos;
			bts2[i].global_pos = global_pos + bts2[i].pos;
		}
		for(int i = 0; i < 5; ++i)
			slider[i].global_pos = global_pos + slider[i].pos;			
		checkbox.global_pos = global_pos + checkbox.pos;
		lbClasses.Event(GuiEvent_Moved);
		tbClassDesc.Move(pos);
		flow_scroll.global_pos = global_pos + flow_scroll.pos;
	}
	else if(e == GuiEvent_Close)
	{
		visible = false;
		lbClasses.Event(GuiEvent_LostFocus);
		tbClassDesc.LostFocus();
		flow_scroll.LostFocus();
	}
	else if(e == GuiEvent_LostFocus)
	{
		lbClasses.Event(GuiEvent_LostFocus);
		tbClassDesc.LostFocus();
		flow_scroll.LostFocus();
	}
	else if(e >= GuiEvent_Custom)
	{
		switch(e)
		{
		case IdCancel:
			CloseDialog();
			event(BUTTON_CANCEL);
			break;
		case IdNext:
			mode = PickAppearance;
			break;
		case IdBack:
			mode = PickClass;
			break;
		case IdCreate:
			if(enter_name)
			{
				GetTextDialogParams params(Format("%s:", txName), name);
				params.event = DialogEvent(this, &CreateCharacterPanel::OnEnterName);
				params.limit = 16;
				params.parent = this;
				GetTextDialog::Show(params);
			}
			else
			{
				CloseDialog();
				event(BUTTON_OK);
			}
			break;
		case IdHardcore:
			game->hardcore_option = checkbox.checked;
			game->cfg.Add("hardcore", game->hardcore_option ? "1" : "0");
			game->SaveCfg();
			break;
		case IdHair:
			unit->human_data->hair = slider[0].val-1;
			slider[0].text = Format("%s %d/%d", txHair, slider[0].val, slider[0].maxv);
			break;
		case IdMustache:
			unit->human_data->mustache = slider[1].val-1;
			slider[1].text = Format("%s %d/%d", txMustache, slider[1].val, slider[1].maxv);
			break;
		case IdBeard:
			unit->human_data->beard = slider[2].val-1;
			slider[2].text = Format("%s %d/%d", txBeard, slider[2].val, slider[2].maxv);
			break;
		case IdColor:
			unit->human_data->hair_color = g_hair_colors[slider[3].val];
			slider[3].text = Format("%s %d/%d", txHairColor, slider[3].val, slider[3].maxv);
			break;
		case IdSize:
			height = slider[4].val;
			unit->human_data->height = lerp(0.9f,1.1f,float(slider[4].val)/100);
			slider[4].text = Format("%s %d/%d", txSize, slider[4].val, slider[4].maxv);
			unit->human_data->ApplyScale(unit->ani->ani);
			unit->ani->need_update = true;
			break;
		}
	}
}

//=================================================================================================
void CreateCharacterPanel::InitInventory()
{
	Unit& u = *unit;
	u.ClearInventory();
	game->ParseItemScript(u, u.data->items);
	anim = DA_STOI;
	t = 1.f;
	for(int i=0; i<(int)Attribute::MAX; ++i)
		u.attrib[i] = u.data->attrib[i].x;
	SkillProfile& sp = u.data->GetSkillProfile();
	for(int i = 0; i<(int)Skill::MAX; ++i)
		u.skill[i] = sp.skill[i];
}

//=================================================================================================
void CreateCharacterPanel::OnEnterName(int id)
{
	if(id == BUTTON_OK)
	{
		CloseDialog();
		event(BUTTON_OK);
	}
}

//=================================================================================================
void CreateCharacterPanel::RenderUnit()
{
	// rysuj obrazek
	HRESULT hr = game->device->TestCooperativeLevel();
	if(hr != D3D_OK)
		return;

	game->SetAlphaBlend(false);
	game->SetAlphaTest(false);
	game->SetNoCulling(false);
	game->SetNoZWrite(false);

	// ustaw render target
	SURFACE surf = NULL;
	if(game->sChar)
		V( game->device->SetRenderTarget(0, game->sChar) );
	else
	{
		V( game->tChar->GetSurfaceLevel(0, &surf) );
		V( game->device->SetRenderTarget(0, surf) );
	}

	// pocz�tek renderowania
	V( game->device->Clear(0, NULL, D3DCLEAR_ZBUFFER | D3DCLEAR_TARGET, 0, 1.f, 0) );
	V( game->device->BeginScene() );

	static vector<Lights> lights;

	game->SetOutsideParams();

	MATRIX matView, matProj;
	D3DXMatrixLookAtLH(&matView, &VEC3(0.f,2.f,dist), &VEC3(0.f,1.f,0.f), &VEC3(0,1,0));
	D3DXMatrixPerspectiveFovLH(&matProj, PI/4, 0.5f, 1.f, 5.f);
	game->tmp_matViewProj = matView * matProj;
	D3DXMatrixInverse(&game->tmp_matViewInv, NULL, &matView);

	game->camera_frustum.Set(game->tmp_matViewProj);
	game->ListDrawObjectsUnit(NULL, game->camera_frustum, true, *unit);
	game->DrawSceneNodes(game->draw_batch.nodes, lights, true);
	game->draw_batch.Clear();

	// koniec renderowania
	V( game->device->EndScene() );

	// kopiuj je�li jest mipmaping
	if(game->sChar)
	{
		V( game->tChar->GetSurfaceLevel(0, &surf) );
		V( game->device->StretchRect(game->sChar, NULL, surf, NULL, D3DTEXF_NONE) );
	}
	surf->Release();

	// przywr�c poprzedni render target
	V( game->device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &surf) );
	V( game->device->SetRenderTarget(0, surf) );
	surf->Release();
}

//=================================================================================================
void CreateCharacterPanel::UpdateUnit(float dt)
{
	// aktualizacja postaci	
	t -= dt;
	if(t <= 0.f)
	{
		switch(anim)
		{
		case DA_ATAK:
		case DA_SCHOWAJ_BRON:
		case DA_SCHOWAJ_LUK:
		case DA_STRZAL:
		case DA_WYJMIJ_BRON:
		case DA_WYJMIJ_LUK:
		case DA_ROZGLADA:
			assert(0);
			break;
		case DA_BLOK:
			if(rand2()%2 == 0)
				anim = DA_ATAK;
			else
				anim = DA_BOJOWY;
			break;
		case DA_BOJOWY:
			if(unit->wyjeta == B_JEDNORECZNA)
			{
				int co = rand2()%(unit->HaveShield() ? 3 : 2);
				if(co == 0)
					anim = DA_ATAK;
				else if(co == 1)
					anim = DA_SCHOWAJ_BRON;
				else
					anim = DA_BLOK;
			}
			else
			{
				if(rand2()%2 == 0)
					anim = DA_STRZAL;
				else
					anim = DA_SCHOWAJ_LUK;
			}
			break;
		case DA_STOI:
		case DA_IDZIE:
			{
				int co = rand2()%(unit->HaveBow() ? 5 : 4);
				if(co == 0)
					anim = DA_ROZGLADA;
				else if(co == 1)
					anim = DA_STOI;
				else if(co == 2)
					anim = DA_IDZIE;
				else if(co == 3)
					anim = DA_WYJMIJ_BRON;
				else
					anim = DA_WYJMIJ_LUK;
			}
			break;
		default:
			assert(0);
			break;
		}
	}

	if(anim != anim2)
	{
		anim2 = anim;
		switch(anim)
		{
		case DA_ATAK:
			unit->attack_id = unit->GetRandomAttack();
			unit->ani->Play(NAMES::ani_attacks[unit->attack_id], PLAY_PRIO2|PLAY_ONCE, 0);
			unit->ani->groups[0].speed = unit->GetAttackSpeed();
			unit->etap_animacji = 0;
			t = 100.f;
			unit->ani->frame_end_info = false;
			break;
		case DA_BLOK:
			unit->ani->Play(NAMES::ani_block, PLAY_PRIO2, 0);
			t = 1.f;
			break;
		case DA_BOJOWY:
			unit->ani->Play(NAMES::ani_battle, PLAY_PRIO2, 0);
			t = 1.f;
			break;
		case DA_IDZIE:
			unit->ani->Play(NAMES::ani_move, PLAY_PRIO2, 0);
			t = 2.f;
			break;
		case DA_ROZGLADA:
			unit->ani->Play("rozglada", PLAY_PRIO2|PLAY_ONCE, 0);
			t = 100.f;
			unit->ani->frame_end_info = false;
			break;
		case DA_SCHOWAJ_BRON:
			unit->ani->Play(unit->GetTakeWeaponAnimation(true), PLAY_PRIO2|PLAY_ONCE|PLAY_BACK, 0);
			unit->ani->groups[1].speed = 1.f;
			t = 100.f;
			unit->etap_animacji = 0;
			unit->stan_broni = BRON_CHOWA;
			unit->wyjeta = B_BRAK;
			unit->chowana = B_JEDNORECZNA;
			unit->ani->frame_end_info = false;
			break;
		case DA_SCHOWAJ_LUK:
			unit->ani->Play(NAMES::ani_take_bow, PLAY_PRIO2|PLAY_ONCE|PLAY_BACK, 0);
			unit->ani->groups[1].speed = 1.f;
			t = 100.f;
			unit->etap_animacji = 0;
			unit->stan_broni = BRON_CHOWA;
			unit->wyjeta = B_BRAK;
			unit->chowana = B_LUK;
			unit->ani->frame_end_info = false;
			break;
		case DA_STOI:
			unit->ani->Play(NAMES::ani_stand, PLAY_PRIO2, 0);
			t = 2.f;
			break;
		case DA_STRZAL:
			unit->ani->Play(NAMES::ani_shoot, PLAY_PRIO2|PLAY_ONCE, 0);
			unit->ani->groups[0].speed = unit->GetBowAttackSpeed();
			unit->etap_animacji = 0;
			t = 100.f;
			if(game->bow_instances.empty())
				unit->bow_instance = new AnimeshInstance(unit->GetBow().ani);
			else
			{
				unit->bow_instance = game->bow_instances.back();
				game->bow_instances.pop_back();
				unit->bow_instance->ani = unit->GetBow().ani;
			}
			unit->bow_instance->Play(&unit->bow_instance->ani->anims[0], PLAY_ONCE|PLAY_PRIO1|PLAY_NO_BLEND, 0);
			unit->bow_instance->groups[0].speed = unit->ani->groups[0].speed;
			unit->ani->frame_end_info = false;
			break;
		case DA_WYJMIJ_BRON:
			unit->ani->Play(unit->GetTakeWeaponAnimation(true), PLAY_PRIO2|PLAY_ONCE, 0);
			unit->ani->groups[1].speed = 1.f;
			t = 100.f;
			unit->etap_animacji = 0;
			unit->stan_broni = BRON_WYJMUJE;
			unit->wyjeta = B_JEDNORECZNA;
			unit->chowana = B_BRAK;
			unit->ani->frame_end_info = false;
			break;
		case DA_WYJMIJ_LUK:
			unit->ani->Play(NAMES::ani_take_bow, PLAY_PRIO2|PLAY_ONCE, 0);
			unit->ani->groups[1].speed = 1.f;
			t = 100.f;
			unit->etap_animacji = 0;
			unit->stan_broni = BRON_WYJMUJE;
			unit->wyjeta = B_LUK;
			unit->chowana = B_BRAK;
			unit->ani->frame_end_info = false;
			break;
		default:
			assert(0);
			break;
		}
	}

	switch(anim)
	{
	case DA_ATAK:
		if(unit->ani->frame_end_info)
		{
			unit->ani->groups[0].speed = 1.f;
			if(rand2()%2==0)
			{
				anim = DA_ATAK;
				anim2 = DA_STOI;
			}
			else
				anim = DA_BOJOWY;
		}
		break;
	case DA_SCHOWAJ_BRON:
	case DA_SCHOWAJ_LUK:
		if(unit->etap_animacji == 0 && (unit->ani->GetProgress() <= unit->data->frames->t[F_TAKE_WEAPON] || unit->ani->frame_end_info))
			unit->etap_animacji = 1;
		if(unit->ani->frame_end_info)
		{
			unit->stan_broni = BRON_SCHOWANA;
			unit->chowana = B_BRAK;
			anim = DA_STOI;
			t = 1.f;
		}
		break;
	case DA_STRZAL:
		if(unit->ani->GetProgress() > 20.f/40 && unit->etap_animacji < 2)
			unit->etap_animacji = 2;
		else if(unit->ani->GetProgress() > 35.f/40)
		{
			unit->etap_animacji = 3;
			if(unit->ani->frame_end_info)
			{
				assert(unit->bow_instance);
				game->bow_instances.push_back(unit->bow_instance);
				unit->bow_instance = NULL;
				unit->ani->groups[0].speed = 1.f;
				if(rand2()%2 == 0)
				{
					anim = DA_STRZAL;
					anim2 = DA_STOI;
				}
				else
					anim = DA_BOJOWY;
				break;
			}
		}
		unit->bow_instance->groups[0].time = min(unit->ani->groups[0].time, unit->bow_instance->groups[0].anim->length);
		unit->bow_instance->need_update = true;
		break;
	case DA_WYJMIJ_BRON:
		if(unit->etap_animacji == 0 && (unit->ani->GetProgress() >= unit->data->frames->t[F_TAKE_WEAPON] || unit->ani->frame_end_info))
			unit->etap_animacji = 1;
		if(unit->ani->frame_end_info)
		{
			unit->stan_broni = BRON_WYJETA;
			anim = DA_ATAK;
		}
		break;
	case DA_WYJMIJ_LUK:
		if(unit->etap_animacji == 0 && (unit->ani->GetProgress() >= unit->data->frames->t[F_TAKE_WEAPON] || unit->ani->frame_end_info))
			unit->etap_animacji = 1;
		if(unit->ani->frame_end_info)
		{
			unit->stan_broni = BRON_WYJETA;
			anim = DA_STRZAL;
		}
		break;
	case DA_ROZGLADA:
		if(unit->ani->frame_end_info)
			anim = DA_STOI;
		break;
	case DA_BLOK:
	case DA_BOJOWY:
	case DA_STOI:
	case DA_IDZIE:
		break;
	default:
		assert(0);
		break;
	}

	unit->ani->Update(dt);
}

//=================================================================================================
void CreateCharacterPanel::Random(Class _clas)
{
	mode = PickClass;
	if(_clas == Class::RANDOM)
		clas = ClassInfo::GetRandomPlayer();
	else
		clas = _clas;
	ClassInfo& ci = g_classes[(int)clas];
	lbClasses.Select(lbClasses.FindIndex((int)clas));
	tbClassDesc.text = ci.desc;
	tbClassDesc.UpdateScrollbar();
	Unit& u = *unit;
	u.data = ci.unit_data;
	u.human_data->beard = rand2()%MAX_BEARD-1;
	u.human_data->hair = rand2()%MAX_HAIR-1;
	u.human_data->mustache = rand2()%MAX_MUSTACHE-1;
	int id = rand2()%n_hair_colors;
	u.human_data->hair_color = g_hair_colors[id];
	u.human_data->height = random(0.95f,1.05f);
	u.human_data->ApplyScale(game->aHumanBase);
	anim = anim2 = DA_STOI;
	u.ani->Play(NAMES::ani_stand, PLAY_PRIO2|PLAY_NO_BLEND, 0);
	slider[0].val = u.human_data->hair+1;
	slider[0].text = Format("%s %d/%d", txHair, slider[0].val, slider[0].maxv);
	slider[1].val = u.human_data->mustache+1;
	slider[1].text = Format("%s %d/%d", txMustache, slider[1].val, slider[1].maxv);
	slider[2].val = u.human_data->beard+1;
	slider[2].text = Format("%s %d/%d", txBeard, slider[2].val, slider[2].maxv);
	slider[3].val = id;
	slider[3].text = Format("%s %d/%d", txHairColor, slider[3].val, slider[3].maxv);
	slider[4].val = int((u.human_data->height-0.9f)*500);
	height = slider[4].val;
	slider[4].text = Format("%s %d/%d", txSize, slider[4].val, slider[4].maxv);
	InitInventory();
}

//=================================================================================================
void CreateCharacterPanel::Redo(Class _clas, HumanData& hd)
{
	clas = _clas;
	ClassInfo& ci = g_classes[(int)clas];
	unit->data = ci.unit_data;
	InitInventory();
	hd.Set(*unit->human_data);
	lbClasses.Select(lbClasses.FindIndex((int)clas));
	tbClassDesc.text = ci.desc;
	tbClassDesc.UpdateScrollbar();
}

//=================================================================================================
void CreateCharacterPanel::Init()
{
	unit->ani = new AnimeshInstance(game->aHumanBase);
	
	for(ClassInfo& ci : g_classes)
	{
		if(ci.IsPickable())
			lbClasses.Add(new DefaultGuiElement(ci.name.c_str(), (int)ci.class_id, ci.icon));
	}
	lbClasses.Sort();
	lbClasses.Init();
}

//=================================================================================================
void CreateCharacterPanel::OnChangeClass(int index)
{
	clas = (Class)lbClasses.GetItem()->value;
	ClassInfo& ci = g_classes[(int)clas];
	unit->data = ci.unit_data;
	InitInventory();
	tbClassDesc.text = ci.desc;
	tbClassDesc.UpdateScrollbar();

	flow_items.clear();

	int y = 0;

	// attributes
	flow_items.push_back(FlowItem(-1, y));
	y += SECTION_H;
	for(int i = 0; i<(int)Attribute::MAX; ++i)
	{
		flow_items.push_back(FlowItem(FlowItem::Type::Attribute, i, 35, 70, ci.unit_data->attrib[i].x, y));
		y += VALUE_H;
	}

	// skills
	SkillGroup group = SkillGroup::NONE;
	SkillProfile& profile = ci.unit_data->GetSkillProfile();
	for(int i = 0; i<(int)Skill::MAX; ++i)
	{
		if(profile.skill[i] >= 0)
		{
			SkillInfo& si = g_skills[i];
			if(si.group != group)
			{
				// start new section
				flow_items.push_back(FlowItem((int)si.group, y));
				y += SECTION_H;
				group = si.group;
			}
			flow_items.push_back(FlowItem(FlowItem::Type::Skill, i, 0, 20, profile.skill[i], y));
			y += VALUE_H;
		}
	}
	flow_scroll.total = y;
	flow_scroll.part = flow_scroll.size.y;
	flow_scroll.offset = 0.f;
}

//=================================================================================================
cstring CreateCharacterPanel::GetText(FlowItem::Type type, int id)
{
	switch(type)
	{
	case FlowItem::Type::Section:
		if(id == -1)
			return "Atrybuty";
		else
			return g_skill_groups[id].name.c_str();
	case FlowItem::Type::Attribute:
		return Format("%s: %d", g_attributes[id].name.c_str(), unit->attrib[id]);
	case FlowItem::Type::Skill:
		return Format("%s: %d", g_skills[id].name.c_str(), unit->skill[id]);
	default:
		return "MISSING";
	}
}
