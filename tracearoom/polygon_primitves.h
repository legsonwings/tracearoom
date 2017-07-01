#pragma once
#include<memory>
#include <algorithm>
#include <vector>
#include <cassert>
#include "geometry.h"

class Object
{
public:
	Object(const Vec3f &obj_color) : color(obj_color) {}
	virtual ~Object() {}
	virtual bool intersect(const Vec3f &, const Vec3f &, float &, uint32_t &, Vec2f &) const = 0;
	virtual void getSurfaceProperties(const Vec3f &, const Vec3f &, const uint32_t &, const Vec2f &, Vec3f &, Vec2f &) const = 0;
	Vec3f color;
};

class TriangleMesh : public Object
{
	bool rayTriangleIntersect(
		const Vec3f &orig, const Vec3f &dir,
		const Vec3f &v0, const Vec3f &v1, const Vec3f &v2,
		float &t, float &u, float &v) const
	{
		Vec3f v0v1 = v1 - v0;
		Vec3f v0v2 = v2 - v0;
		Vec3f pvec = dir.crossProduct(v0v2);
		float det = v0v1.dotProduct(pvec);

		// ray and triangle are parallel if det is close to 0
		if (fabs(det) < kEpsilon) return false;

		float invDet = 1 / det;

		Vec3f tvec = orig - v0;
		u = tvec.dotProduct(pvec) * invDet;
		if (u < 0 || u > 1) return false;

		Vec3f qvec = tvec.crossProduct(v0v1);
		v = dir.dotProduct(qvec) * invDet;
		if (v < 0 || u + v > 1) return false;

		t = v0v2.dotProduct(qvec) * invDet;

		return true;
	}
public:
	// Build a triangle mesh from a face index array and a vertex index array
	TriangleMesh(
		const uint32_t nfaces,
		const std::vector<uint32_t> &faceIndex,
		const std::vector<uint32_t> &vertsIndex,
		const std::vector<Vec3f> &verts,
		std::vector<Vec3f> &normals,
		std::vector<Vec2f> &st, 
		const Vec3f &mesh_color):
		numTris(0), Object(mesh_color)
	{
		// Will cause vector index out of range errors if the size does not match,
		// same with sts
		assert(verts.size() > 0);
		assert(vertsIndex.size() == normals.size());
		assert(vertsIndex.size() == st.size());
		// Initialize translation matrix 
		translation = Matrix44f::create_translation({0.0f, 0.0f, verts[0].z});
		uint32_t k = 0, maxVertIndex = 0;
		// find out how many triangles we need to create for this mesh
		for (uint32_t i = 0; i < nfaces; ++i) {
			numTris += faceIndex[i] - 2;
			for (uint32_t j = 0; j < faceIndex[i]; ++j)
				if (vertsIndex[k + j] > maxVertIndex)
					maxVertIndex = vertsIndex[k + j];
			k += faceIndex[i];
		}
		maxVertIndex += 1;

		// allocate memory to store the position of the mesh vertices
		vertices.reserve(maxVertIndex);
		for (uint32_t i = 0; i < maxVertIndex; ++i) {
			vertices.push_back(verts[i]);
		}

		// allocate memory to store triangle indices
		trisIndex.reserve(numTris * 3);
		uint32_t l = 0;
		// [comment]
		// Generate the triangle index array
		// Keep in mind that there is generally 1 vertex attribute for each vertex of each face.
		// So for example if you have 2 quads, you only have 6 vertices but you have 2 * 4
		// vertex attributes (that is 8 normals, 8 texture coordinates, etc.). So the easiest
		// lazziest method in our triangle mesh, is to create a new array for each supported
		// vertex attribute (st, normals, etc.) whose size is equal to the number of triangles
		// multiplied by 3, and then set the value of the vertex attribute at each vertex
		// of each triangle using the input array (normals[], st[], etc.)
		// [/comment]
		N.reserve(numTris * 3);
		texCoordinates.reserve(numTris * 3);
		for (uint32_t i = 0, k = 0; i < nfaces; ++i) { // for each  face
			for (uint32_t j = 0; j < faceIndex[i] - 2; ++j) { // for each triangle in the face
				//trisIndex[l] = vertsIndex[k];
				//trisIndex[l + 1] = vertsIndex[k + j + 1];
				//trisIndex[l + 2] = vertsIndex[k + j + 2];
				trisIndex.push_back(vertsIndex[k]);
				trisIndex.push_back(vertsIndex[k + j + 1]);
				trisIndex.push_back(vertsIndex[k + j + 2]);

				//N[l] = normals[k];
				//N[l + 1] = normals[k + j + 1];
				//N[l + 2] = normals[k + j + 2];
				N.push_back(normals[k]);
				N.push_back(normals[k + j + 1]);
				N.push_back(normals[k + j + 2]);

				/*texCoordinates[l] = st[k];
				texCoordinates[l + 1] = st[k + j + 1];
				texCoordinates[l + 2] = st[k + j + 2];*/
				
				texCoordinates.push_back(st[k]);
				texCoordinates.push_back(st[k + j + 1]);
				texCoordinates.push_back(st[k + j + 2]);
				l += 3;
			}
			k += faceIndex[i];
		}
		// you can use move if the input geometry is already triangulated
		//N = std::move(normals); // transfer ownership
		//sts = std::move(st); // transfer ownership
	}
	// Test if the ray interesests this triangle mesh
	bool intersect(const Vec3f &orig, const Vec3f &dir, float &tNear, uint32_t &triIndex, Vec2f &uv) const
	{
		uint32_t j = 0;
		bool isect = false;
		for (uint32_t i = 0; i < numTris; ++i) {
			const Vec3f &v0 = vertices[trisIndex[j]];
			const Vec3f &v1 = vertices[trisIndex[j + 1]];
			const Vec3f &v2 = vertices[trisIndex[j + 2]];
			float t = kInfinity, u, v;
			if (rayTriangleIntersect(orig, dir, v0, v1, v2, t, u, v) && t < tNear) {
				tNear = t;
				uv.x = u;
				uv.y = v;
				triIndex = i;
				isect = true;
			}
			j += 3;
		}

		return isect;
	}
	void getSurfaceProperties(
		const Vec3f &hitPoint,
		const Vec3f &viewDirection,
		const uint32_t &triIndex,
		const Vec2f &uv,
		Vec3f &hitNormal,
		Vec2f &hitTextureCoordinates) const
	{
		// face normal
		const Vec3f &v0 = vertices[trisIndex[triIndex * 3]];
		const Vec3f &v1 = vertices[trisIndex[triIndex * 3 + 1]];
		const Vec3f &v2 = vertices[trisIndex[triIndex * 3 + 2]];
		hitNormal = (v1 - v0).crossProduct(v2 - v0);
		hitNormal.normalize();

		// texture coordinates
		const Vec2f &st0 = texCoordinates[triIndex * 3];
		const Vec2f &st1 = texCoordinates[triIndex * 3 + 1];
		const Vec2f &st2 = texCoordinates[triIndex * 3 + 2];
		hitTextureCoordinates = (1 - uv.x - uv.y) * st0 + uv.x * st1 + uv.y * st2;

		// vertex normal
		/*
		const Vec3f &n0 = N[triIndex * 3];
		const Vec3f &n1 = N[triIndex * 3 + 1];
		const Vec3f &n2 = N[triIndex * 3 + 2];
		hitNormal = (1 - uv.x - uv.y) * n0 + uv.x * n1 + uv.y * n2;
		*/
	}

	// Rotate by arbitrary angle along an aritrary axis
	void rotate(const float angle, const Vec3f &axis)
	{
		auto rot_mat = Matrix44f::create_rotation(angle, axis);
		rotation = rotation * rot_mat;
		auto inv_transl = translation.inverse();

		// Translate the mesh to origin
		std::for_each(vertices.begin(), vertices.end(), [&inv_transl](Vec3f &vertex) { inv_transl.multVecMatrix(vertex, vertex); });
		std::for_each(vertices.begin(), vertices.end(), [&rot_mat](Vec3f &vertex) {	rot_mat.multVecMatrix(vertex, vertex); });
		// Restore the position of the mesh
		std::for_each(vertices.begin(), vertices.end(), [this](Vec3f &vertex) { translation.multVecMatrix(vertex, vertex); });
	}

	// Rotate along a pivot
	// Might need research on rigidbody transformations
	void rotate(const Vec3f& pivot, const float angle, const Vec3f &axis)
	{
		auto pivot_transl = Matrix44f::create_translation(pivot);
		auto pivot_transl_inv = pivot_transl.inverse();
		auto inv_transl = translation.inverse();
		// Translate the mesh to origin
		std::for_each(vertices.begin(), vertices.end(), [&inv_transl](Vec3f &vertex) { inv_transl.multVecMatrix(vertex, vertex); });
		std::for_each(vertices.begin(), vertices.end(), [&pivot_transl_inv](Vec3f &vertex) { pivot_transl_inv.multVecMatrix(vertex, vertex); });	
		auto rot_mat = Matrix44f::create_rotation(angle, axis);
		//rotation = rotation * rot_mat;
		std::for_each(vertices.begin(), vertices.end(), [&rot_mat](Vec3f &vertex) {	rot_mat.multVecMatrix(vertex, vertex); });
		std::for_each(vertices.begin(), vertices.end(), [&pivot_transl](Vec3f &vertex) { pivot_transl.multVecMatrix(vertex, vertex); });
		std::for_each(vertices.begin(), vertices.end(), [this](Vec3f &vertex) { translation.multVecMatrix(vertex, vertex); });
	}

	// Translate by specified vector
	void translate(const Vec3f &transl_vector)
	{
		auto new_translation = Matrix44f::create_translation(transl_vector);
		translation = translation * new_translation;
		std::for_each(vertices.begin(), vertices.end(), [&new_translation](Vec3f &vertex) { new_translation.multVecMatrix(vertex, vertex); });
	}
private:
	// member variables
	uint32_t numTris;                         // number of triangles
	std::vector<Vec3f> vertices;              // triangles vertex position
	std::vector<uint32_t> trisIndex;   // vertex index array
	std::vector<Vec3f> N;              // triangles vertex normals
	std::vector<Vec2f> texCoordinates; // triangles texture coordinates
	Matrix44f translation, rotation, rotation_pivot;
};