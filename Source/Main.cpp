/*
  ==============================================================================

    This file was auto-generated and contains the startup code for a PIP.

  ==============================================================================
*/

#include <JuceHeader.h>
#include "BreakbeatMaker.h"

class Application    : public JUCEApplication
{
public:
    //==============================================================================
    Application() {}

    const String getApplicationName() override       { return "BreakbeatMaker"; }
    const String getApplicationVersion() override    { return "1.0.0"; }

    void initialise (const String&) override
    {
        mPropertyFile =
            juce::File::getSpecialLocation(juce::File::SpecialLocationType::userApplicationDataDirectory)
            .getChildFile("Application Support").getChildFile("toous").getChildFile("BreakbeatMaker.xml");
        
        mCommandManager.registerAllCommandsForTarget(this);
        loadProperties();
        
        mMainWindow = std::make_unique<MainWindow>("BreakbeatMaker", &mContent, *this);
        
        mMenuModel.setApplicationCommandManagerToWatch(&mCommandManager);
        #if JUCE_MAC && (!defined(JUCE_IOS))
                juce::MenuBarModel::setMacMainMenu(&mMenuModel, nullptr);
        #endif
        
        juce::MessageManager::callAsync([this]()
        {
            if(mRecentFiles.getNumFiles() && mRecentFiles.getFile(0).existsAsFile())
            {
                auto path = mRecentFiles.getFile(0).getFullPathName();
                mContent.newFileOpened(path);
            }
        });
    }
    void shutdown() override
    {
        saveProperties();
        #if JUCE_MAC && (!defined(JUCE_IOS))
                juce::MenuBarModel::setMacMainMenu(nullptr);
        #endif
    }
    
    enum CommandIds
    {
        FileOpenRecent = 0x2002000,
        OutputFilePath = 0x2002001,
        ExportAudioSlices = 0x2002002
    };
    
    juce::ApplicationCommandTarget* getNextCommandTarget() override
    {
        return nullptr;
    }
    
    void getAllCommands(juce::Array<juce::CommandID>& commands) override
    {
        const juce::CommandID ids[] =
        {
            CommandIds::FileOpenRecent,
            CommandIds::OutputFilePath,
            CommandIds::ExportAudioSlices
        };
        
        commands.addArray(ids, numElementsInArray(ids));
    }
    
    void getCommandInfo(juce::CommandID commandID, juce::ApplicationCommandInfo& result) override
    {
        switch(commandID)
        {
            case CommandIds::FileOpenRecent:
            {
                result.setInfo("Open Recent File", "Open a recently used audio file", "File", 0);
                result.setActive(mRecentFiles.getNumFiles() > 0);
            }
                break;
            case CommandIds::OutputFilePath:
            {
                result.setInfo("Set output path", "Set the output path to write recordings to", "File", 0);
                result.setActive(true);
            }
                break;
            case CommandIds::ExportAudioSlices:
            {
                result.setInfo("Export the current slices", "Export the current slices to individual audio files", "File", 0);
                result.setActive(true); // TODO: only if there is an audio file!
            }
                break;
        }
    }
    
    bool perform(juce::ApplicationCommandTarget::InvocationInfo const& info) override
    {
        switch(info.commandID)
        {
            case CommandIds::FileOpenRecent:
            {
                return true;
            }
            case CommandIds::OutputFilePath:
            {
                mContent.setFileOutputPath();
                return true;
            }
            case CommandIds::ExportAudioSlices:
            {
                mContent.exportAudioSlices();
                return true;
            }
        }
        
        return false;
    }

private:
    class MainWindow    : public juce::DocumentWindow
    {
    public:
        MainWindow (const String& name, Component* c, JUCEApplication& a)
            : DocumentWindow (name, Desktop::getInstance().getDefaultLookAndFeel()
                                                          .findColour (ResizableWindow::backgroundColourId),
                              DocumentWindow::allButtons),
              app (a)
        {
            setUsingNativeTitleBar (true);
            setContentOwned (c, true);

           #if JUCE_ANDROID || JUCE_IOS
            setFullScreen (true);
           #else
            setResizable (true, false);
            setResizeLimits (300, 250, 10000, 10000);
            centreWithSize (getWidth(), getHeight());
           #endif

            setVisible (true);
        }

        void closeButtonPressed() override
        {
            app.systemRequestedQuit();
        }

    private:
        JUCEApplication& app;

        //==============================================================================
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainWindow)
    };
    
    class MainMenuModel
    : public juce::MenuBarModel
    {
    public:
        MainMenuModel() {}
        ~MainMenuModel() override = default;
        
        juce::StringArray getMenuBarNames() override
        {
            auto& commandManager = getCommandManager();
            return commandManager.getCommandCategories();
        }
        
        juce::PopupMenu getMenuForIndex(int index, juce::String const& categoryName) override
        {
            juce::PopupMenu menu;
            
            auto& commandManager = getCommandManager();
            auto const commandIds = commandManager.getCommandsInCategory(categoryName);
            for(auto const commandId : commandIds)
            {
                if(commandId == CommandIds::FileOpenRecent)
                {
                    PopupMenu recentFilesMenu;
                    getApp().mRecentFiles.createPopupMenuItems(recentFilesMenu, static_cast<int>(CommandIds::FileOpenRecent) + 1, false, true);
                    menu.addSubMenu("Open recent file", recentFilesMenu);
                }
                else
                {
                    menu.addCommandItem(&commandManager, commandId);
                }
            }
            
            return menu;
        }
        
        void menuItemSelected(int itemId, int index) override
        {
            if(itemId >= static_cast<int>(CommandIds::FileOpenRecent) + 1)
            {
                auto& app = getApp();
                auto file = app.mRecentFiles.getFile(itemId - (static_cast<int>(CommandIds::FileOpenRecent) - 1));
                
                auto path = file.getFullPathName();
                app.mContent.newFileOpened(path);
            }
        }
    };
    
    static Application& getApp()
    {
        return *dynamic_cast<Application*>(JUCEApplication::getInstance());
    }
    
    static juce::ApplicationCommandManager& getCommandManager()
    {
        return getApp().mCommandManager;
    }
    
    void saveProperties()
    {
        auto xml = std::make_unique<juce::XmlElement>("BreakbeatMaker");
        if(xml == nullptr)
        {
            return;
        }
        
        xml->setAttribute("recentFiles", mRecentFiles.toString());
        xml->writeTo(mPropertyFile);
    }
    
    void loadProperties()
    {
        if(!mPropertyFile.existsAsFile())
        {
            mPropertyFile.create();
        }
        
        auto xml = juce::parseXML(mPropertyFile);
        if(xml != nullptr && xml->hasTagName("BreakbeatMaker"))
        {
            mRecentFiles.restoreFromString(xml->getStringAttribute("recentFiles"));
        }
    }

    std::unique_ptr<MainWindow> mMainWindow;
    MainMenuModel mMenuModel;
    
    juce::ApplicationCommandManager mCommandManager;
    
    juce::RecentlyOpenedFilesList mRecentFiles;
    MainContentComponent mContent {mRecentFiles};
    
    juce::File mPropertyFile;
};

//==============================================================================
START_JUCE_APPLICATION (Application)
