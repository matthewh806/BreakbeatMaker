/*
  ==============================================================================

    FileRecorder.cpp
    Created: 19 Jul 2020 7:52:46pm
    Author:  Matthew

  ==============================================================================
*/

#include "FileRecorder.h"

FileRecorder::FileRecorder(juce::AudioFormatManager& audioFormatManager)
: mFormatManager(audioFormatManager)
, mBackgroundThread("Audio Recording Thread")
{
    mBackgroundThread.startThread();
}

bool FileRecorder::startRecording(juce::File file, int numChannels, double sampleRate, int bitDepth)
{
    auto* audioFormat = mFormatManager.findFormatForFileExtension(file.getFileExtension());
    if(audioFormat == nullptr)
    {
        stopRecording();
        return false;
    }
    
    if(!file.existsAsFile())
    {
        file.create();
    }
    
    if(!file.existsAsFile())
    {
        stopRecording();
        return false;
    }
    
    auto outputStream = std::unique_ptr<juce::FileOutputStream>(file.createOutputStream());
    if(outputStream == nullptr)
    {
        stopRecording();
        return false;
    }
    
    auto writer = std::unique_ptr<juce::AudioFormatWriter>(audioFormat->createWriterFor(outputStream.get(), sampleRate, numChannels, bitDepth, {}, 0));
    if(writer == nullptr)
    {
        stopRecording();
        return false;
    }
    outputStream.release();
    
    auto threadedWriter = std::make_shared<juce::AudioFormatWriter::ThreadedWriter>(writer.get(), mBackgroundThread, 32768);
    if(threadedWriter == nullptr)
    {
        stopRecording();
        return false;
    }
    
    threadedWriter = std::atomic_exchange(&mThreadedWriter, threadedWriter);
    mRecordingDuration = 0;
    writer.release();
    
    juce::MessageManager::callAsync([ptr = std::move(threadedWriter)]()
                                    {
                                        while(ptr.use_count() > 1) { }
                                    });
    
    std::cout << "Start Recording on background thread: " << file.getFullPathName() << "\n";
    
    return true;
}

void FileRecorder::stopRecording()
{
    std::cout << "Stop Recording on background thread\n";
    
    juce::MessageManager::callAsync([ptr = std::move(mThreadedWriter)]()
    {
        while(ptr.use_count() > 1) { }
    });
}

void FileRecorder::processBlock(juce::AudioBuffer<float> const& buffer)
{
    if(auto writer = mThreadedWriter)
    {
        writer->write(buffer.getArrayOfReadPointers(), buffer.getNumSamples());
        mRecordingDuration = mRecordingDuration + static_cast<juce::int64>(buffer.getNumSamples());
    }
}
