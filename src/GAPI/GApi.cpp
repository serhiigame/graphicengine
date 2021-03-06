#include "GApi.h"

#include <algorithm>

#include "../LLR/Llr.h"
#include "../LLR/LLREnum.h"
#include "../GU/ImageIO.h"
#include "../GU/SceneIO.h"

#include "Scene.h"
#include "ResourceManager.h"

#include "Mesh.h"
#include "Material.h"
#include "Camera.h"
#include "Texture2d.h"
#include "TextureCubeMap.h"
#include "Shader.h"
#include "ShaderInputs.h"
#include "PointLight.h"

#include "IResource.h"

namespace engine
{
	namespace graphic
	{
		const size_t w = 640;
		const size_t h = 480;

		namespace globalConstAttchment
		{
			constexpr int Transformation = 0;
			constexpr int Camera = 8;
		}

		namespace globalBufferAttchment
		{
			constexpr int Position = 0;
			constexpr int Normal = 1;
			constexpr int Uv = 2;
		}

		namespace globalTextureAttachment
		{
			constexpr int Color = 0;
			constexpr int Position = 1;
			constexpr int Normal = 2;
			constexpr int Uv = 3;
		}


		class TextureAdapter final
		{
		public:
			TextureAdapter(Llr * llr, size_t w, size_t h, ETextureColorPack colorPack, ETextureDataType dataType) : m_w(w), m_h(h)
			{
				m_llr = llr;

				if (ETextureColorPack::RED == colorPack)
				{
					switch (dataType)
					{
					case ETextureDataType::FLOAT:
						m_dataType = EDataType::FLOAT;
						m_textureSizedFormat = ETextureSizedFormat::RED_FLOAT;
						m_textureFormat = ETextureFormat::RED;

						break;
					case ETextureDataType::UNSIGNED_INT:
						m_dataType = EDataType::UNSIGNED_INT;
						m_textureSizedFormat = ETextureSizedFormat::RED_UNSIGNED_INT_32;
						m_textureFormat = ETextureFormat::RED_INTEGER;

						break;
					default:
						break;
					}
				
				}
				else if (ETextureColorPack::RGB == colorPack)
				{
					switch (dataType)
					{
					case ETextureDataType::FLOAT:
						m_dataType = EDataType::FLOAT;
						m_textureSizedFormat = ETextureSizedFormat::RGB_FLOAT;
						m_textureFormat = ETextureFormat::RGB;

						break;
					case ETextureDataType::UNSIGNED_INT:
						m_dataType = EDataType::UNSIGNED_INT;
						m_textureSizedFormat = ETextureSizedFormat::RGB_UNSIGNED_INT_32;
						m_textureFormat = ETextureFormat::RGB_INTEGER;

						break;
					default:
						break;
					}
				}
				else if (ETextureColorPack::RGBA == colorPack)
				{
					switch (dataType)
					{
					case ETextureDataType::FLOAT:
						m_dataType = EDataType::FLOAT;
						m_textureSizedFormat = ETextureSizedFormat::RGBA_FLOAT;
						m_textureFormat = ETextureFormat::RGBA;

						break;
					case ETextureDataType::UNSIGNED_INT:
						m_dataType = EDataType::UNSIGNED_INT;
						m_textureSizedFormat = ETextureSizedFormat::RGBA_UNSIGNED_INT_32;
						m_textureFormat = ETextureFormat::RGBA_INTEGER;

						break;
					default:
						break;
					}
				}
			}


			ITexture2D * CreateTexture2d()
			{
				return m_llr->CreateTexture2d(m_w, m_h, m_textureFormat, m_textureSizedFormat, m_dataType);
			}

			ITextureCubeMap * CreateTextureCubemap()
			{
				return m_llr->CreateTextureCubeMap(m_w, m_h, m_textureFormat, m_textureSizedFormat, m_dataType);
			}

		private:
			size_t m_w = 0;
			size_t m_h = 0;
			ETextureFormat m_textureFormat = ETextureFormat::NONE;
			ETextureSizedFormat m_textureSizedFormat = ETextureSizedFormat::NONE;
			EDataType m_dataType = EDataType::NONE;

			Llr * m_llr;
		};

		struct RanderStageShaderInputs
		{
			Texture2dBindings Texture2dInputs;
			TextureCubeMapBindings TextureCubeMapInputs;
			ConstantBindings ConstantInputs;
		};

		struct RanderStageShaderOutputs
		{
			Texture2dBindings Texture2dInputs;
		};

		struct RenderStage
		{
			IRenderPass * RenderPass;
			GeShader Shader;

			std::vector<RanderStageShaderInputs> ShaderInputs;
			RanderStageShaderOutputs ShaderOutputs;

			IFramebuffer * OutputFramebuffer;

		};

		class GApiImpl
		{
		public:
			void Init()
			{
				m_llr = Llr::CreateLlr(ELlrType::OPEN_GL);
				
				this->InitGBufferRenderState();
				this->InitLightRenderState();
				this->InitPostEffectsRenderState();
			}

			void Deinit()
			{

				if (m_gBufferFb)
				{
					delete m_gBufferFb;
					m_gBufferFb = nullptr;
				}

				if (m_acumuFb)
				{
					delete m_acumuFb;
					m_acumuFb = nullptr;
				}

				/*if (m_diffuseTexture)
				{
					delete m_diffuseTexture;
					m_diffuseTexture = nullptr;
				}

				if (m_positionTexture)
				{
					delete m_positionTexture;
					m_positionTexture = nullptr;
				}


				if (m_normalTexture)
				{
					delete m_normalTexture;
					m_normalTexture = nullptr;
				}

				if (m_uvTexture)
				{
					delete m_uvTexture;
					m_uvTexture = nullptr;
				}*/


				if (m_rectIndexBuffer)
				{
					delete m_rectIndexBuffer;
					m_rectIndexBuffer = nullptr;
				}


				if (m_acumTexture)
				{
					delete m_acumTexture;
					m_acumTexture = nullptr;
				}

				Llr::DeleteLlr(m_llr);
			}

			void BindMesh(const GeMesh geMesh) {


				const Mesh * mesh = m_resourceManager.GetMesh(geMesh);

				Shader * gShader = m_resourceManager.GetShader(m_gBuffRenderStage.Shader);
				
				gShader->m_shader->AttachAttribute(mesh->m_pos, globalBufferAttchment::Position, 0, 3, EDataType::FLOAT);
				gShader->m_shader->AttachAttribute(mesh->m_norm, globalBufferAttchment::Normal, 0, 3, EDataType::FLOAT);
				gShader->m_shader->AttachAttribute(mesh->m_uv, globalBufferAttchment::Uv, 0, 2, EDataType::FLOAT);

				IBuffer * idx = mesh->m_idx;
				size_t count = idx->GetSize() / sizeof(unsigned int);
				gShader->m_shader->AttachAttribute(idx, 0, 0, count, EDataType::UNSIGNED_INT);

				gShader->m_shader->AttachConstant(mesh->m_transform, globalConstAttchment::Transformation);
			}

			Scene * CreareScene()
			{
				Scene * scene = new Scene;
				m_resources.insert(scene);


				//XXX: please fix me
				m_scene = scene;

				return scene;
			}

			void DeleteScene(Scene * scene)
			{
				m_resources.erase(scene);
				delete scene;
			}

			GeMesh CreateMesh(const RawMeshData & meshData)
			{
				GeMesh geMesh = m_resourceManager.CreateMesh();
				Mesh * mesh = m_resourceManager.GetMesh(geMesh);
				


				//m_resources.insert(mesh);

				mesh->m_pos = m_llr->CreateBuffer(sizeof(meshData.Positions[0]) * meshData.Positions.size());
				mesh->m_norm = m_llr->CreateBuffer(sizeof(meshData.Normals[0]) * meshData.Normals.size());
				mesh->m_uv = m_llr->CreateBuffer(sizeof(meshData.Positions[0]) * meshData.Positions.size());
				mesh->m_idx = m_llr->CreateIndexBuffer(sizeof(meshData.Indexes[0]) * meshData.Indexes.size());

				mesh->m_pos->Write(0, sizeof(meshData.Positions[0]) * meshData.Positions.size(), meshData.Positions.data());
				mesh->m_norm->Write(0, sizeof(meshData.Normals[0]) * meshData.Normals.size(), meshData.Normals.data());
				mesh->m_uv->Write(0, sizeof(meshData.Uv[0]) * meshData.Uv.size(), meshData.Uv.data());
				mesh->m_idx->Write(0, sizeof(meshData.Indexes[0]) * meshData.Indexes.size(), meshData.Indexes.data());

				Mat4f transformMat;
				transformMat.MakeIdentity();
				mesh->m_transform = m_llr->CreateConatant(sizeof(Mat4f));
				mesh->m_transform->Write(0, sizeof(transformMat), &transformMat);

				return geMesh;
			}

			void DeleteMesh(GeMesh mesh)
			{
				//TODO: delere via resource manager
				//m_resources.erase(mesh);
				//delete mesh;
			}

			void SetMeshTransform(GeMesh geMesh, const Mat4f & transform)
			{
				Mesh * mesh = m_resourceManager.GetMesh(geMesh);
				mesh->m_transform->Write(0, sizeof(Mat4f), &transform[0]);
			}

			void SetMeshMaterialInstance(GeMesh mesh, MaterialInstance * materialInstance)
			{
				m_materialManager.SetMeshMaterialInstance(mesh, materialInstance);
				//m_scene->SetMeshMaterialInstance(mesh, materialInstance);
			}

			bool CreateGbuffer(ShaderInfo & shaderInfo)
			{
				GeShader geShader = CreateShader(shaderInfo.vertShaderPath, shaderInfo.fragShaderPath);
				if (!geShader.IsValid())
				{
					return false;
				}

				Shader * shader = m_resourceManager.GetShader(geShader);

				
				RanderStageShaderInputs shaderInputs;
				RanderStageShaderOutputs shaderOutputs;

				for (auto output : shaderInfo.Outputs)
				{
					TextureAdapter textureAdapter(m_llr, w, h, output.TextureFormat, output.DataType);

					ITexture2D * texture = textureAdapter.CreateTexture2d();
					shaderOutputs.Texture2dInputs.emplace(output.Binding, texture);
				}

				//m_materialGbufferObject.m_shaderDesc = shaderInfo;

				m_gBuffRenderStage = CreateGbufferRenderStege(geShader, shaderInputs, shaderOutputs, m_gBufferFb);

				m_materialManager.RegisterGbuffer(geShader, shaderInfo);

				return true;
			}

			bool CreateEnvMap(ShaderInfo & shaderInfo)
			{
				//TODO: GenMaterialId
				int materialId = m_materialIdCounter++;

				GeShader geShader = CreateShader(shaderInfo.vertShaderPath, shaderInfo.fragShaderPath);
				if (!geShader.IsValid())
				{
					return false;
				}

				Shader * shader = m_resourceManager.GetShader(geShader);
				
				shader->m_inputs = shaderInfo.Inputs;
				shader->m_outputs = shaderInfo.Outputs;

				RanderStageShaderInputs globalShaderInputs;
				globalShaderInputs.Texture2dInputs = m_gBuffRenderStage.ShaderOutputs.Texture2dInputs;

				m_EnvMappingStage = CreateLightRenderStege(geShader, globalShaderInputs, m_acumuFb);

				return true;
			}

			GeMaterial CreateMaterial(ShaderInfo & shaderInfo)
			{
				//TODO: GenMaterialId
				int materialId = m_materialIdCounter++;

				GeShader geShader = CreateShader(shaderInfo.vertShaderPath, shaderInfo.fragShaderPath);
				if (!geShader.IsValid())
				{
					return GeMaterial();
				}

				Shader * shader = m_resourceManager.GetShader(geShader);


				GeMaterial geMaterial = m_resourceManager.CreateMaterial();
				MaterialObject * material = m_resourceManager.GetMaterial(geMaterial);

				material->SetMaterilId(materialId);

				material->SetMaterialShaderDesc( shaderInfo );
				material->SetMaterialShader( shader );

				RanderStageShaderInputs globalShaderInputs;

				globalShaderInputs.Texture2dInputs = m_gBuffRenderStage.ShaderOutputs.Texture2dInputs;

				RenderStage renderStage = CreateLightRenderStege(geShader, globalShaderInputs, m_acumuFb);

				m_lightRenderStages.push_back(renderStage);

				m_materialManager.RegistreMaterial(geMaterial);

				return geMaterial;
			}

			MaterialInstance * CreateMaterialInstance( const GeMaterial & geMaterial)
			{
	
				//MaterialInstance * materialInstance = new MaterialInstance(&m_materialGbufferObject, &materialLightingObject[0]);
				MaterialObject * material = m_resourceManager.GetMaterial(geMaterial);
				MaterialInstance * materialInstance = new MaterialInstance(material);

				for (auto inpudDesc : m_materialManager.GetGbufferShaderInfo().Inputs)
				{
					if (inpudDesc->Type == EShaderInputType::TEXTURE_2D)
					{
						ShaderInputTexture2d * shaderTexture2dInput = new ShaderInputTexture2d;
						ShaderInputTexture2dInfoPtr shaderTexture2dInputInfo = std::static_pointer_cast<ShaderInputTexture2dInfo>(inpudDesc);
						shaderTexture2dInput->SetTexture(shaderTexture2dInputInfo->FallbackTexture);

						//m_materialManager.

						materialInstance->AddMaterialInput(shaderTexture2dInput, shaderTexture2dInputInfo);

						//materialInstance->m_materialGbufferInputs[inpudDesc.Binding] = shaderTexture2dInput;
					}
					
					
					//materialInstance->m_materialGbufferInputs[inpudDesc.Binding] = inpudDesc.Fallback;
				}

				//for (auto inpudDesc : materialLightingObject->GetMaterilShaderDesc().Inputs)
				//{
					//materialInstance->m_materialLightingInputs[inpudDesc.Binding] = inpudDesc.Fallback;
				//}


				m_materialManager.RegistreMaterialInstance(geMaterial, materialInstance);
				//TODO: use material instance handlers
				return materialInstance;
			}

			void SetDefaultMaterialInstance(MaterialInstance * material)
			{
				m_materialManager.SetDefaultMaterialInstance(material);
			}

			void SetMaterialInstanceParameterF4(MaterialInstance * material, const std::string & paramName, const Vec4f & param)
			{
				IShaderInput * shaderInput = FindShaderInput(material, paramName);

				if (!shaderInput)
				{
					return;
				}

				if (shaderInput->GetShaderInputType() != EShaderInputType::CONSTANT)
				{
					return;
				}

				SetShaderInputConstant(shaderInput, param);
			}

			void SetMaterialInstanceParameterTex2d(MaterialInstance * material, const std::string & paramName, GeTexture2d geTexture)
			{
				IShaderInput * shaderInput = FindShaderInput(material, paramName);

				if (!shaderInput)
				{
					return;
				}

				if (shaderInput->GetShaderInputType() != EShaderInputType::TEXTURE_2D)
				{
					return;
				}

				SetShaderInputTexture2d(shaderInput, geTexture);
			}

			//void DeleteMaterial(Material * material) {
				//m_resources.erase(material);
			//	delete material;
			//}

			GeCamera CreateCamera()
			{
				GeCamera geCamera = m_resourceManager.CreateCamera();
				Camera * camera = m_resourceManager.GetCamera(geCamera);
			
				Mat4f viewMatrix;
				Mat4f projectionMatrix;

				viewMatrix.MakeIdentity();
				projectionMatrix.MakeIdentity();

				camera->m_cameraTransforms = m_llr->CreateConatant(sizeof(viewMatrix) + sizeof(projectionMatrix));

				camera->m_cameraTransforms->Write(0, sizeof(viewMatrix), &viewMatrix[0]);
				camera->m_cameraTransforms->Write(sizeof(projectionMatrix), sizeof(projectionMatrix), &projectionMatrix[0]);

				return geCamera;
			}

			void DeleteCamera(GeCamera camera)
			{				
				//m_resources.erase(camera);
				//delete camera;
			}

			void SetCameraView(GeCamera geCamera, const Mat4f & view)
			{
				Camera * camera = m_resourceManager.GetCamera(geCamera);
				Llr * llr = GetLlr();
				camera->m_cameraTransforms->Write(0, sizeof(Mat4f), &view[0]);
			}

			void SetCameraProjection(GeCamera geCamera, const Mat4f & projection)
			{
				Camera * camera = m_resourceManager.GetCamera(geCamera);
				camera->m_cameraTransforms->Write(sizeof(Mat4f), sizeof(Mat4f), &projection[0]);
			}

			GeTexture2d CreateTexture2d(size_t w, size_t h, ETextureColorPack colorPack, ETextureDataType dataType)
			{
				GeTexture2d geTexture2d = m_resourceManager.CreateTexture2d();
				Texture2d * texture2d = m_resourceManager.GetTexture2d(geTexture2d);


				TextureAdapter textureAdapter(m_llr, w, h, colorPack, dataType);
				texture2d->m_texture = textureAdapter.CreateTexture2d();
				return geTexture2d;
			}

			void DeleteTexture2d(GeTexture2d texture2d)
			{
				//m_resources.erase(texture2d);
				//delete texture2d;
			}

			void Texture2dWriteImage(GeTexture2d geTexture2d, int xOffset, int yOffset, int w, int h, const void * data)
			{
				Texture2d * texture = m_resourceManager.GetTexture2d(geTexture2d);
				texture->WriteImage(xOffset, yOffset, w, h, data);
			}

			GeShader CreateShader(const std::string & vert, const std::string & frag)
			{
				GeShader geShader = m_resourceManager.CreateShader();
				Shader * shader = m_resourceManager.GetShader(geShader);
				shader->m_shader = m_llr->CreateShader(vert, frag);

				return geShader;
			}

			void DeleteShader(Shader * shader)
			{
				// TODO:
				//m_resources.erase(shader);
				//delete shader;
			}

			GeTextureCubeMap CreateTextureCubeMap(size_t size, ETextureColorPack colorPack, ETextureDataType dataType)
			{
				GeTextureCubeMap geTextureCubeMap = m_resourceManager.CreateTextureCubeMap();
				TextureCubeMap * textureCubeMap = m_resourceManager.GetTextureCubeMap(geTextureCubeMap);

				TextureAdapter textureAdapter(m_llr, size, size, colorPack, dataType);

				textureCubeMap->m_texture = textureAdapter.CreateTextureCubemap();;
				textureCubeMap->m_size = size;
				return geTextureCubeMap;
			}

			void TextureCubeMapWriteFace(GeTextureCubeMap geTextureCubeMap, ETextureCubeMapFace face, const float * data)
			{
				TextureCubeMap * texture = m_resourceManager.GetTextureCubeMap(geTextureCubeMap);
				texture->WriteFace(face, data);
			}

			void DeleteTextureCubeMap(GeTextureCubeMap textureCubeMap)
			{
				// TODO:
				//m_resources.erase(textureCubeMap);
				//delete textureCubeMap;
			}

			PointLight * CreatePointLight()
			{
				PointLight * pointLight = new PointLight;
				m_resources.insert(pointLight);

				Vec3f lightPosition({ 0.f, 0.f, 0.f });

				pointLight->m_pointLight = m_llr->CreateConatant(sizeof(lightPosition));
				pointLight->m_pointLight->Write(0, sizeof(lightPosition), &lightPosition[0]);

				return pointLight;
			}

			void DeletePointLight(PointLight * pointLight)
			{
				m_resources.erase(pointLight);
				delete pointLight;
			}

			void ClearFremebuffers()
			{
				m_gBufferFb->Cleare();
				m_acumuFb->Cleare();
			}

			//void RenderGeometry(const std::vector<GeMesh> meshes, const Camera * camera)
			void RenderGeometry(Scene * scene)
			{
				Shader * shader = m_resourceManager.GetShader(m_gBuffRenderStage.Shader);
				GeCamera geCamera = scene->GetCamera();
				const Camera * camera = m_resourceManager.GetCamera(geCamera);


				shader->m_shader->AttachConstant(camera->m_cameraTransforms, globalConstAttchment::Camera);

				for (auto mesh : scene->GetMeshes())
				{
					MaterialInstance * materialInstance = m_materialManager.GetMeshMaterialInstance(mesh);
					if (!materialInstance)
					{
						materialInstance = m_materialManager.GetDefaultMaterialInstance();

						if (!materialInstance)
						{
							continue;
						}
					}

					BindMesh(mesh);

					for (auto gbuffInput : m_materialManager.GetGbufferInputs(materialInstance))
					{
						int binding = gbuffInput.first;
						IShaderInput * shaderInput = gbuffInput.second;

						if (shaderInput->GetShaderInputType() == EShaderInputType::TEXTURE_2D)
						{
							GeTexture2d geTexture = ((ShaderInputTexture2d *)shaderInput)->GetTexture();

							Texture2d * texture = nullptr;
							if (geTexture.IsValid())
							{
								texture = m_resourceManager.GetTexture2d(geTexture);
								shader->m_shader->AttachTexture2d(texture->m_texture, binding);
							}
							else
							{
								//TODO: warning 
							}
						}
					}

					IRenderPass * renderPass = m_gBuffRenderStage.RenderPass;
					IFramebuffer * framebuffer = m_gBuffRenderStage.OutputFramebuffer;
					renderPass->Execute(shader->m_shader, framebuffer);
				}
			}

			void RenderLights(Scene * scene)
			{
				for (auto lightRenderStage : m_lightRenderStages)
				{
					IRenderPass * renderPass = lightRenderStage.RenderPass;
					IShader * shader = m_resourceManager.GetShader(lightRenderStage.Shader)->m_shader;
					IFramebuffer * acumulator = lightRenderStage.OutputFramebuffer;

					GeTextureCubeMap skybox = scene->GetSkybox();
					if (skybox.IsValid())
					{
						shader->AttachTextureCubeMap(m_resourceManager.GetTextureCubeMap(skybox)->m_texture, 10);
					}

					for (PointLight * pointLight : scene->GetPointLight())
					{
						// TODO: set global constant
						shader->AttachConstant(pointLight->m_pointLight, 1);


						for (auto material : m_materialManager.GetMaterials())
						{
							//for (auto materialInstance : material.GetMaterialInstances())
							for (auto materialInstance : m_materialManager.GetMaterialInstances(material))
							{
								for (auto lightingInput : m_materialManager.GetGbufferInputs(materialInstance))
								{
									int binding = lightingInput.first;
									IShaderInput * shaderInput = lightingInput.second;

									if (shaderInput->GetShaderInputType() == EShaderInputType::CONSTANT)
									{
										shader->AttachConstant(((ShaderInputConstant *)shaderInput)->m_constant, binding);
									}
									else if (shaderInput->GetShaderInputType() == EShaderInputType::TEXTURE_2D)
									{
										GeTexture2d geTexture = ((ShaderInputTexture2d *)shaderInput)->GetTexture();
										Texture2d * texture = m_resourceManager.GetTexture2d(geTexture);
										shader->AttachTexture2d(texture->m_texture, binding);
									}
								}

								renderPass->Execute(shader, acumulator);
							}
						}
					}
				}
			}

			void RenderEnvironment(Scene * scene)
			{
				Shader * shader = m_resourceManager.GetShader(m_EnvMappingStage.Shader);
				GeCamera geCamera = scene->GetCamera();
				const Camera * camera = m_resourceManager.GetCamera(geCamera);

				shader->m_shader->AttachConstant(camera->m_cameraTransforms, globalConstAttchment::Camera);
				

				for (auto shaderInput : shader->m_inputs)
				{
					if (shaderInput->Name == "skybox" && shaderInput->Type == EShaderInputType::TEXTURE_CUBEMAP)
					{
						GeTextureCubeMap cubemap = scene->GetSkybox();
						
						ShaderInputTextureCubeMapInfoPtr shaderInputCubemap = std::static_pointer_cast<ShaderInputTextureCubeMapInfo>(shaderInput);

						
						TextureCubeMap * texture = m_resourceManager.GetTextureCubeMap((scene->GetSkybox().IsValid()) ? scene->GetSkybox() : shaderInputCubemap->FallbackTexture);
						shader->m_shader->AttachTextureCubeMap(texture->m_texture, shaderInput->Binding);
					}
				
				}

				IRenderPass * renderPass = m_EnvMappingStage.RenderPass;
				IFramebuffer * framebuffer = m_EnvMappingStage.OutputFramebuffer;
				renderPass->Execute(shader->m_shader, framebuffer);
			}

			void RenderPEffects()
			{
				for (auto pEffectRenderStage : m_pEffectRenderStages)
				{
					IRenderPass * renderPass = pEffectRenderStage.RenderPass;
					IShader * shader = m_resourceManager.GetShader( pEffectRenderStage.Shader)->m_shader;
					renderPass->Execute(shader);
				}
			}

			Llr * GetLlr() { return m_llr; }


		private:

			void InitGBufferRenderState()
			{
				m_gBufferFb = m_llr->CreateFramebuffer(w, h);
				m_renderbuffer = m_llr->CreateRenderbuffer(w, h);

				m_gBufferFb->SetRenderbuffer(m_renderbuffer);
			}

			void InitLightRenderState() {
				m_acumuFb = m_llr->CreateFramebuffer(w, h);

				TextureAdapter textureAdapter(m_llr, w, h, ETextureColorPack::RGBA, ETextureDataType::FLOAT);

				m_acumTexture = textureAdapter.CreateTexture2d();

				//TODO: use ShaderOutput for m_acumTexture
				m_acumuFb->AttachTextures2d(Texture2dBindings({ { 0, m_acumTexture } }));
				
			}

			void InitPostEffectsRenderState() {
				GeShader shader = CreateShader("../res/shaders/BlackWhite.vrt", "../res/shaders/BlackWhite.pxl");

				RanderStageShaderInputs globalShaderInputs;

				// TODO: use global
				globalShaderInputs.Texture2dInputs[0] = m_acumTexture;

				RenderStage renderStage = CreatePEffectRenderStage(shader
					, globalShaderInputs
					, nullptr);

				m_pEffectRenderStages.push_back(renderStage);
			}


			RenderStage CreateGbufferRenderStege(const GeShader & geShader
				, const RanderStageShaderInputs & shaderInputs
				, const RanderStageShaderOutputs & shaderOutputs
				, IFramebuffer * output)
			{
				if (!m_rectIndexBuffer)
				{
					const GLuint indexes[]{ 0,1,2, 3,2,0 };
					m_rectIndexBuffer = m_llr->CreateIndexBuffer(sizeof(indexes));
					m_rectIndexBuffer->Write(0, sizeof(indexes), indexes);
				}

				RenderStage renderStage;

				renderStage.Shader = geShader;
				renderStage.OutputFramebuffer = output;
				renderStage.RenderPass = m_llr->CreateRenderPass();

				if(!shaderOutputs.Texture2dInputs.empty())
				{
					renderStage.ShaderOutputs.Texture2dInputs = shaderOutputs.Texture2dInputs;
					output->AttachTextures2d(shaderOutputs.Texture2dInputs);
				}

				IBuffer * rectIndexBuffer = GetRectIndexBuffer();
				uint32_t rectIndexCount = GetRectIndexCount();
				EDataType rectIndexType = GetRectIndexType();
				IShader * shader = m_resourceManager.GetShader(renderStage.Shader)->m_shader;
				shader->AttachAttribute(rectIndexBuffer, 0, 0, rectIndexCount, rectIndexType);

				return renderStage;
			}

			RenderStage CreateLightRenderStege(GeShader geShader
				, const RanderStageShaderInputs & globalShaderInputs
				, IFramebuffer * output)
			{
				if (!m_rectIndexBuffer)
				{
					const GLuint indexes[]{ 0,1,2, 3,2,0 };
					m_rectIndexBuffer = m_llr->CreateIndexBuffer(sizeof(indexes));
					m_rectIndexBuffer->Write(0, sizeof(indexes), indexes);
				}

				RenderStage renderStage;

				renderStage.Shader = geShader;
				renderStage.OutputFramebuffer = output;
				renderStage.RenderPass = m_llr->CreateRenderPass();

				IShader * shader = m_resourceManager.GetShader(renderStage.Shader)->m_shader;
				for (auto texture2dInput : globalShaderInputs.Texture2dInputs)
				{
					const uint32_t textureLocation = texture2dInput.first;
					const ITexture2D * texture = texture2dInput.second;

					shader->AttachTexture2d(texture, textureLocation);
				}

				for (auto textureCubeMapInput : globalShaderInputs.TextureCubeMapInputs)
				{
					const uint32_t textureLocation = textureCubeMapInput.first;
					const ITextureCubeMap * texture = textureCubeMapInput.second;

					shader->AttachTextureCubeMap(texture, textureLocation);
				}

				IBuffer * rectIndexBuffer = GetRectIndexBuffer();
				uint32_t rectIndexCount = GetRectIndexCount();
				EDataType rectIndexType = GetRectIndexType();
				shader->AttachAttribute(rectIndexBuffer, 0, 0, rectIndexCount, rectIndexType);

				return renderStage;
			}

			RenderStage CreatePEffectRenderStage(const GeShader & geShader, const RanderStageShaderInputs & inputs, IFramebuffer * output)
			{
				RenderStage renderStage;

				renderStage.Shader = geShader;
				renderStage.RenderPass = m_llr->CreateRenderPass();

				IShader * shader = m_resourceManager.GetShader(renderStage.Shader)->m_shader;
				for (auto textureInput : inputs.Texture2dInputs)
				{
					const uint32_t textureLocation = textureInput.first;
					const ITexture2D * texture = textureInput.second;

					shader->AttachTexture2d(texture, textureLocation);
				}

				IBuffer * rectIndexBuffer = GetRectIndexBuffer();
				uint32_t rectIndexCount = GetRectIndexCount();
				EDataType rectIndexType = GetRectIndexType();
				shader->AttachAttribute(rectIndexBuffer, 0, 0, rectIndexCount, rectIndexType);

				return renderStage;
			}

			template<class T>
			IShaderInput * CreateShaderInputConstant()
			{
				IConstant * constant = m_llr->CreateConatant(sizeof(T));
				ShaderInputConstant * shaderInput = new ShaderInputConstant;

				shaderInput->m_constant = constant;

				return shaderInput;
			}


			//XXX: fix me
			IShaderInput * FindShaderInput(MaterialInstance * material, const std::string & paramName)
			{

				for (auto & input : material->GetMaterialInputs())
				{
					if (input.ShaderInputInfo->Name == paramName)
					{
						return input.ShaderInput;
					}
				}

				return nullptr;
			}

			template<class T>
			void SetShaderInputConstant(IShaderInput * shaderInput, const T & data)
			{
				if (!shaderInput)
				{
					return;
				}

				if (shaderInput->GetShaderInputType() != EShaderInputType::CONSTANT)
				{
					return;
				}

				ShaderInputConstant * constant = ((ShaderInputConstant *)shaderInput);
				constant->m_constant->Write(0, sizeof(T), &data);
			}

			void SetShaderInputTexture2d(IShaderInput * shaderInput, GeTexture2d texture)
			{
				if (!shaderInput)
				{
					return;
				}

				if (shaderInput->GetShaderInputType() != EShaderInputType::TEXTURE_2D)
				{
					return;
				}

				ShaderInputTexture2d * texture2d = ((ShaderInputTexture2d *)shaderInput);
				texture2d->m_texture = texture;
			}

			void DeleteShaderInput(IShaderInput * shaderInput)
			{
				delete shaderInput;
			}

			IBuffer * GetRectIndexBuffer()
			{
				if (!m_rectIndexBuffer)
				{
					const GLuint indexes[]{ 0,1,2, 3,2,0 };
					m_rectIndexBuffer = m_llr->CreateIndexBuffer(sizeof(indexes));
					m_rectIndexBuffer->Write(0, sizeof(indexes), indexes);
				}

				return m_rectIndexBuffer;
			}

			uint32_t GetRectIndexCount() const
			{
				return 6;
			}

			EDataType GetRectIndexType()
			{
				return  EDataType::UNSIGNED_INT;
			}

			Llr * m_llr;

			ResourceManager m_resourceManager;
			MaterialManager m_materialManager;

			Scene * m_scene;

			IFramebuffer * m_gBufferFb = nullptr;
			IFramebuffer * m_acumuFb = nullptr;

			IRenderbuffer * m_renderbuffer = nullptr;

			IBuffer * m_rectIndexBuffer = nullptr;

			ITexture2D * m_acumTexture = nullptr;

			RenderStage m_gBuffRenderStage;
			RenderStage m_EnvMappingStage;
			std::vector<RenderStage> m_lightRenderStages;
			std::vector<RenderStage> m_pEffectRenderStages;

			
			

			//MaterialObject m_materialGbufferObject;
			std::vector<MaterialObject> m_materialLightingObjects;

			std::multimap<int , Mesh *> m_materialIdMeshRelationship;

			int m_materialIdCounter = 1;
			int m_materialInstanceIdCounter = 1;


			std::set<IResource *> m_resources;
			//std::map<int, MaterialObject> m_materialObjects; // materialId / materialObject
		};
		
		GApi::GApi()
		{
		}
		void GApi::Init()
		{
			m_impl = new GApiImpl;
			m_impl->Init();
		}
		void GApi::Deinit()
		{
			m_impl->Deinit();
			delete m_impl;
		}
		Scene * GApi::CreateScene()
		{
			return m_impl->CreareScene();
		}

		void GApi::DeleteScene(Scene * scene)
		{
			m_impl->DeleteScene(scene);
		}

		GeMesh GApi::CreateMesh(const RawMeshData & meshData)
		{
			return m_impl->CreateMesh(meshData);
		}

		void GApi::DeleteMesh(GeMesh mesh)
		{
			m_impl->DeleteMesh(mesh);
		}
		bool GApi::CreateGbuffer(ShaderInfo & shaderDesc)
		{
			return m_impl->CreateGbuffer(shaderDesc);
		}

		bool GApi::CreateEnvMap(ShaderInfo & shaderDesc) {
			return m_impl->CreateEnvMap(shaderDesc);
		}

		GeMaterial GApi::CreateMaterial(ShaderInfo & shaderDesc)
		{
			return m_impl->CreateMaterial(shaderDesc);
		}

		MaterialInstance * GApi::CreateMaterialInstance(const GeMaterial & material)
		{
			return m_impl->CreateMaterialInstance(material);
		}

		void GApi::SetMaterialParameterF4(MaterialInstance * material, const std::string & paramName, const Vec4f & param)
		{
			m_impl->SetMaterialInstanceParameterF4(material, paramName, param);
		}

		void GApi::SetMaterialParameterTex2d(MaterialInstance * material, const std::string & paramName, GeTexture2d texture)
		{
			m_impl->SetMaterialInstanceParameterTex2d(material, paramName, texture);
		}

		void GApi::SetDefaultMaterialInstance(MaterialInstance * material)
		{
			m_impl->SetDefaultMaterialInstance(material);
		}

		GeCamera GApi::CreateCamera()
		{
			return m_impl->CreateCamera();
		}

		void GApi::DeleteCamera(GeCamera camera)
		{
			m_impl->DeleteCamera(camera);
		}

		GeTexture2d GApi::CreateTexture2d(const size_t width, const size_t heigth, ETextureColorPack colorPack, ETextureDataType dataType)
		{
			return   m_impl->CreateTexture2d(width, heigth, colorPack, dataType);
		}

		void GApi::DeleteTexture2d(GeTexture2d texture2d)
		{
			m_impl->DeleteTexture2d(texture2d);
		}

		void GApi::Texture2dWriteImage(GeTexture2d texture2d, int xOffset, int yOffset, int w, int h, const void * data)
		{
			m_impl->Texture2dWriteImage(texture2d, xOffset, yOffset, w, h, data);
		}

		GeTextureCubeMap GApi::CreateTextureCubeMap(size_t size)
		{
			return m_impl->CreateTextureCubeMap(size, ETextureColorPack::RGBA, ETextureDataType::FLOAT);
		}

		void GApi::TextureCubeMapWriteFace(GeTextureCubeMap cubemap, int face, const float * data)
		{
			m_impl->TextureCubeMapWriteFace(cubemap, (ETextureCubeMapFace)face, data);
		}

		void GApi::DeleteTextureCubeMap(GeTextureCubeMap textureCubeMap)
		{
			m_impl->DeleteTextureCubeMap(textureCubeMap);
		}

		void GApi::SetCameraView(GeCamera camera, const Mat4f & view)
		{
			m_impl->SetCameraView(camera, view);
		}

		void GApi::SetCameraProjection(GeCamera geCamera, const Mat4f & projection)
		{
			m_impl->SetCameraProjection(geCamera, projection);
		}

		PointLight * GApi::CreatePointLight()
		{
			return m_impl->CreatePointLight();
		}

		void GApi::DeletePointLight(PointLight * pointLight)
		{
			m_impl->DeletePointLight(pointLight);
		}

		void GApi::SetMeshTransform(GeMesh geMesh, const Mat4f & transform)
		{
			m_impl->SetMeshTransform(geMesh, transform);
		}

		void GApi::SetMeshMaterialInstance(GeMesh mesh, MaterialInstance * material)
		{
			m_impl->SetMeshMaterialInstance(mesh, material);
		}

		void GApi::SetPointLightPosition(PointLight * light, const Vec3f & position)
		{
			light->m_pointLight->Write(0, sizeof(Vec3f), &position[0]);
		}

		void GApi::SetPointLightIntensity(PointLight * light, const float intensity)
		{
			light->m_pointLight->Write(sizeof(Vec3f), sizeof(float), &intensity);
		}

		void GApi::SetSceneCamera(Scene * scene, GeCamera camera)
		{
			scene->SetCamera(camera);
		}

		void GApi::SetSceneSkybox(Scene * scene, GeTextureCubeMap skybox)
		{
			scene->SetSkybox(skybox);
		}

		void GApi::AddSceneMesh(Scene * scene, GeMesh mesh)
		{
			scene->AddMesh(mesh);
		}

		void GApi::AddScenePointLight(Scene * scene, PointLight * pointLight)
		{
			scene->AddPointLight(pointLight);
		}

		GeShader GApi::CreateShader(const std::string & vert, const std::string & frag)
		{
			return m_impl->CreateShader(vert, frag);
		}

		ERenderStatus GApi::Render(Scene * scene, ERenderMode mode)
		{
			m_impl->ClearFremebuffers();

			m_impl->RenderGeometry(scene);

			m_impl->RenderLights(scene);

			m_impl->RenderEnvironment(scene);

			m_impl->RenderPEffects();

			return ERenderStatus::SUCCESS;
		}
	}
}
