Note that all the following notes refer to camy for the most part, but as 
I'm using github.com/sparkon/spark as test bench, some of them might be shared.

Features (or very big fixes):
- Instancing, especially inputlayout
- Add more options to states (Ctrl+F "Todo: Options")
- Rework spark/loaders w/ caching 
- More validation when setting states (Indipendent from backend ??) and less in release mode. Do this when fixing the documentation
- Debug Shapes
- Multithread renderbucket (Already have task-system)
- More testing and avoid crashing

Fixes:
- Rework headers removing things from header files as much as possible ( log for instance has namespec hidden ????????? )
Especially find a way to initialize the different subsystems (memory tracking / logging ) and provide a shutdown for camy::shutdown Rename logggin output_stream to ostream, Platform initialization should be shared (atleast the beginning)
Find a decent way to merge core / graphics and all the "duplicated" headers, do some reserarch
Try and remove camy_enable_logging and make sure no ostream is compiled
- Fix utils.hpp
- Add destroy_*** to ogl render context
- !!! Improve the RenderItem/ParameterBlock/Parameter interface, could be made less verbose and slightly easier to use. 
- Find better opengl error reporting 
- Issue warning if viewport is empty 
- Improve logging make it multithread safe in some way 
 Reset command list state
- Find all uses of STL functions are wrap them ? or decide what to do with it
- Please make sure that all the calls are made when in the right context :) this should be both for OGL and D3D to make the code more portable
- Check stencil on OpenGL and check flush() for creation errors
- OpenGL support renderbuffers ? for RTV or DSV onlyu
- Release the resources
- More consistent naming 
- Add timings
- Check strict aliasing rule in some way ? static ? ( http://cellperformance.beyond3d.com/articles/2006/06/understanding-strict-aliasing.html) ( http://dbp-consulting.com/tutorials/StrictAliasing.html)
- Containers should have something like (to_smart_pointer() that gives up ownership of the container to a smart pointer).
- THIS IS KEY: Make alignment a constructor and then when reallocating just directly copy the memory alignment, do this by implementing realloc, that retrieves the alignment of the allocation and reforces it.
- THIS IS KEY: Rework allocators w/ pointers and not sizes, redesign them a little bit starting from a list of requirements
- THIS IS KEY: Division between typed and untyped containers ( they construct or not
- Remove them 'k's
- Fix smart pointers, embed allocation / deallocation
- CAMY_MACRO no camy_MACRO
- Try and change rsize to uint64 
- Variadic functions should forward
- Set state to default ( D3D11 CommandList does it automatically, but opengl needs to do it manually before flushing)
- PipelineState::
- Depth stencil view, doe a couple of tests for each feature
- puts VAOS somewhere else, hashing is not really ok
- Multiple viewports
- Work better w/ smart pointers, they are actually not bad in certain scenarios, but need to be worked out slightly better
- OpenGL object naming
- Check all invalid handles in command list
- Depends on: GL_EXT_polygon_offset_clamp ( Can easily be removed though 
- Look at resource descriptions and add cases such as Depth Func, Blend, Rasterizer (just support more stuff)
- Make it explicit that offsets are in 16 bytes constant or make it in bytes and let the backend do the conversion ( FIXME)
- Group all conversion functions from and to enumerators and make them force inline
- Deallocate those commandlist stuff
- Check CommandList bindings
- Support array / buffers etc in shaders

```
COMMANDS ARE INHERITED, avoid aliasing urle

{
    byte* pbuffer;
}

draw_indexed_submit(byte*& pbuffer)
{
    assert()
    unpack_args()
    call()
}
```

- InputLayout fai la creazione un pochino piu' cross-platform, troppo HLSL oriented :/
- Go back to separate shader object, no freaking programs. This also solves the binding problem 
- Surface creation code is kind of redundant, especially the mipmap / etc.. filling
if a good idea comes to mind i'll refactor it, otherwise it's not really worth the time,
as it's probably going to be bugged at first due to the many possible cases. BUT
reworking how subsurfaces are specified can be beneficial, just pass a void* and the
pitch and byte sizes can be computed from the format. Same goes for the pixelformats
maybe merge view and format somehow ?  
- OpenGL to check before bindind do glIsObjectType() nice feature
- Grouping of pixel formats https://msdn.microsoft.com/en-us/library/windows/desktop/bb694531(v=vs.85).aspx#Differences

Things that might look like they are implemented, but they haven't been tested / probably not working:
- Cube map arrays
- Stencil state