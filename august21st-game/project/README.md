# Game description:

### Phase descriptions:
Phases are identified by a phase scene identifier, consisting of `{mapset_name}/{scene_name}:{event_name}`,
with the mapset and scene name corresponding to the filesystem path from the project scenes/mapset directory
where the scene can be found.

The game also features a set of common scenes that will be accessed regardless of the specific mapset that a server
is playing on. These include the following:
- **loader**: Initial default scene, used upon game initial start, and will perform
   initial game loading logic to ensure that the game is started correctly as either
   a client or server session
- **loading_screen**: Will wait until the next phase update before attempting
   to slide the player into the correct stage of the game. *(Duration: ~1 minute
   for initial game load OR longest phase length)*

## Mapsets

## rplace
### Notes:
- The phases in this mapset are strictly chronological, the game can not advance
  to the next state without evaluating all previous states, game phases are irreversible.

### Phases:
1. loading_screen
2. rplace/intro
3. rplace/roof:intro
4. rplace/roof:vortex
5. rplace/roof:zombies
6. rplace/roof:evil_zubigri_sky
7. rplace/roof:evil_zubigri_platformer
8. rplace/roof:collapse
9. rplace/end:intro
10. rplace/end:sandbox

### Phase descriptions:
- **intro**: Cinematic introduction presenting the game's storyline, objectives,
   and context. *(Duration: ~2 minutes)*
- **roof:intro**: Camera sequence of storyline characters entering roof, short
   tutorial showing the player notable items and controls. *(Duration: ~30s)*
- **roof:vortex**: Vortex spawns overhead, starts throwing down chairs and
   random objects, that will kill the player on contact. *(Duration: ~3 minutes)*
- **roof:zombies**: Hedgehogs and fur-creatures spawn, you must construct
   walls to block them from coming in, camera sequence of zombies climbing up the
   walls of the building. *(Duration: ~3 minutes)*
- **roof:evil_zubigri_sky**: Evil Zubigri spawns in the sky, and launches
   attacks from the previous phases, Zubigri paints blue triangles onto the
   canvas, which reflect onto the real site, and the players must shoot brushes
   that revert the cyan parts of the canvas. (Duration: ~3 minutes)
- **roof:evil_zubigri_platformer**: Players must beat zubigri at a geometry
   dash-like platformer level, every second they outpace zubigri, he takes some
   damage. *(Duration: ~2 minutes)*
- **roof:collapse**: Zubigri falls and invokes Oppenheimer, who nukes
   everything, causing the tower to slowly collapse and sink into the lava, the
   players must build up to Atatürk who will save them: Zubigri falls and
   invokes Oppenheimer, who nukes everything, causing the tower to slowly
   collapse and sink into the lava, the players must build up to Atatürk who
   will save and transport the players. *(Duration: ~2 minutes)*
- **end:intro**: Camera sequence of storyline characters consolidating the
   success of the players, summarising achievements and showing credits.
   *(Duration: 30s)*
- **end:sandbox**: Player can navigate the end of the canvas, a heaven-like
   dimension, with the corner of the rplace.live canvas just in sight in the
   distance, there is a temple-like structure with various rplace-related
   monuments. *(Duration: 10 minutes before event will restart)*

## Future additions:
- **Dynamic Mapsets**: Expand the game to support dynamically generated or user-contributed mapsets, with custom phases and events.
- **Replayable Challenges**: Allow players to revisit specific phases with leaderboards and new objectives.
- **Co-op Enhancements**: Introduce cooperative mechanics, like team-building challenges or shared phase progression.
- **Procedural Events**: Randomise certain events within phases to add variety and replay value.
- **Customisable Difficulty**: Offer serverside adjustable difficulty settings for phase events to adjust to player experience.


## Game flow diagram:
```
Common Scenes:
  [loader] --> [loading_screen]

Mapset: rplace
  [loading_screen] --> [rplace/intro] --> [rplace/roof:intro] --> 
  [rplace/roof:vortex] --> [rplace/roof:zombies] --> 
  [rplace/roof:evil_zubigri_sky] --> [rplace/roof:evil_zubigri_platformer] --> 
  [rplace/roof:collapse] --> [rplace/end:intro] --> [rplace/end:sandbox]
```