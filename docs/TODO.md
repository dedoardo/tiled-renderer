Note that all the following notes refer to camy for the most part, but as 
I'm using github.com/sparkon/spark as test bench, some of them might be shared

Features (or very big fixes):
- Work out how multiple contexts work
- Instancing, especially inputlayout
- Add more options to states (Ctrl+F "Todo: Options")
- Rework spark/loaders w/ caching 
- More validation when setting states (Indipendent from backend ??) and less in release mode. Do this when fixing the documentation
- Debug Shapes
- Multithread renderbucket (Already have task-system)

Fixes:
- Shared parameters in RenderBucket
- Immediate context functionality in RenderContext
- BindType::Constant removal would slim code down in certain places (e.g. RenderBucket::_compile())
- Improve ShaderVariable usability (+ostream<< overload)
- rename sparker -> spark

Things that might look like they are implemented, but they haven't been tested / probably not working:
- Cube map arrays
- Stencil state