/*
  ==============================================================================

    FileExporter.h
    Created: 20 Jul 2020 12:35:28am
    Author:  Matthew

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include <thread>

class SliceExporter
{
public:
    SliceExporter(juce::AudioFormatManager& audioFormatManager);
    ~SliceExporter() = default;
    
    void startExport(juce::AudioSampleBuffer* readBuffer, juce::String &fileName, juce::String &absoluteDirectoryPath, int64_t sliceSize, int numSlices, int numChannels, double sampleRate, int bitDepth, const int blockSize = 64);
    void stopExport();

private:
    void run(std::unique_ptr<juce::AudioFormatWriter> const& writer, juce::Range<int64_t> const& sampleRange, const int blockSize);
    
    juce::AudioFormatManager& mFormatManager;
    juce::AudioSampleBuffer mReadBuffer;
    
    std::shared_ptr<juce::AudioFormatWriter::ThreadedWriter> mThreadedWriter;
    std::thread mThread;
    std::mutex mMutex;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SliceExporter);
};
