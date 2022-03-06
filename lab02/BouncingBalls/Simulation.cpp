#include "stdafx.h"
#include "Simulation.h"

using namespace std::chrono;

namespace
{

[[nodiscard]] ::Point ToPoint(b2Vec2 v) noexcept
{
	return { v.x, v.y };
}

[[nodiscard]] Circle GetCircle(const b2Body& body)
{
	auto* fixture = body.GetFixtureList();
	Expects(fixture);

	auto fixtureType = fixture->GetType();
	Expects(fixtureType == b2Shape::e_circle);

	auto* shape = fixture->GetShape();
	Expects(shape);

	const auto& circle = dynamic_cast<const b2CircleShape&>(*shape);
	auto center = b2Mul(body.GetTransform(), circle.m_p);

	return { ToPoint(center), circle.m_radius };
}

void DrawBall(IGraphics& g, const b2Body& ball)
{
	g.DrawBall(GetCircle(ball));
}

void DrawBallGun(IGraphics& g, const b2Body& gun)
{
	g.DrawBallGun(GetCircle(gun).center);
}

void DrawObstacle(IGraphics& g, const b2Body& body)
{
	auto* fixture = body.GetFixtureList();
	Expects(fixture);

	auto fixtureType = fixture->GetType();
	Expects(fixtureType == b2Shape::e_polygon);

	auto* shape = fixture->GetShape();
	Expects(shape);

	const auto& poly = dynamic_cast<const b2PolygonShape&>(*shape);
	const auto& transform = body.GetTransform();

	auto vertexCount = poly.m_count;
	Expects(vertexCount == 4);

	auto vertices = Vertices4();
	for (auto i = 0; i < vertexCount; ++i)
	{
		vertices[i] = ToPoint(b2Mul(transform, poly.m_vertices[i]));
	}

	g.DrawObstacle(vertices);
}

struct DDraw : b2Draw
{
	DDraw()
	{
		AppendFlags(DDraw::e_shapeBit);
	}

	void DrawPolygon(const b2Vec2* /*vertices*/, int32 /*vertexCount*/, const b2Color& /*color*/) override
	{
	}

	void DrawSolidPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& /*color*/) override
	{
		std::ostringstream ss;
		for (int32 i = 0; i < vertexCount; ++i)
		{
			const auto& v = vertices[i];
			ss << "(" << v.x << "," << v.y << ") ";
		}

		auto sss = ss.str();
	}

	void DrawCircle(const b2Vec2& center, float32 radius, const b2Color& /*color*/) override
	{
		std::ostringstream ss;
		ss << "(" << center.x << "," << center.y << "), r:" << radius;
		auto sss = ss.str();
	}

	void DrawSolidCircle(const b2Vec2& center, float32 radius, const b2Vec2& /*axis*/, const b2Color& /*color*/) override
	{
		std::ostringstream ss;
		ss << "(" << center.x << "," << center.y << "), r:" << radius;
		auto sss = ss.str();
	}

	void DrawSegment(const b2Vec2& p1, const b2Vec2& p2, const b2Color& /*color*/) override
	{
		std::ostringstream ss;
		ss << "(" << p1.x << "," << p1.y << "); (" << p2.x << "," << p2.y << ")";
		auto sss = ss.str();
	}

	void DrawTransform(const b2Transform& /*xf*/) override
	{
	}

	void DrawPoint(const b2Vec2& /*p*/, float32 /*size*/, const b2Color& /*color*/) override
	{
	}
};

constexpr auto BALL_RADIUS = 0.05f;
constexpr auto INITIAL_BALL_VELOCITY = 7.0f;
constexpr auto RED_LINE = -3.0f - 2 * BALL_RADIUS;

gsl::not_null<b2Body*> CreateBall(b2World& world, b2Vec2 ballPosition)
{
	b2BodyDef bodyDef;
	bodyDef.type = b2_dynamicBody;
	bodyDef.position.Set(ballPosition.x, ballPosition.y);
	bodyDef.bullet = true;
	auto* ball = world.CreateBody(&bodyDef);

	b2CircleShape dynamicBox;
	dynamicBox.m_radius = BALL_RADIUS;

	b2FixtureDef fixtureDef;
	fixtureDef.shape = &dynamicBox;
	fixtureDef.density = 1.0f;
	fixtureDef.friction = 0.5f;
	fixtureDef.restitution = 0.8f;

	// Add the shape to the body.
	ball->CreateFixture(&fixtureDef);
	return ball;
}

gsl::not_null<b2Body*> CreateObstacle(
	b2World& world,
	b2Vec2 center, float32 width, float32 height,
	float32 angle = 0.f)
{
	auto bodyDef = b2BodyDef();
	bodyDef.position = center;
	bodyDef.angle = angle;

	// Call the body factory which allocates memory for the ground body
	// from a pool and creates the ground box shape (also from a pool).
	// The body is also added to the world.
	gsl::not_null body = world.CreateBody(&bodyDef);

	// The extents are the half-widths of the box.
	auto groundBox = b2PolygonShape();
	groundBox.SetAsBox(width / 2, height / 2);

	// Add the ground fixture to the ground body.
	body->CreateFixture(&groundBox, 0.0f);
	return body;
}

std::vector<gsl::not_null<b2Body*>> CreateObstacles(b2World& world)
{
	return {
		CreateObstacle(world, { -1.5f, -1.6f }, 5.5f, 0.16f, -0.35f),
		CreateObstacle(world, { 2.4f, -0.85f }, 3.6f, 0.16f, 0.436f),
		CreateObstacle(world, { 0.03f, -0.7f }, 1.3f, 0.7f, 0.733f),
		CreateObstacle(world, { 2.78f, 1.46f }, 0.82f, 0.64f, 0.0f),
		CreateObstacle(world, { -2.04f, 0.27f }, 0.98f, 0.8f, -0.646f),
	};
}

void AddEdge(b2World& world, b2Vec2 p1, b2Vec2 p2)
{
	auto bodyDef = b2BodyDef();
	bodyDef.position.Set(0.0f, 0.0f);

	auto* body = world.CreateBody(&bodyDef);

	auto edgeShape = b2EdgeShape();
	edgeShape.Set(p1, p2);
	body->CreateFixture(&edgeShape, 0.0f);
}

void AddBoundaries(b2World& world, float32 left, float32 top, float32 right, float32 bottom)
{
	AddEdge(world, { left, top }, { left, bottom });
	AddEdge(world, { right, top }, { right, bottom });
	AddEdge(world, { left, top }, { right, top });
	AddEdge(world, { left, bottom }, { right, bottom });
}

gsl::not_null<b2Body*> CreateBallGun(b2World& world, b2Vec2 position)
{
	auto bodyDef = b2BodyDef();
	bodyDef.position = position;
	gsl::not_null gun = world.CreateBody(&bodyDef);

	auto gunShape = b2CircleShape();
	gunShape.m_radius = Simulation::BALL_GUN_RADIUS;

	auto fixtureDef = b2FixtureDef();
	fixtureDef.shape = &gunShape;
	gun->CreateFixture(&fixtureDef);

	return gun;
}

} // namespace

class Simulation::Impl
{
public:
	static b2Vec2 GetBallGunPosition()
	{
		return { 0, 1 };
	}

	Impl()
	{
		//m_world.SetDebugDraw(&m_debugDraw);

		AddBoundaries(m_world, -4.f, 3.f, 4.f, -3.f - 4 * BALL_RADIUS);
	}

	void Render(IGraphics& g)
	{
		m_world.DrawDebugData();

		DrawBallGun(g, *m_gun);

		for (gsl::not_null obstacle : m_obstacles)
		{
			DrawObstacle(g, *obstacle);
		}

		for (gsl::not_null ball : m_balls)
		{
			DrawBall(g, *ball);
		}
	}

	void Update()
	{
		constexpr auto VELOCITY_ITERATIONS = 6;
		constexpr auto POSITION_ITERATIONS = 2;

		using SecondsFloat32 = duration<float32>;
		static constexpr auto timeStep = duration_cast<SecondsFloat32>(Simulation::TIME_STEP).count() / 2;

		m_world.Step(timeStep, VELOCITY_ITERATIONS, POSITION_ITERATIONS);

		RemoveFallenBalls();
	}

	void LaunchBall(Radians azimuth)
	{
		if (m_balls.size() >= BALL_LIMIT)
		{
			return;
		}

		// Начальное положение мяча находится за пределами пушки.
		constexpr auto MIN_D = BALL_GUN_RADIUS + BALL_RADIUS;

		auto rot = b2Rot(azimuth);
		auto ballVelocity = b2Mul(rot, b2Vec2{ INITIAL_BALL_VELOCITY, 0 });
		auto ballPos = b2Mul(rot, b2Vec2{ MIN_D, 0 }) + GetBallGunPosition();
		auto ball = CreateBall(m_world, ballPos);
		ball->SetLinearVelocity(ballVelocity);
		m_balls.push_back(ball);
	}

private:
	void RemoveFallenBalls()
	{
		const auto destroyIfOutOfScope = [& world = m_world](gsl::not_null<b2Body*> ball) {
			if (auto pos = ball->GetPosition(); pos.y < RED_LINE)
			{
				world.DestroyBody(ball);
				return true;
			}

			return false;
		};

		m_balls.erase(
			std::remove_if(m_balls.begin(), m_balls.end(), destroyIfOutOfScope),
			m_balls.end());
	}

private:
	DDraw m_debugDraw;

	b2World m_world{ b2Vec2{ 0.0f, -10.0f } /*gravity*/ };
	std::vector<gsl::not_null<b2Body*>> m_obstacles = CreateObstacles(m_world);
	gsl::not_null<b2Body*> m_gun = CreateBallGun(m_world, GetBallGunPosition());

	static constexpr size_t BALL_LIMIT = 50;
	std::vector<gsl::not_null<b2Body*>> m_balls;
};

Simulation::Simulation()
	: m_impl{ std::make_unique<Impl>() }
{
}

Simulation::~Simulation() = default;

::Point Simulation::GetBallGunPosition() const
{
	return ToPoint(m_impl->GetBallGunPosition());
}

void Simulation::Update()
{
	m_impl->Update();
}

void Simulation::Render(IGraphics& graphics)
{
	m_impl->Render(graphics);
}

void Simulation::LaunchBall(Radians azimuth)
{
	m_impl->LaunchBall(azimuth);
}
