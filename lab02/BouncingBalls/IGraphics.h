#pragma once

struct Point
{
	float x = 0;
	float y = 0;
};

struct Circle
{
	::Point center;
	float radius = 0;
};

using Vertices4 = std::array<::Point, 4>;

struct IGraphics
{
	virtual void DrawBall(Circle circle) = 0;
	virtual void DrawObstacle(const Vertices4& vertices) = 0;
	virtual void DrawBallGun(::Point position) = 0;

protected:
	~IGraphics() = default;
};
