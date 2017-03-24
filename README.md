**NOTE**   
This is the dev-branch for the 0.9 version, still heavily working on it

# camy
![Preview](https://github.com/sparkon/camy/blob/master/last_sample.png?raw=true)
Cross-platform command-list oriented rendering framework to build a renderer upon.
It is not supposed to be used on it's own, it's still a lowish level API.
(For different renderer implementations see github.com/sparkon/sparker).

### Objectives
Release a somewhat stable 1.0 version with the following features:
- Direct3D11.1 & OpenGL4.5 backend
- Render & Compute queues
- Same shading for all backends (worst case scenario, translate bytecode)
- No external dependencies including STL & w/ custom allocators
- Decent documentation for <camy_graphics>

### Milestones
- 0.9 is the current target version
- There will be no new features from 0.9 to 1.0. Simply bugfixes