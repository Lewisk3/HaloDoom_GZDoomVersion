#include <stdio.h>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <sstream>
#include <string>
#include <iomanip>
#include <vector>
#include <algorithm>
#include <cmath>
#include <regex>

namespace fs = std::filesystem;

// Data for texture defs block
struct TextureDefEntry
{
    std::string spriteHeader;
    double xScale;
    double yScale;
    double xOffset;
    double yOffset;
    std::string patch;
};

// Informs the program where a texturedef block should go.
struct PatchDirectory
{
    fs::path dir;
    std::string type;
    TextureDefEntry entryDef;
};

inline void makeLowercase(std::string& src)
{
     // Ugly ass C++ tolower for entire string.
    std::transform(src.begin(), src.end(), src.begin(), [](unsigned char c) { return std::tolower(c); });
}

fs::path FindAnimationDir(fs::path cur, std::string endpoint, fs::path prev = {})
{
    auto nextPath = cur.parent_path();
    std::string curParent = nextPath.filename().string();
    makeLowercase(curParent);

    if(curParent != endpoint)
        return FindAnimationDir(nextPath, endpoint, cur);
    else
        return cur;
}

static constexpr auto STR_NOTFOUND = std::string::npos;
void ParseTextureDefs(fs::path texFile, std::vector<TextureDefEntry>& entries)
{
    std::ifstream defs(texFile);
    if (!defs.is_open()) return;

    std::string line;
    std::vector<std::string> lines;
    std::string curSprite;

    bool hasTexEntry = false;
    TextureDefEntry texEntry;

    while(std::getline(defs, line))
    {
        if(size_t pos = line.rfind("Sprite"); pos == 0)
        {
            std::string newSprite = line.substr(pos + 6);   
            if(newSprite != curSprite)
            {
                curSprite = newSprite;

                // Push old texDefs if present
                if(hasTexEntry)
                {
                    entries.push_back(texEntry);
                }

                // Create new one
                texEntry = TextureDefEntry{};
                texEntry.spriteHeader = curSprite;
                hasTexEntry = true;
            }
        continue;
        }
        
        // Skip along the file until we find a valid entry.
        if(!hasTexEntry) continue;

        if (size_t pos = line.find("XScale"); pos != STR_NOTFOUND) 
        {
            texEntry.xScale = std::stof(line.substr(pos + 6));
        }
        else if (size_t pos = line.find("YScale"); pos != STR_NOTFOUND) 
        {
            texEntry.yScale = std::stof(line.substr(pos + 6));
        }
        else if (size_t pos = line.find("Offset"); pos != STR_NOTFOUND) 
        {
            size_t delim = line.find(",");
            if(delim != STR_NOTFOUND)
            {
                texEntry.xOffset = std::stoi(line.substr(pos + 6, delim - (pos + 6)));
                texEntry.yOffset = std::stoi(line.substr(delim + 1));
            }
        }
        else if (size_t pos = line.find("Patch"); pos != STR_NOTFOUND) 
        {
            size_t first = line.find('"');
            size_t second = line.find('"', first + 1);
            if (first != STR_NOTFOUND && second != STR_NOTFOUND) 
                texEntry.patch = line.substr(first + 1, second - first - 1);
        }
    }

    // Clean up last entry if present.
    if(hasTexEntry)
        entries.push_back(texEntry);

    defs.close();
}

double CheckTexScale(double sc)
{
    if(sc == 0) sc = 1.0;
    return sc;
}

int main(int argc, const char* argv[])
{
    // Do not sync with cout, so we can still see console output but without the slowdown.
    std::ios::sync_with_stdio(false); 
    std::cout.tie(NULL);

    // Arguments
    if(argc != 2)
    {
        std::cerr << "Usage " << argv[0] << " <base directory>" << std::endl;
        return 1;
    }

    // Take base directory, then iterate through directories within this.
    std::string baseDirectoryString = argv[1];
    fs::path baseDir(baseDirectoryString);

    // Ensure directory exists, and is a valid directory.
    if (!fs::exists(baseDir) || !fs::is_directory(baseDir)) 
    {
        std::cerr << "Error: Invalid path, doesn't exist or isn't a directory.\n";
        return 1;
    }

    // Gather namespace for entire animation
    std::cout << "---------------------------------------\n";
    std::cout << "> Parsing texturedefs in \"" << baseDir.string() << "\" [...]" << std::endl;
    std::cout << "---------------------------------------\n";

    std::vector<TextureDefEntry> textureEntries;
    const char* texDefsName = "textures";
    fs::path texFile;

    // Gather processes to complete, based on user entered settings.
    for (const auto& entry : fs::directory_iterator(baseDir)) 
    {
        if(entry.is_directory()) continue; // Skip directories
        std::string curFilename = entry.path().filename().stem().string();
        makeLowercase(curFilename);

        // Parse texturedefs
        if(curFilename == texDefsName)
        {
            // Found texturedefs entry
            texFile = entry.path();
        }
    }

    // Unable to find texturedefs entry.
    if(texFile.empty())
    {
        std::cerr << "Error: unable to find texturedefs file in " << baseDir.string() << std::endl;
        return 1; 
    }

    // Pre-emptive reserve for speed on massive files.
    textureEntries.reserve(10000);

    ParseTextureDefs(texFile, textureEntries);
    textureEntries.shrink_to_fit();

    std::cout << textureEntries.size() << " patches found.\n";

    std::cout << "--------------------------------------------\n";
    std::cout << "> Searching for patch file directories ...\n";
    std::cout << "--------------------------------------------\n";
    std::string graphicsDir = "graphics";

    std::vector<PatchDirectory> patchList; // Organized patch data.
    std::vector<TextureDefEntry> orphanList; // Used to generate optimized main texturedefs file.
    int patchCount = 0;
    for(auto entry : textureEntries)
    {
        // Scan directories for patches
        bool isOrphan = true;
        for(const auto& fileEntry : fs::recursive_directory_iterator(baseDir)) 
        {
            if(!fileEntry.is_regular_file()) continue;
            std::string curFilename = fileEntry.path().filename().stem().string();
            std::string entryName = entry.patch;
            makeLowercase(curFilename);
            makeLowercase(entryName);

            if(entryName == curFilename)
            {
                isOrphan = false;
                auto animPath = FindAnimationDir(fileEntry.path(), graphicsDir);
                std::string animName = animPath.filename().string();
                std::cout << "Found " << entry.patch << " of type \"" << animName << "\". " << std::endl; 

                PatchDirectory ptd;
                ptd.dir = animPath;
                ptd.type = animName;
                ptd.entryDef = entry;
                patchList.push_back(std::move(ptd)); 
                ++patchCount;
            }
        }
        if(isOrphan) 
        {
            std::cout << "Orphan Patch: " << entry.patch << std::endl;
            orphanList.push_back(entry);
        }
    }
    std::cout << "Completed successfully for " << patchCount << " patches." << std::endl;

    std::cout << "----------------------------------------------------\n";
    std::cout << "> Generating split files for non-orphaned patches...\n";
    std::cout << "----------------------------------------------------\n";

    std::vector<fs::path> toIncludeDefs;
    for(auto patch : patchList)
    {
        auto filePath = (fs::path("Graphics") / patch.type / "texturedefs.txt");
        bool isNewFile = !fs::exists(filePath);

        auto it = std::find(toIncludeDefs.begin(), toIncludeDefs.end(), filePath); 
        bool shouldAddInclude = !(it != toIncludeDefs.end());

        std::ofstream defs(baseDir / filePath, std::ios::out | std::ios::app);
        if(!defs.is_open()) 
        {
            std::cout << "[WARN] Failed to open texturedefs.txt for patch: " << patch.entryDef.patch << std::endl;
            continue;
        }

        if(shouldAddInclude)
        {
            toIncludeDefs.push_back(filePath);
            std::cout << "Modified \"texturedefs.txt\" for " << patch.dir.filename().string() << ".\n";
        }

        // Generate entry
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(4);
        oss << "Sprite" << patch.entryDef.spriteHeader << "\n";
        oss << "{\n";
        oss << "\tXScale " << CheckTexScale(patch.entryDef.xScale) << "\n";
        oss << "\tYScale " << CheckTexScale(patch.entryDef.yScale) << "\n";
        oss << "\tOffset " << static_cast<int>(patch.entryDef.xOffset) << ", " << static_cast<int>(patch.entryDef.yOffset) << "\n";
        oss << "\tPatch \"" << patch.entryDef.patch << "\"" << ", 0, 0\n";
        oss << "}\n";

        defs << oss.str() << "\n";
        defs.close();
    }

    std::cout << "----------------------------------------------------\n";
    std::cout << "> Optimizing base TEXTURES file...\n";
    std::cout << "----------------------------------------------------\n";
    std::ofstream optTextures(baseDir / "TEXTURES_OPTIMIZED.txt");
    if(optTextures.is_open())
    {
        optTextures << "// [HPLAP Organizer] Autogenerated split texture definitions included below.\n";
        for(auto incl : toIncludeDefs)
            optTextures << "#include \"" << incl.string() << "\"\n";
        optTextures << "\n";

        optTextures << "// [HPLAP Organizer] Remaining orphaned patches.\n";
        for(auto orphan : orphanList)
        {
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(4);
            oss << "Sprite" << orphan.spriteHeader << "\n";
            oss << "{\n";
            oss << "\tXScale " << CheckTexScale(orphan.xScale) << "\n";
            oss << "\tYScale " << CheckTexScale(orphan.yScale) << "\n";
            oss << "\tOffset " << static_cast<int>(orphan.xOffset) << ", " << static_cast<int>(orphan.yOffset) << "\n";
            oss << "\tPatch \"" << orphan.patch << "\"" << ", 0, 0\n";
            oss << "}\n";

            optTextures << oss.str() << "\n";
        }
        optTextures.close();
    }
    else
    {
        std::cerr << "[Error] Failed to create optimized textures file." << std::endl;
    }

    std::cout << "Completed." << std::endl;

    return 0;
}
