#include <map>

#include "preference/transverse_section.hpp"

#include "graphlet/filesystem/project/sectionlet.hpp"
#include "graphlet/shapelet.hpp"
#include "graphlet/planetlet.hpp"

#include "datum/flonum.hpp"

#include "module.hpp"

using namespace WarGrey::SCADA;

using namespace Windows::Foundation;

using namespace Microsoft::Graphics::Canvas::UI;
using namespace Microsoft::Graphics::Canvas::Text;
using namespace Microsoft::Graphics::Canvas::Brushes;

static CanvasTextFormat^ label_font = make_bold_text_format("Microsoft Yahei", 16.0F);
static CanvasTextFormat^ sketch_number_font = make_bold_text_format("Consolas", 14.0F);
static CanvasTextFormat^ sketch_font = make_bold_text_format("Consolas", 8.0F);

static CanvasSolidColorBrush^ label_color = Colours::DarkGray;
static CanvasSolidColorBrush^ original_color = Colours::Khaki;
static CanvasSolidColorBrush^ designed_color = Colours::SkyBlue;
static CanvasSolidColorBrush^ axes_color = Colours::Salmon;
static CanvasSolidColorBrush^ water_color = Colours::SeaGreen;

#define Section_Display_Vertex(v, ref, ms, id) ms[id]->set_value(v->ref)
#define Section_Refresh_Vertex(v, ref, ms, id) v->ref = ms[id]->get_value()

/*************************************************************************************************/
namespace {
	// order matters
	private enum class TS {
		Width, MinDepth, MaxDepth,
		DepthDistance, DragHeadsDistance,

		_,

		// for sketch icon
		lb, rt, tide
	};

	private class SketchIcon : public Planet {
	public:
		SketchIcon() : Planet("transverse_section") {}

	public:
		void load(CanvasCreateResourcesReason reason, float width, float height) override {
			float stepsize = 36.0F;
			float thickness = 2.0F;

			this->decorates[TS::MinDepth] = this->insert_one(new ArrowHeadlet(8.0F, -90.0, axes_color));
			this->decorates[TS::MaxDepth] = this->insert_one(new ArrowHeadlet(8.0F, +90.0, axes_color));
			
			{ // load designed section sketch
				Turtle<TS>* s = new Turtle<TS>(stepsize, false, TS::lb);

				s->move_right()->jump_left(0.5F, TS::MaxDepth)->move_up(4.0F, TS::MinDepth)->jump_left(0.5F)->move_right(TS::rt);
				s->jump_right(3.0F)->move_left_down(1.0F, 2.0F)->move_left(1.5F)->move_down(2.0F)->move_left(2.0F)->move_left_up(2.5F, 4.0F);
				s->jump_left_up(0.5F, 2.0F, TS::tide)->move_right(8.0F, TS::Width)->jump_down(8.0F)->move_left(8.0F);

				this->designed_section = this->insert_one(new Tracklet(s, thickness, designed_color));
				this->designed_section->push_subtrack(TS::lb, TS::rt, axes_color);
				this->designed_section->push_subtrack(TS::tide, TS::Width, water_color);
			}

			{ // load original section sketch
				Turtle<TS>* s = new Turtle<TS>(stepsize, false);

				s->drift(8.0F, 0.0F, 3.0F, 0.5F, 6.0F, -0.5F);
				s->jump_down(5.0F)->move_left(8.0F);

				this->original_section = this->insert_one(new Tracklet(s, thickness, original_color));
			}
		}

		void reflow(float width, float height) override {
			this->designed_section->map_graphlet_at_anchor(this->decorates[TS::MinDepth], TS::MinDepth, GraphletAnchor::CT);
			this->designed_section->map_graphlet_at_anchor(this->decorates[TS::MaxDepth], TS::MaxDepth, GraphletAnchor::CB);
			this->move_to(this->original_section, this->designed_section, GraphletAnchor::CB, GraphletAnchor::CB);
		}

	private: // never delete these graphlet manually
		Tracklet<TS>* designed_section;
		Tracklet<TS>* original_section;
		std::map<TS, Shapelet*> decorates;
	};
}

private class WarGrey::SCADA::TransverseSectionSelf {
public:
	TransverseSectionSelf(TransverseSectionEditor* master, Platform::String^ section)
		: master(master), label_max_width(0.0F), section(section), entity(nullptr) {
		this->input_style = make_highlight_dimension_style(label_font->FontSize, 8U, 1U);
		this->input_style.unit_color = label_color;
	}

public:
	void load(CanvasCreateResourcesReason reason, float width, float height, float inset) {
		for (TS id = _E0(TS); id < TS::_; id++) {
			this->labels[id] = this->insert_label(id);
			this->metrics[id] = this->insert_input_field(id, 0.0);
		}
		
		this->transverse_section = new TransverseSectionlet(nullptr, this->section, width - inset * 4.0F);
		this->master->insert(this->transverse_section);

		this->sketch = this->master->insert_one(new Planetlet(new SketchIcon()));
	}

	void reflow(IGraphlet* frame, float width, float height, float inset) {
		float pwidth, pheight;

		this->metrics[TS::Width]->fill_extent(0.0F, 0.0F, &pwidth, &pheight);
		
		inset *= 2.0F;
		this->reflow_input_fields(frame, _E0(TS), TS::_, inset, pheight, TS::DepthDistance);
		
		this->master->move_to(this->sketch, frame, GraphletAnchor::RT, GraphletAnchor::RT, -inset, inset);
		this->master->move_to(this->transverse_section, frame, GraphletAnchor::CB, GraphletAnchor::CB, 0.0F, -inset);
	}

	void on_graphlet_ready(IGraphlet* g) {
		if (this->transverse_section == g) { // also see `this->load()`
			this->entity = this->transverse_section->clone_section(this->entity);
			this->refresh_input_fields();
		}
	}

public:
	bool on_edit(Credit<Dimensionlet, TS>* dim) {
		long double new_value = dim->get_input_number();
		bool modified = (new_value != dim->get_value());

		if (modified) {
			dim->set_value(new_value);

			this->refresh_entity();

			this->transverse_section->moor(GraphletAnchor::CB);
			this->transverse_section->preview(this->entity);
			this->transverse_section->clear_moor();
		}

		return (modified && (dim->id < TS::_));
	}

	bool on_apply() {
		this->refresh_entity(); // duplicate work
		this->transverse_section->refresh(this->entity);

		return true;
	}

	bool on_reset() {
		if (this->transverse_section != nullptr) {
			this->transverse_section->preview(nullptr);
			
			this->entity = this->transverse_section->clone_section(this->entity, true);
			this->refresh_input_fields();
		}

		return true;
	}

public:
	IGraphlet* thumbnail() {
		return this->sketch;
	}

private:
	void refresh_entity() {
		if (this->entity == nullptr) {
			this->entity = ref new TransverseSection();
		}

		Section_Refresh_Vertex(this->entity, width, this->metrics, TS::Width);
		Section_Refresh_Vertex(this->entity, max_depth, this->metrics, TS::MaxDepth);
		Section_Refresh_Vertex(this->entity, min_depth, this->metrics, TS::MinDepth);
		
		Section_Refresh_Vertex(this->entity, depth_distance, this->metrics, TS::DepthDistance);
		Section_Refresh_Vertex(this->entity, dragheads_distance, this->metrics, TS::DragHeadsDistance);
	}

	void refresh_input_fields() {
		if (this->entity != nullptr) {
			this->master->begin_update_sequence();

			Section_Display_Vertex(this->entity, width, this->metrics, TS::Width);
			Section_Display_Vertex(this->entity, max_depth, this->metrics, TS::MaxDepth);
			Section_Display_Vertex(this->entity, min_depth, this->metrics, TS::MinDepth);
			
			Section_Display_Vertex(this->entity, depth_distance, this->metrics, TS::DepthDistance);
			Section_Display_Vertex(this->entity, dragheads_distance, this->metrics, TS::DragHeadsDistance);
			
			this->master->end_update_sequence();
		}
	}

private:
	Labellet* insert_label(TS id) {
		Labellet* label = new Labellet(_speak(id), label_font, label_color);
		float lbl_width;

		label->fill_extent(0.0F, 0.0F, &lbl_width, nullptr);
		this->label_max_width = flmax(this->label_max_width, lbl_width);

		return this->master->insert_one(label);
	}
	
	Credit<Dimensionlet, TS>* insert_input_field(TS id, double v) {
		auto input = new Credit<Dimensionlet, TS>(DimensionState::Input, this->input_style, "meter");

		this->master->insert_one(input, id);
		input->set_value(v);

		return input;
	}

	void reflow_input_fields(IGraphlet* frame, TS id0, TS idp1, float gapsize, float pheight, TS sep) {
		float idx0 = _F(id0);
		float xoff =  this->label_max_width + gapsize * 1.618F;
		float yoff0 = gapsize;
		float hgap = gapsize * 0.618F;

		for (TS id = id0; id < idp1; id++) {
			float yrow = (pheight + gapsize * 0.5F) * (_F(id) - idx0);
			
			if (id == sep) { // add a blank before `sep`
				yoff0 *= 2.0F;
			}

			this->master->move_to(this->labels[id], frame, GraphletAnchor::LT, GraphletAnchor::RT, xoff, yoff0 + yrow);
			this->master->move_to(this->metrics[id], this->labels[id], GraphletAnchor::RC, GraphletAnchor::LC, hgap);
		}
	}

private:
	float label_max_width;
	DimensionStyle input_style;
	TransverseSection^ entity;
	Platform::String^ section;

private: // never delete these graphlet manually
	TransverseSectionlet* transverse_section;
	std::map<TS, Labellet*> labels;
	std::map<TS, Credit<Dimensionlet, TS>*> metrics;
	Planetlet* sketch;
	
private:
	TransverseSectionEditor* master;
};

/*************************************************************************************************/
TransverseSectionEditor::TransverseSectionEditor(Platform::String^ section) : EditorPlanet(__MODULE__) {
	this->self = new TransverseSectionSelf(this, section);
}

TransverseSectionEditor::~TransverseSectionEditor() {
	delete this->self;
}

void TransverseSectionEditor::load(CanvasCreateResourcesReason reason, float width, float height) {
	float bg_width, bg_height;

	EditorPlanet::load(reason, width, height);

	this->background->fill_extent(0.0F, 0.0F, &bg_width, &bg_height);
	this->self->load(reason, bg_width, bg_height, (width - bg_width) * 0.5F);
}

void TransverseSectionEditor::reflow(float width, float height) {
	float bg_width, bg_height;
	
	EditorPlanet::reflow(width, height);

	this->background->fill_extent(0.0F, 0.0F, &bg_width, &bg_height);
	this->self->reflow(this->background, width, height, (width - bg_width) * 0.5F);
}

void TransverseSectionEditor::on_graphlet_ready(IGraphlet* g) {
	this->self->on_graphlet_ready(g);
}

IGraphlet* TransverseSectionEditor::thumbnail_graphlet() {
	return this->self->thumbnail();
}

bool TransverseSectionEditor::on_apply() {
	return this->self->on_apply();
}

bool TransverseSectionEditor::on_reset() {
	return this->self->on_reset();
}

bool TransverseSectionEditor::on_edit(Dimensionlet* dim) {
	return this->self->on_edit(static_cast<Credit<Dimensionlet, TS>*>(dim));
}
