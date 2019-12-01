#ifndef __j1Collision_H__
#define __j1Collision_H__

#define MAX_COLLIDERS 500

#include "j1Module.h"
#include "brofiler/Brofiler/Brofiler.h"

enum COLLIDER_TYPE
{
	COLLIDER_NONE = -1,
	COLLIDER_PLAYER,
	COLLIDER_ENEMY,
	COLLIDER_WALL,
	COLLIDER_DEATH,
	COLLIDER_PLATFORM,
	COLLIDER_LEVEL,
	COLLIDER_MAX,
	COLLIDER_HIT,
};

struct Collider
{
	SDL_Rect rect;
	bool to_delete = false;
	COLLIDER_TYPE type;
	j1Module* callback = nullptr;

	Collider() {}

	Collider(SDL_Rect rectangle, COLLIDER_TYPE type, j1Module* callback = nullptr) :
		rect(rectangle),
		type(type),
		callback(callback)
	{}

	void SetPos(int x, int y)
	{
		rect.x = x;
		rect.y = y;
	}
	void SetSize(int w, int h)
	{
		rect.w = w;
		rect.h = h;
	}
	bool CheckCollision(const SDL_Rect& r) const;
};

class j1Collision : public j1Module
{
public:

	j1Collision();
	~j1Collision();

	bool PreUpdate();
	bool Update(float dt);
	bool CleanUp();

	Collider* AddCollider(SDL_Rect rect, COLLIDER_TYPE type, j1Module* callback = nullptr);
	void DebugDraw();

	bool debug = false;

private:
	Collider* colliders[MAX_COLLIDERS];
	bool matrix[COLLIDER_MAX][COLLIDER_MAX];
	//show colliders
	
};
#endif // __j1Collision_H__