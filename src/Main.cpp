// Recursive ray tracing

#include "Platform/Platform.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <ctime>
#include <iostream>
#include <memory>
#include <sys/time.h>
#include <time.h>
#include <vector>

#include "Constants.h"
#include "Geometry.cpp"
#include "Renderer.h"

// Setup scene
struct Color
{
	int r, g, b;
	Color() :
		r(0),
		g(0),
		b(0)
	{}
	Color(const Color& col) :
		r(col.r),
		g(col.g),
		b(col.b)
	{}
	Color(int re, int gr, int bl) :
		r(re),
		g(gr),
		b(bl)
	{}
	void operator+=(Color& col)
	{
		r = std::min(255, std::max(0, col.r + r));
		g = std::min(255, std::max(0, col.g + g));
		b = std::min(255, std::max(0, col.b + b));
	}
	Color operator*(float f)
	{
		int rr = std::min(255, std::max(0, (int)(r * f)));
		int gg = std::min(255, std::max(0, (int)(g * f)));
		int bb = std::min(255, std::max(0, (int)(b * f)));
		return Color(rr, gg, bb);
	}
};

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
	Color color;
};

struct Sphere
{
	Vec3f position {};
	float radius {};
	Color color {};
	Vec3f velocity {};
	bool forward = true;

	Sphere(Vec3f pos, float rad, Color col, Vec3f vel) :
		position(pos),
		radius(rad),
		color(col),
		velocity(vel)
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
		hit.color = color;

		return dist;
	}

	void Update(float dT)
	{
		if (forward)
			position += velocity * dT;
		else
			position -= velocity * dT;
	}

	void ToggleDirection()
	{
		forward = !forward;
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

	// Gameloop stuff
	time_t lastTick;

	Raytracer()
	{
		struct timeval time_now
		{};
		gettimeofday(&time_now, nullptr);
		lastTick = (time_now.tv_sec * 1000) + (time_now.tv_usec / 1000);

		for (int i = 0; i < 8; i++)
		{
			Vec3f pos = Vec3f(
				-5.0 + (10.0 * ((rand() % 1000) / 1000.0)),
				-3.0 + (6.0 * ((rand() % 1000) / 1000.0)),
				-10.0 - 20.0 * ((rand() % 1000) / 1000));

			Vec3f vel = Vec3f((rand() % 1000), (rand() % 1000), (rand() % 1000));
			vel /= 2000.0;

			Color col = Color(rand() % 255, rand() % 255, rand() % 255);
			Sphere s1 = Sphere(pos, 0.5 + (rand() % 1000) / 1000.0, col, vel);
			spheres.push_back(s1);
		}

		for (int i = 0; i < 2; i++)
		{
			Vec3f pos = Vec3f(
				-5.0 + (10.0 * ((rand() % 1000) / 1000.0)),
				-20.0 + (40.0 * ((rand() % 1000) / 1000.0)),
				-10.0 - 20.0 * ((rand() % 1000) / 1000));

			float brightness = 0.4 + 0.3 * ((rand() % 1000) / 1000.0);

			Light l = Light(pos, brightness);
			lights.push_back(l);
		}
	}

	// Use when camera position is updated
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

	Color castRay(Vec3f& orig,
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

		Color bright {};
		if (dist > 0 && dist < 1e6)
		{
			// Check illumination
			int num = (int)lights.size();
			for (int i = 0; i < num; i++)
			{
				Vec3f path = hit.position - lights[i].position;
				path.normalize();
				Color c = hit.color * (acos(path.dotProduct(hit.normal)) / PI) * lights[i].brightness;
				bright += c;
			};

			Color extra {};
			if (depth < 8)
			{
				Vec3f pos = Vec3f(hit.position.x, hit.position.y, hit.position.z);
				Vec3f ref = Vec3f(hit.reflection.x, hit.reflection.y, hit.reflection.z);
				extra = castRay(pos, ref, depth + 1);
				bright += extra;
			}

			return bright * (std::pow(0.9, depth) / (float)num);
		}

		return bright;
	}

	void Render(std::vector<sf::Uint8>& pixelBuffer)
	{
		for (int i = 0; i < WINDOW_WIDTH * WINDOW_HEIGHT; i++)
		{
			// Cast ray
			Color bright = castRay(orig, directions[i], 0);

			uint ind = i * 4;
			pixelBuffer[ind] = bright.r;
			pixelBuffer[ind + 1] = bright.g;
			pixelBuffer[ind + 2] = bright.b;
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

	void Update()
	{
		struct timeval time_now
		{};
		gettimeofday(&time_now, nullptr);
		time_t t = (time_now.tv_sec * 1000) + (time_now.tv_usec / 1000);
		float dT = (lastTick - t) / 1000.0;
		lastTick = t;
		for (auto& sphere : spheres)
		{
			sphere.Update(dT);
		}
	}

	void ToggleSphereDirections()
	{
		for (auto& sphere : spheres)
		{
			sphere.ToggleDirection();
		}
	}
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

		tracer.Update();

		// To the screen
		window.clear();
		tracer.Render(drawBuffer.pixelBuffer);
		drawBuffer.render(window);

		// FPS
		if (frame % 10 == 0)
		{
			float tick = clock.getElapsedTime().asSeconds();
			int fps = round(1.0f / ((tick - lastTick) / 10.0f));
			lastTick = tick;
			fpsString = std::to_string(fps) + " fps";
			fpsText = sf::Text(fpsString, font, 20);
		}
		if (frame % 120 == 0)
		{
			tracer.ToggleSphereDirections();
		}

		window.draw(fpsText);
		window.display();

		frame += 1;
	}
	return EXIT_SUCCESS;
}
