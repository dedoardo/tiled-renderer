All of the following notes refer to camy_graphics. camy_core contains all 
math/memory/os stuff and for the most part it is exposed as free functions. 

API overview:
RenderContext :: Resource creation and management ( free-threaded ), and commandlist flushing.
CommandList   :: Issuing GPU Commands that get compiled inside a commandlist, 
this can be either hardware supported (ID3D11CommandList, nv_command_list )or a more convenient representation for older backends.
RenderBucket  :: List of items to be rendered -> compiles to CommandList
ComputeBucket :: List of compute commands -> compiles to CommandList
Frame         :: Container of buckets w/ ordering
ShaderStage   :: Extended shader representation that supports reflection

All the graphics free functions are contained inside namespaces, those who are not are not supposed to be used by the user are inside the respective namespaces. 
There is currently no documentation style. All relevants comments are inside
//! or /*! braces. In the future a documentation might be generated grepping all the relevant bits.

<API>
camy has to be bound to a window, that is created upon initialization. As it currently stands
startup(StartupInfo) :: Initializes camy
shutdown() :: Shuts down camy
render_context() :: Returns the "Global" RenderContext, a custom one can be created aswell
RuntimeInfo rc() :: same as render_context(), abbreviated for usability
runtime_info() :: Returns a bunch of data that can be useful for the end-application, some of the stuff is platform specific, such as handles.
info() :: same as runtime_inof(), abbreviated for usability

Global values are part of the API namespace and are divided in 
API limits that independently from the backend impose a limit on resources for design reasons. It's a value that is supported by all backends. number of max possible resources/render targets/ bindable resources/ etc.... They are all evaluated at compile time and the code makes assumption on their value (w/ a static_assert). In case they are changed small changes should be make and static_asserts can be removed
API values. Can't make assumption on them, they are queried via the API nammespace. They don't change at runtime, but are possibly not known at compile time, as they might depend on the hardware the code is running on.

The syntax for both is always all the same
API::kMaxBindableRenderTargets

enum QueryType
{
    ConstantSize,
    MaxConstantNumber
};

API::query(QueryType::ConstantSize)

Backend implementation:
Simply provide a definition for the following 3 classes.
RenderContext
CommandList
ShaderStage

graphics/base.hpp as for camy_core provides all the basic types upon which everything else builds. It follows a specific inclusion order:
- First it includes graphics/<backend>/<backend>_graphics_base.hpp

