#pragma once

#include<algorithm>

#include"raytracer.h"

raytracer::raytracer(std::vector<std::unique_ptr<Object>> &objects, std::vector<std::unique_ptr<PointLight>> &lights, const Vec3f &bkg_color) 
	: targets(std::move(objects)), 
	  point_lights(std::move(lights)),
	  background(bkg_color)
{
}

void raytracer::set_targets(std::vector<std::unique_ptr<Object>> &objects)
{
	if (objects.size() > 0)
		targets = std::move(objects);
}

void raytracer::set_background_color(const Vec3f &bkg_color)
{
	background = bkg_color;
}

Vec3f raytracer::shoot(const Vec3f &orig, const Vec3f &dir)
{
	ray ray(orig, dir);
	return shoot(ray);
}

Vec3f raytracer::shoot(const ray &ray)
{
	Vec3f hitColor = background;
	float tnear = kInfinity;
	Vec2f uv;
	uint32_t index = 0;
	Object *hitObject = nullptr;

	// Find the nearest traiangle that is hit
	for (uint32_t k = 0; k < targets.size(); ++k)
	{
		float tNearTriangle = kInfinity;
		uint32_t indexTriangle;
		Vec2f uvTriangle;
		if (targets[k]->intersect(ray.origin, ray.dir, tNearTriangle, indexTriangle, uvTriangle) && tNearTriangle < tnear) {
			hitObject = targets[k].get();
			tnear = tNearTriangle;
			index = indexTriangle;
			uv = uvTriangle;
		}
	}

	if (hitObject != nullptr) {
		Vec3f hitPoint = ray.origin + ray.dir * tnear;
		Vec3f hitNormal;
		Vec2f hitTexCoordinates;
		hitObject->getSurfaceProperties(hitPoint, ray.dir, index, uv, hitNormal, hitTexCoordinates);
		hitColor = { 0 };
		for (auto &point_light : point_lights)
		{
			float tnear = 0.0f;
			Vec3f light_dir, light_intensity;
			point_light->illuminate(hitPoint, light_dir, light_intensity, tnear);

			hitColor = hitColor + hitObject->color * light_intensity * std::max(0.f, hitNormal.dotProduct(-light_dir));
		}
	}

	return hitColor;
}