#include <stdio.h>
#include <iostream>
#include <fstream>

#define CODEPTR "~"

int main(int argc, const char* argv[])
{
    if(argc < 2)
    {
        std::cout << "Missing argument for texturedefs file. Usage: texturenamer <filehere>" << std::endl;
        return 0;
    }
 
    std::string fileName = argv[1];
    std::ifstream file;
    file.open(".\\" + fileName);
    if(!file.is_open())
    {
        std::cout << "File to open file " << fileName << std::endl;
        return 0;
    }

    // Read contents
    std::string data;
    for(std::string line; std::getline(file, line); data += line + "\n");

    int namespace_number = 1;
    int current_frame = 0;
    std::string sprite_namespace = "";  // Current sprite namespace; format is W#S#0 example W1R1 would be Weapon 1, Reload 1.
    std::string validframes = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"; // Valid frames
    std::string validsequences = "123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

    int head = data.find(CODEPTR, 0); // Code-points for sprite sequences are the & symbol.
    if(head == std::string::npos)
    {
        std::cout << "Texture defs is not setup correctly: unable to find codepoint \"&\"" << std::endl;
        return 0;
    }

    #define nextcodepoint head = data.find(CODEPTR, head+1);
    do
    {
        // Read current namespace
        std::string curNS = data.substr(head-4, 4);
        std::string curSpriteName = data.substr(head-4, 6);

        if(curNS != sprite_namespace) 
        {
            current_frame = namespace_number = 0;
            sprite_namespace = curNS;
        }

        // Generate new name for sprite
        std::string newSpriteName = sprite_namespace.substr(0, 3) + validsequences.at(namespace_number) + validframes.at(current_frame) + "0";
        std::cout << "Generated new name for " << curSpriteName << " > " << newSpriteName << std::endl;

        // Advance namespace information
        current_frame++;
        if(current_frame >= validframes.length())
        {
            current_frame = 0;
            namespace_number++;
        }
        if(namespace_number >= validsequences.length())
        {
            namespace_number = 0;
            std::cout << "Warning: Weapon sprite sequence is too large!" << std::endl;
        }

        // Write into data
        data.replace(head-4, 6, newSpriteName);
        nextcodepoint;
    } while (head != std::string::npos);

    std::ofstream outputfile;
    outputfile.open(".\\output.txt");
    outputfile << data;

    std::cout << "Contents outputted to \"output.txt\" within local directory." << std::endl;   
    return 0;
}