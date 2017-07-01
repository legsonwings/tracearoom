#pragma once

#include<memory>
#include<vector>
#include"geometry.h"
#include "lights.h"
#include"polygon_primitves.h"

struct ray
{
	Vec3f origin;
	Vec3f dir;

	ray(const Vec3f &origin, const Vec3f &dir)
	{
		this->origin = origin;
		this->dir = dir;
	}
};
class raytracer
{
	std::vector<std::unique_ptr<Object>> targets; 
	std::vector<std::unique_ptr<PointLight>> point_lights;
	Vec3f background;
public:
	raytracer(std::vector<std::unique_ptr<Object>> &objects, std::vector<std::unique_ptr<PointLight>> &lights, const Vec3f &background_color = Vec3f(255));
	void set_targets(std::vector<std::unique_ptr<Object>> &objects);
	void set_background_color(const Vec3f &bkg_color);
	Vec3f shoot(const Vec3f &orig, const Vec3f &dir);
	Vec3f shoot(const ray &ray);
};