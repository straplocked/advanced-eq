#include "CascadedFilter.h"
#include <cmath>

static constexpr double kPi = 3.14159265358979323846;

void CascadedFilter::prepare (double sr)
{
    sampleRate = sr;
    for (auto& section : sections)
        section.prepare (sr);
}

void CascadedFilter::reset()
{
    for (auto& section : sections)
        section.reset();
}

void CascadedFilter::setParameters (FilterType type, double freq, double Q,
                                     double gainDB, int slopeDBPerOct)
{
    numActiveSections = slopeToSections (slopeDBPerOct);

    if (numActiveSections <= 0)
        numActiveSections = 1;

    switch (type)
    {
        case FilterType::Bell:
        case FilterType::Notch:
        case FilterType::BandPass:
        case FilterType::TiltShelf:
        case FilterType::FlatTilt:
        {
            // For bell/notch/bandpass: use single section regardless of slope
            // (slope primarily affects cut/shelf filters)
            numActiveSections = 1;
            sections[0].setParameters (type, freq, Q, gainDB);
            break;
        }

        case FilterType::LowShelf:
        case FilterType::HighShelf:
        {
            if (numActiveSections == 1)
            {
                sections[0].setParameters (type, freq, Q, gainDB);
            }
            else
            {
                // Distribute gain across sections for steeper shelves
                double perSectionGain = gainDB / numActiveSections;
                for (int i = 0; i < numActiveSections; ++i)
                    sections[i].setParameters (type, freq, Q, perSectionGain);
            }
            break;
        }

        case FilterType::HighPass:
        case FilterType::LowPass:
        {
            if (numActiveSections == 1)
            {
                // Single second-order section (12 dB/oct)
                // For 6 dB/oct, we use a low Q to approximate first-order
                double actualQ = (slopeDBPerOct <= 6) ? 0.5 : Q;
                sections[0].setParameters (type, freq, actualQ, 0.0);
            }
            else
            {
                // Butterworth cascade: each section gets a specific Q
                // based on Butterworth pole placement
                int order = numActiveSections * 2;
                for (int i = 0; i < numActiveSections; ++i)
                {
                    // Butterworth Q for each second-order section
                    double angle = kPi * (2.0 * i + 1.0) / (2.0 * order);
                    double butterworthQ = 1.0 / (2.0 * std::cos (angle));
                    sections[i].setParameters (type, freq, butterworthQ, 0.0);
                }
            }
            break;
        }
    }
}

float CascadedFilter::processSample (float input)
{
    float output = input;
    for (int i = 0; i < numActiveSections; ++i)
        output = sections[i].processSample (output);
    return output;
}

void CascadedFilter::process (float* buffer, int numSamples)
{
    for (int s = 0; s < numActiveSections; ++s)
        sections[s].process (buffer, numSamples);
}

std::complex<double> CascadedFilter::getResponse (double freq) const
{
    std::complex<double> composite (1.0, 0.0);
    for (int i = 0; i < numActiveSections; ++i)
        composite *= sections[i].getResponse (freq);
    return composite;
}

double CascadedFilter::getMagnitudeAtFrequency (double freq) const
{
    auto response = getResponse (freq);
    return 20.0 * std::log10 (std::max (std::abs (response), 1e-10));
}
