Note that all the following notes refer to camy for the most part, but as 
I'm using github.com/sparkon/spark as test bench, some of them might be shared

Features (or very big fixes):
- Work out how multiple contexts work
- Instancing, especially inputlayout
- Add more options to states (Ctrl+F "Todo: Options")
- Rework spark/loaders w/ caching 

Fixes:
- Check all destructor that call unload() such as RenderContext
- Smart pointers should have embedded allocation / deallocation and different types for Tallocate/deallocate
- REWRITE LOGGIN messages
- Implement realloc pls...
- Consistent naming of internal resources ( __internal: ) ( __generated:  )
- More testing under invalid conditions (find software that does that)
- Technique mask builder
- Check MaterialInfo assignment operator etc because of the data* member
- Shared parameters in  renderbucket to avoid setting the same shit over and over again
- Place as many members/functions as you can in private namespace at the beginning on .cpp (all them camy_to_d3d11/dxgi)
- Provide camy_log w/ float2 float3 float4 outputs
- Polish math API and implement backend
- Use more auto pointers in creation and get rid of gotos
- Allow setting of default states in commandlist, this could be the same as the shared parameters!!!
- When logging shader compilation error, print filename or something else, hard to distinguish
- Add Primitive Topology (for debug shapes)
- Provide more immediate functionality in RenderContext (???)
- Put PipelineState in renderbucket ? (clean the API a bit)
- Descs should have presets for easier creation ( and no bools, and data should be separated from desc )
- Per component vector multiplications

Things that might look like they are implemented, but they haven't been tested / probably not working:
- Cube map arrays
- Stencil state