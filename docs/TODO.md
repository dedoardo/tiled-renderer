Note that all the following notes refer to camy for the most part, but as 
I'm using github.com/sparkon/spark as test bench, some of them might be shared.

Features (or very big fixes):
- Work out how multiple contexts work
- Instancing, especially inputlayout
- Add more options to states (Ctrl+F "Todo: Options")
- Rework spark/loaders w/ caching 
- More validation when setting states (Indipendent from backend ??) and less in release mode. Do this when fixing the documentation
- Debug Shapes
- Multithread renderbucket (Already have task-system)
- More testing and avoid crashing

Fixes:
- Immediate context functionality in RenderContext
- BindType::Constant removal would slim code down in certain places (e.g. RenderBucket::_compile())
- Improve ShaderVariable usability (+ostream<< overload)
- rename sparker -> spark
- rename ForwardPP::prepare in process?? or any other name
- !!! Improve the RenderItem/ParameterBlock/Parameter interface, could be made less verbose and slightly easier to use. 
- SSAO kernel rotation noise can use R32G32 ? 
- opengl_impl_base d3d11_impl_base USE core/THREAD
- Issue warning if viewport is empty 
- It'd be better if the user didn't have to specify the number of miplevels and either 0 or full. 
- Add timings
- Surface creation code is kind of redundant, especially the mipmap / etc.. filling
if a good idea comes to mind i'll refactor it, otherwise it's not really worth the time,
as it's probably going to be bugged at first due to the many possible cases. BUT
reworking how subsurfaces are specified can be beneficial, just pass a void* and the
pitch and byte sizes can be computed from the format. Same goes for the pixelformats
maybe merge view and format somehow ?  
- OpenGL to check before bindind do glIsObjectType() nice feature

Things that might look like they are implemented, but they haven't been tested / probably not working:
- Cube map arrays
- Stencil state