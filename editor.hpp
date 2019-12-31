#pragma once

#include "planet.hpp"

#include "graphlet/ui/textlet.hpp"
#include "graphlet/ui/buttonlet.hpp"

#include "graphlet/shapelet.hpp"

namespace WarGrey::DTPM {
	private class EditorPlanet : public WarGrey::SCADA::Planet {
	public:
		EditorPlanet(Platform::String^ caption, unsigned int initial_mode = 0);

	public:
		void load(Microsoft::Graphics::Canvas::UI::CanvasCreateResourcesReason reason, float width, float height) override;
		void reflow(float width, float height) override;
		
	public:
		bool can_select(WarGrey::SCADA::IGraphlet* g) override;
		void on_tap_selected(WarGrey::SCADA::IGraphlet* g, float local_x, float local_y) override;
		bool on_key(Windows::System::VirtualKey key, bool wargrey_keyboard) override;
		void on_focus(WarGrey::SCADA::IGraphlet* g, bool yes) override;

	public:
		void enable_default(bool on_off);
		void notify_modification();
		void notify_updated();
		bool up_to_date();

	protected:
		virtual bool on_apply() = 0;
		virtual bool on_reset() = 0;
		virtual bool on_edit(WarGrey::SCADA::Dimensionlet* dim) = 0;
		virtual bool on_discard() { return true; }
		virtual bool on_default() { return false; }

	protected: // never delete these graphlets manually
		WarGrey::SCADA::Labellet* caption;
		WarGrey::SCADA::Buttonlet* apply;
		WarGrey::SCADA::Buttonlet* reset;
		WarGrey::SCADA::Buttonlet* discard;
		WarGrey::SCADA::Buttonlet* default;
		WarGrey::SCADA::Shapelet* background;
	};
}
