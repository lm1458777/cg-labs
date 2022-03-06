#pragma once
#include "Simulation.h"
#include "resource.h"

class SimulationView : public CWindowImpl<SimulationView>
{
public:
	struct PaintingKit;

	SimulationView();
	~SimulationView();

	DECLARE_WND_CLASS(NULL)

	BEGIN_MSG_MAP(SimulationView)
		MSG_WM_PAINT(OnPaint)
		MSG_WM_TIMER(OnTimer)
		MSG_WM_CREATE(OnCreate)
		MSG_WM_DESTROY(OnDestroy)
		MSG_WM_ERASEBKGND(OnEraseBkgnd)
		MSG_WM_SIZE(OnSize)
		MSG_WM_MOUSEMOVE(OnMouseMove)
		MSG_WM_LBUTTONDOWN(OnLButtonDown)
	END_MSG_MAP()

	// Handler prototypes (uncomment arguments if needed):
	//	LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	//	LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	//	LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)

private:
	void OnTimer(UINT_PTR nIDEvent);
	int OnCreate(LPCREATESTRUCT lpCreateStruct);
	void OnDestroy();
	void OnPaint(CDCHandle dc);
	void OnSize(UINT nType, CSize size);
	void OnMouseMove(UINT nFlags, CPoint point);
	void OnLButtonDown(UINT nFlags, CPoint point);

	BOOL OnEraseBkgnd(CDCHandle /*dc*/)
	{
		return FALSE;
	}

private:
	void Animate();
	void RenderSceneToBackBuffer();
	void UpdateMousePos(CPoint newPos);
	Simulation::Radians GetBallLaunchDirection() const;

private:
	std::unique_ptr<Gdiplus::Bitmap> m_backBuffer;
	Simulation m_game;
	CPoint m_ballGunCenter;
	CPoint m_mousePos;

	CPoint m_clientAreaCenter;

	std::unique_ptr<PaintingKit> m_paintingKit;
};
