#pragma once
#include "geometry.h"
class Light
{
public:
	Light(const Matrix44f &l2w, const Vec3f &c = 1, const float &i = 1) : lightToWorld(l2w), color(c), intensity(i) {}
	virtual ~Light() {}
	virtual void illuminate(const Vec3f &P, Vec3f &, Vec3f &, float &) const = 0;
	Vec3f color;
	float intensity;
	Matrix44f lightToWorld;
};

class PointLight : public Light
{
	Vec3f pos;
public:
	PointLight(const Matrix44f &l2w, const Vec3f &c = 1, const float &i = 1) : Light(l2w, c, i)
	{
		l2w.multVecMatrix(Vec3f(0), pos);
	}
	// P: is the shaded point
	void illuminate(const Vec3f &P, Vec3f &lightDir, Vec3f &lightIntensity, float &distance) const
	{
		lightDir = (P - pos);
		float r2 = lightDir.norm();
		distance = sqrt(r2);
		lightDir.x /= distance, lightDir.y /= distance, lightDir.z /= distance;
		// avoid division by 0
		float multiplier = 1 / (4 * M_PI * r2);
		lightIntensity = color * intensity * multiplier;
	}
};