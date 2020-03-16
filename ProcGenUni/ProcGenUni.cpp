#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"

#include <random>

// Possible stars colors
constexpr uint32_t g_starColours[8] =
{
	0xFFFFFFFF, 0xFFD9FFFF, 0xFFA3FFFF, 0xFFFFC8C8,
	0xFFFFCB9D, 0xFF9F9FFF, 0xFF415EFF, 0xFF28199D
};

// Possible ring colors
olc::Pixel g_ringColours[3] =
{
	olc::YELLOW, olc::DARK_YELLOW, olc::VERY_DARK_YELLOW
};

// Possible moon colors
olc::Pixel g_moonColours[3] =
{
	olc::GREY, olc::DARK_GREY, olc::VERY_DARK_GREY
};

// Supernova colors
olc::Pixel MIDNIGHTBLUE(25, 25, 112);
olc::Pixel DARKSLATEBLUE(72, 61, 139);
olc::Pixel LIGHTSLATEBLUE(132, 112, 255);

// Black hole colors
olc::Pixel BHORANGE(227, 81, 36);
olc::Pixel BHYELLOW(255, 237, 102);

struct sPlanet
{
	double distance = 0.0;
	double diameter = 0.0;
	double foliage = 0.0;
	double minerals = 0.0;
	double water = 0.0;
	double gases = 0.0;
	double temperature = 0.0;
	double population = 0.0;
	bool ring = false;
	olc::Pixel planetColor = olc::WHITE;
	std::vector<double> vMoons;
};

class cStarSystem
{
public:
	cStarSystem(uint32_t x, uint32_t y, bool bGenerateFullSystem = false)
	{
		// Set seed based on location of star system
		nProcGen = (x & 0xFFFF) << 16 | (y & 0xFFFF);

		// Not all locations contain a system
		starExists = (rndInt(0, 20) == 1);
		if (!starExists)
		{
			// Can be blackhole
			blackHoleExists = (rndInt(0, 1000) == 1);
			if (blackHoleExists)
			{
				isSuperNova = (rndInt(0, 4) == 1);
				blackHoleDiameter = rndDouble(10.0, 40.0);
				blackHoleTemperature = rndInt(500, 3000) * blackHoleDiameter;
				return;
			}

			asteroidExists = (rndInt(0, 300) == 1);
			if (asteroidExists && !blackHoleExists)
			{
				asteroidDiameter = rndDouble(5.0, 25.0);
			}
			return;
		}

		// Generate Star
		starDiameter = rndDouble(10.0, 40.0);
		starColour.n = g_starColours[rndInt(0, 8)];
		starTemperature = rndInt(500, 3000) * starDiameter;

		// When viewing the galaxy map, we only care about the star
		// so abort early
		if (!bGenerateFullSystem) return;

		// If we are viewing the system map, we need to generate the
		// full system

		// Generate Planets
		double dDistanceFromStar = rndDouble(60.0, 200.0);
		int nPlanets = rndInt(0, 10);
		for (int i = 0; i < nPlanets; i++)
		{
			sPlanet p;
			p.distance = dDistanceFromStar;
			dDistanceFromStar += rndDouble(20.0, 200.0);
			p.diameter = rndDouble(4.0, 20.0);

			// Could make temeprature a function of distance from star
			p.temperature = rndDouble(-200.0, 300.0);

			// Composition of planet
			p.foliage = rndDouble(0.0, 1.0);
			p.minerals = rndDouble(0.0, 1.0);
			p.gases = rndDouble(0.0, 1.0);
			p.water = rndDouble(0.0, 1.0);
			p.planetColor = g_starColours[rndInt(0, 8)];

			// Normalise to 100%
			double dSum = 1.0 / (p.foliage + p.minerals + p.gases + p.water);
			p.foliage *= dSum;
			p.minerals *= dSum;
			p.gases *= dSum;
			p.water *= dSum;

			// Population could be a function of other habitat encouraging
			// properties, such as temperature and water
			p.population = std::max(rndInt(-5000000, 20000000), 0);

			// 10% of planets have a ring
			p.ring = rndInt(0, 10) == 1;

			// Satellites (Moons)
			int nMoons = std::max(rndInt(-5, 5), 0);
			for (int n = 0; n < nMoons; n++)
			{
				// A moon is just a diameter for now, but it could be
				// whatever you want!
				p.vMoons.push_back(rndDouble(1.0, 5.0));
			}

			// Add planet to vector
			vPlanets.push_back(p);
		}
	}

	~cStarSystem()
	{

	}

public:
	std::vector<sPlanet> vPlanets;

public:
	bool		starExists = false;
	double		starDiameter = 0.0f;
	olc::Pixel	starColour = olc::WHITE;
	int			starTemperature;
	
public:
	bool		blackHoleExists = false;
	double		blackHoleDiameter = 0.0f;
	int			blackHoleTemperature;
	bool		isSuperNova = false;

public:
	bool		asteroidExists = false;
	double		asteroidDiameter = 0.0f;

	uint32_t nProcGen = 0;

	double rndDouble(double min, double max)
	{
		return ((double)rnd() / (double)(0x7FFFFFFF)) * (max - min) + min;
	}

	int rndInt(int min, int max)
	{
		return (rnd() % (max - min)) + min;
	}

	uint32_t rnd()
	{
		nProcGen += 0xe120fc15;
		uint64_t tmp;
		tmp = (uint64_t)nProcGen * 0x4a39b70d;
		uint32_t m1 = (tmp >> 32) ^ tmp;
		tmp = (uint64_t)m1 * 0x12fad5c9;
		uint32_t m2 = (tmp >> 32) ^ tmp;
		return m2;
	}
};

class olcGalaxy : public olc::PixelGameEngine
{
public:
	olcGalaxy()
	{
		sAppName = "olcGalaxy";
	}

public:
	bool OnUserCreate() override
	{

		return true;
	}

	olc::vf2d vGalaxyOffset = { 0,0 };
	bool bStarSelected = false;
	uint32_t nSelectedStarSeed1 = 0;
	uint32_t nSelectedStarSeed2 = 0;


	bool OnUserUpdate(float fElapsedTime) override
	{
		if (fElapsedTime <= 0.0001f) return true;

		Clear(olc::BLACK);

		// Movement keys
		if (GetKey(olc::W).bHeld) vGalaxyOffset.y -= 50.0f * fElapsedTime;
		if (GetKey(olc::S).bHeld) vGalaxyOffset.y += 50.0f * fElapsedTime;
		if (GetKey(olc::A).bHeld) vGalaxyOffset.x -= 50.0f * fElapsedTime;
		if (GetKey(olc::D).bHeld) vGalaxyOffset.x += 50.0f * fElapsedTime;

		if (GetKey(olc::W).bHeld && GetKey(olc::SHIFT).bHeld) vGalaxyOffset.y -= 75.0f * fElapsedTime;
		if (GetKey(olc::S).bHeld && GetKey(olc::SHIFT).bHeld) vGalaxyOffset.y += 75.0f * fElapsedTime;
		if (GetKey(olc::A).bHeld && GetKey(olc::SHIFT).bHeld) vGalaxyOffset.x -= 75.0f * fElapsedTime;
		if (GetKey(olc::D).bHeld && GetKey(olc::SHIFT).bHeld) vGalaxyOffset.x += 75.0f * fElapsedTime;

		int nSectorsX = ScreenWidth() / 16;
		int nSectorsY = ScreenHeight() / 16;

		olc::vi2d mouse = { GetMouseX() / 16, GetMouseY() / 16 };
		olc::vi2d galaxy_mouse = mouse + vGalaxyOffset;
		olc::vi2d screen_sector = { 0,0 };

		for (screen_sector.x = 0; screen_sector.x < nSectorsX; screen_sector.x++)
			for (screen_sector.y = 0; screen_sector.y < nSectorsY; screen_sector.y++)
			{
				uint32_t seed1 = (uint32_t)vGalaxyOffset.x + (uint32_t)screen_sector.x;
				uint32_t seed2 = (uint32_t)vGalaxyOffset.y + (uint32_t)screen_sector.y;

				cStarSystem star(seed1, seed2);
				// Draw star
				if (star.starExists || star.blackHoleExists)
				{
					if (!star.blackHoleExists)
					{
						FillCircle(screen_sector.x * 16 + 8, screen_sector.y * 16 + 8,
							(int)star.starDiameter / 8, star.starColour);
					}
					

					// For convenience highlight hovered star
					if (mouse.x == screen_sector.x && mouse.y == screen_sector.y)
					{
						DrawCircle(screen_sector.x * 16 + 8, screen_sector.y * 16 + 8, 12, olc::YELLOW);
					}
				}
				// Draw black hole or supernova
				if (star.blackHoleExists)
				{
					if (star.isSuperNova)
					{
						FillCircle(screen_sector.x * 16 + 8, screen_sector.y * 16 + 8,
							(int)star.blackHoleDiameter / 6, olc::VERY_DARK_MAGENTA);
						FillCircle(screen_sector.x * 16 + 8, screen_sector.y * 16 + 8,
							(int)star.blackHoleDiameter / 8, MIDNIGHTBLUE);
						FillCircle(screen_sector.x * 16 + 8, screen_sector.y * 16 + 8,
							(int)star.blackHoleDiameter / 12, DARKSLATEBLUE);
						FillCircle(screen_sector.x * 16 + 8, screen_sector.y * 16 + 8,
							(int)star.blackHoleDiameter / 24, LIGHTSLATEBLUE);
						FillCircle(screen_sector.x * 16 + 8, screen_sector.y * 16 + 8,
							(int)star.blackHoleDiameter / 30, olc::WHITE);
					}
					else
					{
						FillCircle(screen_sector.x * 16 + 8, screen_sector.y * 16 + 8,
							(int)star.blackHoleDiameter / 8, BHORANGE);
						FillCircle(screen_sector.x * 16 + 8, screen_sector.y * 16 + 8,
							(int)star.blackHoleDiameter / 10, BHYELLOW);
						FillCircle(screen_sector.x * 16 + 8, screen_sector.y * 16 + 8,
							(int)star.blackHoleDiameter / 12, olc::BLACK);
					}
				}
				// Draw single asteroid
				if (star.asteroidExists)
				{
					FillCircle(screen_sector.x * 16 + 8, screen_sector.y * 16 + 8,
						(int)star.asteroidDiameter / 8, olc::VERY_DARK_GREY);
				}
				// Draw asteroid field
				
				
			}

		// Handle Mouse Click
		if (GetMouse(0).bPressed)
		{
			uint32_t seed1 = (uint32_t)vGalaxyOffset.x + (uint32_t)mouse.x;
			uint32_t seed2 = (uint32_t)vGalaxyOffset.y + (uint32_t)mouse.y;

			cStarSystem star(seed1, seed2);
			if (star.starExists || star.blackHoleExists)
			{
				bStarSelected = true;
				nSelectedStarSeed1 = seed1;
				nSelectedStarSeed2 = seed2;
			}
			else
				bStarSelected = false;
		}

		// Draw Details of selected star system
		if (bStarSelected)
		{
			// Generate full star system
			cStarSystem star(nSelectedStarSeed1, nSelectedStarSeed2, true);

			// Draw Window
			FillRect(8, 240, 496, 232, olc::DARK_BLUE);
			DrawRect(8, 240, 496, 232, olc::WHITE);

			// Draw Star
			olc::vi2d vBody = { 14, 356 };

			// Draw star info text
			DrawString({ 15, 250 }, "Temp: " + std::to_string(star.starTemperature | star.blackHoleTemperature) + 
				(star.isSuperNova ? "B": "") + 'K');

			if (star.starExists)
			{
				vBody.x += star.starDiameter * 1.375;
				FillCircle(vBody, (int)(star.starDiameter * 1.375), star.starColour);
				vBody.x += (star.starDiameter * 1.375) + 8;
			}
			if (star.blackHoleExists)
			{
				if (star.isSuperNova)
				{
					vBody.x += star.blackHoleDiameter * 1.375;

					FillCircle(vBody, (int)star.blackHoleDiameter * 1.375, olc::VERY_DARK_MAGENTA);
					FillCircle(vBody, (int)star.blackHoleDiameter * 1.125, MIDNIGHTBLUE);
					FillCircle(vBody, (int)star.blackHoleDiameter * 0.825, DARKSLATEBLUE);
					FillCircle(vBody, (int)star.blackHoleDiameter * 0.4125, LIGHTSLATEBLUE);
					FillCircle(vBody, (int)star.blackHoleDiameter * 0.2, olc::WHITE);
					
					vBody.x += (star.blackHoleDiameter * 1.375) + 8;
				}
				else
				{
					vBody.x += star.blackHoleDiameter * 1.375;

					FillCircle(vBody, (int)star.blackHoleDiameter * 1.375, BHORANGE);
					FillCircle(vBody, (int)star.blackHoleDiameter * 1.25, BHYELLOW);
					FillCircle(vBody, (int)star.blackHoleDiameter * 1.175, olc::BLACK);

					vBody.x += (star.blackHoleDiameter * 1.375) + 8;
				}
				
			}

			



			// Draw Planets
			for (auto& planet : star.vPlanets)
			{
				if (vBody.x + planet.diameter >= 496) break;

				vBody.x += planet.diameter;
				FillCircle(vBody, (int)(planet.diameter * 1.0), planet.planetColor);

				// Draw planet ring with 3 cases for ring shape
				if (planet.ring)
				{
					olc::Pixel ringColor = g_ringColours[star.rndInt(0, 2)];
					switch (star.rndInt(0, 3)) {
					case (0):
						DrawLine(vBody.x, vBody.y + planet.diameter + 5, vBody.x, vBody.y - planet.diameter - 5, ringColor);
						break;
					case (1):
						DrawLine(vBody.x - planet.diameter - 5, vBody.y - planet.diameter - 5, vBody.x + planet.diameter + 5, vBody.y + planet.diameter + 5, ringColor);
						break;
					case (2):
						DrawLine(vBody.x + planet.diameter + 5, vBody.y - planet.diameter - 5, vBody.x - planet.diameter - 5, vBody.y + planet.diameter + 5, ringColor);
						break;
					}
				}
					
				
				// Draw significant planet features icons
				int nIconDistance = 12;
				int nIconY = vBody.y - planet.diameter;
				int nIconX = vBody.x;

				if (planet.water > 0.5)
				{
					// Draws a water drop icon o>
					olc::Pixel blue(0, 191, 255);
					FillCircle(nIconX, nIconY - nIconDistance, 2, blue);
					FillTriangle(nIconX - 2, nIconY - nIconDistance, nIconX + 2, nIconY - nIconDistance,
						nIconX, nIconY - 5 - nIconDistance, blue);

					nIconDistance += 12;
				}

				if (planet.foliage > 0.5)
				{
					// Draws a tree icon >=0
					olc::Pixel stump(96, 64, 32);
					
					FillTriangle(nIconX - 2, nIconY - nIconDistance, nIconX + 2, nIconY - nIconDistance,
						nIconX, nIconY - nIconDistance - 6, stump);
					FillRect(nIconX - 1, nIconY - nIconDistance - 6, 3, 6, stump);
					FillCircle(nIconX, nIconY - nIconDistance - 8, 3, olc::DARK_GREEN);

					nIconDistance += 12;
				}

				if (planet.minerals > 0.5)
				{
					FillRect(nIconX - 2, nIconY - nIconDistance  - 3, 5, 3, olc::DARK_GREY);
					DrawRect(nIconX - 2, nIconY - nIconDistance - 3, 5, 3, olc::VERY_DARK_GREY);

					nIconDistance += 12;
				}

				olc::vi2d vMoon = vBody;
				vMoon.y += planet.diameter + 10;

				// Draw Moons
				for (auto& moon : planet.vMoons)
				{
					
					vMoon.y += moon;
					FillCircle(vMoon, (int)(moon * 1.0), g_moonColours[star.rndInt(0, 3)]);
					
					vMoon.y += moon + 10;
				}

				vBody.x += planet.diameter + 8;
			}
		}

		return true;
	}
};

int main()
{
	olcGalaxy demo;
	if (demo.Construct(512, 480, 2, 2, false, false))
		demo.Start();
	return 0;
}