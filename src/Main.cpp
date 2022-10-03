// Recursive ray tracing

#include "Platform/Platform.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <memory>
#include <time.h>
#include <vector>

#include "Constants.h"
#include "Geometry.cpp"
#include "Renderer.h"

// Setup scene
struct Light
{
	Vec3f position {};
	float brightness;

	Light(Vec3f pos, float bright) :
		position(pos),
		brightness(bright)
	{}
};

struct Collision
{
	Vec3f position;
	Vec3f reflection;
	Vec3f normal;
};

struct Sphere
{
	Vec3f position {};
	float radius;

	Sphere(Vec3f pos, float rad) :
		position(pos),
		radius(rad)
	{}

	float RayIntersection(
		const Vec3f& orig,
		const Vec3f& direction,
		Collision& hit)
	{
		Vec3f o_minus_c = orig - position;

		float p = direction.dotProduct(o_minus_c);
		float q = o_minus_c.dotProduct(o_minus_c) - (radius * radius);

		float discriminant = (p * p) - q;
		if (discriminant < 0.0f)
		{
			return -1;
		}

		float dRoot = sqrt(discriminant);
		float dist = std::min(-p - dRoot, -p + dRoot);
		if (dist < 0)
		{
			return -1;
		}

		// Calc hit position and reflection
		hit.position = orig + direction * dist;
		hit.normal = hit.position - position;
		hit.normal.normalize();
		hit.reflection = direction - hit.normal * 2 * direction.dotProduct(hit.normal);

		return dist;
	}
};

template <typename T>
T clip(const T& n, const T& lower, const T& upper)
{
	return std::max(lower, std::min(n, upper));
}

// Raytracer
class Raytracer
{
public:
	// Camera Setup
	float scale = 0.46;
	float aspectRatio = WINDOW_WIDTH / (float)WINDOW_HEIGHT;
	Matrix44f cameraToWorld {};
	Vec3f orig = Vec3f(0);

	// Level (should probably be refactored into separate level class)
	std::vector<Sphere> spheres;
	std::vector<Light> lights;

	// Rays
	std::vector<Vec3f> directions;

	Raytracer()
	{
		for (int i = 0; i < 8; i++)
		{
			Vec3f pos = Vec3f(
				-5.0 + (10.0 * ((rand() % 1000) / 1000.0)),
				-3.0 + (6.0 * ((rand() % 1000) / 1000.0)),
				-10.0 - 20.0 * ((rand() % 1000) / 1000));

			Sphere s1 = Sphere(pos, 0.5 + (rand() % 1000) / 1000.0);
			spheres.push_back(s1);
		}

		for (int i = 0; i < 2; i++)
		{
			Vec3f pos = Vec3f(
				-5.0 + (10.0 * ((rand() % 1000) / 1000.0)),
				-20.0 + (40.0 * ((rand() % 1000) / 1000.0)),
				-10.0 - 20.0 * ((rand() % 1000) / 1000));

			float brightness = 0.1 + 0.4 * ((rand() % 1000) / 1000.0);

			Light l = Light(pos, brightness);
			lights.push_back(l);
		}
	}

	void UpdateRayDirections()
	{
		cameraToWorld.multVecMatrix(Vec3f(0), orig);
		for (uint32_t j = 0; j < WINDOW_HEIGHT; ++j)
		{
			for (uint32_t i = 0; i < WINDOW_WIDTH; ++i)
			{
				float x = (2 * (i + 0.5) / (float)WINDOW_WIDTH - 1) * aspectRatio * scale;
				float y = (1 - 2 * (j + 0.5) / (float)WINDOW_HEIGHT) * scale;
				Vec3f dir {};
				cameraToWorld.multDirMatrix(Vec3f(x, y, -1), dir);
				dir.normalize();
				directions.push_back(dir);
			}
		}
	};

	float castRay(Vec3f& orig,
		const Vec3f& dir,
		int depth)
	{
		Collision hit {};
		float dist = 1e6;

		for (int i = 0; i < (int)spheres.size(); i++)
		{
			Collision c_hit;
			float cdist = spheres[i].RayIntersection(orig, dir, c_hit);
			if (cdist > 0 && cdist < dist)
			{
				hit = c_hit;
				dist = cdist;
			}
		}

		if (dist > 0 && dist < 1e6)
		{
			// Check illumination
			float bright = 0.0;
			int num = (int)lights.size();
			for (int i = 0; i < num; i++)
			{
				Vec3f path = hit.position - lights[i].position;
				path.normalize();
				bright += lights[i].brightness * (acos(path.dotProduct(hit.normal)) / PI);
			};

			float extra = 0.0f;
			if (depth < 4)
			{
				Vec3f pos = Vec3f(hit.position.x, hit.position.y, hit.position.z);
				Vec3f ref = Vec3f(hit.reflection.x, hit.reflection.y, hit.reflection.z);
				extra = castRay(pos, ref, depth + 1);
			}
			if (extra > 0)
			{
				extra = clip(extra, 0.0f, 1.0f);
				bright += extra;
				num += 1;
			}

			return std::pow(0.5, depth) * (bright / num);
		}

		return 0;
	}

	void Render(std::vector<sf::Uint8>& pixelBuffer)
	{
		for (int i = 0; i < WINDOW_WIDTH * WINDOW_HEIGHT; i++)
		{
			// Cast ray
			float bright = castRay(orig, directions[i], 0);
			bright = clip(500.0 * bright, 0.0, 255.0);

			uint ind = i * 4;
			pixelBuffer[ind] = bright;
			pixelBuffer[ind + 1] = bright;
			pixelBuffer[ind + 2] = bright;
		}
	};

	std::vector<sf::Uint8> RenderNoise()
	{
		std::vector<sf::Uint8> pixelBuffer(WINDOW_WIDTH * WINDOW_HEIGHT * 4);

		for (int i = 0; i < WINDOW_WIDTH * WINDOW_HEIGHT * 4; i++)
		{
			pixelBuffer[i] = std::rand() % 255;
		}
		return pixelBuffer;
	};
};

// Run it
int main()
{
	srand(time(NULL));
	util::Platform platform;
	// Create the main window
	sf::RenderWindow window(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "Sunshine 0.1");

	Raytracer tracer;
	// Scene scene;
	Renderer drawBuffer;

	// Create a graphical text to display
	sf::Font font;
	if (!font.loadFromFile("content/Lato-Regular.ttf"))
		return EXIT_FAILURE;

	// FPS clock
	sf::Clock clock;
	clock.restart();
	float lastTick = 0;
	uint frame = 0;
	std::string fpsString = "";
	sf::Text fpsText;

	tracer.UpdateRayDirections();

	// Start the game loop
	while (window.isOpen())
	{
		// Process events
		sf::Event event;
		while (window.pollEvent(event))
		{
			// Close window: exit
			if (event.type == sf::Event::Closed)
				window.close();
		}

		// To the screen
		window.clear();
		tracer.Render(drawBuffer.pixelBuffer);
		drawBuffer.render(window);

		// FPS
		if (frame % 100 == 0)
		{
			float tick = clock.getElapsedTime().asSeconds();
			int fps = round(1.0f / ((tick - lastTick) / 100.0f));
			lastTick = tick;
			fpsString = std::to_string(fps) + " fps";
			fpsText = sf::Text(fpsString, font, 20);
		}

		window.draw(fpsText);
		window.display();

		frame += 1;
	}
	return EXIT_SUCCESS;
}
