#ifndef __NES_FILTER_H
#define __NES_FILTER_H

class NES_Resampler
{
	public:

	enum { MaxLeftover = 1536 };
	enum { MaxWaveOverRead = 16 };	// SSE2

	// Resamples from input_rate to output_rate, allowing for rate_error(output_rate +/- output_rate*rate_error)
	// error in the resample ratio.
	//
	// hp_tc is the time constant(in seconds) for the DC bias removal highpass filter.
	// Higher values will result in more bias removal(and cause a high-pass filter effect), while lower values will lower this effect.
	// Values lower than 200e-6, other than 0, may cause assert()'s to be triggered at some output rates due to overflow issues.
	// A value of 0 will disable it entirely.
	//
	// quality is an arbitrary control of quality(-2 for lowest quality, 3 for highest quality)
	NES_Resampler(double input_rate, double output_rate, double rate_error, double hp_tc, int quality) MDFN_COLD;
	NES_Resampler(const NES_Resampler &resamp) MDFN_COLD;
	~NES_Resampler() MDFN_COLD;

	// Specify volume in percent amplitude(range 0 through 1.000..., inclusive).  Default: SetVolume(1.0);
	void SetVolume(double newvolume);

	// Resamples "inlen" samples from "in", writing the output "out", generating no more output samples than "maxoutlen".
	// The int32 pointed to by leftover is set to the number of input samples left over from this filter iteration.
	// "in" should be aligned to a 16-byte boundary, for the SIMD versions of the filter to work properly.  "out" doesn't have any
	// special alignment requirement.
	NO_INLINE int32 Do(int16 *in, int16 *out, uint32 maxoutlen, uint32 inlen, int32 *leftover);

	// Get the InputRate / OutputRate ratio, expressed as a / b
	void GetRatio(int32 *a, int32 *b)
	{
	 *a = Ratio_Dividend;
	 *b = Ratio_Divisor;
	}

	private:

	// Copy of the parameters passed to the constructor
	double InputRate, OutputRate, RateError;
	int Quality;

        // Number of phases.
        uint32 NumPhases;
	uint32 NumPhases_Padded;

	// Coefficients(in each phase, not total)
	uint32 NumCoeffs;
	uint32 NumCoeffs_Padded;

	uint32 NumAlignments;

	// Index into the input buffer
	uint32 InputIndex;

	// Current input phase
	uint32 InputPhase;

	// In the FIR loop:  InputPhase = PhaseNext[InputPhase]
	std::unique_ptr<uint32[]> PhaseNext;

	// Incrementor for InputIndex.  In the FIR loop, after updating InputPhase:  InputIndex += PhaseStep[InputPhase]
	std::unique_ptr<uint32[]> PhaseStep;

	// One pointer for each phase in each possible alignment
	std::unique_ptr<int16*[]> FIR_Coeffs;

	#define FIR_ENTRY(align, phase, coco) FIR_Coeffs[(phase) * NumAlignments + align][coco]

	// Coefficient counts for the 4 alignments
	std::unique_ptr<uint32[]> FIR_CoCounts;

	int32 SoundVolume;
	std::vector<int16> CoeffsBuffer;
	std::vector<int32> IntermediateBuffer;

	enum
	{
	 SIMD_NONE,
	 SIMD_MMX,
	 SIMD_SSE2,
	 SIMD_ALTIVEC,
	 SIMD_NEON
	};
	uint32 SIMD_Type;

	int32 debias;
	int32 debias_multiplier;

	// for GetRatio()
	int32 Ratio_Dividend;
	int32 Ratio_Divisor;
};
#endif
