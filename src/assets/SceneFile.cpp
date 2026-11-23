#include "SceneFile.h"

#include <cmath>
#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>
#include <utility>

namespace
{
    bool ReadVector(std::istream& input, glm::vec3& value)
    {
        return static_cast<bool>(input >> value.x >> value.y >> value.z)
            && std::isfinite(value.x)
            && std::isfinite(value.y)
            && std::isfinite(value.z);
    }

    bool ReadBool(std::istream& input, bool& value)
    {
        int number;
        if (!(input >> number) || (number != 0 && number != 1))
            return false;

        value = number != 0;
        return true;
    }

    bool ReadUnitFloat(std::istream& input, float& value)
    {
        return static_cast<bool>(input >> value)
            && std::isfinite(value)
            && value >= 0.0f && value <= 1.0f;
    }

    bool ReadQuoted(std::istream& input, std::string& value)
    {
        input >> std::ws;
        if (input.peek() != '"')
            return false;

        return static_cast<bool>(input >> std::quoted(value));
    }

    bool IsColor(const glm::vec3& value)
    {
        return value.x >= 0 && value.x <= 1
            && value.y >= 0 && value.y <= 1
            && value.z >= 0 && value.z <= 1;
    }
}






namespace SceneFile
{
    
    bool Read(
        const std::filesystem::path& path,
        AcornData::SceneDescription& output,
        std::string& error)
    {
        error.clear();

        std::ifstream file(path);
        if (!file)
        {
            error = "Cannot open scene file: " + path.string();
            return false;
        }

        // Build separately so malformed input does not overwrite output.
        AcornData::SceneDescription result{};

        bool haveVersion = false;
        bool haveName = false;
        bool haveID = false;
        bool haveGoal = false;
        std::array<bool, 2> havePlayer{};

        std::string line;
        std::size_t lineNumber = 0;

        auto fail = [&](const std::string& message)
        {
            error = path.string() + ":" + std::to_string(lineNumber)
                  + ": " + message;
            return false;
        };

        while (std::getline(file, line))
        {
            ++lineNumber;
            std::istringstream row(line);
            row >> std::ws;

            if (row.eof() || row.peek() == '#')
                continue;

            std::string tag;
            row >> tag;
            bool valid = true;

            if (tag == "version")
            {
                int version = 0;
                valid = !haveVersion && static_cast<bool>(row >> version)
                     && version == 1;
                haveVersion = true;
            }
            else if (!haveVersion)
            {
                return fail("First record must be: version 1");
            }
            else if (tag == "scene")
            {
                valid = !haveName && ReadQuoted(row, result.name)
                     && !result.name.empty();
                haveName = true;
            }
            else if (tag == "id")
            {
                valid = !haveID && static_cast<bool>(row >> result.sceneID)
                     && result.sceneID >= 0;
                haveID = true;
            }
            else if (tag == "background")
            {
                auto& color = result.settings.defaultBackgroundColor;
                valid = ReadVector(row, color) && IsColor(color);
            }
            else if (tag == "split")
            {
                std::string value;
                row >> value;

                using Split = AcornData::ESplitScreenSettings;

                if (value == "vertical")
                    result.settings.splitScreen = Split::VerticalSplit;
                else if (value == "horizontal")
                    result.settings.splitScreen = Split::HorizontalSplit;
                else if (value == "grid")
                    result.settings.splitScreen = Split::GridSplit;
                else
                    valid = false;
            }
            else if (tag == "player")
            {
                int index = -1;

                if (!(row >> index) || index < 0 || index >= 2)
                    return fail("Player index must be 0 or 1");

                if (havePlayer[index])
                    return fail("Duplicate player start");

                auto& start = result.playerStarts[index];
                valid = ReadVector(row, start.position)
                     && ReadVector(row, start.rotationDegrees);

                havePlayer[index] = true;
            }
            else if (tag == "goal")
            {
                valid = !haveGoal && ReadVector(row, result.goalPosition);
                haveGoal = true;
            }
            else if (tag == "object")
            {
                AcornData::SceneObjectDescription object;
                valid = ReadQuoted(row, object.source.filePath)
                     && !object.source.filePath.empty();

                if (valid)
                    result.objects.push_back(std::move(object));
            }
            else
            {
                if (result.objects.empty())
                    return fail("Unknown record or object property before object: " + tag);

                auto& object = result.objects.back();
                auto& appearance = object.appearance;

                if (tag == "position")
                    valid = ReadVector(row, object.transform.position);
                else if (tag == "rotation")
                    valid = ReadVector(row, object.transform.rotationDegrees);
                else if (tag == "scale")
                {
                    auto& scale = object.transform.scale;
                    valid = ReadVector(row, scale)
                         && scale.x > 0 && scale.y > 0 && scale.z > 0;
                }
                else if (tag == "color")
                {
                    valid = ReadVector(row, appearance.tintColor)
                         && IsColor(appearance.tintColor);
                    appearance.overrideColor = true;
                }
                else if (tag == "texture")
                {
                    valid = ReadQuoted(row, object.textureFilePath);
                    appearance.textureExpected = !object.textureFilePath.empty();
                }
                else if (tag == "import")
                {
                    std::string importer;
                    row >> importer;

                    auto& settings = object.source.importSettings;

                    if (importer == "assimp")
                        settings.importer = Assets::EModelImporter::Assimp;
                    else if (importer == "obj")
                        settings.importer = Assets::EModelImporter::CustomObj;
                    else
                        valid = false;

                    valid = valid
                         && ReadBool(row, settings.flipUVs)
                         && ReadBool(row, settings.skipInvalidFaces);
                }
                else if (tag == "flags")
                {
                    valid = ReadBool(row, appearance.visible)
                         && ReadBool(row, appearance.overrideColor)
                         && ReadBool(row, appearance.textureExpected)
                         && ReadBool(row, appearance.lightingRequested);
                }
                else if (tag == "tint")
                {
                    valid = ReadUnitFloat(row, appearance.tintAlpha)
                         && ReadUnitFloat(row, appearance.tintTextureAlpha);
                }
                else
                {
                    return fail("Unknown keyword: " + tag);
                }
            }

            if (!valid)
                return fail("Invalid or duplicate record: " + tag);

            // Reject typos/extra fields, but permit trailing comments.
            row >> std::ws;
            if (!row.eof() && row.peek() != '#')
                return fail("Unexpected extra data after: " + tag);
        }

        if (file.bad())
            return fail("Error while reading file");

        if (!haveVersion || !haveName || !haveID || !haveGoal
            || !havePlayer[0] || !havePlayer[1])
        {
            return fail("Required: version, scene, id, goal, player 0 and player 1");
        }

        if (result.objects.empty())
            return fail("Scene must contain at least one object");

        output = std::move(result);
        return true;
    }

    
    
    bool Write(
        const std::filesystem::path& path,
        const AcornData::SceneDescription& description,
        std::string& error)
    {
        error.clear();

        std::ofstream file(path);
        if (!file)
        {
            error = "Cannot write scene file: " + path.string();
            return false;
        }

        file << std::setprecision(std::numeric_limits<float>::max_digits10);

        auto vector = [&](const glm::vec3& value)
        {
            file << value.x << ' ' << value.y << ' ' << value.z;
        };

        using Split = AcornData::ESplitScreenSettings;
        const char* split =
            description.settings.splitScreen == Split::HorizontalSplit
            ? "horizontal"
            : description.settings.splitScreen == Split::GridSplit
            ? "grid" : "vertical";

        file << "version 1\nscene " << std::quoted(description.name)
             << "\nid " << description.sceneID << "\nbackground ";
        vector(description.settings.defaultBackgroundColor);
        file << "\nsplit " << split << '\n';

        for (std::size_t i = 0; i < description.playerStarts.size(); ++i)
        {
            const auto& start = description.playerStarts[i];
            file << "player " << i << ' ';
            vector(start.position);
            file << ' ';
            vector(start.rotationDegrees);
            file << '\n';
        }

        file << "goal ";
        vector(description.goalPosition);
        file << '\n';

        for (const auto& object : description.objects)
        {
            const auto& appearance = object.appearance;
            const auto& settings = object.source.importSettings;

            file << "\nobject " << std::quoted(object.source.filePath);
            file << "\nimport "
                 << (settings.importer == Assets::EModelImporter::Assimp
                     ? "assimp" : "obj")
                 << ' ' << settings.flipUVs
                 << ' ' << settings.skipInvalidFaces;

            file << "\nposition ";
            vector(object.transform.position);
            file << "\nrotation ";
            vector(object.transform.rotationDegrees);
            file << "\nscale ";
            vector(object.transform.scale);
            file << "\ncolor ";
            vector(appearance.tintColor);

            file << "\ntexture " << std::quoted(object.textureFilePath);

            // Written after color/texture to preserve the exact flags.
            file << "\nflags "
                 << appearance.visible << ' '
                 << appearance.overrideColor << ' '
                 << appearance.textureExpected << ' '
                 << appearance.lightingRequested;

            file << "\ntint "
                 << appearance.tintAlpha << ' '
                 << appearance.tintTextureAlpha << '\n';
        }

        file.close();

        if (!file)
        {
            error = "Failed while writing scene file: " + path.string();
            return false;
        }

        return true;
    }
    
}
