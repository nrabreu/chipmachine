#ifndef CHIP_MACHINE_H
#define CHIP_MACHINE_H

#include "MusicDatabase.h"
#include "MusicPlayerList.h"
#include "TextScreen.h"
#include "SongInfoField.h"

#include "TelnetInterface.h"

#include "MainScreen.h"
#include "SearchScreen.h"

#include "../demofx/Effect.h"
#include "../demofx/StarField.h"

#include <tween/tween.h>
#include <grappix/grappix.h>
#include <lua/luainterpreter.h>

#include <coreutils/utils.h>

#include <cstdio>
#include <vector>
#include <string>
#include <memory>

namespace chipmachine {

class Scroller : public demofx::Effect {
public:
	Scroller(grappix::RenderTarget &target) : target(target), scr(screen.width()+200, 180) {
		font = Font("data/ObelixPro.ttf", 24, 512 | Font::DISTANCE_MAP);
		program = get_program(TEXTURED_PROGRAM).clone();


		Resources::getInstance().load<std::string>("sine_shader.glsl",
			[=](const std::string &source) {
				try {
					program.setFragmentSource(source);
				} catch(shader_exception &e) {}
			}, sineShaderF);


		fprogram = get_program(FONT_PROGRAM_DF).clone();
		fprogram.setFragmentSource(fontShaderF);
		font.set_program(fprogram);
	}

	virtual void set(const std::string &what, const std::string &val, float seconds = 0.0) {
		if(what == "font") {
			font = Font(val, 24, 512 | Font::DISTANCE_MAP);
		} else {
			scrollText = val;
			xpos = target.width() + 100;
			scrollLen = font.get_width(val, scrollsize);
		}
	}

	virtual void render(uint32_t delta) override {
		if(alpha <= 0.01)
			return;
		if(xpos < -scrollLen)
			xpos = target.width() + 100;
		scr.clear(0x00000000);
		scr.text(font, scrollText, xpos-=scrollspeed, 10, 0xffffff | ((int)(alpha*255) << 24), scrollsize);
		program.use();
		static float uvs[] = { 0,0,1,0,0,1,1,1 };
		target.draw(scr, 0.0f, scrolly, scr.width(), scr.height(), uvs, program);
	}

	float alpha = 1.0;

	int scrollspeed = 4;
	int scrolly = 0;
	float scrollsize = 4.0;

private:
	grappix::RenderTarget target;
	Font font;
	Program program;
	Program fprogram;
	int xpos = -9999;
	Texture scr;
	std::string scrollText;
	int scrollLen;


	const std::string sineShaderF = R"(
		uniform sampler2D sTexture;

		const vec4 color0 = vec4(1.0, 1.0, 0.0, 1.0);
		const vec4 color1 = vec4(0.0, 0.0, 1.0, 1.0);

		varying vec2 UV;

		void main() {

			vec4 rgb = mix(color0, color1, UV.y);
			// MODIFY UV HERE
			vec4 color = texture2D(sTexture, UV);
			// MODIFY COLOR HERE
			gl_FragColor = rgb * color; 
		}
	)";


	const std::string fontShaderF = R"(
		uniform vec4 vColor;
		uniform vec4 vScale;
		uniform sampler2D sTexture;
		//uniform float smoothing;
		varying vec2 UV;

		vec3 glyph_color    = vec3(0.0,1.0,0.0);
		const float glyph_center   = 0.50;
		vec3 outline_color  = vec3(0.0,0.0,1.0);
		const float outline_center = 0.58;
		vec3 glow_color     = vec3(1.0, 1.0, 0.0);
		const float glow_center    = 1.0;

		void main() {
			float dist = texture2D(sTexture, UV).a;
	#ifdef GL_ES
			float smoothing = 1.0 / (vScale.x * 16.0);
			float alpha = smoothstep(glyph_center-smoothing, glyph_center+smoothing, dist);
	#else
			float width = fwidth(dist);
			float alpha = smoothstep(glyph_center-width, glyph_center+width, dist);
			//float alpha = dist;
	#endif

			//gl_FragColor = vec4(1.0, 0.0, 0.0, alpha);
			//vec3 rgb = mix(vec3(0,0,0), vec3(1.0,0.0,0.0), dist);
			//gl_FragColor = vec4(rgb, 1.0);//floor(dist + 0.500));

			gl_FragColor = vec4(vColor.rgb, vColor.a * alpha);

			//gl_FragColor = vec4(1.0, 0.0, 0.0, floor(dist + 0.500));
			//gl_FragColor += vec4(0.0, 1.0, 0.0, floor(dist + 0.533));

			//float mu = smoothstep(outline_center-width, outline_center+width, dist);
			//vec3 rgb = mix(outline_color, glyph_color, mu);
			//gl_FragColor = vec4(rgb, max(alpha,mu));

			//vec3 rgb = mix(glow_color, vec3(1.0,1.0,1.0), alpha);
			//float mu = smoothstep(glyph_center, glow_center, sqrt(dist));
			//gl_FragColor = vec4(rgb, mu);//max(alpha,mu));

		}

	)";


};


class ChipMachine {
public:
	ChipMachine();
	void initLua();
	void play(const SongInfo &si);
	void update();
	void render(uint32_t delta);
	void toast(const std::string &txt, int type);

	void set_scrolltext(const std::string &txt);

	MusicDatabase &music_database() { return mdb; }
	MusicPlayerList &music_player() { return player; }

private:

	MusicDatabase mdb;

	MusicPlayerList player;

	MainScreen mainScreen;
	SearchScreen searchScreen;

	TextScreen textScreen;
	std::shared_ptr<TextScreen::TextField> toastField;

	int currentScreen = 0;

	std::unique_ptr<TelnetInterface> telnet;

	utils::vec2i tv0 = { 80, 54 };
	utils::vec2i tv1 = { 636, 520 };

	grappix::Color spectrumColor = 0xffffffff;
	grappix::Color spectrumColorMain = 0xff00aaee;
	grappix::Color spectrumColorSearch = 0xff111155;
	double spectrumHeight = 20.0;
	int spectrumWidth = 24;
	utils::vec2i spectrumPos;
	std::vector<uint8_t> eq;

	uint32_t bgcolor = 0;
	bool starsOn = true;

	std::string code;

	LuaInterpreter lua;
/*
	grappix::Texture starTexture;
	grappix::Program starProgram;
	float starPos = 0.0;
*/
	demofx::StarField starEffect;
	Scroller scrollEffect;
};

}


#endif // CHIP_MACHINE_H
