#pragma once

#include "editor.hpp"

namespace WarGrey::SCADA {
	private class TrailingSuctionDredgerEditor : public WarGrey::DTPM::EditorPlanet {
	public:
		virtual ~TrailingSuctionDredgerEditor() noexcept;
		TrailingSuctionDredgerEditor(Platform::String^ default_vessel = "vessel");

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
		class Self;
		WarGrey::SCADA::TrailingSuctionDredgerEditor::Self* self;
	};
}
