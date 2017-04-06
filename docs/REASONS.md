<> Descriptions
Resources and objects that require different creation parameters have an associated
description to avoid the clutter. The structures do sometimes have methods associated
with them but they are **exclusively** helpers to avoid lots of manual setting work. 
They are usually named accordingly:
- set_****
- enable_****
see <Presets> for more

<> Presets 
Sometimes these structures (e.g. DrawCall) have presets associated w/ them and they
are named `make_***`. There is a clear distinction between helpers and makers. 
`make_***` generates a valid description as specified, subsequent sets *might* 
render the description unusable. If you are supposed or allowed to modify the 
structure after the make, it is usually specified in the comments. If nothing is
said regarding the argument then nothing will prevent you from doing it, but do it 
at your own risk.

<> Buffers and Constant Buffers
Buffers are currently separated from constant buffers as their dynamics are slighly 
different, and ConstantBuffers are not supposed to be touched outside camy. This 
is obviously a general rule for standard applications. 

<> Program and Shaders 
Even though most of the API is very D3D-like. Programs are present because reflection
in OpenGL works on a program level and thus they are currently grouped. RenderItems
still take single shaders as parameters.

<> Logging
This is the only real part where STL is used. There is no point in rewriting overloads for << operators. Logging uses std::ostream and a recursive specialized
template implementation. When logging is disabled no code is generated.

<> Naming
Resources and objects are named throughout the framework, but the name is not 
lookupable, it's just for easier debugging (makes life a lot easier when reading log).
Naming resources is highly suggested and incurs in no real additional cost.
When naming resources distinction is made between:
- Generated ~ Additional resources generated from a load() or create() (e.g. InputSignature from a VertexShader)
- Internal  ~ Resources used for internal management.
Naming is same as custom C++ compiler attributes:
<type>::<name> 
<type>::<name> __gen(type)
<type>::<name> __internal

