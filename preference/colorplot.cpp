#include <map>

#include "preference/colorplot.hpp"

#include "graphlet/filesystem/configuration/colorplotlet.hpp"
#include "graphlet/ui/colorpickerlet.hpp"

#include "graphlet/shapelet.hpp"
#include "graphlet/planetlet.hpp"

#include "module.hpp"

using namespace WarGrey::SCADA;

using namespace Windows::Foundation;

using namespace Microsoft::Graphics::Canvas::UI;
using namespace Microsoft::Graphics::Canvas::Text;
using namespace Microsoft::Graphics::Canvas::Brushes;

static CanvasTextFormat^ label_font = make_bold_text_format("Microsoft Yahei", 16.0F);

static CanvasSolidColorBrush^ label_color = Colours::DarkGray;

/*************************************************************************************************/
namespace {
	// order matters
	private enum class CP {
		min, max,
		_
	};
}

private class WarGrey::SCADA::ColorPlotSelf {
public:
	ColorPlotSelf(ColorPlotEditor* master, Platform::String^ plot) : master(master), label_max_width(0.0F), entity(nullptr) {
		this->depth_style = make_highlight_dimension_style(label_font->FontSize, 3U, 6U, 2, label_color, Colours::Transparent);
		this->depth_style.label_xfraction = 2.0F / 3.0F;
		this->depth_style.unit_color = this->depth_style.label_color;

		this->plot = new ColorPlotlet(plot, 256.0F);
	}

public:
	void load(CanvasCreateResourcesReason reason, float width, float height, float inset) {
		float depth_width, depth_height;

		for (int idx = 0; idx < ColorPlotSize; idx++) {
			auto dim = new Credit<Dimensionlet, int>(DimensionState::Input, this->depth_style, "meter", (idx + 1).ToString());

			dim->fill_extent(0.0F, 0.0F, &depth_width, &depth_height);

			this->depths[idx] = this->master->insert_one(dim, idx);
			this->pickers[idx] = this->master->insert_one(new Credit<ColorPickerlet, int>(Palette::Xterm256, depth_width, depth_height), idx);
		}

		for (CP id = _E0(CP); id < CP::_; id++) {
			this->labels[id] = this->insert_label(id);
			this->ranges[id] = this->master->insert_one(new Credit<Dimensionlet, CP>(DimensionState::Input, this->depth_style, "meter"), id);
		}

		this->master->insert_one(this->plot);
	}

	void reflow(IGraphlet* frame, float width, float height, float inset) {
		float depth_width, depth_height, picker_width, picker_height;
		int sep = ColorPlotSize / 2;
		
		this->depths[0]->fill_extent(0.0F, 0.0F, &depth_width, &depth_height);
		this->pickers[0]->fill_extent(0.0F, 0.0F, &picker_width, &picker_height);

		this->reflow_depths_fields(frame, inset, 0, sep, inset, flmax(depth_height, picker_height));
		this->reflow_depths_fields(frame, inset * 4.0F + depth_width + picker_width, sep, ColorPlotSize, inset, flmax(depth_height, picker_height));

		this->master->move_to(this->plot, frame, GraphletAnchor::RT, GraphletAnchor::RT, -inset * 2.0F, inset * 2.0F);
		this->master->move_to(this->ranges[CP::min], this->plot, GraphletAnchor::RB, GraphletAnchor::RT, 0.0F, inset * 4.0F);
		this->master->move_to(this->ranges[CP::max], this->ranges[CP::min], GraphletAnchor::RB, GraphletAnchor::RT, 0.0F, inset * 0.5F);

		for (CP id = _E0(CP); id < CP::_; id++) {
			this->master->move_to(this->labels[id], this->ranges[id], GraphletAnchor::LC, GraphletAnchor::RC, -inset * 0.618F);
		}
	}

	void on_graphlet_ready(IGraphlet* g) {
		if (this->plot == g) {
			this->entity = this->plot->clone_plot(this->entity);
			this->refresh_preference_fields();
		}
	}

public:
	bool on_edit(Dimensionlet* dim) {
		long double new_value = dim->get_input_number();
		bool modified = (new_value != dim->get_value());

		if (modified) {
			dim->set_value(new_value);

			this->refresh_entity();
		}

		return modified;
	}

	bool on_apply() {
		this->refresh_entity(); // duplicate work
		this->plot->refresh(this->entity);

		return true;
	}

	bool on_reset() {
		if (this->plot != nullptr) {
			if (!this->master->up_to_date()) {
				this->entity = this->plot->clone_plot(this->entity);
				this->refresh_preference_fields();
			}
		}

		return true;
	}

	bool on_default() {
		if (this->entity == nullptr) {
			this->entity = ref new ColorPlot();
		}

		for (unsigned int idx = 0; idx < ColorPlotSize; idx++) {
			this->depths[idx]->set_value(double(idx + 1));
		}

		this->pickers[0]->color(Colours::make(255U, 255U, 128U));
		this->pickers[1]->color(Colours::make(255U, 255U, 64U));
		this->pickers[2]->color(Colours::make(255U, 255U, 17U));
		this->pickers[3]->color(Colours::make(251U, 251U, 0U));
		this->pickers[4]->color(Colours::make(232U, 232U, 0U));
		this->pickers[5]->color(Colours::make(221U, 221U, 0U));
		this->pickers[6]->color(Colours::make(198U, 198U, 0U));
		this->pickers[7]->color(Colours::make(145U, 145U, 0U));
		this->pickers[8]->color(Colours::make(115U, 115U, 0U));
		this->pickers[9]->color(Colours::make(128U, 255U, 128U));
		this->pickers[10]->color(Colours::make(85U, 255U, 85U));
		this->pickers[11]->color(Colours::make(80U, 182U, 92U));
		this->pickers[12]->color(Colours::make(60U, 255U, 60U));
		this->pickers[13]->color(Colours::make(2U, 255U, 2U));
		this->pickers[14]->color(Colours::make(0U, 206U, 0U));
		this->pickers[15]->color(Colours::make(0U, 168U, 0U));
		this->pickers[16]->color(Colours::make(0U, 121U, 0U));
		this->pickers[17]->color(Colours::make(198U, 138U, 128U));
		this->pickers[18]->color(Colours::make(124U, 68U, 44U));
		this->pickers[19]->color(Colours::make(98U, 49U, 49U));

		return true;
	}

public:
	IGraphlet* thumbnail() {
		return this->plot;
	}

private:
	void refresh_entity() {
		if (this->entity == nullptr) {
			this->entity = ref new ColorPlot();
		}

		for (unsigned int idx = 0; idx < ColorPlotSize; idx++) {
			this->entity->depths[idx] = this->depths[idx]->get_value();
			this->entity->colors[idx] = this->pickers[idx]->color();
			this->entity->enableds[idx] = true;
		}

		this->entity->min_depth = this->ranges[CP::min]->get_value();
		this->entity->max_depth = this->ranges[CP::max]->get_value();
	}

	void refresh_preference_fields() {
		if (this->entity != nullptr) {
			this->master->begin_update_sequence();

			for (unsigned int idx = 0; idx < ColorPlotSize; idx++) {
				this->depths[idx]->set_value(this->entity->depths[idx]);
				this->pickers[idx]->color(this->entity->colors[idx]);
			}

			this->ranges[CP::min]->set_value(this->entity->min_depth);
			this->ranges[CP::max]->set_value(this->entity->max_depth);
			
			this->master->notify_updated();
			this->master->end_update_sequence();
		}
	}

private:
	Labellet* insert_label(CP id) {
		Labellet* label = new Labellet(_speak(id), label_font, label_color);
		float lbl_width;

		label->fill_extent(0.0F, 0.0F, &lbl_width, nullptr);
		this->label_max_width = flmax(this->label_max_width, lbl_width);

		return this->master->insert_one(label);
	}

	void reflow_depths_fields(IGraphlet* frame, float xoff, int idx0, int idxp1, float gapsize, float pheight) {
		float hgap = gapsize * 0.618F;

		for (int idx = idx0; idx < idxp1; idx++) {
			float yrow = (pheight + gapsize) * float(idx - idx0);

			this->master->move_to(this->depths[idx], frame, GraphletAnchor::LT, GraphletAnchor::LT, xoff, yrow + gapsize * 2.0F);
			this->master->move_to(this->pickers[idx], this->depths[idx], GraphletAnchor::RC, GraphletAnchor::LC, gapsize, 0.0F);
		}
	}

private:
	float label_max_width;
	DimensionStyle depth_style;
	ColorPlot^ entity;

private: // never delete these graphlet manually
	ColorPlotlet* plot;
	std::map<CP, Labellet*> labels;
	std::map<CP, Credit<Dimensionlet, CP>*> ranges;
	Credit<Dimensionlet, int>* depths[ColorPlotSize];
	Credit<ColorPickerlet, int>* pickers[ColorPlotSize];
	
private:
	ColorPlotEditor* master;
};

/*************************************************************************************************/
ColorPlotEditor::ColorPlotEditor(Platform::String^ plot) : EditorPlanet(__MODULE__) {
	this->self = new ColorPlotSelf(this, plot);
}

ColorPlotEditor::~ColorPlotEditor() {
	delete this->self;
}

void ColorPlotEditor::load(CanvasCreateResourcesReason reason, float width, float height) {
	float bg_width, bg_height;

	EditorPlanet::load(reason, width, height);

	this->enable_default(true);
	this->background->fill_extent(0.0F, 0.0F, &bg_width, &bg_height);
	this->self->load(reason, bg_width, bg_height, (width - bg_width) * 0.5F);
}

void ColorPlotEditor::reflow(float width, float height) {
	float bg_width, bg_height;
	
	EditorPlanet::reflow(width, height);

	this->background->fill_extent(0.0F, 0.0F, &bg_width, &bg_height);
	this->self->reflow(this->background, width, height, (width - bg_width) * 0.5F);
}

void ColorPlotEditor::on_graphlet_ready(IGraphlet* g) {
	this->self->on_graphlet_ready(g);
}

IGraphlet* ColorPlotEditor::thumbnail_graphlet() {
	return this->self->thumbnail();
}

bool ColorPlotEditor::on_apply() {
	return this->self->on_apply();
}

bool ColorPlotEditor::on_reset() {
	return this->self->on_reset();
}

bool ColorPlotEditor::on_default() {
	return this->self->on_default();
}

bool ColorPlotEditor::on_edit(Dimensionlet* dim) {
	return this->self->on_edit(dim);
}
