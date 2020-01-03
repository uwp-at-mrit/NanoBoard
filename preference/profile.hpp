#pragma once

#include "editor.hpp"

namespace WarGrey::DTPM {
	class ProfileSelf;

	private class ProfileEditor : public WarGrey::DTPM::EditorPlanet {
	public:
		virtual ~ProfileEditor() noexcept;
		ProfileEditor(Platform::String^ default_section = "profile");

	public:
		void load(Microsoft::Graphics::Canvas::UI::CanvasCreateResourcesReason reason, float width, float height) override;
		void reflow(float width, float height) override;
		void on_graphlet_ready(WarGrey::SCADA::IGraphlet* g) override;
		WarGrey::SCADA::IGraphlet* thumbnail_graphlet() override;

	protected:
		bool on_apply() override;
		bool on_reset() override;
		bool on_edit(WarGrey::SCADA::Dimensionlet* dim) override;

	private:
		WarGrey::DTPM::ProfileSelf* self;
	};
}
