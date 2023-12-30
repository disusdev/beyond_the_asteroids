#ifndef __PHYSICS_H__
#define __PHYSICS_H__

#include <defines.h>
#include <raymath.h>
#include <systems/component_system.h>

#include <box2d/box2d.h>
#include <box2d/geometry.h>
#include <box2d/math.h>


typedef struct
{
  i32 entity_id;
  b2BodyId body;
} t_co_physics;


typedef struct
{
  f32 width;
  f32 height;
  f32 density;
  f32 friction;
} t_co_physics_cfg;


static cvec(t_co_physics_cfg) cfgs = 0;


int
physics_system_add_cfg(t_co_physics_cfg cfg)
{
  i32 id = cvec_header(cfgs)->size;
  cfgs = cvec_push(cfgs, &cfg);
  return id;
}


t_co_physics_cfg
physics_system_get_cfg(int id)
{
  return cfgs[id];
}


i32 co_physics_create(i32 entity_id, i32 cfg);

static b2Vec2 gravity = {0.0f, -10.0f};
static b2WorldDef worldDef;
static b2WorldId worldId;
static b2BodyDef groundBodyDef;

static f32 timeStep = 1.0f / 60.0f;
static i32 velocityIterations = 6;
static i32 relaxIterations = 2;


void physics_init()
{
  component_system_add_component("co_physics", sizeof(t_co_physics), 4, 0, &co_physics_create);

  cfgs = cvec_create(sizeof(t_co_physics_cfg), 8, false);

  worldDef = b2DefaultWorldDef();
  worldDef.gravity = gravity;

  worldId = b2CreateWorld(&worldDef);

  groundBodyDef = b2DefaultBodyDef();
	groundBodyDef.position = (b2Vec2){0.0f, -10.0f};

  b2BodyId groundBodyId = b2World_CreateBody(worldId, &groundBodyDef);

	b2Polygon groundBox = b2MakeBox(50.0f, 10.0f);

  b2ShapeDef groundShapeDef = b2DefaultShapeDef();
	b2Body_CreatePolygon(groundBodyId, &groundShapeDef, &groundBox);
}


void physics_update(f32 dt)
{
  b2World_Step(worldId, timeStep, velocityIterations, relaxIterations);

  u64 count = 0;
  t_co_physics* physics_comps = cast_ptr(t_co_physics) component_system_get("co_physics", &count);

  for (u64 i = 0; i < count; i++)
  {
    b2Vec2 position = b2Body_GetPosition(physics_comps[i].body);
	  f32 angle = b2Body_GetAngle(physics_comps[i].body);

    Matrix mat = component_system_get_local_transform(physics_comps[i].entity_id);
    update_rotation(&mat, angle);
    update_position(&mat, (Vector3){position.x, position.y, 0});
    component_system_set_local_transform(physics_comps[i].entity_id, mat);
  }
}


i32 co_physics_create(i32 entity_id, i32 cfg)
{
  t_co_physics_cfg config = cfgs[cfg];

  t_co_physics physics = {0};
  physics.entity_id = entity_id;

  b2BodyDef bodyDef = b2DefaultBodyDef();
	bodyDef.type = b2_dynamicBody;

  Matrix mat = component_system_get_local_transform(entity_id);

  Vector3 pos = extract_position(&mat);

	bodyDef.position = (b2Vec2){ pos.x, pos.y };
	physics.body = b2World_CreateBody(worldId, &bodyDef);

  b2Polygon dynamicBox = b2MakeBox(config.width, config.height);

  b2ShapeDef shapeDef = b2DefaultShapeDef();

  shapeDef.density = config.density;

  shapeDef.friction = config.friction;

  b2Body_CreatePolygon(physics.body, &shapeDef, &dynamicBox);

  return component_system_insert(entity_id, "co_physics", &physics);
}


#endif