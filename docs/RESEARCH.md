# Reference research and design decisions

Reviewed 12 October 2026. This project draws on publicly described mechanics; it contains no extracted code, UI assets or private implementation details from either reference game.

## ICBM: Escalation

Primary source: [Slitherine product page](https://www6.slitherine.com/game/icbm-escalation).

The publisher describes conventional and nuclear warfare, territorial victory, research across technological eras, and longer-form strategic modes. The useful design lesson is that industry, conventional operations and strategic weapons should coexist: nuclear use should not be the only way to win.

ESCALATION adopts conventional capture, research progression and a costly deterrence path. The 1920 start, eight linear research branches and production/logistics chains are project design choices, rather than a claim about the reference's internal formulas.

## State of Command / Layth Issam El Zaim

Primary source: [Steam page](https://store.steampowered.com/app/4465990/State_of_Command/).
User-supplied developer channel: [YouTube](https://www.youtube.com/@laythelzaim).

The developer's Steam description combines national leadership, military organization, economic management and changing fronts. This supports a readable world map with accessible management controls and a nation-focused preparation loop.

The YouTube channel could not be retrieved reliably in this environment. No claim is made that a particular devlog was watched, transcribed, or that the developer's rendering architecture was reproduced. The implementation is based on the accessible primary product description and the user's requested direction.

## Natural Earth

Primary sources: [terms of use](https://www.naturalearthdata.com/about/terms-of-use/) and [official vector repository](https://github.com/nvkelso/natural-earth-vector/tree/v5.1.2/geojson).

Natural Earth's map data is public domain. The bundled data derives from `geojson/ne_110m_admin_0_countries.geojson` at tag `v5.1.2`. Its low vertex count is suitable for the world-view POC. Modern geometry is intentionally labelled as such for the alternate-history start; it is not substituted silently for an accurate 1920 political map.

The data-reduction pipeline drops unrelated attributes and keeps the original geometric coordinates. Subregions are generated gameplay sectors rather than Natural Earth admin-1 regions.

## Native technical choices

Primary sources: [GLFW 3.4 documentation](https://www.glfw.org/docs/3.4/), [Dear ImGui repository](https://github.com/ocornut/imgui/tree/v1.91.8), [earcut.hpp](https://github.com/mapbox/earcut.hpp/tree/v2.2.4), and [picojson](https://github.com/kazuho/picojson/tree/v1.3.0).

GLFW supplies portable desktop windows and contexts; OpenGL 3.3 supplies GPU map rendering; Dear ImGui supplies a functional native-process interface; earcut handles polygon holes; picojson keeps core data import/save dependencies small. Geometry is cached rather than parsed or triangulated every frame. The simulation can be tested separately from graphics.

Dear ImGui is rendered in the native application, not a browser/WebView. It does not use platform-standard OS widgets; it uses its own immediate-mode controls.
