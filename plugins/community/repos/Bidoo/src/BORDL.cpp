#include "global_pre.hpp"
#include "Bidoo.hpp"
#include "dsp/digital.hpp"
#include "BidooComponents.hpp"
#include "global_ui.hpp"
#include <ctime>
#include <iostream>
#include <vector>
#include <random>
#include <algorithm>

using namespace std;

namespace rack_plugin_Bidoo {

struct StepExtended {
	int index = 0;
	int number = 0;
	bool skip = false;
	bool skipParam = false;
	bool slide = false;
	int pulses = 1;
	int pulsesParam = 1;
	float pitch = 3.0f;
	int type = 2;
	float gateProb = 1.0f;
	float pitchRnd = 0.0f;
	float accent = 0.0f;
	float accentRnd = 0.0f;
};

struct PatternExtended {
	int playMode = 0;
	int countMode = 0;
	int numberOfSteps = 8;
	int numberOfStepsParam = 8;
	int rootNote = 0;
	int rootNoteParam = 0;
	int scale = 0;
	int scaleParam = 0;
	float gateTime = 0.5f;
	float slideTime = 0.2f;
	float sensitivity = 1.0f;
	int currentStep = 0;
	int currentPulse = 0;
	bool forward = true;
	std::vector<StepExtended> steps {16};

	void Update(int playMode, int countMode, int numberOfSteps, int numberOfStepsParam, int rootNote, int scale,
		 float gateTime, float slideTime, float sensitivity, std::vector<char> skips, std::vector<char> slides,
		  Param *pulses, Param *pitches, Param *types, Param *probGates,
			Param *rndPitches, Param *accents, Param *rndAccents)
			{
		this->playMode = playMode;
		this->countMode = countMode;
		this->numberOfSteps = numberOfSteps;
		this->numberOfStepsParam = numberOfStepsParam;
		this->rootNote = rootNote;
		this->scale = scale;
		this->rootNoteParam = rootNote;
		this->scaleParam = scale;
		this->gateTime = gateTime;
		this->slideTime = slideTime;
		this->sensitivity = sensitivity;
		int pCount = 0;
		for (int i = 0; i < 16; i++) {
			steps[i].index = i%8;
			steps[i].number = i;
			if (((countMode == 0) && (i < numberOfSteps)) || ((countMode == 1) && (pCount < numberOfSteps))) {
				steps[i].skip = (skips[steps[i].index] == 't');
			}	else {
				steps[i].skip = true;
			}
			steps[i].skipParam = (skips[steps[i].index] == 't');
			steps[i].slide = (slides[steps[i].index] == 't');
			if ((countMode == 1) && ((pCount + (int)pulses[steps[i].index].value) >= numberOfSteps)) {
				steps[i].pulses = max(numberOfSteps - pCount, 0);
			}	else {
				steps[i].pulses = (int)(pulses + steps[i].index)->value;
			}
			steps[i].pulsesParam = (int)(pulses + steps[i].index)->value;
			steps[i].pitch = (pitches + steps[i].index)->value;
			steps[i].type = (int)(types + steps[i].index)->value;
			steps[i].gateProb = (probGates + steps[i].index)->value;
			steps[i].pitchRnd = (rndPitches + steps[i].index)->value;
			steps[i].accent = (accents + steps[i].index)->value;
			steps[i].accentRnd = (rndAccents + steps[i].index)->value;

			pCount = pCount + steps[i].pulses;
		}
	}

	std::tuple<int, int> GetNextStep(bool reset)
	{
		if (reset) {
			if (playMode != 1) {
				currentStep = GetFirstStep();
			} else {
				currentStep = GetLastStep();
			}
			currentPulse = 0;
			return std::make_tuple(currentStep,currentPulse);
		} else {
			if (currentPulse < steps[currentStep].pulses - 1) {
				currentPulse++;
				return std::make_tuple(steps[currentStep%16].index,currentPulse);
			} else {
				if (playMode == 0) {
					currentStep = GetNextStepForward(currentStep);
					currentPulse = 0;
					return std::make_tuple(steps[currentStep%16].index,currentPulse);
				} else if (playMode == 1) {
					currentStep = GetNextStepBackward(currentStep);
					currentPulse = 0;
					return std::make_tuple(steps[currentStep%16].index,currentPulse);
				} else if (playMode == 2) {
					if (currentStep == GetLastStep()) {
						forward = false;
					}
					if (currentStep == GetFirstStep()) {
						forward = true;
					}
					if (forward) {
						currentStep = GetNextStepForward(currentStep);
					} else {
						currentStep = GetNextStepBackward(currentStep);
					}
					currentPulse = 0;
					return std::make_tuple(steps[currentStep%16].index,currentPulse);
				} else if (playMode == 3) {
					std::vector<StepExtended> tmp (steps.size());
				  auto it = std::copy_if (steps.begin(), steps.end(), tmp.begin(), [](StepExtended i){return !(i.skip);} );
				  tmp.resize(std::distance(tmp.begin(),it));  // shrink container to new size
					StepExtended tmpStep = *select_randomly(tmp.begin(), tmp.end());
					currentPulse = 0;
					currentStep = tmpStep.number;
					return std::make_tuple(steps[currentStep%16].index,currentPulse);
				} else if (playMode == 4) {
					int next = GetNextStepForward(currentStep);
					int prev = GetNextStepBackward(currentStep);
					vector<StepExtended> subPattern;
					subPattern.push_back(steps[prev]);
					subPattern.push_back(steps[next]);
					StepExtended choice = *select_randomly(subPattern.begin(), subPattern.end());
					currentPulse = 0;
					currentStep = choice.number;
					return std::make_tuple(steps[currentStep%16].index,currentPulse);
				} else {
					return std::make_tuple(0,0);
				}
			}
		}
	}

	inline StepExtended CurrentStep() {
		return this->steps[currentStep];
	}

	inline int GetFirstStep()
	{
			for (int i = 0; i < 16; i++) {
				if (!steps[i].skip) {
					return i;
				}
			}
			return 0;
	}

	inline int GetLastStep()
	{
			for (int i = 15; i >= 0  ; i--) {
				if (!steps[i].skip) {
					return i;
				}
			}
			return 15;
	}

	inline int GetNextStepForward(int pos)
	{
			for (int i = pos + 1; i < pos + 16; i++) {
				if (!steps[i%16].skip) {
					return i%16;
				}
			}
			return pos;
	}

	inline int GetNextStepBackward(int pos)
	{
			for (int i = pos - 1; i > pos - 16; i--) {
				int j = i%16;
				if (!steps[j + (i<0?16:0)].skip) {
					return j + (i<0?16:0);
				}
			}
			return pos;
	}

	template<typename Iter, typename RandomGenerator> Iter select_randomly(Iter start, Iter end, RandomGenerator& g) {
		std::uniform_int_distribution<> dis(0, std::distance(start, end) - 1);
		std::advance(start, dis(g));
		return start;
	}

	template<typename Iter> Iter select_randomly(Iter start, Iter end) {
		static std::random_device rd;
		static std::mt19937 gen(rd());
		return select_randomly(start, end, gen);
	}

	inline float MinVoltage() {
		float result = 0.0f;
		for (int i = 0; i < 16; i++) {
			if (steps[i].pitch<result) {
				result = steps[i].pitch;
			}
		}
		return result;
	}

	inline float MaxVoltage() {
		float result = 0.0f;
		for (int i = 0; i < 16; i++) {
			if (steps[i].pitch>result) {
				result = steps[i].pitch;
			}
		}
		return result;
	}

};

struct BORDL : Module {
	enum ParamIds {
		CLOCK_PARAM,
		RUN_PARAM,
		RESET_PARAM,
		STEPS_PARAM,
		SLIDE_TIME_PARAM,
		GATE_TIME_PARAM,
		ROOT_NOTE_PARAM,
		SCALE_PARAM,
		PLAY_MODE_PARAM,
		COUNT_MODE_PARAM,
		PATTERN_PARAM,
		SENSITIVITY_PARAM,
		TRIG_COUNT_PARAM = SENSITIVITY_PARAM + 8,
		TRIG_TYPE_PARAM = TRIG_COUNT_PARAM + 8,
		TRIG_PITCH_PARAM = TRIG_TYPE_PARAM + 8,
		TRIG_SLIDE_PARAM = TRIG_PITCH_PARAM + 8,
		TRIG_SKIP_PARAM = TRIG_SLIDE_PARAM + 8,
		TRIG_GATEPROB_PARAM = TRIG_SKIP_PARAM + 8,
		TRIG_PITCHRND_PARAM = TRIG_GATEPROB_PARAM + 8,
		TRIG_ACCENT_PARAM = TRIG_PITCHRND_PARAM + 8,
		TRIG_RNDACCENT_PARAM = TRIG_ACCENT_PARAM + 8,
		LEFT_PARAM = TRIG_RNDACCENT_PARAM + 8,
		RIGHT_PARAM,
		UP_PARAM,
		DOWN_PARAM,
		COPY_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		CLOCK_INPUT,
		EXT_CLOCK_INPUT,
		RESET_INPUT,
		STEPS_INPUT,
		SLIDE_TIME_INPUT,
		GATE_TIME_INPUT,
		ROOT_NOTE_INPUT,
		SCALE_INPUT,
		EXTGATE1_INPUT,
		EXTGATE2_INPUT,
		PATTERN_INPUT,
		TRANSPOSE_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		GATE_OUTPUT,
		PITCH_OUTPUT,
		ACC_OUTPUT,
		STEP_OUTPUT,
		NUM_OUTPUTS = STEP_OUTPUT + 8
	};
	enum LightIds {
		RUNNING_LIGHT,
		RESET_LIGHT,
		GATE_LIGHT,
		STEPS_LIGHTS = GATE_LIGHT + 8,
		SLIDES_LIGHTS = STEPS_LIGHTS + 8,
		SKIPS_LIGHTS = SLIDES_LIGHTS + 8,
		COPY_LIGHT = SKIPS_LIGHTS + 8,
		NUM_LIGHTS
	};

	//copied from http://www.grantmuller.com/MidiReference/doc/midiReference/ScaleReference.html
	int SCALE_AEOLIAN        [7] = {0, 2, 3, 5, 7, 8, 10};
	int SCALE_BLUES          [9] = {0, 2, 3, 4, 5, 7, 9, 10, 11};
	int SCALE_CHROMATIC      [12]= {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
	int SCALE_DIATONIC_MINOR [7] = {0, 2, 3, 5, 7, 8, 10};
	int SCALE_DORIAN         [7] = {0, 2, 3, 5, 7, 9, 10};
	int SCALE_HARMONIC_MINOR [7] = {0, 2, 3, 5, 7, 8, 11};
	int SCALE_INDIAN         [7] = {0, 1, 1, 4, 5, 8, 10};
	int SCALE_LOCRIAN        [7] = {0, 1, 3, 5, 6, 8, 10};
	int SCALE_LYDIAN         [7] = {0, 2, 4, 6, 7, 9, 10};
	int SCALE_MAJOR          [7] = {0, 2, 4, 5, 7, 9, 11};
	int SCALE_MELODIC_MINOR  [9] = {0, 2, 3, 5, 7, 8, 9, 10, 11};
	int SCALE_MINOR          [7] = {0, 2, 3, 5, 7, 8, 10};
	int SCALE_MIXOLYDIAN     [7] = {0, 2, 4, 5, 7, 9, 10};
	int SCALE_NATURAL_MINOR  [7] = {0, 2, 3, 5, 7, 8, 10};
	int SCALE_PENTATONIC     [5] = {0, 2, 4, 7, 9};
	int SCALE_PHRYGIAN       [7] = {0, 1, 3, 5, 7, 8, 10};
	int SCALE_TURKISH        [7] = {0, 1, 3, 5, 7, 10, 11};

	enum Notes {
		NOTE_C,
		NOTE_C_SHARP,
		NOTE_D,
		NOTE_D_SHARP,
		NOTE_E,
		NOTE_F,
		NOTE_F_SHARP,
		NOTE_G,
		NOTE_G_SHARP,
		NOTE_A,
		NOTE_A_SHARP,
		NOTE_B,
		NUM_NOTES
	};

	enum Scales {
		AEOLIAN,
		BLUES,
		CHROMATIC,
		DIATONIC_MINOR,
		DORIAN,
		HARMONIC_MINOR,
		INDIAN,
		LOCRIAN,
		LYDIAN,
		MAJOR,
		MELODIC_MINOR,
		MINOR,
		MIXOLYDIAN,
		NATURAL_MINOR,
		PENTATONIC,
		PHRYGIAN,
		TURKISH,
		NONE,
		NUM_SCALES
	};

	bool running = true;
	SchmittTrigger clockTrigger;
	SchmittTrigger runningTrigger;
	SchmittTrigger resetTrigger;
	SchmittTrigger slideTriggers[8];
	SchmittTrigger skipTriggers[8];
	SchmittTrigger playModeTrigger;
	SchmittTrigger countModeTrigger;
	float phase = 0.0f;
	int index = 0;
	int prevIndex = 0;
	bool reStart = true;
	int pulse = 0;
	int curScaleVal = 0;
	float pitch = 0.0f;
	float previousPitch = 0.0f;
	float candidateForPreviousPitch = 0.0f;
	float tCurrent;
	float tLastTrig;
	std::vector<char> slideState = {'f','f','f','f','f','f','f','f'};
	std::vector<char> skipState = {'f','f','f','f','f','f','f','f'};
	int playMode = 0; // 0 forward, 1 backward, 2 pingpong, 3 random, 4 brownian
	int countMode = 0; // 0 steps, 1 pulses
	int numSteps = 8;
	int selectedPattern = 0;
	int playedPattern = 0;
	bool pitchMode = false;
	bool updateFlag = false;
	bool probGate = true;
	float rndPitch = 0.0f;
	float accent = 5.0f;
	bool loadedFromJson = false;
	int copyPattern = -1;
	PulseGenerator stepPulse[8];
	bool stepOutputsMode = false;
	bool gateOn = false;
	const float invLightLambda = 13.333333333333333333333f;
	bool copyState = false;

	PatternExtended patterns[16];

	BORDL() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {
	}

	void UpdatePattern();

	void step() override;

	// persistence, random & init

	json_t *toJson() override {
		json_t *rootJ = json_object();

		json_object_set_new(rootJ, "running", json_boolean(running));
		json_object_set_new(rootJ, "playMode", json_integer(playMode));
		json_object_set_new(rootJ, "countMode", json_integer(countMode));
		json_object_set_new(rootJ, "stepOutputsMode", json_boolean(stepOutputsMode));
		json_object_set_new(rootJ, "selectedPattern", json_integer(selectedPattern));
		json_object_set_new(rootJ, "playedPattern", json_integer(playedPattern));

		json_t *trigsJ = json_array();
		for (int i = 0; i < 8; i++) {
			json_t *trigJ = json_array();
			json_t *trigJSlide = json_boolean(slideState[i] == 't');
			json_array_append_new(trigJ, trigJSlide);
			json_t *trigJSkip = json_boolean(skipState[i] == 't');
			json_array_append_new(trigJ, trigJSkip);
			json_array_append_new(trigsJ, trigJ);
		}
		json_object_set_new(rootJ, "trigs", trigsJ);

		for (int i = 0; i<16; i++) {
			json_t *patternJ = json_object();
			json_object_set_new(patternJ, "playMode", json_integer(patterns[i].playMode));
			json_object_set_new(patternJ, "countMode", json_integer(patterns[i].countMode));
			json_object_set_new(patternJ, "numSteps", json_integer(patterns[i].numberOfStepsParam));
			json_object_set_new(patternJ, "rootNote", json_integer(patterns[i].rootNoteParam));
			json_object_set_new(patternJ, "scale", json_integer(patterns[i].scaleParam));
			json_object_set_new(patternJ, "gateTime", json_real(patterns[i].gateTime));
			json_object_set_new(patternJ, "slideTime", json_real(patterns[i].slideTime));
			json_object_set_new(patternJ, "sensitivity", json_real(patterns[i].sensitivity));
			for (int j = 0; j < 16; j++) {
				json_t *stepJ = json_object();
				json_object_set_new(stepJ, "index", json_integer(patterns[i].steps[j].index));
				json_object_set_new(stepJ, "number", json_integer(patterns[i].steps[j].number));
				json_object_set_new(stepJ, "skip", json_integer((int)patterns[i].steps[j].skip));
				json_object_set_new(stepJ, "skipParam", json_integer((int)patterns[i].steps[j].skipParam));
				json_object_set_new(stepJ, "slide", json_integer((int)patterns[i].steps[j].slide));
				json_object_set_new(stepJ, "pulses", json_integer(patterns[i].steps[j].pulses));
				json_object_set_new(stepJ, "pulsesParam", json_integer(patterns[i].steps[j].pulsesParam));
				json_object_set_new(stepJ, "pitch", json_real(patterns[i].steps[j].pitch));
				json_object_set_new(stepJ, "type", json_integer(patterns[i].steps[j].type));
				json_object_set_new(stepJ, "gateProb", json_real(patterns[i].steps[j].gateProb));
				json_object_set_new(stepJ, "pitchRnd", json_real(patterns[i].steps[j].pitchRnd));
				json_object_set_new(stepJ, "accent", json_real(patterns[i].steps[j].accent));
				json_object_set_new(stepJ, "accentRnd", json_real(patterns[i].steps[j].accentRnd));
				json_object_set_new(patternJ, ("step" + to_string(j)).c_str() , stepJ);
			}
			json_object_set_new(rootJ, ("pattern" + to_string(i)).c_str(), patternJ);
		}
		return rootJ;
	}

	void fromJson(json_t *rootJ) override {
		json_t *runningJ = json_object_get(rootJ, "running");
		if (runningJ)
			running = json_is_true(runningJ);
		json_t *playModeJ = json_object_get(rootJ, "playMode");
		if (playModeJ)
			playMode = json_integer_value(playModeJ);
		json_t *countModeJ = json_object_get(rootJ, "countMode");
		if (countModeJ)
			countMode = json_integer_value(countModeJ);
		json_t *selectedPatternJ = json_object_get(rootJ, "selectedPattern");
		if (selectedPatternJ)
			selectedPattern = json_integer_value(selectedPatternJ);
		json_t *playedPatternJ = json_object_get(rootJ, "playedPattern");
		if (playedPatternJ)
			playedPattern = json_integer_value(playedPatternJ);
		json_t *stepOutputsModeJ = json_object_get(rootJ, "stepOutputsMode");
		if (stepOutputsModeJ)
			stepOutputsMode = json_is_true(stepOutputsModeJ);
		json_t *trigsJ = json_object_get(rootJ, "trigs");
		if (trigsJ) {
			for (int i = 0; i < 8; i++) {
				json_t *trigJ = json_array_get(trigsJ, i);
				if (trigJ)
				{
					slideState[i] = json_is_true(json_array_get(trigJ, 0)) ? 't' : 'f';
					skipState[i] = json_is_true(json_array_get(trigJ, 1)) ? 't' : 'f';
				}
			}
		}
		for (int i=0; i<16;i++) {
			json_t *patternJ = json_object_get(rootJ, ("pattern" + to_string(i)).c_str());
			if (patternJ){
				json_t *playModeJ = json_object_get(patternJ, "playMode");
				if (playModeJ)
					patterns[i].playMode = json_integer_value(playModeJ);
				json_t *countModeJ = json_object_get(patternJ, "countMode");
				if (countModeJ)
					patterns[i].countMode = json_integer_value(countModeJ);
				json_t *numStepsJ = json_object_get(patternJ, "numSteps");
				if (numStepsJ)
					patterns[i].numberOfStepsParam = json_integer_value(numStepsJ);
				json_t *rootNoteJ = json_object_get(patternJ, "rootNote");
				if (rootNoteJ)
					patterns[i].rootNote = json_integer_value(rootNoteJ);
				json_t *scaleJ = json_object_get(patternJ, "scale");
				if (scaleJ)
					patterns[i].scale = json_integer_value(scaleJ);
				json_t *gateTimeJ = json_object_get(patternJ, "gateTime");
				if (gateTimeJ)
					patterns[i].gateTime = json_number_value(gateTimeJ);
				json_t *slideTimeJ = json_object_get(patternJ, "slideTime");
				if (slideTimeJ)
					patterns[i].slideTime = json_number_value(slideTimeJ);
				json_t *sensitivityJ = json_object_get(patternJ, "sensitivity");
				if (sensitivityJ)
					patterns[i].sensitivity = json_number_value(sensitivityJ);
				for (int j = 0; j < 16; j++) {
					json_t *stepJ = json_object_get(patternJ, ("step" + to_string(j)).c_str());
					if (stepJ) {
						json_t *indexJ= json_object_get(stepJ, "index");
						if (indexJ)
							patterns[i].steps[j].index = json_integer_value(indexJ);
						json_t *numberJ= json_object_get(stepJ, "numer");
						if (numberJ)
							patterns[i].steps[j].number = json_integer_value(numberJ);
						json_t *skipJ= json_object_get(stepJ, "skip");
						if (skipJ)
							patterns[i].steps[j].skip = !!json_integer_value(skipJ);
						json_t *skipParamJ= json_object_get(stepJ, "skipParam");
						if (skipParamJ)
							patterns[i].steps[j].skipParam = !!json_integer_value(skipParamJ);
						json_t *slideJ= json_object_get(stepJ, "slide");
						if (slideJ)
							patterns[i].steps[j].slide = !!json_integer_value(slideJ);
						json_t *pulsesJ= json_object_get(stepJ, "pulses");
						if (pulsesJ)
							patterns[i].steps[j].pulses = json_integer_value(pulsesJ);
						json_t *pulsesParamJ= json_object_get(stepJ, "pulsesParam");
						if (pulsesParamJ)
							patterns[i].steps[j].pulsesParam = json_integer_value(pulsesParamJ);
						json_t *pitchJ= json_object_get(stepJ, "pitch");
						if (pitchJ)
							patterns[i].steps[j].pitch = json_number_value(pitchJ);
						json_t *typeJ= json_object_get(stepJ, "type");
						if (typeJ)
							patterns[i].steps[j].type = json_integer_value(typeJ);
						json_t *probGateJ= json_object_get(stepJ, "gateProb");
						if (probGateJ)
							patterns[i].steps[j].gateProb = json_number_value(probGateJ);
						json_t *rndPitchJ= json_object_get(stepJ, "pitchRnd");
						if (rndPitchJ)
							patterns[i].steps[j].pitchRnd = json_number_value(rndPitchJ);
						json_t *accentJ= json_object_get(stepJ, "accent");
						if (accentJ)
							patterns[i].steps[j].accent = json_number_value(accentJ);
						json_t *rndAccentJ= json_object_get(stepJ, "accentRnd");
						if (rndAccentJ)
							patterns[i].steps[j].accentRnd = json_number_value(rndAccentJ);
					}
				}
			}
		}
		updateFlag = true;
		loadedFromJson = true;
	}

	void randomize() override {
		randomizeSlidesSkips();
	}

	void randomizeSlidesSkips() {
		for (int i = 0; i < 8; i++) {
			slideState[i] = (randomUniform() > 0.8f) ? 't' : 'f';
			skipState[i] = (randomUniform() > 0.85f) ? 't' : 'f';
		}
	}

	void reset() override {
		for (int i = 0; i < 8; i++) {
			slideState[i] = 'f';
			skipState[i] = 'f';
		}
	}

	// Quantization inspired from  https://github.com/jeremywen/JW-Modules

	float closestVoltageInScale(float voltsIn, int rootNote, float scaleVal){
		curScaleVal = scaleVal;
		int *curScaleArr;
		int notesInScale = 0;
		switch(curScaleVal){
			case AEOLIAN:        curScaleArr = SCALE_AEOLIAN;       notesInScale=LENGTHOF(SCALE_AEOLIAN); break;
			case BLUES:          curScaleArr = SCALE_BLUES;         notesInScale=LENGTHOF(SCALE_BLUES); break;
			case CHROMATIC:      curScaleArr = SCALE_CHROMATIC;     notesInScale=LENGTHOF(SCALE_CHROMATIC); break;
			case DIATONIC_MINOR: curScaleArr = SCALE_DIATONIC_MINOR;notesInScale=LENGTHOF(SCALE_DIATONIC_MINOR); break;
			case DORIAN:         curScaleArr = SCALE_DORIAN;        notesInScale=LENGTHOF(SCALE_DORIAN); break;
			case HARMONIC_MINOR: curScaleArr = SCALE_HARMONIC_MINOR;notesInScale=LENGTHOF(SCALE_HARMONIC_MINOR); break;
			case INDIAN:         curScaleArr = SCALE_INDIAN;        notesInScale=LENGTHOF(SCALE_INDIAN); break;
			case LOCRIAN:        curScaleArr = SCALE_LOCRIAN;       notesInScale=LENGTHOF(SCALE_LOCRIAN); break;
			case LYDIAN:         curScaleArr = SCALE_LYDIAN;        notesInScale=LENGTHOF(SCALE_LYDIAN); break;
			case MAJOR:          curScaleArr = SCALE_MAJOR;         notesInScale=LENGTHOF(SCALE_MAJOR); break;
			case MELODIC_MINOR:  curScaleArr = SCALE_MELODIC_MINOR; notesInScale=LENGTHOF(SCALE_MELODIC_MINOR); break;
			case MINOR:          curScaleArr = SCALE_MINOR;         notesInScale=LENGTHOF(SCALE_MINOR); break;
			case MIXOLYDIAN:     curScaleArr = SCALE_MIXOLYDIAN;    notesInScale=LENGTHOF(SCALE_MIXOLYDIAN); break;
			case NATURAL_MINOR:  curScaleArr = SCALE_NATURAL_MINOR; notesInScale=LENGTHOF(SCALE_NATURAL_MINOR); break;
			case PENTATONIC:     curScaleArr = SCALE_PENTATONIC;    notesInScale=LENGTHOF(SCALE_PENTATONIC); break;
			case PHRYGIAN:       curScaleArr = SCALE_PHRYGIAN;      notesInScale=LENGTHOF(SCALE_PHRYGIAN); break;
			case TURKISH:        curScaleArr = SCALE_TURKISH;       notesInScale=LENGTHOF(SCALE_TURKISH); break;
			case NONE:           return voltsIn;
		}
		float closestVal = 0.0f;
		float closestDist = 1.0f;
		int octaveInVolts = sgn(voltsIn) == 1.0f ? int(voltsIn) : (int(voltsIn)-1);
		for (int i = 0; i < notesInScale; i++) {
			float scaleNoteInVolts = octaveInVolts + curScaleArr[i] / 12.0f;
			float distAway = fabs(voltsIn - scaleNoteInVolts);
			if(distAway < closestDist) {
				closestVal = scaleNoteInVolts;
				closestDist = distAway;
			}
		}
		float transposeVoltage = inputs[TRANSPOSE_INPUT].active ? ((((int)rescale(clamp(inputs[TRANSPOSE_INPUT].value,-10.0f,10.0f),-10.0f,10.0f,-48.0f,48.0f)) / 12.0f)) : 0.0f;
		return clamp(closestVal + (rootNote / 12.0f) + transposeVoltage,-4.0f,6.0f);
	}
};

void BORDL::UpdatePattern() {
	patterns[selectedPattern].Update(playMode, countMode, numSteps, roundf(params[STEPS_PARAM].value), roundf(params[ROOT_NOTE_PARAM].value),
	 roundf(params[SCALE_PARAM].value), params[GATE_TIME_PARAM].value, params[SLIDE_TIME_PARAM].value, params[SENSITIVITY_PARAM].value ,
		skipState, slideState, &params[TRIG_COUNT_PARAM], &params[TRIG_PITCH_PARAM], &params[TRIG_TYPE_PARAM], &params[TRIG_GATEPROB_PARAM],
		 &params[TRIG_PITCHRND_PARAM], &params[TRIG_ACCENT_PARAM], &params[TRIG_RNDACCENT_PARAM]);
}

void BORDL::step() {
 	//const float lightLambda = 0.075f;

	float invESR = 1 / engineGetSampleRate();
	// Run
	if (runningTrigger.process(params[RUN_PARAM].value)) {
		running = !running;
	}
	lights[RUNNING_LIGHT].value = running ? 1.0f : 0.0f;
	bool nextStep = false;
	// Phase calculation
	if (running) {
		if (inputs[EXT_CLOCK_INPUT].active) {
			tCurrent += invESR;
			float clockTime = powf(2.0f, params[CLOCK_PARAM].value + inputs[CLOCK_INPUT].value);
			if (tLastTrig > 0.0f) {
				phase = tCurrent / tLastTrig;
			}
			else {
				phase += clockTime * invESR;
			}
			// External clock
			if (clockTrigger.process(inputs[EXT_CLOCK_INPUT].value)) {
				tLastTrig = tCurrent;
				tCurrent = 0.0f;
				phase = 0.0f;
				nextStep = true;
			}
		}
		else {
			// Internal clock
			float clockTime = powf(2.0f, params[CLOCK_PARAM].value + inputs[CLOCK_INPUT].value);
			phase += clockTime * invESR;
			if (phase >= 1.0f) {
				phase--;
				nextStep = true;
			}
		}
	}
	// Reset
	if (resetTrigger.process(params[RESET_PARAM].value + inputs[RESET_INPUT].value)) {
		phase = 0.0f;
		reStart = true;
		nextStep = true;
		lights[RESET_LIGHT].value = 1.0f;
	}

	//copy/paste
	lights[COPY_LIGHT].value = copyState ? 1.0f : 0.0f;

	//patternNumber
	int newPattern = clamp((inputs[PATTERN_INPUT].active ? (int)(rescale(inputs[PATTERN_INPUT].value,0.0f,10.0f,1.0f,16.1f)) : (int)(params[PATTERN_PARAM].value)) - 1, 0, 15);
	if (newPattern != playedPattern) {
		int cStep = patterns[playedPattern].currentStep;
		int cPulse = patterns[playedPattern].currentPulse;
		playedPattern = newPattern;
		patterns[playedPattern].currentStep = cStep;
		patterns[playedPattern].currentPulse = cPulse;
	}

	// Update Pattern
	if ((updateFlag) || (!loadedFromJson)) {
		// Trigs Update
		for (int i = 0; i < 8; i++) {
			if (slideTriggers[i].process(params[TRIG_SLIDE_PARAM + i].value)) {
				slideState[i] = slideState[i] == 't' ? 'f' : 't';
			}
			if (skipTriggers[i].process(params[TRIG_SKIP_PARAM + i].value)) {
				skipState[i] = skipState[i] == 't' ? 'f' : 't';
			}
		}
		// playMode
		if (playModeTrigger.process(params[PLAY_MODE_PARAM].value)) {
			playMode = (((int)playMode + 1) % 5);
		}
		// countMode
		if (countModeTrigger.process(params[COUNT_MODE_PARAM].value)) {
			countMode = (((int)countMode + 1) % 2);
		}
		// numSteps
		numSteps = clamp(roundf(params[STEPS_PARAM].value + inputs[STEPS_INPUT].value), 1.0f, 16.0f);
		UpdatePattern();
		if (!loadedFromJson) {
			loadedFromJson = true;
			updateFlag = true;
		}
	}

	// Steps && Pulses Management
	if (nextStep) {
		// Advance step
		candidateForPreviousPitch = closestVoltageInScale(patterns[playedPattern].CurrentStep().pitch * patterns[playedPattern].sensitivity, clamp(patterns[playedPattern].rootNote + rescale(clamp(inputs[ROOT_NOTE_INPUT].value, 0.0f,10.0f),0.0f,10.0f,0.0f,11.0f), 0.0f,11.0f), patterns[playedPattern].scale + inputs[SCALE_INPUT].value);

		prevIndex = index;
		auto nextT = patterns[playedPattern].GetNextStep(reStart);
		index = std::get<0>(nextT);
		pulse = std::get<1>(nextT);

		if (reStart)
			reStart = false;

		if (((!stepOutputsMode) && (pulse == 0)) || (stepOutputsMode))
			stepPulse[patterns[playedPattern].CurrentStep().index].trigger(10 * invESR);

		probGate = index != prevIndex ? randomUniform() <= patterns[playedPattern].CurrentStep().gateProb : probGate;
		rndPitch = index != prevIndex ? rescale(randomUniform(),0.0f,1.0f,patterns[playedPattern].CurrentStep().pitchRnd * -5.0f, patterns[playedPattern].CurrentStep().pitchRnd * 5.0f) : rndPitch;
		accent = index != prevIndex ? clamp(patterns[playedPattern].CurrentStep().accent + rescale(randomUniform(),0.0f,1.0f,patterns[playedPattern].CurrentStep().accentRnd * -5.0f,patterns[playedPattern].CurrentStep().accentRnd * 5.0f),0.0f,10.0f)  : accent;

		lights[STEPS_LIGHTS+patterns[playedPattern].CurrentStep().index].value = 1.0f;
	}

	// Lights & steps outputs
	for (int i = 0; i < 8; i++) {
		lights[STEPS_LIGHTS + i].value -= lights[STEPS_LIGHTS + i].value * invLightLambda * invESR;
		lights[SLIDES_LIGHTS + i].value = slideState[i] == 't' ? 1.0f - lights[STEPS_LIGHTS + i].value : lights[STEPS_LIGHTS + i].value;
		lights[SKIPS_LIGHTS + i].value = skipState[i]== 't' ? 1.0f - lights[STEPS_LIGHTS + i].value : lights[STEPS_LIGHTS + i].value;

		outputs[STEP_OUTPUT+i].value = stepPulse[i].process(invESR) ? 10.0f : 0.0f;
	}
	lights[RESET_LIGHT].value -= lights[RESET_LIGHT].value * invLightLambda * invESR;
	lights[COPY_LIGHT].value = (copyPattern >= 0) ? 1 : 0;

	// Caclulate Outputs
	gateOn = running && (!patterns[playedPattern].CurrentStep().skip);
	float gateValue = 0.0f;
	if (gateOn){
		if (patterns[playedPattern].CurrentStep().type == 0) {
			gateOn = false;
		}
		else if (((patterns[playedPattern].CurrentStep().type == 1) && (pulse == 0))
				|| (patterns[playedPattern].CurrentStep().type == 2)
				|| ((patterns[playedPattern].CurrentStep().type == 3) && (pulse == patterns[playedPattern].CurrentStep().pulses))) {
			float gateCoeff = clamp(patterns[playedPattern].gateTime - 0.02f + inputs[GATE_TIME_INPUT].value * 0.1f, 0.0f, 0.99f);
			gateOn = phase < gateCoeff;
			gateValue = 10.0f;
		}
		else if (patterns[playedPattern].CurrentStep().type == 3) {
			gateOn = true;
			gateValue = 10.0f;
		}
		else if (patterns[playedPattern].CurrentStep().type == 4) {
			gateOn = true;
			gateValue = inputs[EXTGATE1_INPUT].value;
		}
		else if (patterns[playedPattern].CurrentStep().type == 5) {
			gateOn = true;
			gateValue = inputs[EXTGATE2_INPUT].value;
		}
		else {
			gateOn = false;
			gateValue = 0.0f;
		}
	}
	//pitch management
	pitch = closestVoltageInScale(clamp(patterns[playedPattern].CurrentStep().pitch + rndPitch,-4.0f,6.0f) * patterns[playedPattern].sensitivity,clamp(patterns[playedPattern].rootNote + rescale(clamp(inputs[ROOT_NOTE_INPUT].value, 0.0f,10.0f),0.0f,10.0f,0.0f,11.0f), 0.0f, 11.0f), patterns[playedPattern].scale + inputs[SCALE_INPUT].value);
	if (patterns[playedPattern].CurrentStep().slide) {
		if (pulse == 0) {
			float slideCoeff = clamp(patterns[playedPattern].slideTime - 0.01f + inputs[SLIDE_TIME_INPUT].value * 0.1f, -0.1f, 0.99f);
			pitch = pitch - (1.0f - powf(phase, slideCoeff)) * (pitch - previousPitch);
		}
	}

	// Update Outputs
	outputs[GATE_OUTPUT].value = gateOn ? (probGate ? gateValue : 0.0f) : 0.0f;
	outputs[PITCH_OUTPUT].value = pitch;
	outputs[ACC_OUTPUT].value = accent;

	if (nextStep && gateOn)
		previousPitch = candidateForPreviousPitch;
}

struct BORDLDisplay : TransparentWidget {
	BORDL *module;
	int frame = 0;
	shared_ptr<Font> font;

	string note, scale, steps, playMode, selectedPattern, playedPattern;

	BORDLDisplay() {
		font = Font::load(assetPlugin(plugin, "res/DejaVuSansMono.ttf"));
	}

	void drawMessage(NVGcontext *vg, Vec pos, string note, string playMode, string selectedPattern, string playedPattern, string steps, string scale) {
		nvgFontSize(vg, 18.0f);
		nvgFontFaceId(vg, font->handle);
		nvgTextLetterSpacing(vg, -2.0f);
		nvgFillColor(vg, YELLOW_BIDOO);
		nvgText(vg, pos.x + 4.0f, pos.y + 8.0f, playMode.c_str(), NULL);
		nvgFontSize(vg, 14.0f);
		nvgText(vg, pos.x + 118.0f, pos.y + 7.0f, selectedPattern.c_str(), NULL);

		nvgText(vg, pos.x + 27.0f, pos.y + 7.0f, steps.c_str(), NULL);
		nvgText(vg, pos.x + 3.0f, pos.y + 21.0f, note.c_str(), NULL);
		nvgText(vg, pos.x + 25.0f, pos.y + 21.0f, scale.c_str(), NULL);

		if (++frame <= 30) {
			nvgText(vg, pos.x + 89.0f, pos.y + 7.0f, playedPattern.c_str(), NULL);
		}
		else if (++frame>60) {
			frame = 0;
		}

	}

	string displayRootNote(int value) {
		switch(value){
			case BORDL::NOTE_C:       return "C";
			case BORDL::NOTE_C_SHARP: return "C#";
			case BORDL::NOTE_D:       return "D";
			case BORDL::NOTE_D_SHARP: return "D#";
			case BORDL::NOTE_E:       return "E";
			case BORDL::NOTE_F:       return "F";
			case BORDL::NOTE_F_SHARP: return "F#";
			case BORDL::NOTE_G:       return "G";
			case BORDL::NOTE_G_SHARP: return "G#";
			case BORDL::NOTE_A:       return "A";
			case BORDL::NOTE_A_SHARP: return "A#";
			case BORDL::NOTE_B:       return "B";
			default: return "";
		}
	}

	string displayScale(int value) {
		switch(value){
			case BORDL::AEOLIAN:        return "Aeolian";
			case BORDL::BLUES:          return "Blues";
			case BORDL::CHROMATIC:      return "Chromatic";
			case BORDL::DIATONIC_MINOR: return "Diatonic Minor";
			case BORDL::DORIAN:         return "Dorian";
			case BORDL::HARMONIC_MINOR: return "Harmonic Minor";
			case BORDL::INDIAN:         return "Indian";
			case BORDL::LOCRIAN:        return "Locrian";
			case BORDL::LYDIAN:         return "Lydian";
			case BORDL::MAJOR:          return "Major";
			case BORDL::MELODIC_MINOR:  return "Melodic Minor";
			case BORDL::MINOR:          return "Minor";
			case BORDL::MIXOLYDIAN:     return "Mixolydian";
			case BORDL::NATURAL_MINOR:  return "Natural Minor";
			case BORDL::PENTATONIC:     return "Pentatonic";
			case BORDL::PHRYGIAN:       return "Phrygian";
			case BORDL::TURKISH:        return "Turkish";
			case BORDL::NONE:           return "None";
			default: return "";
		}
	}

	string displayPlayMode(int value) {
		switch(value){
			case 0: return "►";
			case 1: return "◄";
			case 2: return "►◄";
			case 3: return "►*";
			case 4: return "►?";
			default: return "";
		}
	}

	void draw(NVGcontext *vg) override {
		note = displayRootNote(clamp(module->patterns[module->selectedPattern].rootNote + rescale(clamp(module->inputs[BORDL::ROOT_NOTE_INPUT].value,0.0f,10.0f),0.0f,10.0f,0.0f,11.0f),0.0f, 11.0f));
		steps = (module->patterns[module->selectedPattern].countMode == 0 ? "steps:" : "pulses:" ) + to_string(module->patterns[module->selectedPattern].numberOfStepsParam);
		playMode = displayPlayMode(module->patterns[module->selectedPattern].playMode);
		scale = displayScale(module->patterns[module->selectedPattern].scale);
		selectedPattern = "P" + to_string(module->selectedPattern + 1);
		playedPattern = "P" + to_string(module->playedPattern + 1);
		drawMessage(vg, Vec(0.0f, 20.0f), note, playMode, selectedPattern, playedPattern, steps, scale);
	}
};

struct BORDLGateDisplay : TransparentWidget {
	BORDL *module;
	shared_ptr<Font> font;

	int index;

	BORDLGateDisplay() {
		font = Font::load(assetPlugin(plugin, "res/DejaVuSansMono.ttf"));
	}

	void drawGate(NVGcontext *vg, Vec pos) {
		int gateType = (int)module->params[BORDL::TRIG_TYPE_PARAM+index].value;
		nvgStrokeWidth(vg, 1.0f);
		nvgStrokeColor(vg, YELLOW_BIDOO);
		nvgFillColor(vg, YELLOW_BIDOO);
		nvgTextAlign(vg, NVG_ALIGN_CENTER);
		nvgFontSize(vg, 16.0f);
		nvgFontFaceId(vg, font->handle);
		nvgTextLetterSpacing(vg, -2.0f);
		if (gateType == 0) {
			nvgBeginPath(vg);
			nvgRoundedRect(vg,pos.x,pos.y,22.0f,6.0f,0.0f);
			nvgClosePath(vg);
			nvgStroke(vg);
		}
		else if (gateType == 1) {
			nvgBeginPath(vg);
			nvgRoundedRect(vg,pos.x,pos.y,22.0f,6.0f,0.0f);
			nvgClosePath(vg);
			nvgStroke(vg);
			nvgBeginPath(vg);
			nvgRoundedRect(vg,pos.x,pos.y,6.0f,6.0f,0.0f);
			nvgClosePath(vg);
			nvgStroke(vg);
			nvgFill(vg);
		}
		else if (gateType == 2) {
			nvgBeginPath(vg);
			nvgRoundedRect(vg,pos.x,pos.y,6.0f,6.0f,0.0f);
			nvgRoundedRect(vg,pos.x+8.0f,pos.y,6.0f,6.0f,0.0f);
			nvgRoundedRect(vg,pos.x+16.0f,pos.y,6.0f,6.0f,0.0f);
			nvgClosePath(vg);
			nvgStroke(vg);
			nvgFill(vg);
		}
		else if (gateType == 3) {
			nvgBeginPath(vg);
			nvgRoundedRect(vg,pos.x,pos.y,22.0f,6.0f,0.0f);
			nvgClosePath(vg);
			nvgStroke(vg);
			nvgFill(vg);
		}
		else if (gateType == 4) {
		  nvgText(vg, pos.x+11.0f, pos.y+8.0f, "G1", NULL);
		}
		else if (gateType == 5) {
		  nvgText(vg, pos.x+11.0f, pos.y+8.0f, "G2", NULL);
		}
	}

	void draw(NVGcontext *vg) override {
		drawGate(vg, Vec(0.0f, 0.0f));
	}
};

struct BORDLPulseDisplay : TransparentWidget {
	BORDL *module;
	shared_ptr<Font> font;

	int index;

	BORDLPulseDisplay() {
		font = Font::load(assetPlugin(plugin, "res/DejaVuSansMono.ttf"));
	}

	void drawPulse(NVGcontext *vg, Vec pos) {
		nvgStrokeWidth(vg, 1.0f);
		nvgStrokeColor(vg, YELLOW_BIDOO);
		nvgFillColor(vg, YELLOW_BIDOO);
		nvgTextAlign(vg, NVG_ALIGN_CENTER);
		nvgFontSize(vg, 16.0f);
		nvgFontFaceId(vg, font->handle);
		nvgTextLetterSpacing(vg, -2.0f);
		char tCount[128];
		snprintf(tCount, sizeof(tCount), "%1i", (int)module->params[BORDL::TRIG_COUNT_PARAM+index].value);
		nvgText(vg, pos.x, pos.y, tCount, NULL);
	}

	void draw(NVGcontext *vg) override {
		drawPulse(vg, Vec(0.0f, 0.0f));
	}
};

struct BORDLPitchDisplay : TransparentWidget {
	BORDL *module;
	shared_ptr<Font> font;

	int index;

	BORDLPitchDisplay() {
		font = Font::load(assetPlugin(plugin, "res/DejaVuSansMono.ttf"));
	}

	string displayNote(float value) {
		int octave = sgn(value) == 1.0f ? value : (value-1);
		int note = (value-octave)*1000;
		switch(note){
			case 0:  return "C" + to_string(octave+4);
			case 83: return "C#" + to_string(octave+4);
			case 166: return "D" + to_string(octave+4);
			case 250: return "D#" + to_string(octave+4);
			case 333: return "E" + to_string(octave+4);
			case 416: return "F" + to_string(octave+4);
			case 500: return "F#" + to_string(octave+4);
			case 583: return "G" + to_string(octave+4);
			case 666: return "G#" + to_string(octave+4);
			case 750: return "A" + to_string(octave+4);
			case 833: return "A#" + to_string(octave+4);
			case 916: return "B" + to_string(octave+4);
			case 1000: return "C" + to_string(octave+5);
			default: return to_string(octave+4);
		}
	}

	void drawPitch(NVGcontext *vg, Vec pos) {
		nvgStrokeWidth(vg, 3.0f);
		nvgStrokeColor(vg, YELLOW_BIDOO);
		nvgFillColor(vg, YELLOW_BIDOO);
		nvgTextAlign(vg, NVG_ALIGN_CENTER);
		nvgFontSize(vg, 16.0f);
		nvgFontFaceId(vg, font->handle);
		nvgTextLetterSpacing(vg, -2.0f);
		nvgText(vg, pos.x, pos.y-9.0f, displayNote(module->closestVoltageInScale(module->params[BORDL::TRIG_PITCH_PARAM+index].value * module->params[BORDL::SENSITIVITY_PARAM].value , clamp(module->patterns[module->selectedPattern].rootNote + rescale(clamp(module->inputs[BORDL::ROOT_NOTE_INPUT].value, 0.0f,10.0f),0.0f,10.0f,0.0f,11.0f), 0.0f, 11.0f), module->patterns[module->selectedPattern].scale)).c_str(), NULL);
	}

	void draw(NVGcontext *vg) override {
		drawPitch(vg, Vec(0.0f, 0.0f));
	}
};

struct BORDLWidget : ModuleWidget {
	ParamWidget *stepsParam, *scaleParam, *rootNoteParam, *sensitivityParam,
	 *gateTimeParam, *slideTimeParam, *playModeParam, *countModeParam, *patternParam,
		*pitchParams[8], *pulseParams[8], *typeParams[8], *slideParams[8], *skipParams[8],
		 *pitchRndParams[8], *pulseProbParams[8], *accentParams[8], *rndAccentParams[8];

	BORDLWidget(BORDL *module);
	Menu *createContextMenu() override;
};

struct BORDLPatternRoundBlackSnapKnob : RoundBlackSnapKnob {
	void onChange(EventChange &e) override {
			RoundBlackSnapKnob::onChange(e);
			BORDL *module = dynamic_cast<BORDL*>(this->module);
			BORDLWidget *parent = dynamic_cast<BORDLWidget*>(this->parent);
			int target = clamp(value - 1.0f, 0.0f, 15.0f);
			if (module && parent && (target != module->selectedPattern) && module->updateFlag)
			{
				module->updateFlag = false;
				module->selectedPattern = value - 1;
				parent->stepsParam->setValue(module->patterns[target].numberOfStepsParam);
				parent->rootNoteParam->setValue(module->patterns[target].rootNote);
				parent->scaleParam->setValue(module->patterns[target].scale);
				parent->gateTimeParam->setValue(module->patterns[target].gateTime);
				parent->slideTimeParam->setValue(module->patterns[target].slideTime);
				parent->sensitivityParam->setValue(module->patterns[target].sensitivity);
				module->playMode = module->patterns[module->selectedPattern].playMode;
				module->countMode = module->patterns[module->selectedPattern].countMode;
				for (int i = 0; i < 8; i++) {
					parent->pitchParams[i]->setValue(module->patterns[target].steps[i].pitch);
					parent->pulseParams[i]->setValue(module->patterns[target].steps[i].pulsesParam);
					parent->typeParams[i]->setValue(module->patterns[target].steps[i].type);
					module->skipState[i] = module->patterns[target].steps[i].skipParam ? 't' : 'f';
					module->slideState[i] = module->patterns[target].steps[i].slide ? 't' : 'f';
					parent->pulseProbParams[i]->setValue(module->patterns[target].steps[i].gateProb);
					parent->pitchRndParams[i]->setValue(module->patterns[target].steps[i].pitchRnd);
					parent->accentParams[i]->setValue(module->patterns[target].steps[i].accent);
					parent->rndAccentParams[i]->setValue(module->patterns[target].steps[i].accentRnd);
				}
				module->updateFlag = true;
			}
		}
};

struct BORDLCOPYPASTECKD6 : BlueCKD6 {
	void onMouseDown(EventMouseDown &e) override {
		BORDLWidget *bordlWidget = dynamic_cast<BORDLWidget*>(this->parent);
		BORDL *bordlModule = dynamic_cast<BORDL*>(this->module);
		if (!bordlModule->copyState) {
			bordlModule->copyPattern = bordlModule->selectedPattern;
			bordlModule->copyState = true;
		}
		else if (bordlModule && bordlWidget && (bordlModule->copyState) && (bordlModule->copyPattern != bordlModule->selectedPattern) && bordlModule->updateFlag)
		{
			bordlModule->updateFlag = false;
			bordlWidget->stepsParam->setValue(bordlModule->patterns[bordlModule->copyPattern].numberOfStepsParam);
			bordlWidget->rootNoteParam->setValue(bordlModule->patterns[bordlModule->copyPattern].rootNote);
			bordlWidget->scaleParam->setValue(bordlModule->patterns[bordlModule->copyPattern].scale);
			bordlWidget->gateTimeParam->setValue(bordlModule->patterns[bordlModule->copyPattern].gateTime);
			bordlWidget->slideTimeParam->setValue(bordlModule->patterns[bordlModule->copyPattern].slideTime);
			bordlWidget->sensitivityParam->setValue(bordlModule->patterns[bordlModule->copyPattern].sensitivity);
			bordlModule->playMode = bordlModule->patterns[bordlModule->copyPattern].playMode;
			bordlModule->countMode = bordlModule->patterns[bordlModule->copyPattern].countMode;
			for (int i = 0; i < 8; i++) {
				bordlWidget->pitchParams[i]->setValue(bordlModule->patterns[bordlModule->copyPattern].steps[i].pitch);
				bordlWidget->pulseParams[i]->setValue(bordlModule->patterns[bordlModule->copyPattern].steps[i].pulsesParam);
				bordlWidget->typeParams[i]->setValue(bordlModule->patterns[bordlModule->copyPattern].steps[i].type);
				bordlModule->skipState[i] = bordlModule->patterns[bordlModule->copyPattern].steps[i].skipParam ? 't' : 'f';
				bordlModule->slideState[i] = bordlModule->patterns[bordlModule->copyPattern].steps[i].slide ? 't' : 'f';
				bordlWidget->pulseProbParams[i]->setValue(bordlModule->patterns[bordlModule->copyPattern].steps[i].gateProb);
				bordlWidget->pitchRndParams[i]->setValue(bordlModule->patterns[bordlModule->copyPattern].steps[i].pitchRnd);
				bordlWidget->accentParams[i]->setValue(bordlModule->patterns[bordlModule->copyPattern].steps[i].accent);
				bordlWidget->rndAccentParams[i]->setValue(bordlModule->patterns[bordlModule->copyPattern].steps[i].accentRnd);
			}
			bordlModule->copyState = false;
			bordlModule->copyPattern = -1;
			bordlModule->updateFlag = true;
		}
		BlueCKD6::onMouseDown(e);
	}
};

struct BORDLShiftUpBtn : UpBtn {
	void onMouseDown(EventMouseDown &e) override {
		BORDLWidget *bordlWidget = dynamic_cast<BORDLWidget*>(this->parent);
		BORDL *bordlModule = dynamic_cast<BORDL*>(this->module);
		if (bordlModule && bordlWidget && bordlModule->updateFlag && (bordlModule->patterns[bordlModule->selectedPattern].MaxVoltage()<6.0f))
		{
			bordlModule->updateFlag = false;
			for (int i = 0; i < 8; i++) {
				bordlWidget->pitchParams[i]->setValue(min(bordlWidget->pitchParams[i]->value + 0.1f,10.0f));
			}
			bordlModule->updateFlag = true;
		}
		UpBtn::onMouseDown(e);
	}
};

struct BORDLShiftDownBtn : DownBtn {
	void onMouseDown(EventMouseDown &e) override {
		BORDLWidget *bordlWidget = dynamic_cast<BORDLWidget*>(this->parent);
		BORDL *bordlModule = dynamic_cast<BORDL*>(this->module);
		if (bordlModule && bordlWidget && bordlModule->updateFlag && (bordlModule->patterns[bordlModule->selectedPattern].MinVoltage()>-4.0f))
		{
			bordlModule->updateFlag = false;
			for (int i = 0; i < 8; i++) {
				bordlWidget->pitchParams[i]->setValue(max(bordlWidget->pitchParams[i]->value - 0.1f,-10.0f));
			}
			bordlModule->updateFlag = true;
		}
		DownBtn::onMouseDown(e);
	}
};

struct BORDLShiftLeftBtn : LeftBtn {
	void onMouseDown(EventMouseDown &e) override {
		BORDLWidget *bordlWidget = dynamic_cast<BORDLWidget*>(this->parent);
		BORDL *bordlModule = dynamic_cast<BORDL*>(this->module);
		if (bordlModule && bordlWidget && bordlModule->updateFlag)
		{
			bordlModule->updateFlag = false;
			float pitch = bordlWidget->pitchParams[0]->value;
			float pulse = bordlWidget->pulseParams[0]->value;
			float type = bordlWidget->typeParams[0]->value;
			float pProb = bordlWidget->pulseProbParams[0]->value;
			float pRnd = bordlWidget->pitchRndParams[0]->value;
			float acc = bordlWidget->accentParams[0]->value;
			float aRnd = bordlWidget->rndAccentParams[0]->value;
			char skip = bordlModule->skipState[0];
			char slide = bordlModule->slideState[0];
			for (int i = 0; i < 7; i++) {
				bordlWidget->pitchParams[i]->setValue(bordlWidget->pitchParams[i+1]->value);
				bordlWidget->pulseParams[i]->setValue(bordlWidget->pulseParams[i+1]->value);
				bordlWidget->typeParams[i]->setValue(bordlWidget->typeParams[i+1]->value);
				bordlModule->skipState[i] = bordlModule->skipState[i+1];
				bordlModule->slideState[i] = bordlModule->slideState[i+1];
				bordlWidget->pulseProbParams[i]->setValue(bordlWidget->pulseProbParams[i+1]->value);
				bordlWidget->pitchRndParams[i]->setValue(bordlWidget->pitchRndParams[i+1]->value);
				bordlWidget->accentParams[i]->setValue(bordlWidget->accentParams[i+1]->value);
				bordlWidget->rndAccentParams[i]->setValue(bordlWidget->rndAccentParams[i+1]->value);
			}
			bordlWidget->pitchParams[7]->setValue(pitch);
			bordlWidget->pulseParams[7]->setValue(pulse);
			bordlWidget->typeParams[7]->setValue(type);
			bordlWidget->pulseProbParams[7]->setValue(pProb);
			bordlWidget->pitchRndParams[7]->setValue(pRnd);
			bordlWidget->accentParams[7]->setValue(acc);
			bordlWidget->rndAccentParams[7]->setValue(aRnd);
			bordlModule->skipState[7] = skip;
			bordlModule->slideState[7] = slide;
			bordlModule->updateFlag = true;
		}
		LeftBtn::onMouseDown(e);
	}
};

struct BORDLShiftRightBtn : RightBtn {
	void onMouseDown(EventMouseDown &e) override {
		BORDLWidget *bordlWidget = dynamic_cast<BORDLWidget*>(this->parent);
		BORDL *bordlModule = dynamic_cast<BORDL*>(this->module);
		if (bordlModule && bordlWidget && bordlModule->updateFlag)
		{
			bordlModule->updateFlag = false;
			float pitch = bordlWidget->pitchParams[7]->value;
			float pulse = bordlWidget->pulseParams[7]->value;
			float type = bordlWidget->typeParams[7]->value;
			float pProb = bordlWidget->pulseProbParams[7]->value;
			float pRnd = bordlWidget->pitchRndParams[7]->value;
			float acc = bordlWidget->accentParams[7]->value;
			float aRnd = bordlWidget->rndAccentParams[7]->value;
			char skip = bordlModule->skipState[7];
			char slide = bordlModule->slideState[7];
			for (int i = 7; i > 0; i--) {
				bordlWidget->pitchParams[i]->setValue(bordlWidget->pitchParams[i-1]->value);
				bordlWidget->pulseParams[i]->setValue(bordlWidget->pulseParams[i-1]->value);
				bordlWidget->typeParams[i]->setValue(bordlWidget->typeParams[i-1]->value);
				bordlModule->skipState[i] = bordlModule->skipState[i-1];
				bordlModule->slideState[i] = bordlModule->slideState[i-1];
				bordlWidget->pulseProbParams[i]->setValue(bordlWidget->pulseProbParams[i-1]->value);
				bordlWidget->pitchRndParams[i]->setValue(bordlWidget->pitchRndParams[i-1]->value);
				bordlWidget->accentParams[i]->setValue(bordlWidget->accentParams[i-1]->value);
				bordlWidget->rndAccentParams[i]->setValue(bordlWidget->rndAccentParams[i-1]->value);
			}
			bordlWidget->pitchParams[0]->setValue(pitch);
			bordlWidget->pulseParams[0]->setValue(pulse);
			bordlWidget->typeParams[0]->setValue(type);
			bordlWidget->pulseProbParams[0]->setValue(pProb);
			bordlWidget->pitchRndParams[0]->setValue(pRnd);
			bordlWidget->accentParams[0]->setValue(acc);
			bordlWidget->rndAccentParams[0]->setValue(aRnd);
			bordlModule->skipState[0] = skip;
			bordlModule->slideState[0] = slide;
			bordlModule->updateFlag = true;
		}
		RightBtn::onMouseDown(e);
	}
};


BORDLWidget::BORDLWidget(BORDL *module) : ModuleWidget(module) {
	setPanel(SVG::load(assetPlugin(plugin, "res/BORDL.svg")));

	addChild(Widget::create<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
	addChild(Widget::create<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
	addChild(Widget::create<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
	addChild(Widget::create<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

	{
		BORDLDisplay *display = new BORDLDisplay();
		display->module = module;
		display->box.pos = Vec(20.0f, 217.0f);
		display->box.size = Vec(250.0f, 60.0f);
		addChild(display);
	}

	static const float portX0[4] = {20.0f, 58.0f, 96.0f, 135.0f};

	addParam(ParamWidget::create<RoundBlackKnob>(Vec(portX0[0]-2.0f, 36.0f), module, BORDL::CLOCK_PARAM, -2.0f, 6.0f, 2.0f));
	addParam(ParamWidget::create<LEDButton>(Vec(61.0f, 40.0f), module, BORDL::RUN_PARAM, 0.0f, 1.0f, 0.0f));
	addChild(ModuleLightWidget::create<SmallLight<GreenLight>>(Vec(67.0f, 46.0f), module, BORDL::RUNNING_LIGHT));
	addParam(ParamWidget::create<LEDButton>(Vec(99.0f, 40.0f), module, BORDL::RESET_PARAM, 0.0f, 1.0f, 0.0f));
	addChild(ModuleLightWidget::create<SmallLight<GreenLight>>(Vec(105.0f, 46.0f), module, BORDL::RESET_LIGHT));
	stepsParam = ParamWidget::create<BidooBlueSnapKnob>(Vec(portX0[3]-2.0f, 36.0f), module, BORDL::STEPS_PARAM, 1.0f, 16.0f, 8.0f);
	addParam(stepsParam);


 	addInput(Port::create<PJ301MPort>(Vec(portX0[0], 69.0f), Port::INPUT, module, BORDL::CLOCK_INPUT));
	addInput(Port::create<PJ301MPort>(Vec(portX0[1], 69.0f), Port::INPUT, module, BORDL::EXT_CLOCK_INPUT));
	addInput(Port::create<PJ301MPort>(Vec(portX0[2], 69.0f), Port::INPUT, module, BORDL::RESET_INPUT));
	addInput(Port::create<PJ301MPort>(Vec(portX0[3], 69.0f), Port::INPUT, module, BORDL::STEPS_INPUT));

	rootNoteParam = ParamWidget::create<BidooBlueSnapKnob>(Vec(portX0[0]-2.0f, 116.0f), module, BORDL::ROOT_NOTE_PARAM, 0.0f, BORDL::NUM_NOTES-1.0f, 0.0f);
	addParam(rootNoteParam);
	scaleParam = ParamWidget::create<BidooBlueSnapKnob>(Vec(portX0[1]-2.0f, 116.0f), module, BORDL::SCALE_PARAM, 0.0f, BORDL::NUM_SCALES-1.0f, 0.0f);
	addParam(scaleParam);
	gateTimeParam = ParamWidget::create<BidooBlueKnob>(Vec(portX0[2]-2.0f, 116.0f), module, BORDL::GATE_TIME_PARAM, 0.1f, 1.0f, 0.5f);
	addParam(gateTimeParam);
	slideTimeParam = ParamWidget::create<BidooBlueKnob>(Vec(portX0[3]-2.0f, 116.0f), module, BORDL::SLIDE_TIME_PARAM	, 0.1f, 1.0f, 0.2f);
	addParam(slideTimeParam);

	addInput(Port::create<PJ301MPort>(Vec(portX0[0], 149.0f), Port::INPUT, module, BORDL::ROOT_NOTE_INPUT));
	addInput(Port::create<PJ301MPort>(Vec(portX0[1], 149.0f), Port::INPUT, module, BORDL::SCALE_INPUT));
	addInput(Port::create<PJ301MPort>(Vec(portX0[2], 149.0f), Port::INPUT, module, BORDL::GATE_TIME_INPUT));
	addInput(Port::create<PJ301MPort>(Vec(portX0[3], 149.0f), Port::INPUT, module, BORDL::SLIDE_TIME_INPUT));

	playModeParam = ParamWidget::create<BlueCKD6>(Vec(portX0[0]-1.0f, 196.0f), module, BORDL::PLAY_MODE_PARAM, 0.0f, 4.0f, 0.0f);
	addParam(playModeParam);
	countModeParam = ParamWidget::create<BlueCKD6>(Vec(portX0[1]-1.0f, 196.0f), module, BORDL::COUNT_MODE_PARAM, 0.0f, 4.0f, 0.0f);
	addParam(countModeParam);
	addInput(Port::create<PJ301MPort>(Vec(portX0[2], 198.0f), Port::INPUT, module, BORDL::PATTERN_INPUT));
	patternParam = ParamWidget::create<BORDLPatternRoundBlackSnapKnob>(Vec(portX0[3],196.0f), module, BORDL::PATTERN_PARAM, 1.0f, 16.0f, 1.0f);
	addParam(patternParam);

	static const float portX1[8] = {200.0f, 241.0f, 282.0f, 323.0f, 364.0f, 405.0f, 446.0f, 487.0f};

	sensitivityParam = ParamWidget::create<BidooBlueTrimpot>(Vec(portX1[0]-24.0f, 38.0f), module, BORDL::SENSITIVITY_PARAM, 0.1f, 1.0f, 1.0f);
	addParam(sensitivityParam);

	addInput(Port::create<PJ301MPort>(Vec(portX0[0], 286.0f), Port::INPUT, module, BORDL::TRANSPOSE_INPUT));
	addParam(ParamWidget::create<BORDLCOPYPASTECKD6>(Vec(portX0[1]-1.0f, 285.0f), module, BORDL::COPY_PARAM, 0.0f, 1.0f, 0.0f));
	addChild(ModuleLightWidget::create<SmallLight<GreenLight>>(Vec(portX0[1]+23.0f, 283.0f), module, BORDL::COPY_LIGHT));

	addParam(ParamWidget::create<BORDLShiftLeftBtn>(Vec(104.0f, 290.0f), module, BORDL::LEFT_PARAM, 0.0f, 1.0f, 0.0f));
	addParam(ParamWidget::create<BORDLShiftRightBtn>(Vec(134.0f, 290.0f), module, BORDL::RIGHT_PARAM, 0.0f, 1.0f, 0.0f));
	addParam(ParamWidget::create<BORDLShiftUpBtn>(Vec(119.0f, 282.0f), module, BORDL::UP_PARAM, 0.0f, 1.0f, 0.0f));
	addParam(ParamWidget::create<BORDLShiftDownBtn>(Vec(119.0f, 297.0f), module, BORDL::DOWN_PARAM, 0.0f, 1.0f, 0.0f));

	for (int i = 0; i < 8; i++) {
		pitchParams[i] = ParamWidget::create<BidooBlueKnob>(Vec(portX1[i]+1.0f, 56.0f), module, BORDL::TRIG_PITCH_PARAM + i, -4.0f, 6.0f, 0.0f);
		addParam(pitchParams[i]);
		pitchRndParams[i] = ParamWidget::create<BidooBlueTrimpot>(Vec(portX1[i]+27.0f, 81.0f), module, BORDL::TRIG_PITCHRND_PARAM + i, 0.0f, 1.0f, 0.0f);
		addParam(pitchRndParams[i]);
		accentParams[i] = ParamWidget::create<BidooBlueKnob>(Vec(portX1[i]+1.0f, 110.0f), module, BORDL::TRIG_ACCENT_PARAM + i, 0.0f, 10.0f, 0.0f);
		addParam(accentParams[i]);
		rndAccentParams[i] = ParamWidget::create<BidooBlueTrimpot>(Vec(portX1[i]+27.0f, 135.0f), module, BORDL::TRIG_RNDACCENT_PARAM + i, 0.0f, 1.0f, 0.0f);
		addParam(rndAccentParams[i]);
		{
			BORDLPitchDisplay *displayPitch = new BORDLPitchDisplay();
			displayPitch->module = module;
			displayPitch->box.pos = Vec(portX1[i]+15.0f, 55.0f);
			displayPitch->box.size = Vec(20.0f, 10.0f);
			displayPitch->index = i;
			addChild(displayPitch);
		}
		pulseParams[i] = ParamWidget::create<BidooBlueSnapKnob>(Vec(portX1[i]+1.0f, 188.0f), module, BORDL::TRIG_COUNT_PARAM + i, 1.0f, 8.0f,  1.0f);
		addParam(pulseParams[i]);
		pulseProbParams[i] = ParamWidget::create<BidooBlueTrimpot>(Vec(portX1[i]+27.0f, 213.0f), module, BORDL::TRIG_GATEPROB_PARAM + i, 0.0f, 1.0f,  1.0f);
		addParam(pulseProbParams[i]);
		{
			BORDLPulseDisplay *displayPulse = new BORDLPulseDisplay();
			displayPulse->module = module;
			displayPulse->box.pos = Vec(portX1[i]+15.0f, 179.0f);
			displayPulse->box.size = Vec(20.0f, 10.0f);
			displayPulse->index = i;
			addChild(displayPulse);
		}
		typeParams[i] = ParamWidget::create<BidooBlueSnapTrimpot>(Vec(portX1[i]+6.5f, 267.0f), module, BORDL::TRIG_TYPE_PARAM + i, 0.0f, 5.0f,  2.0f);
		addParam(typeParams[i]);
		{
			BORDLGateDisplay *displayGate = new BORDLGateDisplay();
			displayGate->module = module;
			displayGate->box.pos = Vec(portX1[i]+5.0f, 250.0f);
			displayGate->box.size = Vec(20.0f, 10.0f);
			displayGate->index = i;
			addChild(displayGate);
		}
		slideParams[i] = ParamWidget::create<LEDButton>(Vec(portX1[i]+7.0f, 295.0f), module, BORDL::TRIG_SLIDE_PARAM + i, 0.0f, 1.0f,  0.0f);
		addParam(slideParams[i]);
		addChild(ModuleLightWidget::create<SmallLight<BlueLight>>(Vec(portX1[i]+13.0f, 301.0f), module, BORDL::SLIDES_LIGHTS + i));
		skipParams[i] = ParamWidget::create<LEDButton>(Vec(portX1[i]+7.0f, 316.0f), module, BORDL::TRIG_SKIP_PARAM + i, 0.0f, 1.0f, 0.0f);
		addParam(skipParams[i]);
		addChild(ModuleLightWidget::create<SmallLight<BlueLight>>(Vec(portX1[i]+13.0f, 322.0f), module, BORDL::SKIPS_LIGHTS + i));

		addOutput(Port::create<TinyPJ301MPort>(Vec(portX1[i]+9.0f, 344.0f), Port::OUTPUT, module, BORDL::STEP_OUTPUT + i));
	}

	addInput(Port::create<PJ301MPort>(Vec(10.0f, 331.0f), Port::INPUT, module, BORDL::EXTGATE1_INPUT));
	addInput(Port::create<PJ301MPort>(Vec(43.0f, 331.0f), Port::INPUT, module, BORDL::EXTGATE2_INPUT));
	addOutput(Port::create<PJ301MPort>(Vec(76.5f, 331.0f), Port::OUTPUT, module, BORDL::GATE_OUTPUT));
	addOutput(Port::create<PJ301MPort>(Vec(109.5f, 331.0f), Port::OUTPUT, module, BORDL::PITCH_OUTPUT));
	addOutput(Port::create<PJ301MPort>(Vec(143.0f, 331.0f), Port::OUTPUT, module, BORDL::ACC_OUTPUT));
}

struct BORDLRandPitchItem : MenuItem {
	BORDLWidget *bordlWidget;
	void onAction(EventAction &e) override {
		for (int i = 0; i < 8; i++){
			int index = BORDL::TRIG_PITCH_PARAM + i;
			auto it = std::find_if(bordlWidget->params.begin(), bordlWidget->params.end(), [&index](const ParamWidget* m) -> bool { return m->paramId == index; });
			if (it != bordlWidget->params.end())
			{
				auto index = std::distance(bordlWidget->params.begin(), it);
				bordlWidget->params[index]->randomize();
			}
		}
	}
};

struct BORDLRandGatesItem : MenuItem {
	BORDLWidget *bordlWidget;
	void onAction(EventAction &e) override {
		for (int i = 0; i < 8; i++){
			int index = BORDL::TRIG_COUNT_PARAM + i;
			auto it = std::find_if(bordlWidget->params.begin(), bordlWidget->params.end(), [&index](const ParamWidget* m) -> bool { return m->paramId == index; });
			if (it != bordlWidget->params.end())
			{
				auto index = std::distance(bordlWidget->params.begin(), it);
				bordlWidget->params[index]->randomize();
			}
		}
		for (int i = 0; i < 8; i++){
				int index = BORDL::TRIG_TYPE_PARAM + i;
				auto it = std::find_if(bordlWidget->params.begin(), bordlWidget->params.end(), [&index](const ParamWidget* m) -> bool { return m->paramId == index; });
				if (it != bordlWidget->params.end())
				{
					auto index = std::distance(bordlWidget->params.begin(), it);
				bordlWidget->params[index]->randomize();
			}
		}
	}
};

struct BORDLRandSlideSkipItem : MenuItem {
	BORDL *bordlModule;
	void onAction(EventAction &e) override {
		bordlModule->randomizeSlidesSkips();
	}
};

struct BORDLStepOutputsModeItem : MenuItem {
	BORDL *bordlModule;
	void onAction(EventAction &e) override {
		bordlModule->stepOutputsMode = !bordlModule->stepOutputsMode;
	}
	void step() override {
		rightText = bordlModule->stepOutputsMode ? "✔" : "";
		MenuItem::step();
	}
};

struct DisconnectMenuItem : MenuItem {
	ModuleWidget *moduleWidget;
	void onAction(EventAction &e) override {
		moduleWidget->disconnect();
	}
};

struct ResetMenuItem : MenuItem {
	BORDLWidget *bordlWidget;
	BORDL *bordlModule;
	void onAction(EventAction &e) override {
		for (int i = 0; i < BORDL::NUM_PARAMS; i++){
			if (i != BORDL::PATTERN_PARAM) {
				auto it = std::find_if(bordlWidget->params.begin(), bordlWidget->params.end(), [&i](const ParamWidget* m) -> bool { return m->paramId == i; });
				if (it != bordlWidget->params.end())
				{
					auto index = std::distance(bordlWidget->params.begin(), it);
					bordlWidget->params[index]->setValue(bordlWidget->params[index]->defaultValue);
				}
			}
		}
		bordlModule->updateFlag = false;
		bordlModule->reset();
		bordlModule->playMode = 0;
		bordlModule->countMode = 0;
		bordlModule->updateFlag = true;
	}
};

struct RandomizeMenuItem : MenuItem {
	ModuleWidget *moduleWidget;
	void onAction(EventAction &e) override {
		moduleWidget->randomize();
	}
};

struct CloneMenuItem : MenuItem {
	ModuleWidget *moduleWidget;
	void onAction(EventAction &e) override {
		RACK_PLUGIN_UI_RACKWIDGET->cloneModule(moduleWidget);
	}
};

struct DeleteMenuItem : MenuItem {
	ModuleWidget *moduleWidget;
	void onAction(EventAction &e) override {
		RACK_PLUGIN_UI_RACKWIDGET->deleteModule(moduleWidget);
		moduleWidget->finalizeEvents();
		delete moduleWidget;
	}
};

Menu *BORDLWidget::createContextMenu() {
	BORDLWidget *bordlWidget = dynamic_cast<BORDLWidget*>(this);
	assert(bordlWidget);

	BORDL *bordlModule = dynamic_cast<BORDL*>(module);
	assert(bordlModule);

	Menu *menu = rack::global_ui->ui.gScene->createMenu();

	MenuLabel *menuLabel = new MenuLabel();
	menuLabel->text = model->author + " " + model->name;
	menu->addChild(menuLabel);

	ResetMenuItem *resetItem = new ResetMenuItem();
	resetItem->text = "Initialize";
	resetItem->rightText = "+I";
	resetItem->bordlWidget = this;
	resetItem->bordlModule = bordlModule;
	menu->addChild(resetItem);

	DisconnectMenuItem *disconnectItem = new DisconnectMenuItem();
	disconnectItem->text = "Disconnect cables";
	disconnectItem->moduleWidget = this;
	menu->addChild(disconnectItem);

	CloneMenuItem *cloneItem = new CloneMenuItem();
	cloneItem->text = "Duplicate";
	cloneItem->rightText = "+D";
	cloneItem->moduleWidget = this;
	menu->addChild(cloneItem);

	DeleteMenuItem *deleteItem = new DeleteMenuItem();
	deleteItem->text = "Delete";
	deleteItem->rightText = "Backspace/Delete";
	deleteItem->moduleWidget = this;
	menu->addChild(deleteItem);

	MenuLabel *spacerLabel = new MenuLabel();
	menu->addChild(spacerLabel);

	BORDLRandPitchItem *randomizePitchItem = new BORDLRandPitchItem();
	randomizePitchItem->text = "Randomize pitch";
	randomizePitchItem->bordlWidget = bordlWidget;
	menu->addChild(randomizePitchItem);

	BORDLRandGatesItem *randomizeGatesItem = new BORDLRandGatesItem();
	randomizeGatesItem->text = "Randomize gates";
	randomizeGatesItem->bordlWidget = bordlWidget;
	menu->addChild(randomizeGatesItem);

	BORDLRandSlideSkipItem *randomizeSlideSkipItem = new BORDLRandSlideSkipItem();
	randomizeSlideSkipItem->text = "Randomize slides and skips";
	randomizeSlideSkipItem->bordlModule = bordlModule;
	menu->addChild(randomizeSlideSkipItem);

	MenuLabel *spacerLabel2 = new MenuLabel();
	menu->addChild(spacerLabel2);

	BORDLStepOutputsModeItem *stepOutputsModeItem = new BORDLStepOutputsModeItem();
	stepOutputsModeItem->text = "Step trigs on steps (vs. pulses)";
	stepOutputsModeItem->bordlModule = bordlModule;
	menu->addChild(stepOutputsModeItem);

	return menu;
}

} // namespace rack_plugin_Bidoo

using namespace rack_plugin_Bidoo;

RACK_PLUGIN_MODEL_INIT(Bidoo, BORDL) {
   Model *modelBORDL = Model::create<BORDL, BORDLWidget>("Bidoo", "bordL", "bordL sequencer", SEQUENCER_TAG);
   return modelBORDL;
}
