#pragma once
/*
MIT License

Copyright (c) 2021-2022 L. E. Spalt

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#pragma once

#include <vector>
#include <algorithm>
#include <deque>
#include "Overlay.h"
#include "iracing.h"
#include "Config.h"
#include "OverlayDebug.h"

class OverlayRay : public Overlay
{
public:

	const float DefaultFontSize = 17;

	OverlayRay()
		: Overlay("OverlayRay")
	{}

#ifdef _DEBUG
	virtual bool    canEnableWhileNotDriving() const { return true; }
	virtual bool    canEnableWhileDisconnected() const { return true; }
#endif


protected:

	struct Box
	{
		float x0 = 0;
		float x1 = 0;
		float y0 = 0;
		float y1 = 0;
		float w = 0;
		float h = 0;
		std::string title;
	};

	virtual float2 getDefaultSize()
	{
		return float2(600, 150);
	}

	virtual void onEnable()
	{
		onConfigChanged();
	}

	virtual void onDisable()
	{
		m_text.reset();
	}

	virtual void onConfigChanged()
	{
		// Font stuff
		{
			m_text.reset(m_dwriteFactory.Get());

			const std::string font = g_cfg.getString(m_name, "font", "Arial");
			const float fontSize = g_cfg.getFloat(m_name, "font_size", DefaultFontSize);
			const int fontWeight = g_cfg.getInt(m_name, "font_weight", 500);

			HRCHECK(m_dwriteFactory->CreateTextFormat(toWide(font).c_str(), NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, fontSize, L"en-us", &m_textFormat));
			m_textFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
			m_textFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);

			HRCHECK(m_dwriteFactory->CreateTextFormat(toWide(font).c_str(), NULL, DWRITE_FONT_WEIGHT_BOLD, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, fontSize, L"en-us", &m_textFormatBold));
			m_textFormatBold->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
			m_textFormatBold->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);

			HRCHECK(m_dwriteFactory->CreateTextFormat(toWide(font).c_str(), NULL, DWRITE_FONT_WEIGHT_BOLD, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, fontSize * 1.2f, L"en-us", &m_textFormatLarge));
			m_textFormatLarge->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
			m_textFormatLarge->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);

			HRCHECK(m_dwriteFactory->CreateTextFormat(toWide(font).c_str(), NULL, DWRITE_FONT_WEIGHT_LIGHT, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, fontSize * 0.7f, L"en-us", &m_textFormatSmall));
			m_textFormatSmall->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
			m_textFormatSmall->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);

			HRCHECK(m_dwriteFactory->CreateTextFormat(toWide(font).c_str(), NULL, DWRITE_FONT_WEIGHT_BOLD, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, fontSize * 0.8f, L"en-us", &m_textFormatSmall2));
			m_textFormatSmall2->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
			m_textFormatSmall2->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);

			HRCHECK(m_dwriteFactory->CreateTextFormat(toWide(font).c_str(), NULL, DWRITE_FONT_WEIGHT_LIGHT, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, fontSize * 0.6f, L"en-us", &m_textFormatVerySmall));
			m_textFormatVerySmall->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
			m_textFormatVerySmall->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);

			HRCHECK(m_dwriteFactory->CreateTextFormat(toWide(font).c_str(), NULL, DWRITE_FONT_WEIGHT_BOLD, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, fontSize * 3.0f, L"en-us", &m_textFormatGear));
			m_textFormatGear->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
			m_textFormatGear->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
		}

		// Background geometry
		{
			Microsoft::WRL::ComPtr<ID2D1GeometrySink>  geometrySink;
			m_d2dFactory->CreatePathGeometry(&m_backgroundPathGeometry);
			m_backgroundPathGeometry->Open(&geometrySink);

			const float w = (float)m_width;
			const float h = (float)m_height;

			geometrySink->BeginFigure(float2(0, h), D2D1_FIGURE_BEGIN_FILLED);
			geometrySink->AddLine(float2(0, 0));
			geometrySink->AddLine(float2(0, h));
			geometrySink->AddLine(float2(w, h));
			geometrySink->AddLine(float2(w, 0));
			geometrySink->AddLine(float2(0, 0));

			geometrySink->EndFigure(D2D1_FIGURE_END_CLOSED);

			geometrySink->Close();
		}

		// Box geometries
		{
			Microsoft::WRL::ComPtr<ID2D1GeometrySink>  geometrySink;
			m_d2dFactory->CreatePathGeometry(&m_boxPathGeometry);
			m_boxPathGeometry->Open(&geometrySink);

			const int borderSize = g_cfg.getInt(m_name, "border_size", 20);
			const float boxHeight = (float)m_height;
			const float boxWidth = (float)m_width;
			const float contentHeight = boxHeight - (2 * borderSize);
			const float contentWidth = boxWidth - (2 * borderSize);

			const float w1 = contentWidth / 7;
			const float w2 = (2 * w1) - borderSize;
			const float h1 = (contentHeight - (2 * borderSize)) / 3;
			const float h2 = (h1 * 2) + borderSize;
			const float h3 = (h1 * 3) + (2 * borderSize);

			const float col1 = (boxWidth / 2) - (contentWidth / 2);
			const float col2 = col1 + w2 + borderSize;
			const float col3 = col2 + w2 + borderSize;
			const float col4 = col3 + w2 + borderSize;
			const float row1 = (boxHeight / 2) - (contentHeight / 2);
			const float row2 = row1 + h1 + borderSize;
			const float row3 = row2 + h1 + borderSize;

			m_boxFuel = makeBoxPixel(col1, w2, row1, h3, "Fuel");
			addBoxFigure(geometrySink.Get(), m_boxFuel);

			m_boxBest = makeBoxPixel(col2, w2, row1, h1, "Best");
			addBoxFigure(geometrySink.Get(), m_boxBest);

			m_boxLast = makeBoxPixel(col2, w2, row2, h1, "Last");
			addBoxFigure(geometrySink.Get(), m_boxLast);

			m_boxTires = makeBoxPixel(col2, w2, row3, h1, "Tires");
			addBoxFigure(geometrySink.Get(), m_boxTires);

			m_boxSession = makeBoxPixel(col3, w2, row1, h1, "Session");
			addBoxFigure(geometrySink.Get(), m_boxSession);

			m_boxLaps = makeBoxPixel(col3, w2, row2, h2, "Lap");
			addBoxFigure(geometrySink.Get(), m_boxLaps);

			m_boxPos = makeBoxPixel(col4, w1, row1, h1, "Pos");
			addBoxFigure(geometrySink.Get(), m_boxPos);

			m_boxBias = makeBoxPixel(col4, w1, row2, h1, "Bias");
			addBoxFigure(geometrySink.Get(), m_boxBias);

			m_boxInc = makeBoxPixel(col4, w1, row3, h1, "Inc");
			addBoxFigure(geometrySink.Get(), m_boxInc);

			geometrySink->Close();
		}
	}

	virtual void onSessionChanged()
	{
		m_isValidFuelLap = false;  // avoid confusing the fuel calculator logic with session changes
	}

	virtual void onUpdate()
	{
		const float  fontSize = g_cfg.getFloat(m_name, "font_size", DefaultFontSize);
		const float4 outlineCol = g_cfg.getFloat4(m_name, "outline_col", float4(0.7f, 0.7f, 0.7f, 0.9f));
		const float4 textCol = g_cfg.getFloat4(m_name, "text_col", float4(1, 1, 1, 0.9f));
		const float4 goodCol = g_cfg.getFloat4(m_name, "good_col", float4(0, 0.8f, 0, 0.6f));
		const float4 badCol = g_cfg.getFloat4(m_name, "bad_col", float4(0.8f, 0.1f, 0.1f, 0.6f));
		const float4 fastestCol = g_cfg.getFloat4(m_name, "fastest_col", float4(0.8f, 0, 0.8f, 0.6f));
		const float4 serviceCol = g_cfg.getFloat4(m_name, "service_col", float4(0.36f, 0.61f, 0.84f, 1));
		const float4 warnCol = g_cfg.getFloat4(m_name, "warn_col", float4(1, 0.6f, 0, 1));

		const float4 normalCol = g_cfg.getFloat4(m_name, "normal_col", float4(0.125f, 0.125f, 0.125f, 1.0f));


		const int  carIdx = ir_session.driverCarIdx;
		const bool imperial = ir_DisplayUnits.getInt() == 0;

		const DWORD tickCount = GetTickCount();

		// General lap info
		const bool   sessionIsTimeLimited = ir_SessionLapsTotal.getInt() == 32767 && ir_SessionTimeRemain.getDouble() < 48.0 * 3600.0;  // most robust way I could find to figure out whether this is a time-limited session (info in session string is often misleading)
		const double remainingSessionTime = sessionIsTimeLimited ? ir_SessionTimeRemain.getDouble() : -1;
		const int    remainingLaps = sessionIsTimeLimited ? int(0.5 + remainingSessionTime / ir_estimateLaptime()) : (ir_SessionLapsRemainEx.getInt() != 32767 ? ir_SessionLapsRemainEx.getInt() : -1);
		const int    currentLap = ir_isPreStart() ? 0 : std::max(0, ir_CarIdxLap.getInt(carIdx));
		const bool   lapCountUpdated = currentLap != m_prevCurrentLap;
		m_prevCurrentLap = currentLap;
		if (lapCountUpdated)
			m_lastLapChangeTickCount = tickCount;

		dbg("isUnlimitedTime: %d, isUnlimitedLaps: %d, rem laps: %d, total laps: %d, rem time: %f", (int)ir_session.isUnlimitedTime, (int)ir_session.isUnlimitedLaps, ir_SessionLapsRemainEx.getInt(), ir_SessionLapsTotal.getInt(), ir_SessionTimeRemain.getFloat());

		wchar_t s[512];

		m_renderTarget->BeginDraw();
		m_brush->SetColor(textCol);

		// Background
		{
			m_renderTarget->Clear(float4(0, 0, 0, 0));
			m_brush->SetColor(g_cfg.getFloat4(m_name, "background_col", float4(0, 0, 0, 0.9f)));
			m_renderTarget->FillGeometry(m_backgroundPathGeometry.Get(), m_brush.Get());
		}

		// Laps
		{
			DrawModuleBG(m_boxLaps, normalCol);
			m_brush->SetColor(textCol);

			char lapsStr[32];

			const int totalLaps = ir_SessionLapsTotal.getInt();

			if (totalLaps == SHRT_MAX)
				sprintf(lapsStr, "--");
			else
				sprintf(lapsStr, "%d", totalLaps);
			swprintf(s, _countof(s), L"%d / %S", currentLap, lapsStr);
			m_text.render(m_renderTarget.Get(), s, m_textFormat.Get(), m_boxLaps.x0, m_boxLaps.x1, m_boxLaps.y0 + m_boxLaps.h * 0.275f, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_CENTER);

			if (remainingLaps < 0)
				sprintf(lapsStr, "--");
			else if (sessionIsTimeLimited)
				sprintf(lapsStr, "~%d", remainingLaps);
			else
				sprintf(lapsStr, "%d", remainingLaps);
			swprintf(s, _countof(s), L"%S", lapsStr);
			m_text.render(m_renderTarget.Get(), s, m_textFormatLarge.Get(), m_boxLaps.x0, m_boxLaps.x1, m_boxLaps.y0 + m_boxLaps.h * 0.6f, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_CENTER);
			m_text.render(m_renderTarget.Get(), L"TO GO", m_textFormatVerySmall.Get(), m_boxLaps.x0, m_boxLaps.x1, m_boxLaps.y0 + m_boxLaps.h * 0.80f, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_CENTER);
		}

		// Position
		{
			DrawModuleBG(m_boxPos, normalCol);
			m_brush->SetColor(textCol);

			int pos = ir_getPosition(ir_session.driverCarIdx);
			if (pos)
			{
				swprintf(s, _countof(s), L"%d", pos);
				std::string str = "P" + std::to_string(pos);
				m_text.render(m_renderTarget.Get(), toWide(str).c_str(), m_textFormatLarge.Get(), m_boxPos.x0, m_boxPos.x1, m_boxPos.y0 + m_boxPos.h * 0.5f, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_CENTER);
			}
			else
			{
				m_text.render(m_renderTarget.Get(), toWide("-").c_str(), m_textFormatLarge.Get(), m_boxPos.x0, m_boxPos.x1, m_boxPos.y0 + m_boxPos.h * 0.5f, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_CENTER);
			}
		}

		// Best time
		{
			// Figure out if we have the fastest lap across all cars
			bool haveFastestLap = false;
			{
				int fastestLapCarIdx = -1;
				float fastest = FLT_MAX;
				for (int i = 0; i < IR_MAX_CARS; ++i)
				{
					const Car& car = ir_session.cars[i];
					if (car.isPaceCar || car.isSpectator || car.userName.empty())
						continue;

					const float best = ir_CarIdxBestLapTime.getFloat(i);
					if (best > 0 && best < fastest) {
						fastest = best;
						fastestLapCarIdx = i;
					}
				}
				haveFastestLap = fastestLapCarIdx == ir_session.driverCarIdx;
			}

			const float t = ir_LapBestLapTime.getFloat();
			std::string str = "";
			float4 bgColor = normalCol;
			if (t > 0)
			{
				str = formatLaptime(t);

				bool vsb = true;
				if (t < m_prevBestLapTime && tickCount - m_lastLapChangeTickCount < 5000)  // blink
					vsb = (tickCount % 800) < 500;
				else
					m_prevBestLapTime = t;

				if (vsb)
				{
					bgColor = haveFastestLap ? fastestCol : goodCol;
				}
			}
			else
			{
				str = "-";
			}

			DrawModuleBG(m_boxBest, bgColor);
			m_brush->SetColor(textCol);
			m_text.render(m_renderTarget.Get(), toWide("Best").c_str(), m_textFormatSmall.Get(), m_boxBest.x0, m_boxBest.x1, m_boxBest.y0 + m_boxBest.h * 0.25f, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_CENTER);
			m_text.render(m_renderTarget.Get(), toWide(str).c_str(), m_textFormatBold.Get(), m_boxBest.x0, m_boxBest.x1, m_boxBest.y0 + m_boxBest.h * 0.7f, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_CENTER);

		}

		// Last time
		{
			const float t = ir_LapLastLapTime.getFloat();
			std::string str = "";
			if (t > 0)
				str = formatLaptime(t);
			else
				str = "-";

			DrawModuleBG(m_boxLast, normalCol);
			m_brush->SetColor(textCol);
			m_text.render(m_renderTarget.Get(), toWide("Last").c_str(), m_textFormatSmall.Get(), m_boxLast.x0, m_boxLast.x1, m_boxLast.y0 + m_boxLast.h * 0.25f, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_CENTER);
			m_text.render(m_renderTarget.Get(), toWide(str).c_str(), m_textFormatBold.Get(), m_boxLast.x0, m_boxLast.x1, m_boxLast.y0 + m_boxLast.h * 0.7f, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_CENTER);
		}

		// Fuel
		{
			DrawModuleBG(m_boxFuel, normalCol);

			const float xoff = 7;

			// Progress bar
			{
				const float x0 = m_boxFuel.x0 + xoff;
				const float x1 = m_boxFuel.x1 - xoff;
				D2D1_RECT_F r = { x0, m_boxFuel.y0 + 12, x1, m_boxFuel.y0 + m_boxFuel.h * 0.125f };
				m_brush->SetColor(float4(0.5f, 0.5f, 0.5f, 0.5f));
				m_renderTarget->FillRectangle(&r, m_brush.Get());

				const float fuelPct = ir_FuelLevelPct.getFloat();
				r = { x0, m_boxFuel.y0 + 12, x0 + fuelPct * (x1 - x0), m_boxFuel.y0 + m_boxFuel.h * 0.125f };
				m_brush->SetColor(fuelPct < 0.1f ? warnCol : goodCol);
				m_renderTarget->FillRectangle(&r, m_brush.Get());
			}

			m_brush->SetColor(textCol);
			m_text.render(m_renderTarget.Get(), L"Laps", m_textFormat.Get(), m_boxFuel.x0 + xoff, m_boxFuel.x1, m_boxFuel.y0 + m_boxFuel.h * 3.0f / 12.0f, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_LEADING);
			m_text.render(m_renderTarget.Get(), L"Rem", m_textFormatSmall.Get(), m_boxFuel.x0 + xoff, m_boxFuel.x1, m_boxFuel.y0 + m_boxFuel.h * 5.5f / 12.0f, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_LEADING);
			m_text.render(m_renderTarget.Get(), L"Per", m_textFormatSmall.Get(), m_boxFuel.x0 + xoff, m_boxFuel.x1, m_boxFuel.y0 + m_boxFuel.h * 7.25f / 12.0f, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_LEADING);
			m_text.render(m_renderTarget.Get(), L"Fin+", m_textFormatSmall.Get(), m_boxFuel.x0 + xoff, m_boxFuel.x1, m_boxFuel.y0 + m_boxFuel.h * 9.0f / 12.0f, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_LEADING);
			m_text.render(m_renderTarget.Get(), L"Add", m_textFormatSmall.Get(), m_boxFuel.x0 + xoff, m_boxFuel.x1, m_boxFuel.y0 + m_boxFuel.h * 10.75f / 12.0f, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_LEADING);

			const float estimateFactor = g_cfg.getFloat(m_name, "fuel_estimate_factor", 1.1f);
			const float remainingFuel = ir_FuelLevel.getFloat();

			// Update average fuel consumption tracking. Ignore laps that weren't entirely under green or where we pitted.
			float avgPerLap = 0;
			{
				if (lapCountUpdated)
				{
					const float usedLastLap = std::max(0.0f, m_lapStartRemainingFuel - remainingFuel);
					m_lapStartRemainingFuel = remainingFuel;

					if (m_isValidFuelLap)
						m_fuelUsedLastLaps.push_back(usedLastLap);

					const int numLapsToAvg = g_cfg.getInt(m_name, "fuel_estimate_avg_green_laps", 4);
					while (m_fuelUsedLastLaps.size() > numLapsToAvg)
						m_fuelUsedLastLaps.pop_front();

					m_isValidFuelLap = true;
				}
				if ((ir_SessionFlags.getInt() & (irsdk_yellow | irsdk_yellowWaving | irsdk_red | irsdk_checkered | irsdk_crossed | irsdk_oneLapToGreen | irsdk_caution | irsdk_cautionWaving | irsdk_disqualify | irsdk_repair)) || ir_CarIdxOnPitRoad.getBool(carIdx))
					m_isValidFuelLap = false;

				for (float v : m_fuelUsedLastLaps) {
					avgPerLap += v;
					dbg("%f", v);
				}
				if (!m_fuelUsedLastLaps.empty())
					avgPerLap /= (float)m_fuelUsedLastLaps.size();

				dbg("valid fuel lap: %d", (int)m_isValidFuelLap);
			}

			// Est Laps
			const float perLapConsEst = avgPerLap * estimateFactor;  // conservative estimate of per-lap use for further calculations
			if (perLapConsEst > 0)
			{
				const float estLaps = remainingFuel / perLapConsEst;
				swprintf(s, _countof(s), L"%.*f", estLaps < 10 ? 1 : 0, estLaps);
				m_text.render(m_renderTarget.Get(), s, m_textFormatLarge.Get(), m_boxFuel.x0, m_boxFuel.x1 - xoff, m_boxFuel.y0 + m_boxFuel.h * 3.0f / 12.0f, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_TRAILING);
			}

			// Remaining
			if (remainingFuel >= 0)
			{
				float val = remainingFuel;
				if (imperial)
					val *= 0.264172f;
				swprintf(s, _countof(s), imperial ? L"%.2f gl" : L"%.2f lt", val);
				m_text.render(m_renderTarget.Get(), s, m_textFormatSmall2.Get(), m_boxFuel.x0, m_boxFuel.x1 - xoff, m_boxFuel.y0 + m_boxFuel.h * 5.5f / 12.0f, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_TRAILING);
			}

			// Per Lap
			if (avgPerLap > 0)
			{
				float val = avgPerLap;
				if (imperial)
					val *= 0.264172f;
				swprintf(s, _countof(s), imperial ? L"%.2f gl" : L"%.2f lt", val);
				m_text.render(m_renderTarget.Get(), s, m_textFormatSmall2.Get(), m_boxFuel.x0, m_boxFuel.x1 - xoff, m_boxFuel.y0 + m_boxFuel.h * 7.25f / 12.0f, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_TRAILING);
			}

			// To Finish
			if (remainingLaps >= 0 && perLapConsEst > 0)
			{
				float toFinish = std::max(0.0f, remainingLaps * perLapConsEst - remainingFuel);

				if (toFinish > ir_PitSvFuel.getFloat() || (toFinish > 0 && !ir_dpFuelFill.getFloat()))
					m_brush->SetColor(warnCol);
				else
					m_brush->SetColor(goodCol);

				if (imperial)
					toFinish *= 0.264172f;
				swprintf(s, _countof(s), imperial ? L"%3.1f gl" : L"%3.1f lt", toFinish);
				m_text.render(m_renderTarget.Get(), s, m_textFormatSmall2.Get(), m_boxFuel.x0, m_boxFuel.x1 - xoff, m_boxFuel.y0 + m_boxFuel.h * 9.0f / 12.0f, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_TRAILING);
				m_brush->SetColor(textCol);
			}

			// Add
			float add = ir_PitSvFuel.getFloat();
			if (add >= 0)
			{
				if (ir_dpFuelFill.getFloat())
					m_brush->SetColor(serviceCol);

				if (imperial)
					add *= 0.264172f;
				swprintf(s, _countof(s), imperial ? L"%3.1f gl" : L"%3.1f lt", add);
				m_text.render(m_renderTarget.Get(), s, m_textFormatSmall2.Get(), m_boxFuel.x0, m_boxFuel.x1 - xoff, m_boxFuel.y0 + m_boxFuel.h * 10.75f / 12.0f, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_TRAILING);
				m_brush->SetColor(textCol);
			}
		}

		// Tires
		{
			DrawModuleBG(m_boxTires, normalCol);

			const float lf = 100.0f * std::min(std::min(ir_LFwearL.getFloat(), ir_LFwearM.getFloat()), ir_LFwearR.getFloat());
			const float rf = 100.0f * std::min(std::min(ir_RFwearL.getFloat(), ir_RFwearM.getFloat()), ir_RFwearR.getFloat());
			const float lr = 100.0f * std::min(std::min(ir_LRwearL.getFloat(), ir_LRwearM.getFloat()), ir_LRwearR.getFloat());
			const float rr = 100.0f * std::min(std::min(ir_RRwearL.getFloat(), ir_RRwearM.getFloat()), ir_RRwearR.getFloat());

			// Left
			if (ir_dpLTireChange.getFloat())
				m_brush->SetColor(serviceCol);
			else
				m_brush->SetColor(textCol);
			swprintf(s, _countof(s), L"%d", (int)(lf + 0.5f));
			m_text.render(m_renderTarget.Get(), s, m_textFormatSmall.Get(), m_boxTires.x0, m_boxTires.x0 + m_boxTires.w / 2 - 20, m_boxTires.y0 + m_boxTires.h * 0.3f, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_CENTER);
			swprintf(s, _countof(s), L"%d", (int)(lr + 0.5f));
			m_text.render(m_renderTarget.Get(), s, m_textFormatSmall.Get(), m_boxTires.x0, m_boxTires.x0 + m_boxTires.w / 2 - 20, m_boxTires.y0 + m_boxTires.h * 0.7f, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_CENTER);

			// Right
			if (ir_dpRTireChange.getFloat())
				m_brush->SetColor(serviceCol);
			else
				m_brush->SetColor(textCol);
			swprintf(s, _countof(s), L"%d", (int)(rf + 0.5f));
			m_text.render(m_renderTarget.Get(), s, m_textFormatSmall.Get(), m_boxTires.x0 + m_boxTires.w / 2 + 20, m_boxTires.x1, m_boxTires.y0 + m_boxTires.h * 0.3f, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_CENTER);
			swprintf(s, _countof(s), L"%d", (int)(rr + 0.5f));
			m_text.render(m_renderTarget.Get(), s, m_textFormatSmall.Get(), m_boxTires.x0 + m_boxTires.w / 2 + 20, m_boxTires.x1, m_boxTires.y0 + m_boxTires.h * 0.7f, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_CENTER);

			m_brush->SetColor(textCol);
			m_text.render(m_renderTarget.Get(), toWide("Tires").c_str(), m_textFormatSmall.Get(), m_boxTires.x0, m_boxTires.x1, m_boxTires.y0 + m_boxTires.h * 0.45f, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_CENTER);

			/* TODO: why doesn't iracing report 255 here in an AI session where we DO have unlimited tire sets??

			// Left available
			int avail = ir_LeftTireSetsAvailable.getInt();
			if( avail < 255 )
			{
				swprintf( s, _countof(s), L"%d", avail );
				m_text.render( m_renderTarget.Get(), s, m_textFormatSmall.Get(), m_boxTires.x0, m_boxTires.x0+m_boxTires.w/4, m_boxTires.y0+m_boxTires.h*0.5f, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_CENTER );
			}

			// Right available
			avail = ir_RightTireSetsAvailable.getInt();
			if( avail < 255 )
			{
				swprintf( s, _countof(s), L"%d", avail );
				m_text.render( m_renderTarget.Get(), s, m_textFormatSmall.Get(), m_boxTires.x0+m_boxTires.w*3.0f/4.0f, m_boxTires.x1, m_boxTires.y0+m_boxTires.h*0.5f, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_CENTER );
			}
			*/
		}

		// Session
		{

			const double sessionTime = remainingSessionTime >= 0 ? remainingSessionTime : ir_SessionTime.getDouble();

			const int    hours = int(sessionTime / 3600.0);
			const int    mins = int(sessionTime / 60.0) % 60;
			const int    secs = (int)fmod(sessionTime, 60.0);
			if (hours)
				swprintf(s, _countof(s), L"%d:%02d:%02d", hours, mins, secs);
			else
				swprintf(s, _countof(s), L"%02d:%02d", mins, secs);

			DrawModuleBG(m_boxSession, normalCol);
			m_brush->SetColor(textCol);
			m_text.render(m_renderTarget.Get(), toWide("Session").c_str(), m_textFormatSmall.Get(), m_boxSession.x0, m_boxSession.x1, m_boxSession.y0 + m_boxSession.h * 0.25f, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_CENTER);
			m_text.render(m_renderTarget.Get(), s, m_textFormat.Get(), m_boxSession.x0, m_boxSession.x1, m_boxSession.y0 + m_boxSession.h * 0.7f, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_CENTER);
		}

		// Incidents
		{
			const int inc = ir_PlayerCarMyIncidentCount.getInt();
			swprintf(s, _countof(s), L"%dx", inc);

			DrawModuleBG(m_boxInc, normalCol);
			m_brush->SetColor(textCol);
			m_text.render(m_renderTarget.Get(), s, m_textFormatBold.Get(), m_boxInc.x0, m_boxInc.x1, m_boxInc.y0 + m_boxInc.h * 0.5f, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_CENTER);
		}

		// Brake bias
		{
			const float bias = ir_dcBrakeBias.getFloat();
			swprintf(s, _countof(s), L"%+3.1f", bias);

			DrawModuleBG(m_boxBias, normalCol);
			m_brush->SetColor(textCol);
			m_text.render(m_renderTarget.Get(), s, m_textFormatBold.Get(), m_boxBias.x0, m_boxBias.x1, m_boxBias.y0 + m_boxBias.h * 0.5f, m_brush.Get(), DWRITE_TEXT_ALIGNMENT_CENTER);
		}

		m_renderTarget->EndDraw();
	}

	void DrawModuleBG(const Box box, const float4& colorBG)
	{
		D2D1_RECT_F r = { box.x0, box.y0, box.x1, box.y1 };
		m_brush->SetColor(colorBG);
		m_renderTarget->FillRectangle(&r, m_brush.Get());
	}

	void addBoxFigure(ID2D1GeometrySink* geometrySink, const Box& box)
	{
		geometrySink->BeginFigure(float2(box.x0, box.y0), D2D1_FIGURE_BEGIN_FILLED);
		geometrySink->AddLine(float2(box.x0, box.y1));
		geometrySink->AddLine(float2(box.x1, box.y1));
		geometrySink->AddLine(float2(box.x1, box.y0));
		geometrySink->EndFigure(D2D1_FIGURE_END_CLOSED);
	}

	float r2ax(float rx)
	{
		return rx * (float)m_width;
	}

	float r2ay(float ry)
	{
		return ry * (float)m_height;
	}

	Box makeBox(float x0, float w, float y0, float h, const std::string& title)
	{
		Box r;

		if (w <= 0 || h <= 0)
			return r;

		r.x0 = r2ax(x0);
		r.x1 = r2ax(x0 + w);
		r.y0 = r2ay(y0);
		r.y1 = r2ay(y0 + h);
		r.w = r.x1 - r.x0;
		r.h = r.y1 - r.y0;
		r.title = title;
		return r;
	}

	Box makeBoxPixel(float x0, float w, float y0, float h, const std::string& title)
	{
		Box r;

		if (w <= 0 || h <= 0)
			return r;

		r.x0 = x0;
		r.x1 = x0 + w;
		r.y0 = y0;
		r.y1 = y0 + h;
		r.w = r.x1 - r.x0;
		r.h = r.y1 - r.y0;
		r.title = title;
		return r;
	}

protected:

	virtual bool hasCustomBackground()
	{
		return true;
	}

	Box m_boxGear;
	Box m_boxLaps;
	Box m_boxPos;
	Box m_boxLapDelta;
	Box m_boxBest;
	Box m_boxLast;
	Box m_boxP1Last;
	Box m_boxDelta;
	Box m_boxSession;
	Box m_boxInc;
	Box m_boxBias;
	Box m_boxFuel;
	Box m_boxTires;
	Box m_boxOil;
	Box m_boxWater;

	Microsoft::WRL::ComPtr<IDWriteTextFormat>  m_textFormat;
	Microsoft::WRL::ComPtr<IDWriteTextFormat>  m_textFormatBold;
	Microsoft::WRL::ComPtr<IDWriteTextFormat>  m_textFormatLarge;
	Microsoft::WRL::ComPtr<IDWriteTextFormat>  m_textFormatSmall;
	Microsoft::WRL::ComPtr<IDWriteTextFormat>  m_textFormatSmall2;
	Microsoft::WRL::ComPtr<IDWriteTextFormat>  m_textFormatVerySmall;
	Microsoft::WRL::ComPtr<IDWriteTextFormat>  m_textFormatGear;

	Microsoft::WRL::ComPtr<ID2D1PathGeometry1> m_boxPathGeometry;
	Microsoft::WRL::ComPtr<ID2D1PathGeometry1> m_backgroundPathGeometry;

	TextCache           m_text;

	int                 m_prevCurrentLap = 0;
	DWORD               m_lastLapChangeTickCount = 0;

	float               m_prevBestLapTime = 0;

	float               m_lapStartRemainingFuel = 0;
	std::deque<float>   m_fuelUsedLastLaps;
	bool                m_isValidFuelLap = false;
};

