/// @file GameStateAccess.cpp
/// @brief Implements the global GameState accessor functions.
///        A single file-scope variable tracks the singleton entity so all systems
///        can call gameState() without passing pointers through the call stack.
#include "Game.h"
#include "Components.h"

using bagel::Entity;
using bagel::ent_type;

static ent_type g_gs{-1};

/// @brief Returns the entity ID of the singleton GameState entity.
bagel::ent_type gameStateEntity() { return g_gs; }

/// @brief Returns a reference to the singleton GameState component.
GameState& gameState() { return Entity(g_gs).get<GameState>(); }

/// @brief Stores the singleton entity ID; called once by setupFreshPlayScene (and by the test harness).
void bindGameState(bagel::ent_type id) { g_gs = id; }
