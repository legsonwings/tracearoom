#pragma once

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
#include "raytracer.h"
#include "bitmap_utils.h"

using namespace std;

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

void render(
    const Options &options,
    std::vector<std::unique_ptr<Object>> &objects)
{
    std::unique_ptr<Vec3f []> framebuffer(new Vec3f[options.width * options.height]);
    Vec3f *pix = framebuffer.get();
    float scale = tan(deg2rad(options.fov * 0.5f));
    float imageAspectRatio = options.width / (float)options.height;
    Vec3f orig;
    options.cameraToWorld.multVecMatrix(Vec3f(0), orig);
	
	std::vector<std::unique_ptr<PointLight>> point_lights;
	Matrix44f l2w = Matrix44f(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 3.0f, 0.0f, 0.0f, -15.0f, 1.0f);
	point_lights.push_back(std::make_unique<PointLight>(l2w, 1, 580));
	raytracer raytracer(objects, point_lights, options.backgroundColor);
    
	for (uint32_t j = 0; j < options.height; ++j) {
        for (uint32_t i = 0; i < options.width; ++i) {
            // generate primary ray direction
            float x = (2 * (i + 0.5f) / (float)options.width - 1) * imageAspectRatio * scale;
            float y = (1 - 2 * (j + 0.5f) / (float)options.height) * scale;
            Vec3f dir, pixel_coord;
            options.cameraToWorld.multDirMatrix(Vec3f(x, y, (orig.z -1)), pixel_coord);
			dir = pixel_coord - orig;
            dir.normalize();
            *(pix++) = raytracer.shoot(orig, dir);
        }
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

unique_ptr<TriangleMesh> generateQuadMesh(vector<Vec3f> &quad_vertices)
{
	auto faceIdx = vector<uint32_t>(2);
	auto vertIdx = vector<uint32_t>(6);
	auto normals = vector<Vec3f>(6);
	auto st = vector<Vec2f>(6);

	faceIdx[0] = 3;
	faceIdx[1] = 3;

	vertIdx[0] = 0;
	vertIdx[1] = 1;
	vertIdx[2] = 2;
	vertIdx[3] = 2;
	vertIdx[4] = 3;
	vertIdx[5] = 0;
	
	return unique_ptr<TriangleMesh>(new TriangleMesh(2, faceIdx, vertIdx, quad_vertices, normals, st, { 0, 1,0 }));
}

// create a unit quad in XY plane and (0, 0, z_offset) as pivot
// z_offset should be smaller than camera position's z-coordinate
unique_ptr<TriangleMesh> generateQuadMesh(float scalex, float scaley, float z_offset = 0.0f, const Vec3f &color = { 1, 0, 0 })
{
	auto faceIdx = vector<uint32_t>(2);
	auto vertIdx = vector<uint32_t>(6);
	auto normals = vector<Vec3f>(6);
	auto st = vector<Vec2f>(6);
	vector<Vec3f> quad_vertices = { {scalex, scaley, z_offset}, {-scalex, scaley, z_offset }, {-scalex, -scaley, z_offset }, { scalex ,-scaley, z_offset } };
	
	faceIdx[0] = 3;
	faceIdx[1] = 3;

	vertIdx[0] = 0;
	vertIdx[1] = 1;
	vertIdx[2] = 2;
	vertIdx[3] = 2;
	vertIdx[4] = 3;
	vertIdx[5] = 0;

	return unique_ptr<TriangleMesh>(new TriangleMesh(2, faceIdx, vertIdx, quad_vertices, normals, st, color));
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
	Matrix44f tmp = Matrix44f(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, -10.0f, 1.0f);
	options.cameraToWorld = tmp;
    options.fov = 50.0393f;
    
	unique_ptr<TriangleMesh> wall1 = generateQuadMesh(6.5f, 5, -23);
	unique_ptr<TriangleMesh> wall2 = generateQuadMesh(5, 4.5f, -20, {0.1f, 0.8f, 0});
	wall1->translate({ 2, 0, 0 });
	wall2->rotate(20, { 0, 1, 0 });
	wall2->translate({ -5, 0, 0 });
	std::vector<std::unique_ptr<Object>> objects;
	objects.push_back(std::move(wall1));
	objects.push_back(std::move(wall2));

	// finally, render
    render(options, objects);

    return 0;
}
