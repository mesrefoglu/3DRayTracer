#include <limits>
#include <iostream>
#include <fstream>
#include <vector>
#include "geometry.h"

const float PI = 3.14159265359f;
const int REFLECION_MAX_DEPTH = 4;

struct Light {
    vec3 position;
    float intensity;
};

struct Material {
	float refractive_index = 1;
    vec4 albedo = {1, 0, 0, 0};
    vec3 diffuse_color = {0, 0, 0};
    float specular_exponent = 0;
};

struct Sphere {
    vec3 center;
    float radius;
	Material material;

    // constructor
    Sphere(const vec3 &c, const float &r, const Material &m) : center(c), radius(r), material(m) {}

    // determine if a ray from point orig with direction dir (normalized) intersect the sphere
    bool ray_intersect(const vec3 &orig, const vec3 &dir, float &t0) const {
        vec3 dist = center - orig;								// distance b/w center of sphere and orig
        float projToOrigin = dist * dir;						// distance b/w the projection of the center on the ray and orig
        float d2 = dist * dist - projToOrigin * projToOrigin;	// sqr of the distance b/w ray and center
        if (d2 > radius * radius) return false;					// if the d2 is greater than radius, no intersection
        float projToIntersections = sqrt(radius * radius - d2);	// distance b/w intersections and projection
        t0 = projToOrigin - projToIntersections;				// distance from origin to first intersection
        float t1 = projToOrigin + projToIntersections;			// distance from origin to second intersection
        if (t0 < 0) t0 = t1;									// this happens when ray is inside the sphere
        if (t0 < 0) return false;								// this happens when ray is in front of the sphere
        return true;
    }
};

// calculate the reflection using Phong Reflection Model
vec3 reflect(const vec3 &I, const vec3 &N) {
    return I - N * 2.f * (I * N);
}

// calculate the refraction using Snell's Law
vec3 refract(const vec3 &I, const vec3 &N, const float eta_t, const float eta_i=1.f) { // Snell's law
    float cosi = - std::max(-1.f, std::min(1.f, I * N));
    if (cosi<0) return refract(I, -N, eta_i, eta_t); // if the ray comes from the inside the object, swap the air and the media
    float eta = eta_i / eta_t;
    float k = 1 - eta * eta * (1 - cosi * cosi);
    return k < 0 ? vec3{1,0,0} : I * eta + N * (eta * cosi - sqrt(k));
}

// return true if a sphere hit the ray, false otherwise. mutate variables to show what is the last hit
bool scene_intersect(const vec3 &orig, const vec3 &dir, const std::vector<Sphere> &spheres, vec3 &hit, vec3 &N, Material &material) {
    float spheres_dist = std::numeric_limits<float>::max();	// the distance to the closest sphere
    for (size_t i = 0; i < spheres.size(); i++) {
        float dist_i;
		// check if intersects and closer than the closest sphere so far
        if (spheres[i].ray_intersect(orig, dir, dist_i) && dist_i < spheres_dist) {
            spheres_dist = dist_i; 						// make this one closer
            hit = orig + dir * dist_i;					// the point ray hits the sphere
            N = (hit - spheres[i].center).normalize();	// the normalized direction towards the hit from center
            material = spheres[i].material;				// mutate material to show it later
        }
    }

    float checkerboard_dist = std::numeric_limits<float>::max();
    if (fabs(dir.y) > 1e-3)  {
        float d = -(orig.y+4) / dir.y; // the checkerboard plane has equation y = -4
        vec3 pt = orig + dir * d;
        if (d > 0 && fabs(pt.x) < 10 && pt.z > 10 && pt.z < 30 && d < spheres_dist) {
            checkerboard_dist = d;
            hit = pt;
            N = vec3{0, 1, 0};
            material.diffuse_color = (int(.5 * hit.x + 1000) + int(.5 * hit.z)) & 1 ? vec3{1, 1, 1} : vec3{1, .7, .3};
            material.diffuse_color = material.diffuse_color * .3;
        }
    }

    return std::min(spheres_dist, checkerboard_dist) < 1000;
}

vec3 cast_ray(const vec3 &orig, const vec3 &dir, const std::vector<Sphere> &spheres, const std::vector<Light> &lights, size_t depth = 0) {
    vec3 point, N; 		// point is where an object hits the ray, N is the normal from that sphere's center to the ray
    Material material;	// material of the sphere hit

    if (depth > REFLECION_MAX_DEPTH || !scene_intersect(orig, dir, spheres, point, N, material)) {
        return vec3{0.4, 0.85, 1}; // background color
    }

	vec3 reflect_dir = reflect(dir, N);
    vec3 reflect_orig = reflect_dir * N < 0 ? point - N * 1e-3 : point + N * 1e-3;
    vec3 reflect_color = cast_ray(reflect_orig, reflect_dir, spheres, lights, depth + 1);

	vec3 refract_dir = refract(dir, N, material.refractive_index).normalize();
	vec3 refract_orig = refract_dir * N < 0 ? point - N * 1e-3 : point + N * 1e-3;
	vec3 refract_color = cast_ray(refract_orig, refract_dir, spheres, lights, depth + 1);

    float diffuse_light_intensity = 0, specular_light_intensity = 0;
    for (size_t i = 0; i < lights.size(); i++) { // add more intensity for each light source
        vec3 light_dir = (lights[i].position - point).normalize();	// direction of the light

		// shadows
        float light_distance = (lights[i].position - point).norm();

		// check if the point lies in the shadow of the lights[i]
        vec3 shadow_orig = light_dir * N < 0 ? point - N * 1e-3 : point + N * 1e-3;
        
		//basically uses the same idea with the rays and intersection with shadows
		vec3 shadow_pt, shadow_N;
        Material tmpmaterial;
        if (scene_intersect(shadow_orig, light_dir, spheres, shadow_pt, shadow_N, tmpmaterial)
			&& (shadow_pt - shadow_orig).norm() < light_distance) continue;
		// shadows end

		// if the angle between light_dir and N is less, the result of
		//   light_dir * N will be greater, meaning a higher intensity of light. (At least 0)
        diffuse_light_intensity += std::max(0.f, light_dir * N) * lights[i].intensity;
        specular_light_intensity += pow(std::max(0.f, reflect(light_dir, N) * dir), material.specular_exponent) * lights[i].intensity;
    }
    return material.diffuse_color * diffuse_light_intensity * material.albedo[0] + vec3{1., 1., 1.} * specular_light_intensity
		* material.albedo[1] + reflect_color * material.albedo[2] + refract_color * material.albedo[3];
}

void render(const std::vector<Sphere> &spheres, const std::vector<Light> &lights) {
    const int width = 3840;
    const int height = 2160;
    const float hFOV = PI / 2.f; // horizontal field of view is 90 degrees (half pi)
    std::vector<vec3> framebuffer(width * height);

    #pragma omp parallel for
	for (size_t i = 0; i < width; i++) {
        for (size_t j = 0; j < height; j++) {
            float x = (i + 0.5) -  width / 2.;			// find x component of ray
            float y = -(j + 0.5) + height / 2.;			// find y component of ray
            float z = width / (2. * tan(hFOV / 2.f));	// find z component of ray
            framebuffer[i + j * width] = cast_ray(vec3{0, 0, 0}, vec3{x, y, z}.normalize(), spheres, lights);
        }
    }

    std::ofstream ofs; // save the framebuffer to file
    ofs.open("./out.ppm", std::ofstream::binary);
    ofs << "P6\n" << width << " " << height << "\n255\n"; // set he ppm file properties
    for (vec3& c : framebuffer) {
		// if any of the RGB values of c is too high, scale it down to make it one.
		float max = std::max(c[0], std::max(c[1], c[2]));
        if (max > 1) c = c * (1. / max);

		// output each pixel color
        ofs << (char)(int)(255.f * c[0]) // red
            << (char)(int)(255.f * c[1]) // green
            << (char)(int)(255.f * c[2]); // blue
    }
    ofs.close();
}

int main() {
    const Material      ivory = {1.0, {0.6,  0.3, 0.1, 0.0}, {0.4, 0.4, 0.3},   50.};
    const Material      glass = {1.5, {0.0,  0.5, 0.1, 0.8}, {0.6, 0.7, 0.8},  125.};
    const Material red_rubber = {1.0, {0.9,  0.1, 0.0, 0.0}, {0.3, 0.1, 0.1},   10.};
    const Material     mirror = {1.0, {0.0, 10.0, 0.8, 0.0}, {1.0, 1.0, 1.0}, 1425.};

    std::vector<Sphere> spheres = {
        Sphere{vec3{-3,    0,   16}, 2,      ivory},
        Sphere{vec3{-1.0, -1.5, 12}, 2,      glass},
        Sphere{vec3{ 1.5, -0.5, 18}, 3, red_rubber},
        Sphere{vec3{ 7,    5,   18}, 4,     mirror}
    };

    std::vector<Light> lights = {
        {{-20, 20, -20}, 1.5},
        {{ 30, 50,  25}, 1.8},
        {{ 30, 20, -30}, 1.7}
    };

    render(spheres, lights);
    return 0;
}