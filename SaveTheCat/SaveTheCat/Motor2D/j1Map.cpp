#include "p2Defs.h"
#include "p2Log.h"
#include "j1App.h"
#include "j1Render.h"
#include "j1Textures.h"
#include "j1Map.h"
#include "j1Collision.h"
#include "j1Window.h"
#include "j1Player.h"
#include "j1Scene.h"
#include "j1Pathfinding.h"
#include <math.h>

j1Map::j1Map() : j1Module(), map_loaded(false)
{
	name.create("map");
}

// Destructor
j1Map::~j1Map()
{}

// Called before render is available
bool j1Map::Awake(pugi::xml_node& config)
{
	LOG("Loading Map Parser");
	bool ret = true;
	App->collision->AddCollider({ 0,0,1,1000 }, COLLIDER_WALL);
	folder.create(config.child("folder").child_value());
	level_1_player_x = config.child("level1").child("player_position").attribute("x").as_int();
	level_1_player_y = config.child("level1").child("player_position").attribute("y").as_int();
	level_2_player_x = config.child("level1").child("player_position").attribute("x").as_int();
	level_2_player_y = config.child("level1").child("player_position").attribute("y").as_int();

	level_1_bat_x = config.child("level1").child("bat_position").attribute("x").as_int();
	level_1_bat_x = config.child("level1").child("bat_position").attribute("y").as_int();
	level_2_bat_x = config.child("level2").child("bat_position").attribute("x").as_int();
	level_2_bat_x = config.child("level2").child("bat_position").attribute("y").as_int();
	return ret;
}

void j1Map::Draw()
{
	BROFILER_CATEGORY("Update_MapDraw", Profiler::Color::CadetBlue)
	if(map_loaded == false)
		return;
	else
	{
		p2List_item<MapLayer*>* layer;
		p2List_item<TileSet*>* tileset;
		layer = data.layers.start;
		tileset = data.tilesets.start;
		iPoint position;

		while (tileset != NULL)
		{
			while (layer != NULL)
			{
				for (int y = 0; y < layer->data->height; y++)
				{
					for (int x = 0; x < layer->data->width; x++)
					{
						position = MapToWorld(x, y-map_offset);
						App->render->Blit(tileset->data->texture, position.x, position.y, &tileset->data->GetRect(layer->data->tile_gid[layer->data->Get(x, y)]));
					}
				}
				layer = layer->next;
			}
			tileset = tileset->next;
		}
	}
}

// Called before quitting
bool j1Map::CleanUp()
{
	LOG("Unloading map");

	// Remove all tilesets
	p2List_item<TileSet*>* item;
	item = data.tilesets.start;

	while(item != NULL)
	{
		RELEASE(item->data);
		item = item->next;
	}
	data.tilesets.clear();

	//Remove all layers

	p2List_item<MapLayer*>* item2;
	item2 = data.layers.start;

	while (item2 != NULL)
	{
		RELEASE(item2->data);
		item2 = item2->next;
	}
	data.layers.clear();

	//Remove all colliders

	p2List_item<ObjectGroup*>* item3;
	item3 = data.objectgroups.start;
	while (item3 != NULL)
	{
		LOG("Objectgroups releasing");
		try
		{
			for (uint i = 0; i < item3->data->size; i++)
			{
				while ((item3->data->collider[i] != nullptr) && (item3->data->collider[i]->to_delete == false))
				{
					item3->data->collider[i]->to_delete = true;
					item3->data->collider[i] = nullptr;
				}
			}
		}
		catch (EXCEPINFO e)
		{
			return false;
		}
		
		delete[] item3->data->collider;
		RELEASE(item3->data);
		item3 = item3->next;
	}
	data.objectgroups.clear();

	// Clean up the pugui tree
	map_file.reset();

	return true;
}

// Load new map
bool j1Map::Load(const char* file_name)
{
	bool ret = true;
	p2SString tmp("%s%s", folder.GetString(), file_name);

	pugi::xml_parse_result result = map_file.load_file(tmp.GetString());


	if(result == NULL)
	{
		LOG("Could not load map xml file %s. pugi error: %s", file_name, result.description());
		ret = false;
	}

	if (file_name == "Level1.tmx") {
		App->scene->current_level = LEVEL_1;
		App->scene->player_x_position = level_1_player_x;
		App->scene->player_y_position = level_1_player_y;

		App->scene->backTex = App->tex->Load("sprites/parallax/back1.png");
		App->scene->farTex = App->tex->Load("sprites/parallax/far1.png");
		App->scene->midTex = App->tex->Load("sprites/parallax/mid1.png");
		App->scene->closeTex = App->tex->Load("sprites/parallax/close1.png");
		App->scene->backPos.x = 0; App->scene->backPos.y = -App->render->camera.y;
		App->scene->farPos.x = 0; App->scene->farPos.y = -App->render->camera.y;
		App->scene->midPos.x = 0; App->scene->midPos.y = -App->render->camera.y;
		App->scene->closePos.x = 0; App->scene->closePos.y = -App->render->camera.y;
	}
	if (file_name == "Level2.tmx") {
		App->scene->current_level = LEVEL_2; 
		App->scene->player_x_position = level_2_player_x;
		App->scene->player_y_position = level_2_player_y;

		App->scene->backTex = App->tex->Load("sprites/parallax/back2.png");
		App->scene->farTex = App->tex->Load("sprites/parallax/far2.png");
		App->scene->midTex = App->tex->Load("sprites/parallax/mid2.png");
		App->scene->closeTex = App->tex->Load("sprites/parallax/close2.png");
		App->scene->backPos.x = 0; App->scene->backPos.y = -App->render->camera.y;
		App->scene->farPos.x = 0; App->scene->farPos.y = -App->render->camera.y;
		App->scene->midPos.x = 0; App->scene->midPos.y = -App->render->camera.y;
		App->scene->closePos.x = 0; App->scene->closePos.y = -App->render->camera.y;
	}

	int w, h;
	uchar* data2 = NULL;
	if (App->map->CreateWalkabilityMap(w, h, &data2)) App->pathfinding->SetMap(w, h, data2);
	RELEASE_ARRAY(data2);

	// Load general info ----------------------------------------------
	if(ret == true)
	{
		ret = LoadMap();
	}

	//load map's player info

	data.player_initial_x = map_file.child("map").child("player").child("position").attribute("x").as_int();
	data.player_initial_y = map_file.child("map").child("player").child("position").attribute("y").as_int();
	LOG("Player position x: %i y: %i", data.player_initial_x, data.player_initial_y);

	// Load all tilesets info ----------------------------------------------
	pugi::xml_node tileset;
	for(tileset = map_file.child("map").child("tileset"); tileset && ret; tileset = tileset.next_sibling("tileset"))
	{
		TileSet* set = new TileSet();

		if(ret == true)
		{
			ret = LoadTilesetDetails(tileset, set);
		}

		if(ret == true)
		{
			ret = LoadTilesetImage(tileset, set);
		}

		data.tilesets.add(set);
	}

	// Load layer info ----------------------------------------------
	pugi::xml_node layer;
	for (layer = map_file.child("map").child("layer"); layer && ret; layer = layer.next_sibling("layer"))
	{
		MapLayer* set = new MapLayer();

		if (ret == true)
		{
			ret = LoadLayer(layer, set);
		}
		
		data.layers.add(set);
	}

	//Load objectgroup info -------------------------------------
	
	pugi::xml_node objectgroup;
	for (objectgroup = map_file.child("map").child("objectgroup"); objectgroup && ret; objectgroup = objectgroup.next_sibling("objectgroup"))
	{
		ObjectGroup* set = new ObjectGroup();

		if (ret == true) 
		{
			ret = LoadObjectGroup(objectgroup, set);
		}
		data.objectgroups.add(set);
	}

	if(ret == true)
	{
		LOG("Successfully parsed map XML file: %s", file_name);
		LOG("width: %d height: %d", data.width, data.height);
		LOG("tile_width: %d tile_height: %d", data.tile_width, data.tile_height);

		p2List_item<TileSet*>* item = data.tilesets.start;
		while(item != NULL)
		{
			TileSet* s = item->data;
			LOG("Tileset ----");
			LOG("name: %s firstgid: %d", s->name.GetString(), s->firstgid);
			LOG("tile width: %d tile height: %d", s->tile_width, s->tile_height);
			LOG("spacing: %d margin: %d", s->spacing, s->margin);
			item = item->next;
		}

		p2List_item<MapLayer*>* item_layer = data.layers.start;
		while(item_layer != NULL)
		{
			MapLayer* l = item_layer->data;
			LOG("Layer ----");
			LOG("name: %s", l->name.GetString());
			LOG("tile width: %d tile height: %d", l->width, l->height);
			item_layer = item_layer->next;
		}

		p2List_item<ObjectGroup*>* item_object = data.objectgroups.start;
		while (item_object != NULL)
		{
			ObjectGroup* o = item_object->data;
			LOG("ObjectGroup ----");
			LOG("name: %s", o->name.GetString());
			item_object = item_object->next;
		}
		
	}

	map_loaded = ret;

	return ret;
}

// Load map general properties
bool j1Map::LoadMap()
{
	bool ret = true;
	pugi::xml_node map = map_file.child("map");

	if(map == NULL)
	{
		LOG("Error parsing map xml file: Cannot find 'map' tag.");
		ret = false;
	}
	else
	{
		data.width = map.attribute("width").as_int();
		data.height = map.attribute("height").as_int();
		data.tile_width = map.attribute("tilewidth").as_int();
		data.tile_height = map.attribute("tileheight").as_int();
		p2SString bg_color(map.attribute("backgroundcolor").as_string());

		data.background_color.r = 0;
		data.background_color.g = 0;
		data.background_color.b = 0;
		data.background_color.a = 0;

		if(bg_color.Length() > 0)
		{
			p2SString red, green, blue;
			bg_color.SubString(1, 2, red);
			bg_color.SubString(3, 4, green);
			bg_color.SubString(5, 6, blue);

			int v = 0;

			sscanf_s(red.GetString(), "%x", &v);
			if(v >= 0 && v <= 255) data.background_color.r = v;

			sscanf_s(green.GetString(), "%x", &v);
			if(v >= 0 && v <= 255) data.background_color.g = v;

			sscanf_s(blue.GetString(), "%x", &v);
			if(v >= 0 && v <= 255) data.background_color.b = v;
		}

		p2SString orientation(map.attribute("orientation").as_string());

		if(orientation == "orthogonal")
		{
			data.type = MAPTYPE_ORTHOGONAL;
		}
		else if(orientation == "isometric")
		{
			data.type = MAPTYPE_ISOMETRIC;
		}
		else if(orientation == "staggered")
		{
			data.type = MAPTYPE_STAGGERED;
		}
		else
		{
			data.type = MAPTYPE_UNKNOWN;
		}
	}

	return ret;
}

bool j1Map::LoadTilesetDetails(pugi::xml_node& tileset_node, TileSet* set)
{
	bool ret = true;
	set->name.create(tileset_node.attribute("name").as_string());
	set->firstgid = tileset_node.attribute("firstgid").as_int();
	set->tile_width = tileset_node.attribute("tilewidth").as_int();
	set->tile_height = tileset_node.attribute("tileheight").as_int();
	set->margin = tileset_node.attribute("margin").as_int();
	set->spacing = tileset_node.attribute("spacing").as_int();
	pugi::xml_node offset = tileset_node.child("tileoffset");

	if(offset != NULL)
	{
		set->offset_x = offset.attribute("x").as_int();
		set->offset_y = offset.attribute("y").as_int();
	}
	else
	{
		set->offset_x = 0;
		set->offset_y = 0;
	}

	return ret;
}

bool j1Map::LoadTilesetImage(pugi::xml_node& tileset_node, TileSet* set)
{
	bool ret = true;
	pugi::xml_node image = tileset_node.child("image");

	if(image == NULL)
	{
		LOG("Error parsing tileset xml file: Cannot find 'image' tag.");
		ret = false;
	}
	else
	{
		set->texture = App->tex->Load(PATH(folder.GetString(), image.attribute("source").as_string()));
		int w, h;
		SDL_QueryTexture(set->texture, NULL, NULL, &w, &h);
		set->tex_width = image.attribute("width").as_int();

		if(set->tex_width <= 0)
		{
			set->tex_width = w;
		}

		set->tex_height = image.attribute("height").as_int();

		if(set->tex_height <= 0)
		{
			set->tex_height = h;
		}

		set->num_tiles_width = set->tex_width / set->tile_width;
		set->num_tiles_height = set->tex_height / set->tile_height;
	}

	return ret;
}

bool j1Map::LoadLayer(pugi::xml_node& node, MapLayer* layer)
{
	bool ret = true;
	uint size = 0u;
	pugi::xml_node tile = node.child("data").child("tile");
	

	if (tile == NULL)
	{
		LOG("Error loading layer");
		ret = false;
	}

	else
	{
		layer->name = node.attribute("name").as_string();
		layer->width = node.attribute("width").as_uint();
		layer->height = node.attribute("height").as_uint();

		size = layer->width * layer->height * sizeof(uint);
		layer->tile_gid = new uint[layer->width * layer->height];

		memset(layer->tile_gid, 0, size);

		for (uint i = 0u; i < layer->width * layer->height; i++)
		{
			layer->tile_gid[i] = tile.attribute("gid").as_uint();
			tile = tile.next_sibling("tile");
		}
	}
	return ret;
}

bool j1Map::LoadObjectGroup(pugi::xml_node& node, ObjectGroup* objectgroup) {
	bool ret = true;
	pugi::xml_node object = node.child("object");
	SDL_Rect rect = { 0,0,0,0 };
	objectgroup->name = node.attribute("name").as_string();
	uint i = 0u;
	p2SString type;

		if (object == NULL)
		{
			LOG("Error loading object group");
			ret = false;
		}
		
		else
		{
			objectgroup->object = new SDL_Rect[MAX_COLLIDERS];
			objectgroup->collider = new Collider*[MAX_COLLIDERS];

			while (object != NULL)
			{
				objectgroup->object[i].x = object.attribute("x").as_int();
				objectgroup->object[i].y = object.attribute("y").as_int();
				objectgroup->object[i].w = object.attribute("width").as_int();
				objectgroup->object[i].h = object.attribute("height").as_int();
				objectgroup->object[i].y -= COLLIDER_OFFSET;
				
				p2SString type(object.attribute("type").as_string());

				if (type =="Collider")
				objectgroup->collider[i] = App->collision->AddCollider(objectgroup->object[i], COLLIDER_WALL);

				if (type == "Checkpoint")
					objectgroup->collider[i] = App->collision->AddCollider(objectgroup->object[i], COLLIDER_CHECKPOINT);

				if (type == "Death")
				objectgroup->collider[i] = App->collision->AddCollider(objectgroup->object[i], COLLIDER_DEATH);

				if (type == "Level")
					objectgroup->collider[i] = App->collision->AddCollider(objectgroup->object[i], COLLIDER_LEVEL);

				object = object.next_sibling("object");
				i++;
			}
			objectgroup->size = i;
		}

	return ret;
}

int Properties::Get(const char* value, int default_value) const
{
	p2List_item<Property*>* item = list.start;

	while (item)
	{
		if (item->data->name == value)
			return item->data->value;
		item = item->next;
	}

	return default_value;
}

SDL_Rect TileSet::GetRect(int tile_id) {
		SDL_Rect rect;
		tile_id = tile_id - firstgid;
		rect.x = margin + ((tile_width + spacing) * (tile_id % num_tiles_width));
		rect.y = margin + ((tile_height + spacing) * (tile_id / num_tiles_width));
		rect.w = tile_width;
		rect.h = tile_height;
		return rect;
}

iPoint j1Map::MapToWorld(int x, int y) const
{
	iPoint ret(0, 0);

	if (data.type == MAPTYPE_ORTHOGONAL) {
		ret.x = x * data.tile_width;
		ret.y = y * data.tile_height;
	}

	if (data.type == MAPTYPE_ISOMETRIC)
	{
		ret.x = (x - y) * (data.tile_width * 0.5f);
		ret.y = (x + y) * (data.tile_height * 0.5f);
	}
	return ret;
}

iPoint j1Map::WorldToMap(int x, int y) const
{
	iPoint ret(0, 0);
	if (data.type == MAPTYPE_ORTHOGONAL) {
		ret.x = x / data.tile_width;
		ret.y = y / data.tile_height;
	}

	if (data.type == MAPTYPE_ISOMETRIC)
	{
		ret.x = x / data.tile_width + y / data.tile_height;
		ret.y = y / data.tile_height - x / data.tile_width;
	}

	return ret;
}

TileSet* j1Map::GetTilesetFromTileId(int id) const
{
	p2List_item<TileSet*>* item = data.tilesets.start;
	TileSet* set = item->data;

	while (item)
	{
		if (id < item->data->firstgid)
		{
			set = item->prev->data;
			break;
		}
		set = item->data;
		item = item->next;
	}

	return set;
}

// Load a group of properties from a node and fill a list with it
bool j1Map::LoadProperties(pugi::xml_node& node, Properties& properties)
{
	bool ret = false;

	pugi::xml_node data = node.child("properties");

	if (data != NULL)
	{
		pugi::xml_node prop;

		for (prop = data.child("property"); prop; prop = prop.next_sibling("property"))
		{
			Properties::Property* p = new Properties::Property();

			p->name = prop.attribute("name").as_string();
			p->value = prop.attribute("value").as_int();

			properties.list.add(p);
		}
	}

	return ret;
}

bool j1Map::CreateWalkabilityMap(int& width, int& height, uchar** buffer) const
{
	bool ret = false;
	p2List_item<MapLayer*>* item;
	item = data.layers.start;

	for (item = data.layers.start; item != NULL; item = item->next)
	{
		MapLayer* layer = item->data;

		if (layer->properties.Get("Navigation", 0) == 0)
			continue;

		uchar* map = new uchar[layer->width * layer->height];
		memset(map, 1, layer->width * layer->height);

		for (int y = 0; y < data.height; ++y)
		{
			for (int x = 0; x < data.width; ++x)
			{
				int i = (y * layer->width) + x;

				int tile_id = layer->Get(x, y);
				TileSet* tileset = (tile_id > 0) ? GetTilesetFromTileId(tile_id) : NULL;

				if (tileset != NULL)
				{
					map[i] = (tile_id - tileset->firstgid) > 0 ? 0 : 1;
					
				}
			}
		}

		*buffer = map;
		width = data.width;
		height = data.height;
		ret = true;

		break;
	}

	return ret;
}