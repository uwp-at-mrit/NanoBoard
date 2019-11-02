#pragma once

#include "editor.hpp"

namespace WarGrey::SCADA {
	class ColorPlotSelf;

	private class ColorPlotEditor : public WarGrey::SCADA::EditorPlanet {
	public:
		virtual ~ColorPlotEditor() noexcept;
		ColorPlotEditor(Platform::String^ default_plot = "colorplot");

	public:
		void load(Microsoft::Graphics::Canvas::UI::CanvasCreateResourcesReason reason, float width, float height) override;
		void reflow(float width, float height) override;
		void on_graphlet_ready(WarGrey::SCADA::IGraphlet* g) override;
		IGraphlet* thumbnail_graphlet() override;

	protected:
		bool on_apply() override;
		bool on_reset() override;
		bool on_default() override;
		bool on_edit(WarGrey::SCADA::Dimensionlet* dim) override;

	private:
		WarGrey::SCADA::ColorPlotSelf* self;
	};
}
