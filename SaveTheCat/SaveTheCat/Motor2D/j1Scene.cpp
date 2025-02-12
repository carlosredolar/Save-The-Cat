#include "p2Defs.h"
#include "p2Log.h"
#include "j1App.h"
#include "j1Input.h"
#include "j1Textures.h"
#include "j1Audio.h"
#include "j1Render.h"
#include "j1Window.h"
#include "j1Map.h"
#include "j1Scene.h"
#include "j1Player.h"
#include "j1Bat.h"
#include "j1Collision.h"
#include "j1Pathfinding.h"

j1Scene::j1Scene() : j1Module()
{
	name.create("scene");
	debugPath = false;
}

// Destructor
j1Scene::~j1Scene()
{}

// Called before render is available
bool j1Scene::Awake()
{
	LOG("Loading Scene");
	bool ret = true;

	return ret;
}

// Called before the first frame
bool j1Scene::Start()
{
	
	if (App->render->camera.y < 1)
	{
		top_edge = App->render->camera.y + App->render->camera.h / 4;
		bottom_edge = App->render->camera.y + App->render->camera.h * 3 / 4;
	}
	
	
	container = new SDL_Rect{0,0,8000,1000};
	cam_death = SDL_Rect{ 0,0,10,App->render->camera.h*2 };
	farTimer = 0;
	midTimer = 0;
	closeTimer = 0;

	
	if (App->map->Load("Level1.tmx") == true)
	{
		int w, h;
		uchar* data = NULL;
		if (App->map->CreateWalkabilityMap(w, h, &data))
			App->pathfinding->SetMap(w, h, data);

		RELEASE_ARRAY(data);
	}
	
	App->render->camera.y = 0;
	speedCount = 0;

	debug_tex = App->tex->Load("maps/path.png");

	collider = App->collision->AddCollider(cam_death, COLLIDER_DEATH, (j1Module*)App->scene);

	return true;
}

// Called each loop iteration
bool j1Scene::PreUpdate() 
{ 
	BROFILER_CATEGORY("PreUpdate_Scene", Profiler::Color::AliceBlue)
	
	// debug pathfing ------------------
		static iPoint origin;
	static bool origin_selected = false;

	int x, y;
	App->input->GetMousePosition(x, y);
	iPoint p = App->render->ScreenToWorld(x, y);
	p = App->map->WorldToMap(p.x, p.y);

	if (App->input->GetMouseButtonDown(SDL_BUTTON_LEFT) == KEY_DOWN)
	{
		if (origin_selected == true)
		{

			App->pathfinding->CreatePath(origin, p);
			origin_selected = false;
		}
		else
		{
			origin = p;
			origin_selected = true;
		}
	}
	return true; 
}

// Called each loop iteration
bool j1Scene::Update(float dt)
{
	BROFILER_CATEGORY("Update_Scene", Profiler::Color::AntiqueWhite)
	iPoint* player_position = &App->player->position;

	cam_run_start = SDL_GetTicks();

	if (cam_run_start > cam_run_start_timer && App->render->camera.x > -3770 && !App->player->god)
	{
		App->render->camera.x -= CAMERA_RUN_SPEED * dt;
		collider->SetPos(-App->render->camera.x, 0);

		//Parallax
		farTimer++; midTimer++; closeTimer++;
		backPos.x += CAMERA_RUN_SPEED * dt;
		if (farTimer >= CAMERA_RUN_SPEED * dt) { farPos.x -= (CAMERA_RUN_SPEED) * dt; farTimer = 0; }
		if (midTimer >= CAMERA_RUN_SPEED + 1 * dt) { midPos.x -= (CAMERA_RUN_SPEED/2) * dt; midTimer = 0; }
		if (closeTimer >= CAMERA_RUN_SPEED + 2 * dt) { closePos.x = 0; closeTimer = 0; }
	}
	

	App->render->Blit(backTex, backPos.x, backPos.y,container);
	App->render->Blit(farTex, farPos.x, farPos.y, container);
	App->render->Blit(midTex, midPos.x, midPos.y, container);
	App->render->Blit(closeTex, closePos.x, closePos.y, container);

	//player inputs ---------------
	if (current_level == LEVEL_1)
	{
		want_to_load = 2;
	}
	else if (current_level == LEVEL_2)
	{
		want_to_load = 1;
	}

	if (App->input->GetKey(SDL_SCANCODE_F1) == KEY_DOWN) {
		if (current_level == LEVEL_1)
		{
			ResetLevel();
			Reset_Camera();
		}
		else
		{
			App->player->game_saved = false;
			App->map->CleanUp();
			
			App->map->Load("Level1.tmx");
			ResetLevel();
			Reset_Camera();
		}
	}

	if (App->input->GetKey(SDL_SCANCODE_F2) == KEY_DOWN) {

		if (current_level == LEVEL_2)
		{
			ResetLevel();
			Reset_Camera();
		}
		else
		{
			App->player->game_saved = false;
			App->map->CleanUp();
		
			App->map->Load("Level2.tmx");
			ResetLevel();
			Reset_Camera();
		}
	}

	if (App->input->GetKey(SDL_SCANCODE_F3) == KEY_DOWN)
		ResetLevel();
	if (App->input->GetKey(SDL_SCANCODE_F5) == KEY_DOWN)
		App->SaveGame();

	if(App->input->GetKey(SDL_SCANCODE_F6) == KEY_DOWN)
		App->LoadGame();


	//camera limits	

	if (((player_position->y < top_edge))&&(top_edge > 0 - App->player->current_animation->GetCurrentFrame().h)&&(App->render->camera.y > 300)) {
			App->render->camera.y += App->player->speed * dt;
			top_edge -= App->player->speed * dt;
			bottom_edge -= App->player->speed * dt;
	}

	if (((player_position->y + App->player->current_animation->GetCurrentFrame().h > bottom_edge))&&(top_edge < 0+ 360)) {
		App->render->camera.y -= App->player->speed * dt;
		top_edge+= App->player->speed * dt;
		bottom_edge+= App->player->speed * dt;
	}
	
	//camera manual control --------------

	if(App->input->GetKey(SDL_SCANCODE_UP) == KEY_REPEAT)
		App->render->camera.y += CAMERA_SPEED * dt;

	if(App->input->GetKey(SDL_SCANCODE_DOWN) == KEY_REPEAT)
		App->render->camera.y -= CAMERA_SPEED * dt;

	if((App->input->GetKey(SDL_SCANCODE_LEFT) == KEY_REPEAT)&&(App->render->camera.x < 0))
		App->render->camera.x += CAMERA_SPEED * dt;

	if(App->input->GetKey(SDL_SCANCODE_RIGHT) == KEY_REPEAT)
		App->render->camera.x -= CAMERA_SPEED * dt;
	


	App->map->Draw();

	
	
	// Debug pathfinding ------------------------------
	if (App->input->GetKey(SDL_SCANCODE_F5) == KEY_DOWN) debugPath=!debugPath;

	int x, y;
	SDL_Rect Debug_rect = { 0,0,32,32 };
	App->input->GetMousePosition(x, y);
	iPoint p = App->render->ScreenToWorld(x, y);
	p = App->map->WorldToMap(p.x, p.y);
	p = App->map->MapToWorld(p.x, p.y);

	//App->render->Blit(debug_tex, p.x, p.y);
	Debug_rect.x = p.x;
	Debug_rect.y = p.y;
	if (App->collision->debug) App->render->DrawQuad(Debug_rect, 0, 0, 255, 80);

	const p2DynArray<iPoint>* path = App->pathfinding->GetLastPath();

	for (uint i = 0; i < path->Count(); ++i)
	{
		iPoint pos = App->map->MapToWorld(path->At(i)->x, path->At(i)->y);
		Debug_rect.x = pos.x;
		Debug_rect.y = pos.y;
		if (App->collision->debug) App->render->DrawQuad(Debug_rect, 90, 850, 230, 80);
	}

	
	
	return true;
}

// Called each loop iteration
bool j1Scene::PostUpdate()
{
	bool ret = true;

	
	return ret;
}

// Called before quitting
bool j1Scene::CleanUp()
{
	LOG("Freeing scene");

	return true;
}

void j1Scene::Reset_Camera() {
	App->render->camera.x = 0;
	App->render->camera.y = 0;
	top_edge = App->render->camera.y + App->render->camera.h / 4;
	bottom_edge = App->render->camera.y + App->render->camera.h * 3/4;
	collider->SetPos(-App->render->camera.x, 0);
	backPos.x = 0;
	farPos.x = 0;
	midPos.x = 0;
	closePos.x = 0;
	
}

void j1Scene::ResetLevel() {

	App->player->position.x = player_x_position;
	App->player->position.y = player_y_position;

	App->bat->position.x = bat_x_position;
	App->bat->position.y = bat_y_position;
	App->bat->state = FLY;

	cam_run_start = SDL_GetTicks();
	cam_run_start_timer = cam_run_start + 5000;
	App->player->winSound = false;
}