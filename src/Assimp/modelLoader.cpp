#include <PCH.h>

#include "modelLoader.h"
#include "Core/DebugTools.h"
#include "Core/Scene.h"
#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"

#include "Core/Components/Entity.h"
#include "Core/Components/Vertex.h"
#include "Core/Components/Mesh.h"

#include <Static/Renderer/Renderer.h>

#include<assimp/quaternion.h>
#include<assimp/vector3.h>
#include<assimp/matrix4x4.h>
#include<glm/glm.hpp>
#include<glm/gtc/quaternion.hpp>


namespace PetrolEngine {

	std::mutex  ModelLoader::mutex;
	ModelLoader ModelLoader::modelLoader;

	ModelLoader& ModelLoader::Get() { return modelLoader; }

	Entity* ModelLoader::loadModel(const char* _path, Scene* _scene) {
		auto pPath = std::filesystem::current_path();
		auto path  = std::filesystem::path(_path);
		
		auto filename   = path.filename().string();
		auto workingDir = path.remove_filename();

		std::filesystem::current_path(workingDir);

		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile(filename, aiProcess_GenSmoothNormals | aiProcess_Triangulate);
		//  aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace
		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
            LOG(std::string("ASSIMP: ") + importer.GetErrorString(), 2);
			return {};
		}

        LOG(std::string("Running down the Root node of model(") + _path + ")", 1);
	
		Entity* model = (Entity*)_scene->createGameObject(filename.c_str());

		modelLoader.processNode(scene->mRootNode, scene, model);

		//for (auto& process : modelLoader.meshProcesses) process.wait();

		//modelLoader.meshProcesses.clear();

		std::filesystem::current_path(pPath);

		return model;
	}
	Entity* ModelLoader::loadModel(const char* _path, Entity* _parent) {

		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile(_path, aiProcess_GenNormals);
		//  aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace
		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
            LOG(std::string("ASSIMP:: ") + importer.GetErrorString(), 2);
			return {};
		}

        LOG(std::string("Running down the Root node of model(") + _path + ")", 1);

		std::string fileName(_path);

		Entity* model = (Entity*)_parent->getScene()->createGameObject(fileName.substr(fileName.find_last_of("\\/") + 1).c_str(), _parent);

		modelLoader.processNode(scene->mRootNode, scene, model);

		//for (auto& process : modelLoader.meshProcesses) process.wait();

		//modelLoader.meshProcesses.clear();

		return model;
	}
	void ModelLoader::texturesFromMaterial(aiMaterial* material, aiTextureType type, Vector< Ref<Texture> >* textures) {
        //LOG("Detected " + std::to_string(material->GetTextureCount(type))+ " textures of " + std::to_string(static_cast<int>(type)), 1);

		for (uint i = 0; i < (material->GetTextureCount(type)); i++) {
			aiString path;
			material->GetTexture(type, i, &path);
			auto image = Image::create(path.C_Str());
			auto tex = Renderer::createTexture(*image);
			textures->push_back(tex);
		}
	}
	
	void ModelLoader::processMesh(aiMesh* mesh, const aiScene* scene, Mesh* outputMesh) const {
        // std::vector<Vertex> vertices;
		// std::vector< uint > indices;

        outputMesh->vertices.clear();
        outputMesh->indices .clear();

        outputMesh->vertices.reserve(mesh->mNumVertices );
        outputMesh->indices .reserve(mesh->mNumFaces * 3);
		
		outputMesh->meshRenderer->material.shader = shader;

		for (uint i = 0; i < mesh->mNumVertices; i++) {
			{ outputMesh->boneIDs.push_back({-1, -1, -1, -1}); }
			{ outputMesh->boneWeights.push_back({0.f, 0.f, 0.f, 0.f}); }
			{ outputMesh->vertices.emplace_back(); }

            auto& meshPosition =      mesh->mVertices[i];
            auto& vertPosition = outputMesh->vertices[i];

			vertPosition.x = meshPosition.x;
			vertPosition.y = meshPosition.y;
			vertPosition.z = meshPosition.z;
			
			if (mesh->HasNormals()) {
                { outputMesh->normals.emplace_back(); }

                auto& meshNormal =      mesh->mNormals[i];
                auto& vertNormal = outputMesh->normals[i];

				vertNormal.x = meshNormal.x;
				vertNormal.y = meshNormal.y;
				vertNormal.z = meshNormal.z;
			}

			if (mesh->mTextureCoords[0]) {
                { outputMesh->textureCoordinates.emplace_back(); }

                auto& textureCoords = mesh->mTextureCoords[0][i];

                outputMesh->textureCoordinates[i] = {
                        textureCoords.x,
                        textureCoords.y
                };
			}
            /*
			if (mesh->HasTangentsAndBitangents()) {
                auto& meshTangent = mesh->mTangents[i];
                auto& vertTangent = vertices[i].tangent;

				vertTangent.x = meshTangent.x;
				vertTangent.y = meshTangent.y;
				vertTangent.z = meshTangent.z;

                auto& meshBitangent = mesh->mBitangents[i];
                auto& vertBitangent = vertices[i].bitangent;

                vertBitangent.x = meshBitangent.x;
				vertBitangent.y = meshBitangent.y;
				vertBitangent.z = meshBitangent.z;
			}
            */
		}

		for (uint i = 0; i < mesh->mNumFaces; i++) {
			aiFace face = mesh->mFaces[i];

			for (uint j = 0; j < face.mNumIndices; j++) {
				outputMesh->indices.emplace_back(face.mIndices[j]);
			}
		}

				for (int boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex)
        {
            int boneID = -1;
            std::string boneName = mesh->mBones[boneIndex]->mName.C_Str();
            if (outputMesh->m_BoneInfoMap.find(boneName) == outputMesh->m_BoneInfoMap.end())
            {
                BoneInfo newBoneInfo;
                newBoneInfo.id = outputMesh->m_BoneCounter;
                newBoneInfo.offset = AssimpGLMHelpers::ConvertMatrixToGLMFormat(mesh->mBones[boneIndex]->mOffsetMatrix);
                outputMesh->m_BoneInfoMap[boneName] = newBoneInfo;
                boneID = outputMesh->m_BoneCounter;
                outputMesh->m_BoneCounter++;
            }
            else
            {
                boneID = outputMesh->m_BoneInfoMap[boneName].id;
            }
            assert(boneID != -1);
            auto weights = mesh->mBones[boneIndex]->mWeights;
            int numWeights = mesh->mBones[boneIndex]->mNumWeights;

            for (int weightIndex = 0; weightIndex < numWeights; ++weightIndex)
            {
                int vertexId = weights[weightIndex].mVertexId;
                float weight = weights[weightIndex].mWeight;
                assert(vertexId <= vertices.size());
      					for (int i = 0; i < 4; ++i) {
            			if (outputMesh->boneIDs[vertexId][i] < 0) {
                		outputMesh->boneWeights[vertexId][i] = weight;
                		outputMesh->boneIDs[vertexId][i] = boneID;
              		  break;
        			    }
				        }
                //SetVertexBoneData(vertices[vertexId], boneID, weight);
            }
        }


        outputMesh->recalculateMesh();

		// outputMesh->vertexBuffer->setData(vertices.data(), vertices.size() * sizeof(Vertex));
		// outputMesh-> indexBuffer->setData(indices .data(), indices .size() * sizeof( uint ));
		
		aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
		{
			texturesFromMaterial(material, aiTextureType_DIFFUSE, &outputMesh->meshRenderer->material.textures);
			//texturesFromMaterial(material, aiTextureType_AMBIENT , TextureType::HEIGHT  , &outputMesh->material.textures);
			//texturesFromMaterial(material, aiTextureType_HEIGHT  , TextureType::NORMAL  , &outputMesh->material.textures);
			//texturesFromMaterial(material, aiTextureType_SPECULAR, TextureType::SPECULAR, &outputMesh->material.textures);
		}
	}
	void ModelLoader::processNode(aiNode* node, const aiScene* _scene, Entity* parent) {
		for (uint32 i = 0; i < node->mNumMeshes; i++) {
			aiMesh* _mesh_ = _scene->mMeshes[node->mMeshes[i]];
			const char* name = (node->mName.length == 0) ? "New node" : node->mName.C_Str();

			Scene* scene = parent->getScene();
			Entity* meshEntity = (Entity*)scene->createGameObject(name, parent);
			Mesh*  mesh = &meshEntity->addComponent<Mesh>();

			ModelLoader::processMesh(_mesh_, _scene, mesh);
		}
		for (uint32 i = 0; i < node->mNumChildren; i++) {
			processNode(node->mChildren[i], _scene, parent);
		}
	}
}

