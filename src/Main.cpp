// Recursive ray tracing

#include "Platform/Platform.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <ctime>
#include <iostream>
#include <memory>
#include <mutex>
#include <sys/time.h>
#include <thread>
#include <time.h>
#include <vector>

#include "Constants.h"
#include "Geometry.cpp"

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
	void operator+=(const Color& col)
	{
		r = std::min(255, col.r + r);
		g = std::min(255, col.g + g);
		b = std::min(255, col.b + b);
	}
	Color operator*(const float f) const
	{
		int rr = std::min(255, (int)(r * f));
		int gg = std::min(255, (int)(g * f));
		int bb = std::min(255, (int)(b * f));
		return Color(rr, gg, bb);
	}
	void operator*=(const float f)
	{
		r = std::min(255, (int)(r * f));
		g = std::min(255, (int)(g * f));
		b = std::min(255, (int)(b * f));
	}
};

struct Light
{
	Vec3f position {};
	float brightness;

	Light(const Vec3f pos, const float bright) :
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

	Sphere(const Vec3f pos, const float rad, const Color col, const Vec3f vel) :
		position(pos),
		radius(rad),
		color(col),
		velocity(vel)
	{}

	float RayIntersection(
		const Vec3f& orig,
		const Vec3f& direction,
		Collision& hit) const
	{
		const auto o_minus_c = orig - position;

		const auto p = direction.dotProduct(o_minus_c);
		const auto q = o_minus_c.dotProduct(o_minus_c) - (radius * radius);

		const auto discriminant = (p * p) - q;
		if (discriminant < 0.0f)
		{
			return -1;
		}

		const auto dRoot = sqrt(discriminant);
		// auto dist = std::min(-p - dRoot, -p + dRoot);
		const auto dist = -p - dRoot;
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

	void Update(const float dT)
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

// Raytracer (actually more like a everything class...)
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

	// Ray directions are cached bc only a change in camera pos/rot will change them
	std::vector<Vec3f> directions;

	// Pixel output and associated mutex
	std::vector<sf::Uint8> pixelBuffer;
	sf::Texture texture;
	sf::RectangleShape sprite;
	std::mutex pixelMutex;

	// Gameloop stuff
	time_t lastTick;

	Raytracer() :
		pixelBuffer(WINDOW_WIDTH * WINDOW_HEIGHT * 4)
	{
		struct timeval time_now
		{};
		gettimeofday(&time_now, nullptr);
		lastTick = (time_now.tv_sec * 1000) + (time_now.tv_usec / 1000);

		// Graphics
		texture.create(WINDOW_WIDTH, WINDOW_HEIGHT);
		sprite.setSize({ WINDOW_WIDTH, WINDOW_HEIGHT });
		sprite.setTexture(&texture);

		// No transparency
		for (int i = 0; i < WINDOW_WIDTH * WINDOW_HEIGHT; i++)
		{
			pixelBuffer[i * 4 + 3] = 255;
		}

		GenerateLevel();
	}

	void GenerateLevel()
	{
		for (int i = 0; i < 8; i++)
		{
			const auto pos = Vec3f(
				-5.0 + (10.0 * ((rand() % 1000) / 1000.0)),
				-3.0 + (6.0 * ((rand() % 1000) / 1000.0)),
				-10.0 - 20.0 * ((rand() % 1000) / 1000));

			auto vel = Vec3f((rand() % 1000), (rand() % 1000), (rand() % 1000));
			vel /= 2000.0;

			const auto col = Color(rand() % 255, rand() % 255, rand() % 255);
			auto s1 = Sphere(pos, 0.5 + (rand() % 1000) / 1000.0, col, vel);
			spheres.push_back(s1);
		}

		for (int i = 0; i < 2; i++)
		{
			const auto pos = Vec3f(
				-5.0 + (10.0 * ((rand() % 1000) / 1000.0)),
				-20.0 + (40.0 * ((rand() % 1000) / 1000.0)),
				-10.0 - 20.0 * ((rand() % 1000) / 1000));

			const float brightness = 0.7 + 0.3 * ((rand() % 1000) / 1000.0);

			auto l = Light(pos, brightness);
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
				const float x = (2 * (i + 0.5) / (float)WINDOW_WIDTH - 1) * aspectRatio * scale;
				const float y = (1 - 2 * (j + 0.5) / (float)WINDOW_HEIGHT) * scale;
				Vec3f dir {};
				cameraToWorld.multDirMatrix(Vec3f(x, y, -1), dir);
				dir.normalize();
				directions.push_back(dir);
			}
		}
	};

	void castRay(const Vec3f& orig,
		const Vec3f& dir,
		const int depth,
		Color& out_color)

	{
		Collision hit {};
		float dist = 1e6;

		for (int i = 0; i < (int)spheres.size(); i++)
		{
			Collision c_hit;
			const float cdist = spheres[i].RayIntersection(orig, dir, c_hit);
			if (cdist > 0 && cdist < dist)
			{
				hit = c_hit;
				dist = cdist;
			}
		}

		if (dist > 0 && dist < 1000)
		{
			// Check illumination
			const int num = (int)lights.size();
			for (int i = 0; i < num; i++)
			{
				auto path = hit.position - lights[i].position;
				path.normalize();
				hit.color *= (acos(path.dotProduct(hit.normal)) / PI) * lights[i].brightness;
				out_color += hit.color;
			};

			if (depth < 5)
			{
				Color extra {};
				castRay(hit.position, hit.reflection, depth + 1, extra);
				extra *= std::pow(0.6, depth + 1);
				out_color += extra;
			}
		}
	};

	void RenderSingleThread(sf::RenderTarget& target)
	{
		for (int i = 0; i < WINDOW_WIDTH * WINDOW_HEIGHT; i++)
		{
			// Cast ray
			Color bright {};
			castRay(orig, directions[i], 0, bright);

			auto* ptr = &pixelBuffer.at(i * 4);
			ptr[0] = bright.r;
			ptr[1] = bright.g;
			ptr[2] = bright.b;
		}

		texture.update(pixelBuffer.data());
		target.draw(sprite);
	};

	void RenderMultiThread(sf::RenderTarget& target, int numThreads = 8)
	{
		int portion = (WINDOW_WIDTH * WINDOW_HEIGHT) / numThreads;
		std::vector<std::thread> workers;

		for (int i = 0; i < numThreads; i++)
		{
			workers.push_back(std::thread([&](int iLocal) {
				std::vector<sf::Uint8> partialBuffer(portion * 4);
				for (int j = 0; j < portion; j++)
				{
					// Cast rays
					Color color {};
					castRay(orig, directions[j + iLocal * portion], 0, color);
					const int indLocal = j * 4;
					partialBuffer[indLocal] = color.r;
					partialBuffer[indLocal + 1] = color.g;
					partialBuffer[indLocal + 2] = color.b;
					partialBuffer[indLocal + 3] = 255;
				}

				// Need mutex to copy into "global" pixelBuffer
				pixelMutex.lock();
				std::copy(partialBuffer.begin(), partialBuffer.end(), pixelBuffer.begin() + (iLocal * portion * 4));
				pixelMutex.unlock();
			},
				i));
		}

		for (auto& worker : workers)
		{
			worker.join();
		}

		texture.update(pixelBuffer.data());
		target.draw(sprite);
	};

	void Update()
	{
		struct timeval time_now
		{};
		gettimeofday(&time_now, nullptr);
		const auto t = (time_now.tv_sec * 1000) + (time_now.tv_usec / 1000);
		const float dT = (lastTick - t) / 1000.0;
		lastTick = t;
		for (auto& sphere : spheres)
		{
			sphere.Update(dT);
		}
	};

	void ToggleSphereDirections()
	{
		for (auto& sphere : spheres)
		{
			sphere.ToggleDirection();
		}
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
		tracer.RenderMultiThread(window, 8);

		// FPS
		if (frame % 10 == 0)
		{
			const float tick = clock.getElapsedTime().asSeconds();
			const int fps = round(1.0f / ((tick - lastTick) / 10.0f));
			lastTick = tick;
			fpsString = std::to_string(fps) + " fps";
			fpsText = sf::Text(fpsString, font, 20);
		}
		if (frame % 200 == 0)
		{
			tracer.ToggleSphereDirections();
		}

		window.draw(fpsText);
		window.display();

		frame += 1;
	}
	return EXIT_SUCCESS;
}
