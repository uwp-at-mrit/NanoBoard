#pragma once

#include "editor.hpp"

#include "graphlet/filesystem/configuration/gpslet.hpp"

#include "cs/wgs_xy.hpp"

namespace WarGrey::SCADA {
	class GPSCSSelf;

	private class IGPSConvertor abstract {
	public:
		virtual WarGrey::SCADA::double3 gps_to_xyz(double latitude, double longitude, double altitude, GCSParameter& gcs) = 0;
		virtual WarGrey::SCADA::double3 xyz_to_gps(double x, double y, double z, GCSParameter& gcs) = 0;
	};

	private class GPSCSPlanet : public WarGrey::SCADA::EditorPlanet {
	public:
		virtual ~GPSCSPlanet() noexcept;
		GPSCSPlanet(WarGrey::SCADA::IGPSConvertor* gc = nullptr, Platform::String^ default_gps = "gps");

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
		WarGrey::SCADA::GPSCSSelf* self;
	};
}
