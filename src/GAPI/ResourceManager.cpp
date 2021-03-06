#include "ResourceManager.h"

#include "Mesh.h"
#include "Camera.h"
#include "Material.h"

#include "Texture2d.h"
#include "TextureCubeMap.h"

#include "Shader.h"

namespace engine
{
	namespace graphic
	{
		template <class Handler, class Obj>
		inline Handler createGeObject(std::map<Handler, Obj * > & mho, uint64_t id)
		{
			Handler handler;
			handler.SetId(id);
			mho.emplace(handler, new Obj);
			return handler;
		}


		void ResourceManager::Init()
		{
		
		}

		void ResourceManager::Deinit()
		{
		
		}

		GeMesh ResourceManager::CreateMesh() {
			return createGeObject(m_meshes, GenerateId());
		}

		Mesh * ResourceManager::GetMesh(const GeMesh & geMesh)
		{
			return m_meshes.at(geMesh);
		}
		GeMaterial ResourceManager::CreateMaterial()
		{
			return createGeObject(m_materialStorages, GenerateId());
		}
		MaterialObject * ResourceManager::GetMaterial(const GeMaterial & geMaterial)
		{
			return m_materialStorages.at(geMaterial);
		}
		GeCamera ResourceManager::CreateCamera()
		{
			return createGeObject(m_cameras, GenerateId());
		}
		Camera * ResourceManager::GetCamera(const GeCamera & geMesh)
		{
			return m_cameras.at(geMesh);
		}
		GeTexture2d ResourceManager::CreateTexture2d()
		{
			return createGeObject(m_textures2d, GenerateId());
		}
		Texture2d * ResourceManager::GetTexture2d(const GeTexture2d & geTexture2d)
		{
			return m_textures2d.at(geTexture2d);
		}
		GeTextureCubeMap ResourceManager::CreateTextureCubeMap()
		{
			return createGeObject(m_texturesCubeMap, GenerateId());
		}
		TextureCubeMap * ResourceManager::GetTextureCubeMap(const GeTextureCubeMap & geTexture2d)
		{
			return m_texturesCubeMap.at(geTexture2d);
		}
		GeShader ResourceManager::CreateShader()
		{
			return  createGeObject(m_shaders, GenerateId());
		}
		Shader * ResourceManager::GetShader(const GeShader & geShader)
		{
			return m_shaders.at(geShader);
		}
	}
}