#include "model.hpp"

#include <core/io/logger.hpp>
#include <core/math/vector3.hpp>
#include <core/io/zipfile.hpp>
#include <core/math/matrix.hpp>
#include <core/math/mat4.hpp>

namespace s2 {
#pragma pack(push,1)
    struct SMDLHeader {
        uint32_t version;
        uint32_t meshcount;
        uint32_t spritecount;
        uint32_t surfcount;
        uint32_t bonecount;
        vector3f bbmin;
        vector3f bbmax;
    };
    struct SMDLBlockMesh {
        uint32_t Id;
        uint32_t Mode;
        uint32_t NumVertices;
        vector3f BoundingMin;
        vector3f BoundingMax;
        uint32_t BoneLink;
        uint8_t MeshNameLength;
        uint8_t MatNameLength;
    };
    struct SMDLBlockSurf {
        uint32_t Prop0;
        uint32_t NumPlanes;
        uint32_t NumPoints;
        uint32_t NumEdges;
        uint32_t NumTris;
    };
    static_assert(sizeof(SMDLBlockMesh) == 42);
#pragma pack(pop)
    bool LoadHeader(core::bytestream& stream, SMDLHeader* out) {
        out->version = stream.readDword();
        if (out->version != 3) {
            core::error("SMDLHeader invalid version %d (expected 3)\n", out->version);
            return false;
        }

        out->meshcount = stream.readDword();
        out->spritecount = stream.readDword();
        out->surfcount = stream.readDword();
        out->bonecount = stream.readDword();
        out->bbmin = stream.read<vector3f>();
        out->bbmax = stream.read<vector3f>();
        return true;
    }

    std::shared_ptr<model> model::Load(core::bytestream& stream) {
        const bool verboseLoad = false;
        if (stream.readDwordBE() != 'SMDL') {
            core::warning("Invalid SMDL signature.\n");
            return nullptr;
        }
        model modelOut;

        struct SMDLBlock {
            uint32_t name;
            size_t offset;
            uint32_t length;
        };
        vector<SMDLBlock> blocks;
        while (!stream.eof()) {
            uint32_t blockname = stream.readDwordBE();
            uint32_t len = stream.readDword();
            SMDLBlock block;
            block.name = blockname;
            block.offset = stream.tell();
            block.length = len;
            stream.advance(len);
            blocks.push_back(block);
        }

        SMDLHeader hdr;
        if (blocks[0].name != 'head')
            core::error("SMDL has invalid start block.");

        stream.seek(blocks[0].offset);
        if (!LoadHeader(stream, &hdr))
            core::error("Failed to load model header.");

        modelOut.mBounds.min = hdr.bbmin;
        modelOut.mBounds.max = hdr.bbmax;

        struct MeshInfo {
            string name;
            string material;
            SMDLBlockMesh data;
        };
        
        for (int i = 1; i < (int)blocks.size(); i++) {
            stream.seek(blocks[i].offset);
            switch (blocks[i].name) {
            case 'mesh':
            {
                auto meshdata = stream.read<SMDLBlockMesh>();
                auto name = stream.readString();
                stream.seek(blocks[i].offset + sizeof(SMDLBlockMesh) + name.length() + 1);
                auto mat = stream.readString();
                if(verboseLoad)
                    core::info("Read meshdata #%d : %s(%s)\n", meshdata.Id, name, mat);
                //meshes[meshdata.Id] = { .name = name, .material = mat, .data = meshdata };
                if (modelOut.NumMeshes() != meshdata.Id)
                    core::error("Read meshdata id #%d out of order (num meshdataes %d)\n", meshdata.Id, modelOut.NumMeshes());
                auto& mesh = modelOut.NewMesh();
                mesh.name = name;
                mesh.material = mat;
                mesh.mode = meshdata.Mode;
                if (name.find("_foliage") != string::npos)
                    mesh.flags |= MeshFlags::Foliage;
                else if (name.find("_nohit") != string::npos)
                    mesh.flags |= MeshFlags::NoHit;
                else if (name.find("_invis") != string::npos)
                    mesh.flags |= MeshFlags::Invis;
                else if (name.find("_trisurf") != string::npos)
                    mesh.flags |= MeshFlags::Invis;
                mesh.bounds.min = meshdata.BoundingMin;
                mesh.bounds.max = meshdata.BoundingMax;
                mesh.AllocVertices(meshdata.NumVertices);
            }
            break;
            case 'surf':
            {
                auto startpos = stream.tell();
                surface& surf = modelOut.mSurface;
                auto surfdata = stream.read<SMDLBlockSurf>();
                if (verboseLoad)
                    core::info("Surf block. %d planes, %d points, %d edges, %d tris\n",
                        surfdata.NumPlanes, surfdata.NumPoints,
                        surfdata.NumEdges, surfdata.NumTris);
                auto bminf = stream.read<vector3f>();
                auto bmaxf = stream.read<vector3f>();
                auto flags = stream.readDword();
                surf.planes.resize(surfdata.NumPlanes);
                for (uint32_t i = 0; i < surfdata.NumPlanes; i++) {
                    auto v = stream.read<vector3f>();
                    auto p = stream.readFloat();
                    surf.planes.push_back({ v, p });
                }
                auto ptsoffs = surf.points.size();
                for (uint32_t i = 0; i < surfdata.NumPoints; i++) {
                    surf.points.push_back(stream.read<vector3f>());
                }
                for (uint32_t i = 0; i < surfdata.NumEdges; i++) {
                    auto e1 = stream.read<vector3f>();
                    auto e2 = stream.read<vector3f>();
                    surf.edges.push_back(e1);
                    surf.edges.push_back(e2);
                }
                for (uint32_t i = 0; i < surfdata.NumTris; i++) {
                    surf.triangles.push_back(ptsoffs + stream.readDword());
                    surf.triangles.push_back(ptsoffs + stream.readDword());
                    surf.triangles.push_back(ptsoffs + stream.readDword());
                }
                surf.bounds.min = bminf;
                surf.bounds.max = bmaxf;
                surf.flags = flags;
                if (verboseLoad) {
                    core::info("   bminf %s\n", bminf.str());
                    core::info("   bmaxf %s\n", bmaxf.str());
                    core::info("   flags %X\n", flags);
                    auto endpos = stream.tell();
                    core::info("Parsed surf chunk %d/%d\n", endpos - startpos, blocks[i].length);
                }
            } break;
            case 'bone':
            {
                vector<string> bone_names;
                vector<int> bone_parents;
                bone_parents.resize(hdr.bonecount);
                bone_names.resize(hdr.bonecount);
                for (uint32_t i = 0; i < hdr.bonecount; i++) {
                    int parent_bone_idx = stream.readInt();

                    mat4f invmat = {
                        stream.readFloat(), stream.readFloat(), stream.readFloat(), 0.f,
                        stream.readFloat(), stream.readFloat(), stream.readFloat(), 0.f,
                        stream.readFloat(), stream.readFloat(), stream.readFloat(), 0.f,
                        stream.readFloat(), stream.readFloat(), stream.readFloat(), 1.f,
                    };
                    mat4f mat = {
                        stream.readFloat(), stream.readFloat(), stream.readFloat(), 0.f,
                        stream.readFloat(), stream.readFloat(), stream.readFloat(), 0.f,
                        stream.readFloat(), stream.readFloat(), stream.readFloat(), 0.f,
                        stream.readFloat(), stream.readFloat(), stream.readFloat(), 1.f,
                    };
                    auto nameLen = stream.readByte();
                    string name = stream.readString(nameLen);
                    bone_names[i] = name;
                    bone_parents[i] = parent_bone_idx;
                    stream.readByte();
                }
                for (uint32_t i = 0; i < hdr.bonecount; i++) {
                    auto parent_bone_idx = bone_parents[i];
                    auto name = bone_names[i];
                    if (verboseLoad) {
                        if (parent_bone_idx > 0)
                            core::info("Bone #%d %s <--- (#%d)%s\n", i, name, parent_bone_idx, bone_names[parent_bone_idx]);
                        else
                            core::info("Bone #%d %s\n", i, name);
                    }
                }
            } break;
            case 'vrts':
            {
                auto meshidx = stream.readDword();
                auto& mesh = modelOut.GetMesh(meshidx);
                mesh.vertices.resize(mesh.NumVertices());
                for (uint32_t i = 0; i < mesh.NumVertices(); i++) {
                    auto v = stream.read<vector3f>();
                    mesh.vertices[i] = v;
                }
                if (verboseLoad)
                    core::info("Read %d vertices.\n", mesh.NumVertices());
            } break;
            case 'face':
            {
                auto startpos = stream.tell();
                auto meshidx = stream.readDword();
                auto& mesh = modelOut.GetMesh(meshidx);
                auto numFaces = stream.readDword();
                auto idxsize = stream.readByte();
                mesh.AllocFaces(numFaces);
                switch (idxsize) {
                case 1:
                    for (uint32_t i = 0; i < numFaces; i++) {
                        mesh.faces[3 * i + 0] = (int)stream.readByte();
                        mesh.faces[3 * i + 1] = (int)stream.readByte();
                        mesh.faces[3 * i + 2] = (int)stream.readByte();
                    }
                    break;
                case 2:
                    for (uint32_t i = 0; i < numFaces; i++) {
                        mesh.faces[3 * i + 0] = (int)stream.readShort();
                        mesh.faces[3 * i + 1] = (int)stream.readShort();
                        mesh.faces[3 * i + 2] = (int)stream.readShort();
                    }
                    break;
                case 4:
                    for (uint32_t i = 0; i < numFaces; i++) {
                        mesh.faces[3 * i + 0] = (int)stream.readInt();
                        mesh.faces[3 * i + 1] = (int)stream.readInt();
                        mesh.faces[3 * i + 2] = (int)stream.readInt();
                    }
                    break;
                default:
                    core::error("Model 'face' chunk specified invalid index size %d\n", idxsize);
                }
                if (verboseLoad)
                    core::info("Read %d faces (%d/%d)\n", mesh.faces.size() / 3, stream.tell()-startpos, blocks[i].length);
            } break;
            case 'texc':
            {
                auto meshidx = stream.readDword();
                auto texidx = stream.readDword();
                auto& mesh = modelOut.GetMesh(meshidx);
                mesh.texcoords[texidx].resize(mesh.NumVertices());
                for (uint32_t i = 0; i < mesh.NumVertices(); i++) {
                    float u = stream.readFloat();
                    float v = stream.readFloat();
                    mesh.texcoords[texidx][i] = { u,v };
                }
                if (verboseLoad)
                    core::info("Read texc[%d] block (%d/%d)\n", texidx, mesh.NumVertices() * 8 + 8, blocks[i].length);
            } break;
            case 'tang':
            {
                auto meshidx = stream.readDword();
                auto tngidx = stream.readDword();
                auto& mesh = modelOut.GetMesh(meshidx);
                mesh.tangents[tngidx].resize(mesh.NumVertices());
                vector3f t;
                for (uint32_t i = 0; i < mesh.NumVertices(); i++) {
                    t = stream.read<vector3f>();
                    t.x = -t.x;
                    t.y = -t.y;
                    mesh.tangents[tngidx][i] = t;
                }
                if (verboseLoad) {
                    core::info("t (%.2f,%.2f,%.2f)\n", t.x, t.y, t.z);
                    core::info("Read tang block %d vertices (%d/%d)\n", mesh.NumVertices(), 12 * mesh.NumVertices() + 8, blocks[i].length);
                }
            } break;
            case 'nrml':
            {
                auto meshidx = stream.readDword();
                auto& mesh = modelOut.GetMesh(meshidx);
                vector3f n;
                mesh.normals.resize(mesh.NumVertices());
                for (uint32_t i = 0; i < mesh.NumVertices(); i++) {
                    n = stream.read<vector3f>();
                    n.x = -n.x;
                    n.y = -n.y;
                    mesh.normals[i] = n;
                }
                if (verboseLoad) {
                    core::info("n (%.2f,%.2f,%.2f) %g\n", n.x, n.y, n.z, n.length());
                    core::info("Read nrml block %d vertices (%d/%d)\n", mesh.NumVertices(), 12 * mesh.NumVertices() + 4, blocks[i].length);
                }
            } break;
            case 'colr':
            {
                auto meshidx = stream.readDword();
                auto clridx = stream.readDword();
                if (clridx > 1)
                    core::error("Invalid colors array index.\n");
                auto& mesh = modelOut.GetMesh(meshidx);
                mesh.colors[clridx].resize(mesh.NumVertices());
                for (uint32_t i = 0; i < mesh.NumVertices(); i++) {
                    auto c = stream.readDword();
                    mesh.colors[clridx][i] = c;
                }
                if (verboseLoad)
                    core::info("Read colr block %d vertices (%d/%d)\n", mesh.NumVertices(), 4 * mesh.NumVertices() + 4, blocks[i].length);
            } break;
            case 'lnk1':
            {
                // Blended link block
                auto meshidx = stream.readDword();
                auto& mesh = modelOut.GetMesh(meshidx);
                auto numverts = stream.readDword();
                if (mesh.NumVertices() != numverts) {
                    core::error("Invalid blended link block (num vertices didnt match mesh vertices)\n");
                }
                auto remaining  = blocks[i].length - 4 * numverts - 8;
            } break;
            case 'lnk2':
            {
                // Single link block
                auto meshidx = stream.readDword();
                auto& mesh = modelOut.GetMesh(meshidx);
                auto numverts = stream.readDword();
                if (mesh.NumVertices() != numverts) {
                    core::error("Invalid single link block (num vertices didnt match mesh vertices)\n");
                }
                auto remaining = blocks[i].length - 4 * numverts - 8;
            } break;
            case 'sign':
            {
            } break;
            default:
            {
                char* name = (char*)&blocks[i].name;
                core::warning("Unhandled block %c%c%c%c\n", name[3], name[2], name[1], name[0]);
            } break;
            }
        }
        return std::make_shared<model>(modelOut);
    }
}
