// Recursive ray tracing

#include "Platform/Platform.hpp"
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
		const Vec3f& direction)
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
		return dist;
	}
};

// Raytracer
class Raytracer
{
public:
	// Camera Setup
	float scale = 0.46;
	float aspectRatio = WINDOW_WIDTH / (float)WINDOW_HEIGHT;
	Matrix44f cameraToWorld {};
	Vec3f orig = Vec3f(0);

	// Level
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

	sf::Uint8 castRay(Vec3f& orig,
		const Vec3f& dir)
	{
		// int min_ind = -1;
		// float min_dist = 1e6;

		for (int i = 0; i < (int)spheres.size(); i++)
		{
			float dist = spheres[i].RayIntersection(orig, dir);
			if (dist > 0)
				return 100;
		}

		// const std::vector<std::unique_ptr<Sphere>>&spheres,
		// const std::vector<std::unique_ptr<Light>>&lights
		// if (scene.spheres.size() > 0 && dir.length() > 0 && orig.length())
		// {};
		return 0;
	}

	void Render(std::vector<sf::Uint8>& pixelBuffer)
	{
		// UpdateRayDirections();
		// pixelBuffer[0] = 100;
		for (int i = 0; i < WINDOW_WIDTH * WINDOW_HEIGHT; i++)
		{
			// Cast ray
			auto bright = castRay(orig, directions[i]);
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

	// Rendering stuff
	// std::vector<sf::Uint8> pixelBuffer(WINDOW_WIDTH * WINDOW_HEIGHT * 4);
	// sf::Texture texture;
	// sf::RectangleShape sprite;

	// texture.create(WINDOW_WIDTH, WINDOW_HEIGHT);
	// sprite.setSize({ WINDOW_WIDTH, WINDOW_HEIGHT });

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

		// Raycasting
		// drawBuffer.clearPixels();
		// drawBuffer.setPixel(300, 300, sf::Color(255, 0, 0, 255));

		// To the screen
		window.clear();
		tracer.Render(drawBuffer.pixelBuffer);
		// drawBuffer.pixelBuffer = tracer.RenderNoise();
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
