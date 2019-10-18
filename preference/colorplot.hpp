#pragma once

#include "device/editor.hpp"

#include "graphlet/filesystem/gpslet.hpp"

#include "cs/wgs_xy.hpp"

namespace WarGrey::SCADA {
	class ColorPlotSelf;

	private class ColorPlotPlanet : public WarGrey::SCADA::EditorPlanet {
	public:
		virtual ~ColorPlotPlanet() noexcept;
		ColorPlotPlanet(Platform::String^ default_gps = "colorplot");

	public:
		void load(Microsoft::Graphics::Canvas::UI::CanvasCreateResourcesReason reason, float width, float height) override;
		void reflow(float width, float height) override;
		void on_graphlet_ready(WarGrey::SCADA::IGraphlet* g) override;
		IGraphlet* thumbnail_graphlet() override;

	protected:
		bool on_apply() override;
		bool on_reset() override;
		bool on_edit(WarGrey::SCADA::Dimensionlet* dim) override;

	private:
		WarGrey::SCADA::ColorPlotSelf* self;
	};
}
