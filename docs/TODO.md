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

Cool stuff:
- Some kind of global illumination, either voxels or probes (Multi-Scale global illumination in Quantum Break)
- Real time area lighting
- Filmic/Temporal/Morphological AA
- Partecipating medias / raymarching volumes
- SSR
- Postprocessing