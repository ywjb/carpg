#pragma once

//-----------------------------------------------------------------------------
class AbilityLoader;
class BuildingLoader;
class ClassLoader;
class DialogLoader;
class ItemLoader;
class ObjectLoader;
class QuestLoader;
class RequiredLoader;
class UnitLoader;

//-----------------------------------------------------------------------------
class Content
{
public:
	enum class Id
	{
		Items,
		Objects,
		Abilities,
		Dialogs,
		Classes,
		Units,
		Buildings,
		Musics,
		Quests,
		Required,

		Max
	};

	Content();
	~Content();
	void Cleanup();
	void LoadContent(delegate<void(Id)> callback);
	void LoadVersion();
	void CleanupContent();
	void WriteCrc(BitStreamWriter& f);
	void ReadCrc(BitStreamReader& f);
	bool GetCrc(Id type, uint& my_crc, cstring& type_crc);
	bool ValidateCrc(Id& type, uint& my_crc, uint& player_crc, cstring& type_str);

	string system_dir;
	uint errors;
	uint warnings;
	uint crc[(int)Id::Max];
	uint client_crc[(int)Id::Max];
	uint version;
	bool require_update;

private:
	AbilityLoader* ability_loader;
	BuildingLoader* building_loader;
	ClassLoader* class_loader;
	DialogLoader* dialog_loader;
	ItemLoader* item_loader;
	ObjectLoader* object_loader;
	QuestLoader* quest_loader;
	RequiredLoader* required_loader;
	UnitLoader* unit_loader;
};

extern Content content;
