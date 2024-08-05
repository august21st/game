# Game description:

### Notes:
- There is a 5s server-enforced buffer for scene loads, so in-game scene
  lengths will be slightly longer than the scene duration, in order to ensure
  phases are not cut off short in case of client slowdown.
- The phases are strictly chronological, the game can not advance to the next
  state without evaluating all previous states, game phases are irreverable.

### Phases:
1. loading_screen
2. intro
3. roof:intro
4. roof:vortex
5. roof:zombies
6. roof:evil_zubigri_sky
7. roof:evil_zubigri_platformer
8. roof:collapse
9. end:intro
10. end:sandbox

### Phase descriptions:
- **loading_screen**: Will wait until the next phase update before attempting
   to slide the player into the correct stage of the game. *(Duration: ~3 minutes
   for initial game load OR longest phase length)*
- **intro**: Camera sequence of main event storyline, introducing the player to
   the protagonists and greater event context. *(Duration: ~2 minutes)*
- **roof:intro**: Camera sequence of intro characters entering roof, guide
   showing the player where notable items are, and describing game
   controls. *(Duration: ~30s)*
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
   will save and transport the players. *(Duration: 2~ minutes)*
- **end:intro**: Canera sequence of characters consolidating the success of the
   event, alongside credits and info about the sandbox. *(Duration: 30s)*
- **end:sandbox**: Can navigate the end of the canvas, a heaven-like dimension,
   with the corner of the real game canvas just in sight in the distance, there
   is a temple-like structure with various Rplace related monuments and credits.
   *(Duration: 15 minutes before event will restart)*
