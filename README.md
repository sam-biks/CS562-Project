# CS562-Project  
This repo is the culmination of 4 seperate projects from Digipen CS562 a class on advanced realtime rendering techniques.  
## Topics
 - [Deferred Rendering](deferred-rendering)
 - [Moment Shadow Mapping](moment-shadow-mapping)
 - [Physically Based and Image Based Lighting](physically-based-and-image-based-lighting)
 - [Ambient Occlusion](ambient-occlusion)
## Deferred Rendering
## Moment Shadow Mapping
**Relative Depths**
Relative depths are acheived using the function z = (zf - z0) / (z1 - z0). Where zf is the camera space depth at that fragment, and z0, z1 are the minimum and maximum depths from the light's perspective.
**Shadow Map Filtering**
Shadow map filtering is achieved using a fixed width blur using a convolution filter with a Gaussian weight kernel. This is done in a compute shader.
**Moment Shadow Maopping**
Moment shadow mapping is achieved using the Hamburger 4MSM algoritm.  
**Pre-blur Depth**  
![Pre-blur Depth](https://github.com/user-attachments/assets/131ee0ca-b3e2-4186-a82c-324ad7a9cf39)  
**Post-blur Depth**  
![Post-blur Depth](https://github.com/user-attachments/assets/649098d4-ddea-4b52-90ab-8d170a8396a5)  
**Image with blur width of 4**  
![Image with blur width of 4](https://github.com/user-attachments/assets/5aa0f48c-de6a-46d9-b33c-29d340645e60)  
**Image with blur width of 24**  
![Image with blur width of 24](https://github.com/user-attachments/assets/b0038862-83d5-49cf-aa67-db774848ee5d)  
## Physically Based and Image Based Lighting
A BRDF calculation is applied to all objects within radius of a light. The Geometry term is GGX, the Fresnel term is Schlick's approximation, the Distribution term is also GGX.  
Image based Lighting is accomplished using an HDR sky dome which is sampled for the specular portion of the IBL. As well as an irradiance map for the diffuse portion.
