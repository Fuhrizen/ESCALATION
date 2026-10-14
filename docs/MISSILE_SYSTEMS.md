# Missile and engagement foundation

ICBM: Escalation's official developer diaries describe strategic assets covering tactical through intercontinental ranges, separately researched warheads, and distinct ship weapon systems. Those foundations guide this implementation. Exact damage, ranges, flight speeds, ammunition and reload timers below are ESCALATION gameplay tuning rather than copied commercial balance data.

References: [operational scale](https://www6.slitherine.com/news/icbm-escalation-dev-diary-5), [warhead research](https://www.slitherine.com/news/icbm-escalation-dev-diary-4), [independent naval weapons](https://www6.slitherine.com/news/icbm-escalation-dev-diary-3).

## Actual launch and impact

Operational friendly missile sites unlock through Guided missiles. Conventional ballistic strikes target hostile controlled land between 25 and 1,800 km; Intercontinental missiles increases the maximum to 12,000 km. Sites reload for 45 simulation seconds. Flight takes at least eight seconds, otherwise distance divided by 25 km/s. Conventional impacts have an eight-kilometre blast area and damage hostile formations and infrastructure; they do not capture territory.

Nuclear delivery remains an explicitly confirmed action requiring the intercontinental research and a manufactured warhead. A launch consumes the warhead immediately, then flies to the selected location. Its 65 km blast affects formations and infrastructure of either side, and reduces national stability on arrival. The previous instant scripted mutual-destruction ending has been removed. Automatic retaliation, nuclear interception, fallout and population casualty simulation are not implemented.

Artillery, mobile tactical missiles and researched submarine cruise missiles can also fire explicit point strikes. Point strikes consume the same mount ammunition and cooldown as automatic formation engagements. Target-specific automatic fire remains independent per mount; minimum ranges and target domains are enforced. Guided automatic missiles follow their originally selected formation; explicit ground targets and indirect shells retain their target coordinates. Ballistic flight flags distinguish raised missile arcs from surface trajectories. Impact records persist briefly for visible impact pulses and cannot inflict damage twice.

## Strategic operational envelopes

Formation counters represent dispersed operational groups rather than individual weapons. Infantry now reaches 18 km, armour's main gun 28 km, special forces 12 km, artillery 90 km and tactical missiles 600 km. Researched anti-tank missiles reach 40 km against armour only. These envelopes are chosen to make interactions readable on a world map while preserving distinct close combat, indirect fire and long-range strike roles. Naval mounts retain separate surface, submerged and land-attack envelopes.

## Persistence and integration

Save version 8 stores ballistic/nuclear/point-strike flags, projectile owner and source site, impact animation lifetime and missile site reload. Versions 1–7 remain readable. Existing in-flight unit engagements are preserved when loading version 7.

HUD integration uses `unitAttackReason` / `orderUnitAttackAt` and `missileStrikeReason` / `launchMissile`. Invalid targets, peace, ownership, technology, readiness, envelope, ammunition and reload reject an order without consuming ammunition. Nuclear confirmation belongs to the UI; conventional targeting is an ordinary RTS action. Projectiles resolve after launch even if their source formation or site is destroyed.
