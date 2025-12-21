#include <Magick++.h>
#include <iostream>
#include <experimental/filesystem>
#include <fstream>
#include <sstream>
#include <regex>
#include <string>
#include <iomanip>

using namespace Magick;
namespace fs = std::experimental::filesystem;

void DoResizeLanczos(std::string file, std::string outputFile, std::string scaleStr)
{
    Image img(file);  
    // Preserve sharp transparency during resize (no edge bleed)
    img.matte(true);
    img.backgroundColor("none");

    // High-quality resize
    img.filterType(LanczosFilter);
    //img.blur(0.4);
    img.resize(Geometry(scaleStr + "%>"));
    img.sharpen(0.5, 0.3);

    img.write(outputFile);    
}

bool UpdateTextureDefs(std::string& texturedefs, std::vector<std::string>& entries, double scale) 
{
    std::ifstream defs(texturedefs);
    std::ofstream output("resized_" + texturedefs);
    
    // File patterns
    const std::regex spriteHeader("Sprite\\s+\"([^\"]+)\",\\s*([0-9]+),\\s*([0-9]+)");
    const std::regex xScale("XScale\\s+([0-9]+(\\.[0-9]+)?)");
    const std::regex yScale("YScale\\s+([0-9]+(\\.[0-9]+)?)");
    const std::regex offset("Offset\\s+(-?[0-9]+),\\s*(-?[0-9]+)");
    const std::regex patch("Patch\\s+\"([^\"]+)\"");

    // Data storage
    std::string line;
    std::vector<std::string> lines;
    std::smatch match;
    size_t blockStart = 0;

    // Read file
    while(std::getline(defs, line)) lines.push_back(line);
    defs.close();

    int errCount = 0;
    for(const auto& spriteName : entries) 
    {
        // Search for patch file.
        size_t patchLine = 0;
        bool foundPatch = false;
        
        for(size_t i = 0; i < lines.size(); ++i) 
        {
            if(std::regex_search(lines[i], match, patch) && match[1].str() == spriteName) 
            {
                patchLine = i;
                foundPatch = true;
                break;
            }
        }
        
        // Error, not found.
        if(!foundPatch)
        {
            std::cout << "Warning: " << spriteName << " not found." << std::endl; 
            errCount++;
            continue;  
        } 

        while(foundPatch)
        {
            // Find the sprite header.
            for(int i = patchLine; i >= 0; --i) 
            {
                if(std::regex_search(lines[i], match, spriteHeader)) 
                {
                    blockStart = i;
                    break;
                }
            }

            // Modify relevant content
            for(size_t i = blockStart; i < lines.size(); ++i) 
            {
                line = lines[i];
                if(i == blockStart) 
                {
                    int width = std::stoi(match[2].str()) * scale;
                    int height = std::stoi(match[3].str()) * scale;
                    std::ostringstream oss;
                    oss << "Sprite \"" << match[1] << "\", " << width << ", " << height;
                    lines[i] = oss.str();
                    continue;
                }
                       
                if(std::regex_search(line, match, xScale)) 
                {
                    double xscale = std::stod(match[1].str()) * scale;
                    std::ostringstream oss;
                    oss << "\tXScale " << std::fixed << std::setprecision(3) << xscale;
                    lines[i] = oss.str();
                }
                else if(std::regex_search(line, match, yScale)) 
                {
                    double yscale = std::stod(match[1].str()) * scale;
                    std::ostringstream oss;
                    oss << "\tYScale " << std::fixed << std::setprecision(3) << yscale;
                    lines[i] = oss.str();
                }
                else if(std::regex_search(line, match, offset)) 
                {
                    int xoff = std::stoi(match[1].str()) * scale;
                    int yoff = std::stoi(match[2].str()) * scale;
                    std::ostringstream oss;
                    oss << "\tOffset " << xoff << ", " << yoff;
                    lines[i] = oss.str();
                }
                
                // Stop when } is found.
                if(line.find("}") != std::string::npos) break;
            }
            
            // Repeat patch search
            foundPatch = false;
            for(size_t i = patchLine+1; i < lines.size(); ++i) 
            {
                if(std::regex_search(lines[i], match, patch) && match[1].str() == spriteName) 
                {
                    patchLine = i;
                    foundPatch = true;
                    break;
                }
            }
        }
    }

    // Write new lines vector to output file, then close.
    for(const auto& line : lines) output << line << std::endl;
    output.close();

    std::cout << "Finished with " << errCount << " issue(s)." << std::endl;
    return true;
}

int main(int argc, char** argv) 
{
    // Do not sync with cout, so we can still see console output but without the slowdown.
    std::ios::sync_with_stdio(false); 
    std::cout.tie(NULL);              

    InitializeMagick(*argv);

    if(argc < 5)
    {
        std::cerr << "Error: Usage: " << argv[0] << " <input_dir> <output_dir> <texturedefs> <scale> [--skipimages]" << std::endl;
        return 1;
    }

    std::string str_inputPath = argv[1];
    std::string str_outputPath = argv[2];
    std::string str_texdefs = argv[3];
    std::string str_scale = argv[4];

    std::string str_additionArgs = "";
    if(argc > 5) str_additionArgs = argv[5];
    bool skipResize = (str_additionArgs == "--skipimages");

    fs::path inputDir(argv[1]);
    fs::path outputDir(argv[2]);

    // Iterate through valid files
    int fileCount = 0;
    std::vector<std::string> patchNameEntries;
    for(const auto& file : fs::recursive_directory_iterator(inputDir))
    {
        fs::path filePath = file.path();
        std::string fileName = filePath.string();
        if(!fs::is_regular_file(filePath) || filePath.extension() != ".png") continue;

        std::string patchName = filePath.stem().string();
        patchNameEntries.push_back(patchName);
        
        if(!skipResize)
        {
            // Found valid files, generate new file path based on relative location.        
            fs::path outputPath = outputDir / fileName;

            // Create the new directory.
            fs::create_directories(outputPath.parent_path());

            // Proceed to resize.
            try {
                DoResizeLanczos(filePath.string(), outputPath.string(), str_scale);
                std::cout << "Resizing: " << "[ " << filePath.string() << " -> " << outputPath.string() << "]" << std::endl; 
            } catch (Exception& resizeError) {
                std::cerr << "Errored on file " << filePath.string() << ": " << resizeError.what() << std::endl;
                return 1;
            }
            ++fileCount;
        }
    }
    std::cout << "Successfully Resized: " << fileCount << " file(s)." << std::endl;
    std::cout << "Updating texturedefs... " << std::endl;
    bool success = UpdateTextureDefs(str_texdefs, patchNameEntries, std::stod(str_scale) / 100.0);
    if(success) 
        std::cout << "Completed." << std::endl;
    else
        std::cout << "Failed." << std::endl;

    return 0;
}