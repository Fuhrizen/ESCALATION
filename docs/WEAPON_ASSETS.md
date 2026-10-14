# Weapon-slot artwork

Weapon tiles and the expanded weapon detail list now use cropped artwork from the user-supplied `AssetLib.tar.gz`. NATO counters remain the map representation.

| Profile IDs | Supplied artwork |
|---|---|
| rifle, carbine, coax, machine_gun | cyan bullet in `UI/UI_2048.png` |
| tank_gun, howitzer, naval_gun | white ammunition silhouette in `UI/UI_2048.png` |
| atgm, ground_missile, anti_ship, cruise | white missile silhouette in `UI/UI_2048.png` |
| depth_charge | white bomb silhouette in `UI/UI_2048.png` |
| torpedo, asw_torpedo, sub_torpedo | `Units/IMAGES/PROJECTILES/TORPEDO.PNG` |

These are ammunition/projectile symbols, not photographs of the exact weapon model. This limited extract does not contain a distinct rifle weapon-slot icon. The actual weapon name, target domains, range and damage remain available on hover and in the expanded detail list.

`data/weapon-icons.png` is a transparent RGBA atlas with five 128-pixel cells. Source crop coordinates and SHA-256 hashes are in `data/weapon-icons.json`. Only the two named regular files were decoded from the archive, without bulk archive extraction. Artwork was cropped and scaled proportionally, with transparent gutters and filtered texture mip levels. Texture loading and cleanup belong to `SurfaceAssets`; absent atlas files and unknown future profile IDs retain the existing vector fallback.

The source is the asset archive supplied by the user for this implementation. No public-domain status or redistribution license is asserted for these game assets.
