#include "editor.hpp"

namespace sdf_editor
{

using namespace Zep;

zep_editor::zep_editor(const std::string& startup_file_path, const std::string& config_path)
    : editor(std::make_unique<ZepEditor_ImGui>(config_path, NVec2f(1.0f)))
    //, fileWatcher(spEditor->GetFileSystem().GetConfigPath(), std::chrono::seconds(2))
{
    // ZepEditor_ImGui will have created the fonts for us; but we need to build
    // the font atlas
    //auto fontPath = std::string(SDL_GetBasePath()) + "Cousine-Regular.ttf";
    //auto& display = static_cast<ZepDisplay_ImGui&>(spEditor->GetDisplay());

    //int fontPixelHeight = (int)dpi_pixel_height_from_point_size(DemoFontPtSize, GetPixelScale().y);

    //auto& io = ImGui::GetIO();
    //ImVector<ImWchar> ranges;
    //ImFontGlyphRangesBuilder builder;
    //builder.AddRanges(io.Fonts->GetGlyphRangesDefault()); // Add one of the default ranges
    //builder.AddRanges(io.Fonts->GetGlyphRangesCyrillic()); // Add one of the default ranges
    //builder.AddRanges(greek_range);
    //builder.BuildRanges(&ranges); // Build the final result (ordered ranges with all the unique characters submitted)

    //ImFontConfig cfg;
    //cfg.OversampleH = 4;
    //cfg.OversampleV = 4;

    //auto pImFont = ImGui::GetIO().Fonts->AddFontFromFileTTF(fontPath.c_str(), float(fontPixelHeight), &cfg, ranges.Data);

    //display.SetFont(ZepTextType::UI, std::make_shared<ZepFont_ImGui>(display, pImFont, fontPixelHeight));
    //display.SetFont(ZepTextType::Text, std::make_shared<ZepFont_ImGui>(display, pImFont, fontPixelHeight));
    //display.SetFont(ZepTextType::Heading1, std::make_shared<ZepFont_ImGui>(display, pImFont, int(fontPixelHeight * 1.75)));
    //display.SetFont(ZepTextType::Heading2, std::make_shared<ZepFont_ImGui>(display, pImFont, int(fontPixelHeight * 1.5)));
    //display.SetFont(ZepTextType::Heading3, std::make_shared<ZepFont_ImGui>(display, pImFont, int(fontPixelHeight * 1.25)));

    //unsigned int flags = 0; // ImGuiFreeType::NoHinting;
    //ImGuiFreeType::BuildFontAtlas(ImGui::GetIO().Fonts, flags);

    editor->RegisterCallback(this);

    //ZepRegressExCommand::Register(*spEditor);


    editor->InitWithFileOrDir(startup_file_path);


    // File watcher not used on apple yet ; needs investigating as to why it doesn't compile/run
    // The watcher is being used currently to update the config path, but clients may want to do more interesting things
    // by setting up watches for the current dir, etc.
    /*fileWatcher.start([=](std::string path, FileStatus status) {
        if (spEditor)
        {
            ZLOG(DBG, "Config File Change: " << path);
            spEditor->OnFileChanged(spEditor->GetFileSystem().GetConfigPath() / path);
        }
    });*/
}

}