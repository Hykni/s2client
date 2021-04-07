#pragma once

#include <core/prerequisites.hpp>
#include <core/io/bytestream.hpp>
#include <core/math/vector3.hpp>

namespace s2 {
	struct plane {
		vector3f v;
		float p;
	};
	struct MeshFlags {
		static const uint64_t Foliage = 0x400ull;
		static const uint64_t NoHit = 0x1000ull;
		static const uint64_t Invis = 0x200000000ull;
	};
	class mesh {
	public:
		string name;
		string material;
		uint32_t mode{};
		uint64_t flags{};
		struct {
			vector3f min, max;
		} bounds;
		vector<vector3f> vertices;
		vector<vector3f> normals;
		vector<uint32_t> colors[2];
		vector<int> faces;
		struct texcoord {
			float u, v;
		};
		vector<texcoord> texcoords[8];
		vector<vector3f> tangents[8];

		uint32_t bonelink{};

		mesh() = default;
		mesh(const mesh& other) = default;
		mesh(mesh&& other)noexcept = default;
		~mesh() = default;

		void AllocFaces(size_t count) {
			faces.resize(count * 3);
		}
		const size_t NumFaces()const {
			return faces.size() / 3;
		}
		void AllocVertices(size_t count) {
			vertices.resize(count);
		}
		const size_t NumVertices()const {
			return vertices.size();
		}
	};

	class surface {
	public:
		struct {
			vector3f min, max;
		} bounds;
		uint32_t flags;
		vector<plane> planes;
		vector<vector3f> points;
		vector<vector3f> edges;
		vector<uint32_t> triangles;
	};

	class model {
		vector<mesh> mMeshes;
		struct {
			vector3f min, max;
		} mBounds;
	public:

		surface mSurface;
		model() = default;
		model(const model & other) = default;
		model(model && other)noexcept = default;
		~model() = default;
		static std::shared_ptr<model> Load(core::bytestream& stream);

		constexpr const vector3f& BBMin()const {
			return mBounds.min;
		}
		constexpr const vector3f& BBMax()const {
			return mBounds.max;
		}
		mesh& NewMesh() {
			mMeshes.push_back(mesh{});
			return mMeshes.back();
		}
		mesh& GetMesh(int id) {
			return mMeshes[id];
		}
		size_t NumMeshes()const {
			return mMeshes.size();
		}
	};
}
