#include<iostream>
#include"pch.h"
using namespace std;

#define FTYPE double 
#include "olcNoiseMaker.h"


namespace synth {
	FTYPE w(FTYPE dHertz)
	{
		return dHertz * 2.0 * PI;
	}

	const int OSC_SINE = 0;
	const int OSC_SQUARE = 1;
	const int OSC_TRIANGLE = 2;
	const int OSC_SAW_ANALOG = 3;
	const int OSC_SAW_DIGITAL = 4;
	const int OSC_NOISE = 5;


	FTYPE osc(const FTYPE dTime, const FTYPE dHertz, const int nType = OSC_SINE, const FTYPE dFLOHertz = 0.0, const FTYPE  dFLOAmplitude = 0.0, FTYPE dCustom = 50.0)
	{
		FTYPE dFreq = w(dHertz)*dTime + dFLOAmplitude * dHertz * sin(w(dFLOHertz) *dTime);

		switch (nType)
		{
		case OSC_SINE:// Sine wave
			return  sin(dFreq);
		case OSC_SQUARE: // Square wave
			return sin(dFreq) > 00 ? 1.0 : -1.0;
		case OSC_TRIANGLE:// Triangle Wave
			return asin(sin(dFreq)) * 2.0 * PI;
		case OSC_SAW_ANALOG:
		{
			FTYPE dOutput = 0.0;

			for (FTYPE n = 1.0; n < 100.0; n++)
				dOutput += (sin(n*dFreq)) / n;

			return dOutput * (2.0 / PI);
		}

		case OSC_SAW_DIGITAL:
			return (2.0 / PI)*(dHertz*PI*fmod(dTime, 1.0 / dHertz) - (PI / 2.0));

		case OSC_NOISE:
			return 2.0 * ((FTYPE)rand() / (FTYPE)RAND_MAX) - 1.0;
		default:
			return 0;
		}
	}

	const int SCALE_DEFAULT = 0;

	FTYPE scale(const int nNoteID, const int nScaleID = SCALE_DEFAULT)
	{
		switch (nScaleID)
		{
		case SCALE_DEFAULT: default:
			return 256 * pow(1.0594630943592952645618252949463, nNoteID);
		}
	}

	struct note
	{
		int id;    //Position in scale
		FTYPE on;  // Time note was activated
		FTYPE off; // Time note was deactivated
		bool activate;
		int channel;

		note()
		{
			id = 0;
			on = 0.0;
			off = 0.0;
			activate = false;
			channel = 0;
		}
	};

	struct envelope
	{
		virtual FTYPE amplitude(const FTYPE dTime, const FTYPE dTimeOn, const FTYPE dTimeOff) = 0;
	};

	struct envelope_adsr : public envelope
	{
		FTYPE dAttackTime;
		FTYPE dDecayTime;
		FTYPE dSustainAmplitude;
		FTYPE dReleaseTime;
		FTYPE dStartAmplitude;

		envelope_adsr()
		{
			dAttackTime = 0.1;
			dDecayTime = 0.1;
			dSustainAmplitude = 1.0;
			dReleaseTime = 0.2;
			dStartAmplitude = 1.0;
		}

		virtual FTYPE amplitude(const FTYPE dTime, const FTYPE dTimeOn, const FTYPE dTimeOff)
		{
			FTYPE dAmplitude = 0.0;
			FTYPE dReleaseAmplitude = 0.0;

			if (dTimeOn > dTimeOff) // Note is on
			{
				FTYPE dLifeTime = dTime - dTimeOn;

				if (dLifeTime <= dAttackTime)
					dAmplitude = (dLifeTime / dAttackTime) * dStartAmplitude;

				if (dLifeTime > dAttackTime && dLifeTime <= (dAttackTime + dDecayTime))
					dAmplitude = ((dLifeTime - dAttackTime) / dDecayTime) * (dSustainAmplitude - dStartAmplitude) + dStartAmplitude;

				if (dLifeTime > (dAttackTime + dDecayTime))
					dAmplitude = dSustainAmplitude;
			}
			else // Note is off
			{
				FTYPE dLifeTime = dTimeOff - dTimeOn;

				if (dLifeTime <= dAttackTime)
					dReleaseAmplitude = (dLifeTime / dAttackTime) * dStartAmplitude;

				if (dLifeTime > dAttackTime && dLifeTime <= (dAttackTime + dDecayTime))
					dReleaseAmplitude = ((dLifeTime - dAttackTime) / dDecayTime) * (dSustainAmplitude - dStartAmplitude) + dStartAmplitude;

				if (dLifeTime > (dAttackTime + dDecayTime))
					dReleaseAmplitude = dSustainAmplitude;

				dAmplitude = ((dTime - dTimeOff) / dReleaseTime) * (0.0 - dReleaseAmplitude) + dReleaseAmplitude;
			}

			// Amplitude should not be negative
			if (dAmplitude <= 0.000)
				dAmplitude = 0.0;

			return dAmplitude;
		}
	};

	FTYPE env(const FTYPE dTime, envelope &env, const FTYPE dTimeOn, const FTYPE dTimeOff)
	{
		return env.amplitude(dTime, dTimeOn, dTimeOff);
	}

	struct instrument_base
	{
		FTYPE dVolume;
		synth::envelope_adsr env;
		virtual FTYPE sound(const FTYPE dTime, synth::note n, bool &bNoteFinished) = 0;
	};

	struct instrument_bell : public instrument_base
	{
		instrument_bell8()
		{
			env.dAttackTime = 0.01;
			env.dDecayTime = 0.5;
			env.dSustainAmplitude = 0.8;
			env.dReleaseTime = 1.0;

			dVolume = 1.0;
		}

		virtual FTYPE sound(const FTYPE dTime, synth::note n, bool &bNoteFinished)
		{
			FTYPE dAmplitude = synth::env(dTime, env, n.on, n.off);
			if (dAmplitude <= 0.0) bNoteFinished = true;

			FTYPE dSound =
				+1.00 * synth::osc(n.on - dTime, synth::scale(n.id), synth::OSC_SQUARE, 5.0, 0.001)
				+ 0.50 * synth::osc(n.on - dTime, synth::scale(n.id + 12))
				+ 0.25 * synth::osc(n.on - dTime, synth::scale(n.id + 24));

			return dAmplitude * dSound * dVolume;
		}

	};

	struct instrument_harmonica : public instrument_base
	{
		instrument_harmonica()
		{
			env.dAttackTime = 0.05;
			env.dDecayTime = 1.0;
			env.dSustainAmplitude = 0.95;
			env.dReleaseTime = 0.1;

			dVolume = 1.0;
		}

		virtual FTYPE sound(const FTYPE dTime, synth::note n, bool &bNoteFinished)
		{
			FTYPE dAmplitude = synth::env(dTime, env, n.on, n.off);
			if (dAmplitude <= 0.0) bNoteFinished = true;

			FTYPE dSound =
				//+ 1.0  * synth::osc(n.on - dTime, synth::scale(n.id-12), synth::OSC_SAW_ANA, 5.0, 0.001, 100)
				+1.00 * synth::osc(n.on - dTime, synth::scale(n.id), synth::OSC_SQUARE, 5.0, 0.001)
				+ 0.50 * synth::osc(n.on - dTime, synth::scale(n.id + 12), synth::OSC_SQUARE)
				+ 0.05  * synth::osc(n.on - dTime, synth::scale(n.id + 24), synth::OSC_NOISE);

			return dAmplitude * dSound * dVolume;
		}

	};
}

vector<synth::note> vecNotes;
mutex muxNotes;
synth::instrument_bell instBell;
synth::instrument_harmonica instHarm;

	atomic<FTYPE> dFrequencyOutput = 0.0;
	FTYPE dOctaveBaseFrequency = 220.0; //A2
	FTYPE d12thRootOf2 = pow(2.0, 1.0 / 12.0);
	synth::envelope_adsr envelope;

	synth::instrument_base *voice = nullptr;

	double MakeNoise(FTYPE dTime)
	{
		FTYPE dOutput = voice->sound(dTime, dFrequencyOutput);
		return dOutput * 0.4;
	}



	int main()
	{
		wcout << "A.FUCKING.SYNTHESIZER" << endl;

		//Get all sound hardware
		vector<wstring> devices = olcNoiseMaker<short>::Enumerate();

		//Display findings
		for (auto d : devices)
		{
			wcout << "Found Output Device:" << d << endl;
		}

		//Create sound machine!!
		olcNoiseMaker<short> sound(devices[0], 44100, 1, 8, 512);

		voice = new synth::instrument_bell();
		//Link noise function with sound machine
		sound.SetUserFunction(MakeNoise);


		bool bKeyPressed = false;
		int nCurrentKey = -1;
		while (1)
		{
			//Add a keyboard


			bKeyPressed = false;
			for (int k = 0; k < 15; k++)
			{
				if (GetAsyncKeyState((unsigned char)("ZSXCFVGBNJMK\xbcL\xbe"[k])) & 0x8000)
				{
					if (nCurrentKey != k)
					{
						dFrequencyOutput = dOctaveBaseFrequency * pow(d12thRootOf2, k);
						voice->env.(sound.GetTime());
						nCurrentKey = k;
					}
					bKeyPressed = true;
				}
			}
			if (!bKeyPressed)
			{
				if (nCurrentKey != -1)
				{
					voice->env.NoteOff(sound.GetTime());
					nCurrentKey = -1;
				}
			}
		}
		return 0;
	}