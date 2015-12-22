#pragma once

#include "lice_control.h"
#include "envelope_model.h"

struct fx_param_t
{
	int tracknum = -1;
	int fxnum = 0;
	int paramnum = 0;
};

struct point
{
	point() {}
	point(int x, int y) : m_x(x), m_y(y) {}
	int m_x = 0;
	int m_y = 0;
	fx_param_t m_x_target;
	fx_param_t m_y_target;
};

// Development test control
class TestControl : public LiceControl
{
public:
	TestControl(HWND parent, bool delwhendraggedoutside = false);
	void paint(LICE_IBitmap* bm) override;
	void mousePressed(const MouseEvent& ev) override;
	void mouseDoubleClicked(const MouseEvent& ev) override;
	void mouseMoved(const MouseEvent& ev) override;
	void mouseReleased(const MouseEvent& ev) override;
	void mouseWheel(int x, int y, int delta) override;
	bool keyPressed(const ModifierKeys& modkeys, int keycode) override;
	std::function<void(int, double, double)> PointMovedCallback;
	fx_param_t* getFXParamTarget(int index, int which);
	std::string getType() const override { return "MultiXYControl"; }
private:
	
	std::vector<point> m_points;
	int find_hot_point(int x, int y);
	int m_hot_point = -1;
	float m_circlesize = 10.0f;
	bool m_mousedown = false;
	bool m_delete_point_when_dragged_outside = false;
	std::string m_test_text;
	void shift_points(double x, double y);
	LICE_CachedFont m_font;
};

class WaveformPainter
{
public:
	WaveformPainter(int initialnumpeaks = 500)
	{
		m_minpeaks.resize(initialnumpeaks);
		m_maxpeaks.resize(initialnumpeaks);
	}
	bool paint(LICE_IBitmap* bm, double starttime, double endtime, int x, int y, int w, int h);
	void set_source(PCM_source* src)
	{
		if (src == nullptr)
		{
			m_src = nullptr;
			return;
		}
		m_src = std::shared_ptr<PCM_source>(src->Duplicate());
		if (m_src != nullptr)
		{
			if (m_src->PeaksBuild_Begin() != 0) // should build peaks
			{
				while (true)
				{
					if (m_src->PeaksBuild_Run() == 0)
						break;
				}
				m_src->PeaksBuild_Finish();
			}
		}
		m_last_w = 0;
		m_last_start = 0.0;
		m_last_end = 0.0;
	}
	PCM_source* getSource()
	{
		return m_src.get();
	}
private:
	std::shared_ptr<PCM_source> m_src;
	std::vector<double> m_minpeaks;
	std::vector<double> m_maxpeaks;
	int m_last_w = 0;
	double m_last_start = 0.0;
	double m_last_end = 0.0;
	PCM_source_peaktransfer_t m_peaks_transfer = { 0 };
};

class WaveformControl : public LiceControl
{
public:
	WaveformControl(HWND parent);
	void setSource(PCM_source* src);
	void paint(LICE_IBitmap* bm) override;
	void mousePressed(const MouseEvent& ev) override;
	void mouseMoved(const MouseEvent& ev) override;
	void mouseReleased(const MouseEvent& ev) override;
	void mouseDoubleClicked(const MouseEvent& ev) override;
	bool keyPressed(const ModifierKeys& modkeys, int keycode) override;
	std::string getType() const override { return "WaveformControl"; }
	double getFloatingPointProperty(int which) override;
	void setFloatingPointProperty(int which, double v) override;
private:
	std::shared_ptr<PCM_source> m_src;
	std::vector<double> m_minpeaks;
	std::vector<double> m_maxpeaks;
	double m_peaks_gain = 1.0;
	double m_view_start = 0.0;
	double m_view_end = 1.0;
	double m_sel_start = 0.0;
	double m_sel_end = 0.0;
	int m_drag_start_x = 0;
	int m_drag_start_y = 0;
	bool m_mouse_down = false;
	int get_hot_time_sel_edge(int x, int y);
	int m_hot_sel_edge = 0;
	bool m_use_reaper_peaks_drawing = false;
	LICE_CachedFont m_font;
};

class EnvelopeControl : public LiceControl
{
public:
	EnvelopeControl(HWND parent);
	void paint(LICE_IBitmap* bm) override;
	
	void mousePressed(const MouseEvent& ev) override;
	void mouseMoved (const MouseEvent& ev) override;
	void mouseReleased(const MouseEvent& ev) override;

	std::string getType() const override { return "BreakpointEnvelope"; }
	void add_envelope(std::shared_ptr<breakpoint_envelope> env);
	// Get envelope. -1 returns active envelope if exists. Returns null on failure
	std::shared_ptr<breakpoint_envelope> getEnvelope(int index);
	void set_waveformpainter(std::shared_ptr<WaveformPainter> painter);
	std::pair<double, double> getViewTimeRange() const { return { m_view_start_time, m_view_end_time }; }
	void setViewStartTime(double t) { m_view_start_time = t; repaint(); }
	void setViewEndTime(double t) { m_view_start_time = t; repaint(); }
	void setViewTimeRange(double t0, double t1)
	{
		m_view_start_time = t0;
		m_view_end_time = t1;
		repaint();
	}
	void fitEnvelopeTimeRangesIntoView();
	double getFloatingPointProperty(int index) override;
	void setFloatingPointProperty(int index, double value) override;
protected:
	std::vector<std::shared_ptr<breakpoint_envelope>> m_envs;
	std::shared_ptr<WaveformPainter> m_wave_painter;
	LICE_CachedFont m_font;
	bool m_mouse_down = false;
	std::pair<int, int> m_node_to_drag{ -1,-1 };
	std::pair<int, int> find_hot_envelope_point(double xcor, double ycor);
	double m_view_start_time = 0.0;
	double m_view_end_time = 1.0;
	double m_view_start_value = 0.0;
	double m_view_end_value = 1.0;
	std::string m_text;
	int m_active_envelope = -1;
	bool m_point_was_moved = false;
	void sanitize_view_ranges()
	{
		if (m_view_start_time == m_view_end_time)
			m_view_end_time += 0.001;
		else if (m_view_start_time > m_view_end_time)
			std::swap(m_view_start_time, m_view_end_time);
	}
};

class PitchBenderEnvelopeControl : public EnvelopeControl
{
public:
	PitchBenderEnvelopeControl(HWND parent) : EnvelopeControl(parent)
	{

	}
	bool keyPressed(const ModifierKeys& modkeys, int keycode) override;
	void mousePressed(const MouseEvent& ev) override;
	std::string getType() const override { return "PitchBenderEnvelopeControl"; }
private:
	int m_resampler_mode = -1;
};

std::string pitch_bend_selected_item(std::shared_ptr<breakpoint_envelope> pchenv,
	std::shared_ptr<breakpoint_envelope> volenv,int mode);

class EnvelopeGeneratorEnvelopeControl : public EnvelopeControl
{
public:
	EnvelopeGeneratorEnvelopeControl(HWND parent);
	
};