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
**Physically Based Lighting using BRDF lighting function**  
Teapot, roughness: 0.001  
![image](https://github.com/user-attachments/assets/7eaf6486-749f-4a2f-b4bf-39bcac5cc43e)  
Teapot, roughness: .5  
![image](https://github.com/user-attachments/assets/37f13fc3-6599-4f70-b91c-f04d085fd500)  
Teapot, roughness: 1  
![image](https://github.com/user-attachments/assets/8f07b694-d0b6-49a4-9627-6a7f33bfd072)  
**Image Based Lighting (Diffuse on left, diffuse and specular on right)**  
Teapot, roughness: 0.001  
![image](https://github.com/user-attachments/assets/dcd710af-1ec8-4acb-84a5-f5befef239a5)
![image](https://github.com/user-attachments/assets/02c98de4-fcec-4244-9349-f21d2c46e309)  
Teapot, roughness: .1  
![image](https://github.com/user-attachments/assets/fb0b68f3-5e53-4b90-b3c1-934b37a8473b)
![image](https://github.com/user-attachments/assets/9ad43cda-7228-49a6-a645-9bd42994901e)  
Teapot, roughness: .5  
![image](https://github.com/user-attachments/assets/cc249f25-863b-4971-8420-ba5f01af9900)
![image](https://github.com/user-attachments/assets/4a69e2bc-ff8e-4f70-a445-5951453ff4e0)  
Teapot, roughness: 1  
![image](https://github.com/user-attachments/assets/ba61cde2-f734-4d3d-a9e2-2e027a6ec5e2)
![image](https://github.com/user-attachments/assets/d46b3727-ef0e-4b0e-975b-1427c0665d22)
## Ambient Occlusion
Ambient Occlusion is implemented using an AlchemyAO algorithm in combination with blurring.
