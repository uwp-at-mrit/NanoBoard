#pragma once

#include "editor.hpp"

namespace WarGrey::DTPM {
	private class DredgeTrackEditor : public WarGrey::DTPM::EditorPlanet {
	public:
		virtual ~DredgeTrackEditor() noexcept;
		DredgeTrackEditor(Platform::String^ default_track = "track");

	public:
		void load(Microsoft::Graphics::Canvas::UI::CanvasCreateResourcesReason reason, float width, float height) override;
		void reflow(float width, float height) override;
		void on_graphlet_ready(WarGrey::SCADA::IGraphlet* g) override;
		WarGrey::SCADA::IGraphlet* thumbnail_graphlet() override;

	protected:
		bool on_apply() override;
		bool on_reset() override;
		bool on_default() override;
		bool on_edit(WarGrey::SCADA::Dimensionlet* dim) override;

	private:
		class Self;
		WarGrey::DTPM::DredgeTrackEditor::Self* self;
	};
}
