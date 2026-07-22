#include <iostream>
#include <experimental/filesystem>
#include <string>
#include <iomanip>
#include <sstream>
#include <vector>
#include <algorithm>

namespace fs = std::experimental::filesystem;

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

int main(int argc, char* argv[]) 
{
    if (argc != 3) 
    {
        std::cerr << "Usage: " << argv[0] << " <4-char-prefix> <directory>\n";
        return 1;
    }

    std::string prefix = argv[1];
    if (prefix.length() != 4) 
    {
        std::cerr << "Sprite name must be exactly 4 characters.\n";
        return 1;
    }

    fs::path dir(argv[2]);
    if (!fs::exists(dir) || !fs::is_directory(dir)) 
    {
        std::cerr << "Invalid directory: " << dir << "\n";
        return 1;
    }

    // Prepare files for renaming
    std::vector<fs::path> sortedFiles;
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
            oss << prefix << std::setfill('0') << std::setw(4) << index;
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

            if (index > 9999) 
            {
                std::cout << "Warning: frame limit reached for: " << prefix << std::endl;
                break;  // Limit to pattern range
            }
        }
    } 
    catch (const std::exception& e) 
    {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    std::cout << "Renamed " << index << " files.\n";
    return 0;
}