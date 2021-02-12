#pragma once

#include <zep/filesystem.h>
#include <zep/imgui/display_imgui.h>
#include <zep/imgui/editor_imgui.h>
#include <zep/mode_standard.h>
#include <zep/mode_vim.h>
#include <zep/tab_window.h>
#include <zep/theme.h>
#include <zep/window.h>
//#include <zep/regress.h>
#include <memory>

namespace sdf_editor
{

struct zep_editor : public Zep::IZepComponent
{
    zep_editor(const std::string& startup_file_path, const std::string& config_path);
    //~zep_editor() = default;

    //void Destroy()
    ~zep_editor()
    {
        editor->UnRegisterCallback(this);
        editor.reset();
    }

    void Notify(std::shared_ptr<Zep::ZepMessage> message) override final
    {
        /*if (message->messageId == Msg::GetClipBoard)
        {
            clip::get_text(message->str);
            message->handled = true;
        }
        else if (message->messageId == Msg::SetClipBoard)
        {
            clip::set_text(message->str);
            message->handled = true;
        }
        else if (message->messageId == Msg::RequestQuit)
        {
            quit = true;
        }
        else if (message->messageId == Msg::ToolTip)
        {
            auto spTipMsg = std::static_pointer_cast<ToolTipMessage>(message);
            if (spTipMsg->location.Valid() && spTipMsg->pBuffer)
            {
                auto pSyntax = spTipMsg->pBuffer->GetSyntax();
                if (pSyntax)
                {
                    if (pSyntax->GetSyntaxAt(spTipMsg->location).foreground == ThemeColor::Identifier)
                    {
                        auto spMarker = std::make_shared<RangeMarker>(*spTipMsg->pBuffer);
                        spMarker->SetDescription("This is an identifier");
                        spMarker->SetHighlightColor(ThemeColor::Identifier);
                        spMarker->SetTextColor(ThemeColor::Text);
                        spTipMsg->spMarker = spMarker;
                        spTipMsg->handled = true;
                    }
                    else if (pSyntax->GetSyntaxAt(spTipMsg->location).foreground == ThemeColor::Keyword)
                    {
                        auto spMarker = std::make_shared<RangeMarker>(*spTipMsg->pBuffer);
                        spMarker->SetDescription("This is a keyword");
                        spMarker->SetHighlightColor(ThemeColor::Keyword);
                        spMarker->SetTextColor(ThemeColor::Text);
                        spTipMsg->spMarker = spMarker;
                        spTipMsg->handled = true;
                    }
                }
            }
        }*/
    }

    Zep::ZepEditor& GetEditor() const override final
    {
        return *editor;
    }

    bool quit = false;
    std::unique_ptr<Zep::ZepEditor_ImGui> editor;
    //FileWatcher fileWatcher;
};

}