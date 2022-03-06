#pragma once

#include "IGraphics.h"

class Simulation
{
public:
	using Radians = float;

	static constexpr std::chrono::milliseconds TIME_STEP{ 20 };
	static constexpr float BALL_GUN_RADIUS = 0.4f;

	Simulation();
	~Simulation();

	::Point GetBallGunPosition() const;

	void Update();
	void Render(IGraphics& graphics);
	void LaunchBall(Radians azimuth);

private:
	class Impl;
	std::unique_ptr<Impl> m_impl;
};
