### Commits
Summary format:
+<name> : New feature added
~<name> : Reworked that particular code/interface
-<name> : Removed a specific feature
!<name> : Fixed a bug
Small changes are not reported currently. If there is not enough space in the summary
(72 chars currently) finish summary with '..' and start Description with '..'. 
End the extended summary with '.'. If <name> is hard to find, simply put the name
of the interface/class/file/module and more details later. This is just to quickly
keep track of what parts of the code have been modified. Multiple modifiers can be 
present

Description simply expands on the Summary giving some more detailed info or references.
It has the same exact order of the Summary where each info is in one line and starts
with the same modified [+,-,~,!] as the corresponding summary. If multiple modifiers 
for a single element are present, feel free to unwind them.

Starting from 1/4/2017

+ Added support for per-element views in surface arrays and cubemaps
+ MSAA now working and msaa_levels should be correct
~ Fixed unused parameter in CommandList::set_parameter
+ Added CommandList::set_cbuffer
+ Added some limited(intentionally) immediate context functionality to the render context.
- Removed DirectXMath 
+ Now using simdpp, that sits roughly at the same level of DirectXMathb 
- Removed some libc wrappers and cleaned up utilities

+ Added some more math operations to the floatX structs
- Resolved many warnings
+ Added float3x3 
~ Cleaned up math API removing DirectXMath dependency
+ to_shader_order(float4x4)
+ Added types to different enumerations
+ Cleaned up resource descriptions
+ Uniform declaration for all handles. struct w/ custom operators and no more global variables to check against is_valid() is_invalid() make_invalid()
+ Added simple filesystem interface: os/file
+ Reworked logging adding presets and moved it to os/log.hpp

+ All resources and subresources are now named appropriately and typed
+ path_extract_filename
! Fixed StaticString::append

+ ostream overloads for vectors
! Fixed bug in LinearPage::next_array() and PagedLinearVector::next_array()
! Fixed bug in RenderBucket::_compile() block caching now working
? Temporarly included DirectXMath for matrix inversion
! Fixed bug in PagedLinearVector destructor