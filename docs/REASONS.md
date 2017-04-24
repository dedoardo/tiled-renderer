# Reasons
Comments including links/videos on how different parts of the framework are implemented. They are a mix of personal notes
and maybe interesting details. They are a mix of design choices and detail on how certain graphic features are implemented.
They are not all associated with `camy` as I am using `github.com/sparkon/spark` as test bench, thus some are referred to that aswell.

#### Descriptions
Resources and objects that require different creation parameters have an associated description to avoid clutter. The structures do sometimes have methods associated with them but they are exclusively helpers to avoid lots of manual setting work. 
They are usually named accordingly:
`set_****`
`enable_****`

#### Presets
Sometimes these structures (e.g. DrawCall) have presets associated with them and they are named `make_***`. There is a clear distinction between helpers and makers. `make_***` generates a valid description as specified, subsequent sets might render the description unusable. If you are supposed or allowed to modify the structure after the make, it is usually specified in the comments. If nothing is said regarding the argument then nothing will prevent you from doing it, but do it at your own risk.

#### Buffers and Constant Buffers 
Buffers are currently separated from constant buffers as their dynamics are slighly different, and ConstantBuffers are not supposed to be touched outside `camy`. This is obviously a general rule for standard applications.

#### Logging 
This is the only real part where STL is used. There is no point in rewriting overloads for << operators. Logging uses std::ostream and a recursive specialized template implementation. When logging is disabled no code is generated and STL is
not even included. There is no real benefit in rewriting this kind of operations.

#### Naming resources
Resources and Objects are named throughout the framework, but the name is not lookupable, it's just for easier debugging (makes life a lot easier when reading logs and debugging using VisualStudio/RenderDoc/Pix). Naming resources is highly suggested and incurs in no real additional cost. When naming resources distinction is made between:
- Generated ~ Additional resources generated from a load() or create() (e.g. InputSignature from a VertexShader)
- Internal ~ Resources used for internal management. 

Naming is as follows: `<type>::<name> [__internal|__gen(<viewtype>)]`

#### Shading
##### Indirect diffuse: 
starting from Lo = Le + { f * Li * cos } Indirect diffuse is prefiltered offline, the diffuse BRDF is the classic lambert f(x) = c / PI as it is constant we can move it out of the rendering equation integral having Lo = Le + c/PI * { Li * cos } Parameterizing the surface integral in spherical coordinates. The why there is an added sin is pretty easy to understand as the area of the small bits of surface is reduced as we move along phi(zenit). The mathematical reason is related to the change of variable theorem in multi-variable calculus. http://math.stackexchange.com/questions/904483/solid-angle-integration http://math.stackexchange.com/questions/188490/why-is-the-differential-solid-angle-have-a-sin-theta-term-in-integration-in-s http://mathworld.wolfram.com/ChangeofVariablesTheorem.html (removing emissive) Lo = c/PI { Li * cos(theta) * sin(theta) * d_theta * d_phi } Now we run the simulation offline using a montecarlo estimator where for each possible direction L we calculate the accumulated irradiance. phi = [0, PI / 2] theta = [0, 2 * PI] Thus we need to multiply by the area and divide for the number of samples. The result is a double sum: ((c/PI) * (2*PI/samples_theta) * ((PI/2)/samples_phi)) * sum_phi(sum_theta(Li * cos(theta) * sin(theta))). This is run in the pixel shader one per face. Pretty much the same as: http://www.codinglabs.net/article_physically_based_rendering.aspx.

##### GPU Importance sampling
As this is more a playground than an actual production renderer the code is not extremely optimized, but still does its job. Doing importance sampling in a fairly standard way.
- Hammersley sequence
- Mipmap chosen based on roughness. NVidia proposed http://http.developer.nvidia.com/GPUGems3/gpugems3_ch20.html, but roughness is somewhat direct measure of the solid angle we are going 
to sample and their approach requires a pdf(). Doing importance sampling of a microfacet model
w/ GGX distribution has the very big advantage that the very N term is both in the brdf and the pdf itself, thus they cancel out w/o the need to directly calculate it. 
Using an Hammersley sequence implies that every fragment samples w/ the same kernel and
not choosing the correct miplevel will inevitably result in noticable artifacts (ghosting-like?). It can be probably softened rotating the kernel is some random way (very common for post-processing effects like SSAO), but it could introduct noise-like artifacts
as the camera is moved.
The Hammersley function can be precomputed, even though as they operato

#### Vignette (Postprocessing implementation)
Vignette is probably one of the few things that has no inspiration from any online resource. I saw the result and tried to implement it. Some random notes:
- To avoid passing viewport information to the pixel shader, I let the vertex shader output the position in homogeneous clip space coordinates and then manually do the perspective divide in the pixel shader (thus avoiding the viewport transform).
- The darkness is scaled gradually from the center of the screen (smoothstep). Some sort of noise is added, this is exclusively to avoid banding, no artistic reason behind it. 
- The next step would be tweaking the vignette using some pattern or noise texture.
