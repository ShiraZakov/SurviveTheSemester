#include "Game.h"
#include "Components.h"

using bagel::Entity;
using bagel::ent_type;

static ent_type g_gs{-1};

bagel::ent_type gameStateEntity() { return g_gs; }

GameState& gameState() { return Entity(g_gs).get<GameState>(); }

void bindGameState(bagel::ent_type id) { g_gs = id; }
