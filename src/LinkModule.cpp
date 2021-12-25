#include "plugin.hpp"
#include <atomic>

struct LinkModule : Module
{

public:
	enum ParamId
	{
		PARAMS_LEN
	};
	enum InputId
	{
		INPUTS_LEN
	};
	enum OutputId
	{
		START_OUTPUT,
		RESET_OUTPUT,
		CLOCK_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId
	{
		BEAT_LIGHT,
		LIGHTS_LEN
	};

	LinkModule()
	{
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configOutput(START_OUTPUT, "");
		configOutput(RESET_OUTPUT, "");
		configOutput(CLOCK_OUTPUT, "");

		g_link = new ableton::Link(120.0);
		g_link->enable(true);
		g_link->enableStartStopSync(true);
	}

	~LinkModule()
	{
		g_link->enable(false);
		delete g_link;
	}

	void process(const ProcessArgs &args) override
	{
		const auto time = g_link->clock().micros();
		const auto timeline = g_link->captureAppSessionState();
		const auto phase = timeline.phaseAtTime(time, 4.0);
		const auto beats = timeline.beatAtTime(time, 4.0);

		float ledState = (fmodf(phase, 1.0) < 0.1 ? 1.0 : 0.0);
		float clockState = (fmodf(phase, .25) < 0.1 ? 1.0 : 0.0);
		float resetState = (fmodf(phase, .25 * 32) < 0.1 ? 1.0 : 0.0);

		if (timeline.isPlaying() && beats >= 0)
		{
			outputs[CLOCK_OUTPUT].setVoltage(clockState * 10.0);
			lights[BEAT_LIGHT].setBrightness(ledState);
		}

		outputs[START_OUTPUT].setVoltage(timeline.isPlaying() ? 10.0 : 0.0);

		if (timeline.isPlaying())
		{
			outputs[RESET_OUTPUT].setVoltage(resetState * 10.0);
		}
	}

private:
	ableton::Link *g_link = nullptr;
	std::atomic<bool> g_link_initialized;
};

struct LinkModuleWidget : ModuleWidget
{
	LinkModuleWidget(LinkModule *module)
	{
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/LinkModule.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(15.24, 75.813)), module, LinkModule::START_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(15.24, 91.673)), module, LinkModule::RESET_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(15.24, 108.713)), module, LinkModule::CLOCK_OUTPUT));

		addChild(createLightCentered<MediumLight<GreenLight>>(mm2px(Vec(15.24, 25.81)), module, LinkModule::BEAT_LIGHT));
	}
};

Model *modelLinkModule = createModel<LinkModule, LinkModuleWidget>("LinkModule");