#include "ObjLoader.h"


// Standard library
#include <array>        // Fixed-size list of OBJ face corners
#include <charconv>     // std::from_chars for OBJ indices
#include <cstddef>      // std::size_t and offsetof for vertex attributes
#include <fstream>      // std::ifstream to open OBJ files
#include <iostream>     // std::cout and std::cerr for diagnostics
#include <sstream>      // std::istringstream to parse an OBJ line
#include <string>       // File lines and tokens
#include <string_view>  // Views of the fields in a face token
#include <system_error> // std::errc when checking parsed indices
#include <utility>      // std::move when finishing a mesh

// Third-party libraries
#include <glm/glm.hpp>          // Vectors, matrices, dot and cross products



namespace
{
    
    struct FaceCorner
    {
        AcornData::Vertex vertex{};
        bool hasUV = false;
        bool hasNormal = false;
    };    
    
    
    // Reads one index per call. 
    // Converts one OBJ face field, such as "12" or "-2", to a zero-based vector index. OBJ counts forward from 1, and C++ vectors count from 0.
    // On success, writes the result to index and returns true.
    // On failure, returns false and leaves index unchanged.
    bool ResolveIndex(std::string_view field, std::size_t count, std::size_t& index)
    {
        // Read one field from a face corner.
        if (field.empty())
        {
            return false;
        }

        long long objIndex = 0;
        
        // Parse the characters in this field. end points just past the last character.
        const char* end = field.data() + field.size();
        const auto result = std::from_chars(field.data(), end, objIndex);
        
        // Reject parse errors, leftover characters, and OBJ index 0.
        if (result.ec != std::errc{} || result.ptr != end || objIndex == 0)
        {
            return false;
        }
        
        // Keep the same count in a signed type so the calculation can go below zero.
        const auto signedCount = static_cast<long long>(count);
        
        // Positive OBJ indices count from 1. Negative indices count back from the end.
        // With four entries, 4 - 1 and 4 + (-1) both give index 3.
        const long long resolved = 
            objIndex > 0 ? objIndex - 1 : signedCount + objIndex;
        
        // Valid vector indices run from 0 through count - 1.
        if (resolved < 0 || resolved >= signedCount)
        {
            return false;
        }
        
        // Write the checked index back to the caller through the reference.
        index = static_cast<std::size_t>(resolved);
        return true;
    }
    
    
    
    // Reads one OBJ face corner, such as "3", "3/7", "3//2", or "3/7/2".
    // Looks up the position and any supplied UV and normal, then copies them into corner.
    // Pass a freshly initialized corner. On failure, returns false and corner may be partially filled.
    bool ReadFaceCorner(
        std::string_view token,                         // token: v/uv/nv - full info,  v// - only pos data,  v//nv - no uv data
        const std::vector<glm::vec3>& positions,        // list filled with all recorded vertex positions 
        const std::vector<glm::vec2>& uvs,              // list filled with all recorded uv coords
        const std::vector<glm::vec3>& normals,          // list filled with all recorded vertex normals 
        FaceCorner& corner)                             // result 
    {
        
        // start reading at the beginning of the current token: 
        // The position field ends at the first slash, or at the end of the token.
        const auto firstSlash = token.find('/');
        const auto positionField = token.substr(0, firstSlash);
        std::size_t index = 0;
        
        // find the position that matches this token's index from the saved positions   
        if (!ResolveIndex(positionField, positions.size(), index))
        {
            return false;                               // if resolving index fails, return false 
        }
        corner.vertex.position = positions[index];      // copy the coordinates at this index.
        
                                                        // npos is the special value find() returns when it can’t find what it's looking for,
        if (firstSlash == std::string_view::npos)       // if there was no slash anywhere in this token, means position-only corner 
        {
            return true; // v                           // Valid position-only corner, such as "3". 
        }
        
        
        // read the uv's between the slashes:
        // Make a new view starting after the first slash. "3/7/2" becomes "7/2".
        // The next find() and substr() count from the start of that view.
        const auto attributes = token.substr(firstSlash + 1);
        const auto secondSlash = attributes.find('/');
        const auto uvField = attributes.substr(0, secondSlash);
        
        if (!uvField.empty())                           // An empty UV field is allowed when the token supplies a normal, as in "3//2".
        {
            if (!ResolveIndex(uvField, uvs.size(), index))
            {
                return false;                           // so only return false if there was data to read that failed to resolve
            }
            corner.vertex.uv = uvs[index];              // copy the uv's at this index.
            corner.hasUV = true;                        // set the flag that will tell my program this mesh has valid uv's 
        }
        
        if (secondSlash == std::string_view::npos)      // if there was no second slash, but !uvField.empty(), then the corner should have recorded uv coords at this point 
        {
            return corner.hasUV; // v/vt                //when only one slash, it requires a UV, as in "3/7".
        }
        
        
        // read the normals after the two slashes:
        // A second slash introduces a required normal index, with or without a UV.
        const auto normalField = attributes.substr(secondSlash + 1);
        if (!ResolveIndex(normalField, normals.size(), index))
        {
            return false;
        }
        corner.vertex.normal = normals[index];          // copy the normals at this index.
        corner.hasNormal = true;                        // set the flag that will tell my program this mesh has valid normals 
        
        return true;                                    // corner has succeeded parsing and recording all information properly. 
    }
    
    
    // Reads the corners after `f` on one OBJ face line.
    // Appends the face's vertices and one or two triangles to mesh.
    // If the face is invalid or unsupported, returns false before modifying mesh.
    bool AppendFace(
        std::istringstream& fields,
        const std::vector<glm::vec3>& positions,
        const std::vector<glm::vec2>& uvs,
        const std::vector<glm::vec3>& normals,
        AcornData::CpuMeshData& mesh)
    {
        // Store up to four corners so the whole face can be checked before appending it.
        std::array<FaceCorner, 4> corners{};            // corners has four slots because the biggest face this loader accepts is a quad.
        std::size_t count = 0;                          // tracks how many slots have been filled 
        std::string token;                              // the next word read from the line.
        
        
        // Read one whitespace-separated corner token at a time, such as "3/7/2".
        while (fields >> token)                         // (fields >> token) skips whitespace, reads characters up to the next whitespace, and stores them in token
        {
            if (count == corners.size())                // a fifth corner exceeds this loader's triangle and quad support.
            {
                // N-gon function-call handling here later
                
                std::cerr << "N-gon found in loading model" << std::endl;
                return false; 
            }
            
            // read each token, filling into the temporary corners array
            if (!ReadFaceCorner(token, positions, uvs, normals, corners[count]))
            {
                return false;
            }
            ++count;                                    // tracks the actual corner amount for this face 
        }
        
        if (count < 3)                                  // needs to be a valid triangle face at least
        {
            return false;
        }
        
        // Quad face Handling:
        bool useFirstDiagonal = true;                   // quads need to be split into tris, try the a-c diagonal first. 
        if (count == 4)
        {
            const auto& a = corners[0].vertex.position;
            const auto& b = corners[1].vertex.position;
            const auto& c = corners[2].vertex.position;
            const auto& d = corners[3].vertex.position;
            
            // For each split, use the corner positions to compare the two triangle normals.
            //  On a flat quad, a positive dot product means the triangles face the same way.
            //  A negative result means they face opposite ways.
            //  Zero means at least one triangle has collapsed to a line or point.
            
            // Try the a-c diagonal with triangles (a, b, c) and (a, c, d).
            useFirstDiagonal = glm::dot(                
                glm::cross(b - a, c - a),           
                glm::cross(c - a, d - a)            
            ) > 0.0f;                                   
            
            // If a-c fails, try b-d with triangles (a, b, d) and (b, c, d).
            // Reject the face if this split also fails.
            if (!useFirstDiagonal && glm::dot(
                    glm::cross(b - a, d - a),       
                    glm::cross(c - b, d - b)        
                ) <= 0.0f)
            {
                return false;
            }
        }
        
        
        // Start with both flags set for a new mesh. 
        // Each stays true only if every corner appended to the mesh supplies that attribute.
        if (mesh.vertices.empty())                      
        {
            mesh.hasUVs = true;
            mesh.hasNormals = true;
        }
        
        // The current size is the index the first new vertex will get.
        const auto first = static_cast<unsigned int>(mesh.vertices.size()); // current size before anything is appended.  
        
        for (std::size_t i = 0; i < count; ++i)
        {
            mesh.vertices.push_back(corners[i].vertex);                     // copies each temp-corner’s assembled vertex into mesh.vertices
            mesh.hasUVs = mesh.hasUVs && corners[i].hasUV;                  // turns false when the first record lacking uv is added 
            mesh.hasNormals = mesh.hasNormals && corners[i].hasNormal;      // turns false when the first record lacking normals is added 
        }
        
        
        const std::size_t firstIndex = mesh.indices.size();                 // for adding FaceRange
        
        
        // Recording the indices in correct order: 
        // Each group of three indices draws one triangle from the new vertices.
        if (count == 3)
        {
            // For tris: appends the listed numbers to the end of the index vector.
            mesh.indices.insert(mesh.indices.end(), {first, first + 1, first + 2}); 
        }
        else if (useFirstDiagonal)      // For quads: appending order depends on what direction each tri should be drawn in
        {
            // Split along a-c into (a, b, c) and (a, c, d).
            mesh.indices.insert(mesh.indices.end(), {
                first, first + 1, first + 2,                        // (a, b, c)	Top-left, top-right, bottom-right 
                first, first + 2, first + 3                         // (a, c, d)	Top-left, bottom-right, bottom-left
            });
        }
        else
        {
            // Split along b-d into (a, b, d) and (b, c, d).
            mesh.indices.insert(mesh.indices.end(), {
                first, first + 1, first + 3,                        // (a, b, d)	Top-left, top-right, bottom-left
                first + 1, first + 2, first + 3                     // (b, c, d)	Top-right, bottom-right, bottom-left
            });
        }
        
        
        // Records one range per original f line to FaceRange
        mesh.faces.push_back({firstIndex, mesh.indices.size() - firstIndex});  
        
        
        return true;                                               // face was successfully appended to the MeshData 
    }
    
}





namespace OBJ_Loader
{
    
    // Reads an OBJ file into CPU-side meshes, using `o` lines as mesh boundaries.
    // Optionally flips the vertical UV coordinate. Returns an empty vector on failure.
    // If skipInvalidFaces is true, log and skip rejected faces. Otherwise, abort the import.
    std::vector<AcornData::CpuMeshData> LoadMesh(const char* filePath, bool flipUVs, bool skipInvalidFaces)
    {
        std::ifstream file(filePath);                               // creates an input file stream and tries to open the file at that path.
        if (!file)                                                  // file:	The OBJ file on disk
        {
            std::cerr << "Could not open OBJ: " << filePath << '\n';
            return {};
        }
        
        // Temp containers to keep data while reading, before being properly stored in a MeshData
        // Keeps the OBJ's raw attribute lists for the whole file.
        // Faces in later objects can reference entries defined before an `o` line.
        std::vector<glm::vec3> temp_positions;
        std::vector<glm::vec2> temp_uvs;
        std::vector<glm::vec3> temp_normals;
        
        std::vector<AcornData::CpuMeshData> meshes;                    // the finished meshes, what LoadModel() eventually returns.
        AcornData::CpuMeshData mesh;                                   // the current mesh being worked on that AppendFace() is building.
        
        
        // Lambda: If mesh that was worked on has recorded info, adds the current mesh to the finished list.
        const auto finishMesh = [&]()
        {
            if (!mesh.indices.empty())
            {
                meshes.push_back(std::move(mesh));                  // transfer the mesh's vectors without copying them.
                mesh = AcornData::CpuMeshData{};                       // start a fresh mesh for the next object.
            }
        };
        
        
        std::string line;                                           // will hold the current line’s text.
        std::size_t lineNumber = 0;                                 // starts at 0 and increases after each successful line read, so the first line is numbered 1
        
        // Lambda:  Report the file, current line number, and reason for the failure.
        //          `return fail(...)` exits LoadMesh() with an empty result.
        const auto fail = [&](const char* reason)
        {
            std::cerr << filePath << ':' << lineNumber << ": " << reason << '\n';
            return std::vector<AcornData::CpuMeshData>{};              // returns an empty vector to whoever called LoadModel()
        };
        
        // Read one line from the file per iteration, including blank lines.
        while (std::getline(file, line))                      // tries to read the next line from the OBJ file. 
        {                                                           // It replaces the contents of line, removes the newline delimiter from the text it stores,
            ++lineNumber;                                           // and leaves the stream ready to read the following line.
                                                                    // getline() returns the stream, and the while condition checks whether the read succeeded.
            
            // Lambda:  Strip comments by searching for the first #. 
            //          If one exists, erase(comment) deletes it and everything after it. 
            //          A line like "v 1 0 0 # right corner" becomes "v 1 0 0 ". A line that starts with # becomes empty.
            const auto comment = line.find('#');
            if (comment != std::string::npos)
            {
                line.erase(comment);
            }
            
            // Read the record type. The stream then holds the remaining fields.
            std::istringstream fields(line);                        // fields:	The text of the current line
            std::string record; 
            if (!(fields >> record))                                // (fields >> record) reads the first whitespace-separated word. For "v 1 0 0", record becomes "v" and fields is positioned just after that word, ready to read 1.
            {
                continue;                                           // if line was empty, whitespace-only, or just a comment that you erased, the read fails and skips line.
            }
            
            // Read record and store fields-content in correct temp container pos/uv/norm, 
            // or time to append faces, or finnish and store the current mesh.
            if (record == "v" || record == "vn")                    // expects three numbers to follow either of these signs
            {
                // Both records use three components. The record type chooses the list.
                glm::vec3 value{};
                if (!(fields >> value.x >> value.y >> value.z))
                {
                    return fail("Expected three position or normal components");
                }

                auto& values = record == "v" ? temp_positions : temp_normals;
                values.push_back(value);
            }
            else if (record == "vt")                                // expects two numbers to follow this sign      (or one number if one-dimensional texture) 
            {
                // U is required. A vt entry supplies U and may omit V. Store an omitted V as 0 before flipping.
                // OBJ supports one-dimensional textures.
                glm::vec2 uv{0.0f};
                if (!(fields >> uv.x))
                {
                    return fail("Expected a texture coordinate");
                }
                
                // Skip trailing spaces before checking whether a V field is present.
                fields >> std::ws;
                if (!fields.eof() && !(fields >> uv.y))
                {
                    return fail("Invalid second texture coordinate");
                }

                if (flipUVs)
                {
                    uv.y = 1.0f - uv.y;
                }
                temp_uvs.push_back(uv);
            }
            else if (record == "o")                                 // reached a new mesh, so finnish up the current one 
            {
                // A new object ends the current mesh. This loader discards the name.
                finishMesh();
            }
            else if (record == "f")                                 // start making faces of the collected v/vt/vn records 
            {
                // Resolve the corner references and append vertices and triangle indices.
                if (!AppendFace(fields, temp_positions, temp_uvs, temp_normals, mesh))
                {
                    if (skipInvalidFaces == false)
                    {
                        return fail("Invalid face or index. Expected a triangle or quad.");
                    }
                    
                    std::cerr << filePath << ':' << lineNumber
                              << ": Skipping invalid or unsupported face." << std::endl;              
                }
            }
        }
        
        // Check for a serious stream error after the read loop ends.
        if (file.bad())                                             
        {
            return fail("Error while reading OBJ");
        }
        
        
        // The last mesh ends at EOF, so save it here, and then return the result.
        finishMesh();                                               

        if (meshes.empty())
        {
            return fail("No faces found");
        }

        return meshes;
    }
    
    
    
}

