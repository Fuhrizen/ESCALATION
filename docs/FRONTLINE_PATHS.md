# Frontlines and routes

The supplied ICBM: Escalation battlefield reference uses continuous opposing ownership contours with short facing ticks. ESCALATION now anchors its contour to campaign region ownership rather than drawing only intersections of isolated army pressure blobs. Two signed distance fields define the boundary across all visible campaign land. Active ground-unit pressure bends that boundary locally. Teal and orange tracks match the controlling nations' map palette; short facing ticks identify each side. A small spatial filter removes raster stair steps. Water and the hidden globe hemisphere are excluded.

This is a tactical influence display, not a second occupation system: city capture still controls regional ownership. The field is refreshed at eight samples per second and immediately after camera or ownership changes. It does not claim political borders move merely because a unit passes nearby.

Selected moving units show the actual movement planner's remaining waypoints. The active order is a continuous cyan line with a dark outline; subsequent queued orders are dashed at a constant screen-pixel cadence. Route rendering precedes unit-centre visibility rejection, so a selected unit outside the visible globe hemisphere can still show its visible destination-side route. All segments clip at the globe horizon and wrap short-way across the dateline. Geographic corner rounding is disabled for movement, preserving the planner's exact land/ocean-safe route.

Destinations have crosshair markers, queued orders have sequence labels, and the final destination shows the complete queue's arrival estimate at normal simulation speed. Dashes no longer depend on geographic sample count, which previously made short routes vanish or long ones change cadence while zooming.

## Missile presentation

Fired ballistic engagements show a rising and falling arc with a nation-colored trail, a brief launch flash, a directional missile glyph and exhaust. The visual arc is a strategic screen-space convention; the source/target positions and arrival timing come from the live simulation record. Surface missiles remain on their surface trajectory. Both kinds clip against the globe horizon and use short dateline wrapping. The display accepts point strikes and silo launches that have no target or attacker unit index.

Impact animation starts only when the simulation applies the strike and sets the engagement's impact timer. The effect appears at the actual target, expands across the recorded damage radius and fades; nuclear strikes retain their stronger flash for the simulation's longer impact lifetime. An order by itself does not fabricate a projectile or an explosion.
