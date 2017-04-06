Fixes:
- Implement fullscreen 
- Check all destructor that call unload() such as RenderContext
- Smart pointers should have embedded allocation / deallocation and different types for Tallocate/deallocate
- Do the same currently done for SHaderVariable::invalid/is_valid() for HResource and make it "smarter"
- Consistent logging
- Implement realloc pls...
- Consistent naming of internal resources ( __internal: ) ( __generated:  )
- More testing under invalid conditions (find software that does that)
- Technique mask builder
- Check MaterialInfo assignment operator etc because of the data* member
- Shared parameters in  renderbucket to avoid setting the same shit over and over again
- Place as many members/functions as you can in private namespace at the beginning on .cpp (all them camy_to_d3d11/dxgi)
- Provide camy_log w/ float2 float3 float4 outputs
- Polish math API and implement backend
- Separate desc from initial data
- Fix them resource descriptions (bools)
- Use more auto pointers in creation and get rid of gotos
- More OS functionality (replace technique code and loader)
- Allow setting of default states in commandlist, this could be the same as the shared parameters!!!
- Rename set_parameter(void*) w/ set_cbuffer_incremental set_cbuffer_offset
- When logging shader compilation error, print filename or something else, hard to distinguish
- Finish adding all type enums
- Add Primitive Topology (for debug shapes)
- Provide more immediate functionality in RenderContext (???)
- Put PipelineState in renderbucket ? (clean the API a bit)
- Make it more explicit that we are working on a left-handed coordinate system
- Descs should have presets for easier creation

Cool stuff:
- Some kind of global illumination, either voxels or probes (Multi-Scale global illumination in Quantum Break)
- Real time area lighting
- Filmic/Temporal/Morphological AA
- Partecipating medias / raymarching volumes
- SSR
- Postprocessing

Things that might look like they are implemented, but they don't work:
- Cube map arrays
- Stencil state