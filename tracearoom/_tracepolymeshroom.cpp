#include <cstdlib>
#include <memory>
#include <vector>
#include <utility>
#include <cstdint>
#include <iostream>
#include <fstream>
#include <cmath>
#include <sstream>
#include <algorithm>

#include "geometry.h"
#include "bitmap_utils.h"

using namespace std;

static const float kInfinity = std::numeric_limits<float>::max();
static const float kEpsilon = 1e-8f;
static const Vec3f kDefaultBackgroundColor = Vec3f(255.0f, 255.0f, 255.0f);

inline
float clamp(const float &lo, const float &hi, const float &v)
{ return std::max(lo, std::min(hi, v)); }

inline
float deg2rad(const float &deg)
{ return deg * M_PI / 180.0f; }

struct Options
{
    uint32_t width = 640;
    uint32_t height = 480;
    float fov = 90;
    Vec3f backgroundColor = kDefaultBackgroundColor;
    Matrix44f cameraToWorld;
};

class Object
{
 public:
    Object() {}
    virtual ~Object() {}
    virtual bool intersect(const Vec3f &, const Vec3f &, float &, uint32_t &, Vec2f &) const = 0;
    virtual void getSurfaceProperties(const Vec3f &, const Vec3f &, const uint32_t &, const Vec2f &, Vec3f &, Vec2f &) const = 0;
};

bool rayTriangleIntersect(
    const Vec3f &orig, const Vec3f &dir,
    const Vec3f &v0, const Vec3f &v1, const Vec3f &v2,
    float &t, float &u, float &v)
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

class TriangleMesh : public Object
{
public:
    // Build a triangle mesh from a face index array and a vertex index array
    TriangleMesh(
        const uint32_t nfaces,
        const std::unique_ptr<uint32_t []> &faceIndex,
        const std::unique_ptr<uint32_t []> &vertsIndex,
        const std::unique_ptr<Vec3f []> &verts,
        std::unique_ptr<Vec3f []> &normals,
        std::unique_ptr<Vec2f []> &st) :
        numTris(0)
    {
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
        P = std::unique_ptr<Vec3f []>(new Vec3f[maxVertIndex]);
        for (uint32_t i = 0; i < maxVertIndex; ++i) {
            P[i] = verts[i];
        }
        
        // allocate memory to store triangle indices
        trisIndex = std::unique_ptr<uint32_t []>(new uint32_t [numTris * 3]);
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
        N = std::unique_ptr<Vec3f []>(new Vec3f[numTris * 3]);
        texCoordinates = std::unique_ptr<Vec2f []>(new Vec2f[numTris * 3]);
        for (uint32_t i = 0, k = 0; i < nfaces; ++i) { // for each  face
            for (uint32_t j = 0; j < faceIndex[i] - 2; ++j) { // for each triangle in the face
                trisIndex[l] = vertsIndex[k];
                trisIndex[l + 1] = vertsIndex[k + j + 1];
                trisIndex[l + 2] = vertsIndex[k + j + 2];
                N[l] = normals[k];
                N[l + 1] = normals[k + j + 1];
                N[l + 2] = normals[k + j + 2];
                texCoordinates[l] = st[k];
                texCoordinates[l + 1] = st[k + j + 1];
                texCoordinates[l + 2] = st[k + j + 2];
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
            const Vec3f &v0 = P[trisIndex[j]];
            const Vec3f &v1 = P[trisIndex[j + 1]];
            const Vec3f &v2 = P[trisIndex[j + 2]];
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
        const Vec3f &v0 = P[trisIndex[triIndex * 3]];
        const Vec3f &v1 = P[trisIndex[triIndex * 3 + 1]];
        const Vec3f &v2 = P[trisIndex[triIndex * 3 + 2]];
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
    // member variables
    uint32_t numTris;                         // number of triangles
    std::unique_ptr<Vec3f []> P;              // triangles vertex position
    std::unique_ptr<uint32_t []> trisIndex;   // vertex index array
    std::unique_ptr<Vec3f []> N;              // triangles vertex normals
    std::unique_ptr<Vec2f []> texCoordinates; // triangles texture coordinates
};
bool trace(
    const Vec3f &orig, const Vec3f &dir,
    const std::vector<std::unique_ptr<Object>> &objects,
    float &tNear, uint32_t &index, Vec2f &uv, Object **hitObject)
{
    *hitObject = nullptr;
    for (uint32_t k = 0; k < objects.size(); ++k) {
        float tNearTriangle = kInfinity;
        uint32_t indexTriangle;
        Vec2f uvTriangle;
        if (objects[k]->intersect(orig, dir, tNearTriangle, indexTriangle, uvTriangle) && tNearTriangle < tNear) {
            *hitObject = objects[k].get();
            tNear = tNearTriangle;
            index = indexTriangle;
            uv = uvTriangle;
        }
    }

    return (*hitObject != nullptr);
}

Vec3f castRay(
    const Vec3f &orig, const Vec3f &dir,
    const std::vector<std::unique_ptr<Object>> &objects,
    const Options &options)
{
    Vec3f hitColor = options.backgroundColor;
    float tnear = kInfinity;
    Vec2f uv;
    uint32_t index = 0;
    Object *hitObject = nullptr;
    if (trace(orig, dir, objects, tnear, index, uv, &hitObject)) {
        Vec3f hitPoint = orig + dir * tnear;
        Vec3f hitNormal;
        Vec2f hitTexCoordinates;
        hitObject->getSurfaceProperties(hitPoint, dir, index, uv, hitNormal, hitTexCoordinates);
        float NdotView = std::max(0.f, hitNormal.dotProduct(-dir));
        const int M = 10;
		float checker = (float)((fmod(hitTexCoordinates.x * M, 1.0f) > 0.5f) ^ (fmod(hitTexCoordinates.y * M, 1.0f) < 0.5f));
        float c = 0.3f * (1 - checker) + 0.7f * checker;
        
        //hitColor = c * NdotView; //Vec3f(uv.x, uv.y, 0);
		hitColor = Vec3f(255.0f, 0.0f, 0.0f);
    }

    return hitColor;
}

// [comment]
// The main render function. This where we iterate over all pixels in the image, generate
// primary rays and cast these rays into the scene. The content of the framebuffer is
// saved to a file.
// [/comment]
void render(
    const Options &options,
    const std::vector<std::unique_ptr<Object>> &objects)
{
    std::unique_ptr<Vec3f []> framebuffer(new Vec3f[options.width * options.height]);
    Vec3f *pix = framebuffer.get();
    float scale = tan(deg2rad(options.fov * 0.5f));
    float imageAspectRatio = options.width / (float)options.height;
    Vec3f orig;
    options.cameraToWorld.multVecMatrix(Vec3f(0), orig);

    for (uint32_t j = 0; j < options.height; ++j) {
        for (uint32_t i = 0; i < options.width; ++i) {
            // generate primary ray direction
            float x = (2 * (i + 0.5f) / (float)options.width - 1) * imageAspectRatio * scale;
            float y = (1 - 2 * (j + 0.5f) / (float)options.height) * scale;
            Vec3f dir;
            options.cameraToWorld.multDirMatrix(Vec3f(x, y, -1), dir);
            dir.normalize();
            *(pix++) = castRay(orig, dir, objects, options);
        }
        fprintf(stderr, "\r%3d%c", uint32_t(j / (float)options.height * 100), '%');
    }
	string filepath = "D:\\out.bmp";

	auto imgdata = std::unique_ptr<unsigned char []>(new unsigned char[options.width * options.height * 3]);
    //ofs << "P6\n" << options.width << " " << options.height << "\n255\n";
    for (uint32_t i = 0; i < options.height * options.width; ++i) {
        auto r = (unsigned char)(255 * clamp(0, 1, framebuffer[i].x));
        auto g = (unsigned char)(255 * clamp(0, 1, framebuffer[i].y));
        auto b = (unsigned char)(255 * clamp(0, 1, framebuffer[i].z));
		imgdata[i * 3 + 0] = r;
		imgdata[i * 3 + 1] = g;
		imgdata[i * 3 + 2] = b;
        //ofs << r << g << b;
    }
	cout << "\nRaytracing done...\n";
	cout << "Creating bitmap file...\n";
	bitmap_utils::bitmap_image bmp_image(options.width, options.height, std::move(imgdata));
	bmp_image.write_to_file(filepath);
}

unique_ptr<TriangleMesh> generateQuadMesh()
{
	auto faceIdx = unique_ptr<uint32_t []>(new uint32_t[1]);
	auto vertIdx = unique_ptr<uint32_t []>(new uint32_t[3]);
	auto verts = unique_ptr<Vec3f []>(new Vec3f[3]);
	auto normals = unique_ptr<Vec3f []>(new Vec3f[3]);
	auto st = unique_ptr<Vec2f []>(new Vec2f[3]);
	
	faceIdx[0] = 3;
	vertIdx[0] = 0;
	vertIdx[1] = 1;
	vertIdx[2] = 2;
	verts[0] = Vec3f(0,0,0);
	verts[1] = Vec3f(5,0,0);
	verts[2] = Vec3f(0,4,0);
	
	return unique_ptr<TriangleMesh>(new TriangleMesh(1, faceIdx, vertIdx, verts, normals, st));
}

// [comment]
// In the main function of the program, we create the scene (create objects and lights)
// as well as set the options for the render (image widht and height, maximum recursion
// depth, field-of-view, etc.). We then call the render function().
// [/comment]
int main(int argc, char **argv)
{
    // setting up options
    Options options;
    //options.cameraToWorld[3][2] = 10;
    //Matrix44f tmp = Matrix44f(0.707107, -0.331295, 0.624695, 0, 0, 0.883452, 0.468521, 0, -0.707107, -0.331295, 0.624695, 0, -1.63871, -5.747777, -40.400412, 1);
	Matrix44f tmp = Matrix44f(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, -10.0f, 1.0f);
    options.cameraToWorld = tmp.inverse();
    options.fov = 50.0393f;
    
	// creating the scene (adding objects and lights)
	std::vector<std::unique_ptr<Object>> objects;
	objects.push_back(generateQuadMesh());

    // finally, render
    render(options, objects);

    return 0;
}
