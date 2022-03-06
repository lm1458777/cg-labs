#include "stdafx.h"
#include "SimulationView.h"

namespace
{

constexpr auto SCALE = 100.f;
constexpr auto _1px = 1.f / SCALE;

} // namespace

using namespace Gdiplus;

struct SimulationView::PaintingKit
{
	Pen pen{ gsl::narrow<ARGB>(Color::DarkCyan), _1px };

	Pen gunPen{ gsl::narrow<ARGB>(Color::DarkRed), _1px / 2 * Simulation::BALL_GUN_RADIUS };
	SolidBrush gunBrush{ gsl::narrow<ARGB>(Color::White) };
};

namespace
{

constexpr auto ANIMATION_TIMER_ID = 1;

class SimulationGraphics : public IGraphics
{
public:
	SimulationGraphics(
		Gdiplus::Graphics& graphics,
		Simulation::Radians gunBarrelDirection,
		const SimulationView::PaintingKit& paintingKit)
		: m_graphics{ graphics }
		, m_gunBarrelDirection{ gunBarrelDirection }
		, m_paintingKit{ paintingKit }
	{
	}

	void DrawBall(Circle ball) override
	{
		auto d = 2 * ball.radius;
		m_graphics.DrawEllipse(
			&m_paintingKit.pen,
			ball.center.x - ball.radius, ball.center.y - ball.radius, d, d);

		//// Закрашиваем эллипс кистью желтого цвета
		//Gdiplus::SolidBrush brush(Gdiplus::Color(255, 255, 0));
		//g.FillEllipse(&brush, 100, 100, 200, 100);

		//// Рисуем границу эллипса красным пером толщиной 2 пикселя
		//Gdiplus::Pen pen(Gdiplus::Color(255, 0, 0), 2);
		//g.DrawEllipse(&pen, 100, 100, 200, 100);
	}

	void DrawObstacle(const Vertices4& vertices) override
	{
		Gdiplus::PointF points[] = {
			{ vertices[0].x, vertices[0].y },
			{ vertices[1].x, vertices[1].y },
			{ vertices[2].x, vertices[2].y },
			{ vertices[3].x, vertices[3].y },
		};

		static_assert(Vertices4{}.size() == std::size(points));

		m_graphics.DrawPolygon(
			&m_paintingKit.pen,
			points, gsl::narrow<INT>(std::size(points)));
	}

	void DrawBallGun(::Point position) override
	{
		auto transform = Matrix();
		m_graphics.GetTransform(&transform);
		auto restoreTransform = gsl::finally([this, &transform] {
			m_graphics.SetTransform(&transform);
		});

		m_graphics.TranslateTransform(position.x, position.y);

		m_graphics.ScaleTransform(Simulation::BALL_GUN_RADIUS, Simulation::BALL_GUN_RADIUS);

		auto directionDegrees = m_gunBarrelDirection * 180 / 3.14159265f;
		m_graphics.RotateTransform(directionDegrees);

		// base
		m_graphics.DrawEllipse(&m_paintingKit.gunPen, -1.f, -1.f, 2.f, 2.f);

		// barrel
		constexpr auto BARREL_RADIUS = 0.125f;
		m_graphics.DrawRectangle(&m_paintingKit.gunPen, 0.f, -BARREL_RADIUS, 0.9f, 2 * BARREL_RADIUS);

		// top
		constexpr auto GUN_TOP_RADIUS = 0.4f;
		auto gunTop = RectF{ -GUN_TOP_RADIUS, -GUN_TOP_RADIUS, 2 * GUN_TOP_RADIUS, 2 * GUN_TOP_RADIUS };
		m_graphics.FillEllipse(&m_paintingKit.gunBrush, gunTop);
		m_graphics.DrawEllipse(&m_paintingKit.gunPen, gunTop);
	}

private:
	Gdiplus::Graphics& m_graphics;
	Simulation::Radians m_gunBarrelDirection = 0;

	const SimulationView::PaintingKit& m_paintingKit;
};

[[nodiscard]] CPoint WorldPointToClientAreaPoint(::Point pt, CPoint clientAreaCenter)
{
	return {
		gsl::narrow_cast<int>(pt.x * SCALE + clientAreaCenter.x),
		gsl::narrow_cast<int>(clientAreaCenter.y - pt.y * SCALE)
	};
}

} // namespace

void SimulationView::OnPaint(CDCHandle /*dc*/)
{
	RenderSceneToBackBuffer();

	CPaintDC dc(m_hWnd);
	Graphics g{ dc };
	g.DrawImage(m_backBuffer.get(), 0, 0);
}

void SimulationView::OnSize(UINT /*nType*/, CSize size)
{
	m_clientAreaCenter.SetPoint(size.cx / 2, size.cy / 2);
	m_ballGunCenter = WorldPointToClientAreaPoint(m_game.GetBallGunPosition(), m_clientAreaCenter);

	m_backBuffer = std::make_unique<Gdiplus::Bitmap>(size.cx, size.cy, PixelFormat32bppARGB);
}

void SimulationView::OnMouseMove(UINT /*nFlags*/, CPoint point)
{
	UpdateMousePos(point);
}

void SimulationView::OnLButtonDown(UINT /*nFlags*/, CPoint point)
{
	UpdateMousePos(point);
	m_game.LaunchBall(GetBallLaunchDirection());
}

int SimulationView::OnCreate(LPCREATESTRUCT)
{
	m_paintingKit = std::make_unique<PaintingKit>();
	SetTimer(ANIMATION_TIMER_ID, gsl::narrow<UINT>(Simulation::TIME_STEP.count()), nullptr);
	return 0;
}

void SimulationView::OnDestroy()
{
	KillTimer(ANIMATION_TIMER_ID);
	m_paintingKit.reset();
}

SimulationView::SimulationView() = default;

SimulationView::~SimulationView() = default;

void SimulationView::OnTimer(UINT_PTR nIDEvent)
{
	switch (nIDEvent)
	{
	case ANIMATION_TIMER_ID:
		Animate();
		break;
	}
}

void SimulationView::Animate()
{
	m_game.Update();

	Invalidate();
	UpdateWindow();
}

void SimulationView::RenderSceneToBackBuffer()
{
	Gdiplus::Graphics g(m_backBuffer.get());

	// Устанавливаем режим сглаживания границ векторных примитивов,
	// обеспечивающий высокое качество рисования
	g.SetSmoothingMode(Gdiplus::SmoothingModeHighQuality);

	g.Clear(gsl::narrow<ARGB>(Color::White));

	g.TranslateTransform(
		gsl::narrow<REAL>(m_clientAreaCenter.x), gsl::narrow<REAL>(m_clientAreaCenter.y));
	g.ScaleTransform(SCALE, -SCALE);

	SimulationGraphics simulationGraphics{ g, GetBallLaunchDirection(), *m_paintingKit };
	m_game.Render(simulationGraphics);
}

void SimulationView::UpdateMousePos(CPoint newPos)
{
	if (m_ballGunCenter != newPos)
	{
		m_mousePos = newPos;
	}
}

Simulation::Radians SimulationView::GetBallLaunchDirection() const
{
	ATLASSERT(m_ballGunCenter != m_mousePos);
	auto delta = m_mousePos - m_ballGunCenter;

	auto direction = atan2(
		static_cast<Simulation::Radians>(delta.cy),
		static_cast<Simulation::Radians>(delta.cx));

	// В экранных координатах ось Y направлена вниз, а в координатах игрового мира - вверх.
	direction = -direction;

	return direction;
}
