/*
    HDE Processor of Large Animation Presets (HPLAP)
    Written by: Lewisk3 (Redxone)

    [MIT License]
        Copyright 2026 Lewisk3

        Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), 
        to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, 
        and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
            The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

        THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
        FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
        LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS 
        IN THE SOFTWARE.
*/

#include <stdio.h>
#include <iostream>
#include <fstream>
#include <Magick++.h>
#include <filesystem>
#include <sstream>
#include <regex>
#include <string>
#include <iomanip>
#include <vector>
#include <algorithm>
#include <cmath>

using namespace Magick;
namespace fs = std::filesystem;

struct DirectoryInfo
{
    std::string sourceDir;
    std::string outDir;
    std::string workDir;
    std::string groupName;
    std::string spriteName;
};

struct SpriteInfo
{
    std::string patchName;
    std::string spriteName;
    std::string spritePrefix;
    int width;
    int height;
};

enum class ProcessStatus
{
    SUCCESS,
    FAILED
};


int CountImages(fs::path dir)
{
    int count = 0;
    for(const auto& file : fs::directory_iterator(dir))
    {
        auto ext = file.path().extension();
        if( file.is_regular_file() && (ext == ".png" || ext == ".PNG") )
            count++;
    }
    return count;
}

// ---------------- RESIZE ------------------- //
void DoResizeLanczos(std::string file, std::string outputFile, std::string scaleStr)
{
    Image img(file);  
    img.matte(true);
    img.backgroundColor("none"); 

    img.filterType(LanczosFilter);
    //img.blur(0.4);
    img.resize(Geometry(scaleStr + "%>"));
    img.sharpen(0.5, 0.3);

    img.write(outputFile);    
}

ProcessStatus ResizeSprites(const DirectoryInfo& dirInfo, std::string scaleStr)
{
    fs::path inputDir(dirInfo.sourceDir);
    fs::path outputDir(dirInfo.outDir);

    // Iterate through valid files
    int fileCount = 0;
    std::vector<std::string> patchNameEntries;
    for(const auto& file : fs::directory_iterator(inputDir))
    {
        fs::path filePath = file.path();
        std::string fileName = filePath.string();
        auto ext = filePath.extension();
        if( !fs::is_regular_file(filePath) || (ext != ".png" && ext != ".PNG") ) continue;

        std::string patchName = filePath.stem().string();
        patchNameEntries.push_back(patchName);
        
        // Found valid files, generate new file path based on relative location.        
        fs::path outputPath = outputDir / filePath.parent_path().filename() / filePath.filename();

        // Create the new directory.
        fs::create_directories(outputPath.parent_path());

        // Proceed to resize.
        try {
            DoResizeLanczos(filePath.string(), outputPath.string(), scaleStr);
            std::cout << "Resizing: " << "[ " << filePath.string() << " -> " << outputPath.string() << "]" << std::endl; 
        } catch (Exception& resizeError) {
            std::cerr << "Errored on file " << filePath.string() << ": " << resizeError.what() << std::endl;
            return ProcessStatus::FAILED;
        }
        ++fileCount;
    }

    std::cout << "Successfully Resized: " << fileCount << " file(s)." << std::endl;
    return ProcessStatus::SUCCESS;
}
// ---------------- END RESIZE ------------------- //

// ---------------- RENAME ----------------- //
int getFrameNumber(const std::string& stem) 
{
    size_t pos = stem.find_last_not_of("0123456789");

    // If our file is all digits, treat it as such and move on.
    if (pos == std::string::npos) 
        return std::stoi(stem);
    
    std::string frameNumberStr = stem.substr(pos + 1);
    if (frameNumberStr.empty()) 
    {
        // Sprite is not labeled, so we have no idea what order it's meant to be in.
        std::cout << "Warning: file \"" << stem << "\"" << " has no numbers to indicate it's order. (Animation may be renamed incorrectly!)" << std::endl;
        return 0;
    }
    
    try 
    {
        // Output frame number.
        return std::stoi(frameNumberStr);
    } 
    catch (...) 
    {
        std::cout << "Warning: failed to find framenumber for file \"" << stem << "\"." << std::endl;
        return 0;
    }
}

bool fileSort(const fs::path& a, const fs::path& b) 
{
    int frameNumberA = getFrameNumber(a.stem().string());
    int frameNumberB = getFrameNumber(b.stem().string());
    return frameNumberA < frameNumberB;
}

ProcessStatus RenameSprites(const DirectoryInfo& dirInfo, std::vector<SpriteInfo>& patchList)
{
    fs::path dir(dirInfo.workDir);
    std::vector<fs::path> sortedFiles;
    std::string prefix = dirInfo.groupName;
    int suffixWidth = 8 - prefix.length();

    for (const auto& entry : fs::directory_iterator(dir)) 
    {
         // Grab file path.
        const fs::path& oldPath = entry.path();

        // Skip irrelevant files.
        if (!fs::is_regular_file(oldPath)) continue;
        
        // Push actual files
        sortedFiles.push_back(entry.path());
    }

    // Sort by filename.
    std::sort(sortedFiles.begin(), sortedFiles.end(), fileSort);

    uint index = 0;
    try 
    {
        for (const auto& file : sortedFiles) 
        {
            // Generate new path with new name.
            std::ostringstream oss;
            oss << prefix << std::setfill('0') << std::setw(suffixWidth) << index;
            fs::path newFile = dir / (oss.str() + file.extension().string());
            
            // Skip if target exists
            if (fs::exists(newFile)) 
            {
                std::cerr << "Warning: file already exists: " << newFile << " skipping..." << "\n";
                ++index;
                continue;
            }

            // Rename file.
            fs::rename(file, newFile);
            std::cout << "File renamed: " << file.filename() << " -> " << newFile.filename() << "\n";
            ++index;

            // Generate sprite info
            SpriteInfo spInfo;
            spInfo.patchName = newFile.filename().stem().string();
            spInfo.spritePrefix = dirInfo.spriteName;

            Image sprite;
            sprite.ping(newFile.string());
            spInfo.width = sprite.columns();
            spInfo.height = sprite.rows();
            patchList.push_back(spInfo);

            int nameLimit = std::pow(10, suffixWidth) - 1;
            if (index > nameLimit) 
            {
                std::cout << "Warning: frame limit reached for: " << prefix << std::endl;
                break;  // Limit to pattern range
            }
        }
    } 
    catch (const std::exception& e) 
    {
        std::cerr << "Error: " << e.what() << "\n";
        return ProcessStatus::FAILED;
    }

    std::cout << "Renamed " << index << " files.\n";
    return ProcessStatus::SUCCESS;
}
// --------------- END RENAME ----------------- //

// --------------- TEXTUREDEFS ---------------- //
std::string GenerateTextureEntry(const SpriteInfo& spInfo)
{
    double baseWidth = 320 * 1.2;
    double baseHeight = 200 * 1.2;
    double middle = 160;
    double shift = 5;

    double XScale = spInfo.width / baseWidth;
    double YScale = 1.2 * (spInfo.height / baseHeight);
    int xOffs = ( (spInfo.width / XScale) * 0.5 ) - middle + shift;
    int yOffs = (spInfo.height / YScale) - middle - shift;
    
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1);
    oss << "Sprite " << "\"" << spInfo.spriteName << "\"" << ", " << spInfo.width << ", " << spInfo.height << "\n";
    oss << "{\n";
    oss << "\tXScale " << XScale << "\n";
    oss << "\tYScale " << YScale << "\n";
    oss << "\tOffset " << xOffs << ", " << yOffs << "\n";
    oss << "\tPatch \"" << spInfo.patchName << "\"" << ", 0, 0\n";
    oss << "}\n";

    return oss.str();
}
// --------------- END TEXTUREDEFS ---------------- //

int main(int argc, const char* argv[])
{
    // Do not sync with cout, so we can still see console output but without the slowdown.
    std::ios::sync_with_stdio(false); 
    std::cout.tie(NULL);

    // Arguments
    if(argc != 4)
    {
        std::cerr << "Usage " << argv[0] << " <base directory> <output directory> <scale>" << std::endl;
        return 1;
    }

    // Setup imagemagick
    InitializeMagick(*argv);

    // Take base directory, then iterate through directories within this.
    std::string baseDirectoryStr = argv[1];
    std::string outDirectoryStr = argv[2];
    std::string scaleStr = argv[3];
    fs::path basePath(baseDirectoryStr);
    fs::path outPath(outDirectoryStr);

    // Ensure directory exists, and is a valid directory.
    if (!fs::exists(basePath) || !fs::is_directory(basePath)) 
    {
        std::cerr << "Error: Invalid path, doesn't exist or isn't a directory.\n";
        return 1;
    }

    // Gather namespace for entire animation
    std::cout << "---------------------------------------\n";
    std::cout << "> Setup: " << std::endl;
    std::cout << "---------------------------------------\n";

    std::string animationNamespace;
    std::cout << "[INPUT] Namespace for Animation (W1, W2, etc): " << std::flush;
    std::cin >> animationNamespace;

    // Gather processes to complete, based on user entered settings.
    std::vector<DirectoryInfo> ProcessList;
    for (const auto& dir : fs::directory_iterator(basePath)) 
    {
        if(!dir.is_directory()) continue;

        std::string dirName = dir.path().filename().string();
        if(dirName.empty()) continue;

        std::cout << "---------------------------------------\n";
        std::cout << "> Directory: " << dirName << std::endl;
        std::cout << "---------------------------------------\n";

        // Settings 
        DirectoryInfo settings;
        settings.sourceDir = dir.path().string();
        settings.outDir = outPath.string();
        settings.workDir = (outPath / dirName).string();

        // Input
        int fileCount = CountImages(dir);
        int optFileLength = 8 - (static_cast<int>(std::log10(fileCount)) + 1);

        bool nameTooLong = false;
        do
        {
            std::cout << "[INPUT] Texture Name [" << optFileLength << "]: " << std::flush;
            std::cin >> settings.groupName;

            nameTooLong = (settings.groupName.length() > optFileLength);
            if(nameTooLong) std::cout << "[ERROR] Texture Name is too long!" << std::endl;

        } while(nameTooLong);

        // Generate spriteName using first letter of directory.
        settings.spriteName = animationNamespace + dirName.front(); 

        // Add Process
        ProcessList.push_back(settings);
    }

    std::cout << "------------------------------------------------------\n";
    std::cout << "HPLAP is doing it's magic... Hold on to your butts!" << std::endl;
    std::cout << "------------------------------------------------------\n";

    std::vector<SpriteInfo> spritePatches;
    for(const auto& proc : ProcessList)
    {
        ProcessStatus stat = ResizeSprites(proc, scaleStr);
        if(stat == ProcessStatus::FAILED) return 1;

        //Rename files
        stat = RenameSprites(proc, spritePatches);
        if(stat == ProcessStatus::FAILED) return 1;
    }

    // Generate texturedef entries
    std::cout << "------------------------------------------------------\n";
    std::cout << "Generating texturedefs template..." << std::endl;
    std::cout << "------------------------------------------------------\n";

    std::ofstream texdefs(outPath / "texturedefs.txt");
    if(!texdefs.is_open())
    {
        std::cerr << "[ERROR] Failed to create \"texturedefs\" file." << std::endl;
        return 1;
    }

    int curFrame = 0;
    std::string curSpriteNamespace = "";
    for(auto& patch : spritePatches)
    {
        // Reset counter when group name changes.
        if(patch.spritePrefix != curSpriteNamespace)
        {
            curFrame = 0;
            curSpriteNamespace = patch.spritePrefix;
        }

        int animIndex    =  1  + (curFrame / 26);
        char frameLetter = 'A' + (curFrame % 26);

        // Generate spriteName
        std::ostringstream oss;
        oss << patch.spritePrefix << animIndex << frameLetter << "0";
        patch.spriteName = oss.str();

        std::cout << patch.patchName << " -> " << patch.spriteName << std::endl;
        std::string entry = GenerateTextureEntry(patch);
        texdefs << entry << std::endl;

        curFrame++;
    }

    // Finish writing.
    texdefs.close();

    std::cout << "------------------------------------------------------\n";
    std::cout << "HPLAP has finished successfully!" << std::endl;
    std::cout << "------------------------------------------------------\n";

    return 0;
}
