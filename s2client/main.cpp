#include <stdio.h>
#include <core/prerequisites.hpp>
#include <core/utils/input.hpp>
#include <core/io/logger.hpp>
#include <network/network.hpp>
#include <network/httpclient.hpp>
#include <s2/resourcemanager.hpp>
#include <s2/masterserver.hpp>
#include <s2/userclient.hpp>
#include <s2/replay.hpp>
#include <core/math/vector3.hpp>
#include <core/utils/color.hpp>

#include <core/win/window.hpp>
#include <core/ogl/glrenderer.hpp>
#include <core/ogl/shaders.hpp>
#include <core/ogl/gltex2d.hpp>

#include <core/ai/neuralnet.hpp>
#include <core/math/vectorN.hpp>
#include <core/math/matrix.hpp>
#include <core/math/geom.hpp>
#include <core/io/zipfile.hpp>
#include <s2/model.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <ext/stb/stb_image.h>
#include <ext/tinyxml2/tinyxml2.h>

void UIThread(s2::userclient* client) {
    core::window wnd(640, 480);
    if (!wnd.create())
        core::error(">:(");

    wnd.show();

    string knownworld;
    core::glrenderer gfx(wnd);
    {
        core::gltex2d heightmaptex(wnd.width(), wnd.height(), GL_RGBA);
        mat4f worldToScreen = mat4f::identity();
        vector3f camScale(1.f, 1.f, 1.f);
        while (client->connected()) {
            gfx.clear(Color::White);
            if (client->ingame()) {
                auto world = client->currentworld();
                if (world && 0 != world->name().compare(knownworld)) {
                    if (!knownworld.empty())
                        heightmaptex.reset();
                    worldToScreen = mat4f::scale(heightmaptex.width / world->worldsize(), heightmaptex.height / world->worldsize(), 0.f);
                    core::info("Processing world %s\n", world->name());
                    float* pixelData = new float[4 * size_t(heightmaptex.width) * heightmaptex.height];
                    const std::pair<float, core::color> heightclrs[] = {
                        {-32768.0f, core::color(25,  42,  86)},
                        {-5000.0f,  core::color(39,  60,  117)},
                        {-1000.0f,  core::color(0,   148, 50)},
                        {0.0f,      core::color(68,  189, 50)},
                        {200.0f,    core::color(76,  209, 55)},
                        {500.0f,    core::color(100, 112, 12)},
                        {800.0f,    core::color(62,  71,  81)},
                        {1200.0f,   core::color(50,  50,  51)},
                        {3001.0f,   core::color(245, 246, 250)}
                    };
                    const int nclrs = sizeof(heightclrs) / sizeof(heightclrs[0]);
                    auto h2col = [&heightclrs](float height) -> core::color {
                        auto col = heightclrs[0].second;
                        for (int i = 1; i < nclrs; i++) {
                            if (height < heightclrs[i].first) {
                                float m = (height - heightclrs[i - 1].first) / (heightclrs[i].first - heightclrs[i - 1].first);
                                m = max(0.f, min(m, 1.f));
                                m = int(m * 2) * 0.5f;
                                col = heightclrs[i - 1].second + (heightclrs[i].second - heightclrs[i - 1].second) * m;
                                break;
                            }
                            else {
                                col = heightclrs[i].second;
                            }
                        }
                        return col;
                    };
                    float worldsize = world->worldsize();
                    for (int y = 0; y < heightmaptex.height; y++) {
                        for (int x = 0; x < heightmaptex.width; x++) {
                            auto clr = (core::color*) & pixelData[4 * (y * heightmaptex.width + x)];
                            float worldx = (float(x) * worldsize) / heightmaptex.width;
                            float worldy = (float(heightmaptex.height - y) * worldsize) / heightmaptex.height;
                            bool blocked = world->isblocked(worldx, worldy);
                            if (blocked) {
                                *clr = Color::Black;
                            }
                            else {
                                auto height = world->terrainheight(worldx, worldy);
                                auto slope = world->terrainslope(worldx, worldy);
                                if (slope > 1.25f)
                                    *clr = Color::Red;
                                else
                                    *clr = h2col(height);
                            }
                        }
                    }
                    heightmaptex.generate(GL_RGBA, GL_FLOAT, pixelData);
                    delete[] pixelData;
                    core::info("Finished generating world!\n");
                    knownworld = world->name();
                }
            }
            gfx.ortho();
            mat4f view;
            view = mat4f::scale(camScale.x, camScale.y, camScale.z);
            gfx.view(view);
            if (GetAsyncKeyState(VK_ADD) & 1)
                camScale.x = camScale.y = camScale.x + 0.1f;
            else if (GetAsyncKeyState(VK_SUBTRACT) & 1)
                camScale.x = camScale.y = camScale.x - 0.1f;
            gfx.texrect(heightmaptex, 0, 0, 640, 480, Color::White);

            auto clientinfo = client->clientinfo();
            auto local = client->localent();
            if (client->ingame() && clientinfo && local) {
                auto title = core::format("cid %d; health %.2f; team %d; status %d; recvd %d; sent %d;",
                    clientinfo->clientNumber, local->m_fHealth, local->m_iTeam, local->m_yStatus,
                    client->recvdsnapshots(), client->sentsnapshots());
                SetConsoleTitleA(title.c_str());

                static bool drawnavmesh = false;
                if (GetAsyncKeyState('N') & 1)
                    drawnavmesh = !drawnavmesh;
                if (drawnavmesh) {
                    auto lines = client->currentworld()->navmeshlines();
                    if (!lines.empty()) {
                        for (size_t i = 0; i < lines.size(); i++) {
                            lines[i] = worldToScreen * lines[i];
                        }
                        gfx.lines(Color::Black, lines.data(), lines.size() / 2);
                    }
                }
                
                auto& game = client->game();
                auto& ents = game.entities();
                for (auto& ep : ents) {
                    auto& e = ep.second;
                    auto tn = e.typname();
                    auto worldPos = e.m_v3Position;
                    auto screenPos = worldToScreen * worldPos;
                    if (e.dormant())
                        continue;
                    if (tn.starts_with("Player")) {
                        auto clr = Color::White;
                        if (e.m_yStatus == 2)
                            clr = Color::Indigo;//0x3D9970;
                        if (e.m_yStatus == 3)
                            clr = 0xB10DC9;
                        if (e.m_yStatus == 4)
                            clr = 0x85144B;
                        gfx.rect(clr, screenPos.x - 1.f, screenPos.y - 1.f, screenPos.x + 1.f, screenPos.y + 1.f);
                    }
                    else if (tn.starts_with("Npc")) {
                        gfx.rect(Color::Yellow, screenPos.x - 2.f, screenPos.y - 2.f, screenPos.x + 2.f, screenPos.y + 2.f);
                    }
                    else if (tn.starts_with("Gadget")) {
                        gfx.rect(Color::Red, screenPos.x - 3.f, screenPos.y - 3.f, screenPos.x + 3.f, screenPos.y + 3.f);
                    }
                    else if (tn.starts_with("Building")) {
                        gfx.rect(Color::Red, screenPos.x - 7.f, screenPos.y - 7.f, screenPos.x + 7.f, screenPos.y + 7.f);
                    }
                }
                auto props = client->currentworld()->props();
                for (auto& prop : props) {
                    auto worldPos = prop.pos;
                    auto screenPos = worldToScreen * worldPos;
                    auto min = prop.model->BBMin() * prop.scale;
                    auto max = prop.model->BBMax() * prop.scale;
                    auto screenMin = worldToScreen * (worldPos + min);
                    auto screenMax = worldToScreen * (worldPos + max);
                    auto c1 = min; auto c2 = vector3f(min.x, max.y, min.z); auto c3 = max; auto c4 = vector3f(max.x, min.y, max.z);
                    auto mrot = mat4f::zrotation(-prop.angles.z * float(M_PI) / 180.f);
                    c1 = mrot * c1; c2 = mrot * c2; c3 = mrot * c3; c4 = mrot * c4;
                    c1 = worldToScreen * (worldPos + c1);
                    c2 = worldToScreen * (worldPos + c2);
                    c3 = worldToScreen * (worldPos + c3);
                    c4 = worldToScreen * (worldPos + c4);
                    //c1.z = c2.z = c3.z = c4.z = 0.f;
                    if (prop.type.find("_Mine") != string::npos) {
                        vector3f tris[6] = {
                            c1, c2, c3,
                            c1, c3, c4
                        };
                        gfx.triangles(Color::Orange, tris, 2);
                    }
                    else {
                        gfx.line(Color::Magenta, c1.x, c1.y, c2.x, c2.y);
                        gfx.line(Color::Magenta, c2.x, c2.y, c3.x, c3.y);
                        gfx.line(Color::Magenta, c3.x, c3.y, c4.x, c4.y);
                        gfx.line(Color::Magenta, c4.x, c4.y, c1.x, c1.y);
                    }
                }

                deque<vector3f> path = client->path();
                if (!path.empty()) {
                    for (size_t i = 0; i < path.size() - 1; i++) {
                        auto p0 = worldToScreen * path[i];
                        auto p1 = worldToScreen * path[i + 1];
                        gfx.line(Color::White, p0.x, p0.y, p1.x, p1.y);
                    }
                }
                
                auto me = game.localent();
                if (me) {
                    if (!client->haspath()) {
                        auto strongholds = game.entsoftype("Building_Stronghold");
                        auto lairs = game.entsoftype("Building_Lair");
                        if (!strongholds.empty() && !lairs.empty()) {
                            auto stronghold = strongholds.back();
                            auto lair = lairs.back();
                            auto target = ((me->m_v3Position - stronghold->m_v3Position).length() > (me->m_v3Position - lair->m_v3Position).length()) ? stronghold : lair;
                            client->pathtowards(target->m_v3Position, 825.f);
                        }
                    }
                }
            }
            //gfx.line(Color::Olive, 0, 0, 640, 480);
            gfx.flush();
            wnd.pump();
            Sleep(50);
        }
    }
}

void DisplayModel(core::glrenderer& gfx, core::window& wnd, std::shared_ptr<s2::model>& model) {
    static vector3f campos = vector3f(200.f, 200.f, 50.f);
    auto& surf = model->mSurface;
    vector<vector3f> vrts;
    vector<vector3f> bounds;
    if (!surf.triangles.empty()) {
        bounds.push_back(surf.bounds.min);
        bounds.push_back(surf.bounds.max);
    }
    for (uint32_t i = 0; i < (surf.triangles.size() / 3); i++) {
        vrts.push_back(surf.points[surf.triangles[3 * i + 0]]);
        vrts.push_back(surf.points[surf.triangles[3 * i + 1]]);
        vrts.push_back(surf.points[surf.triangles[3 * i + 2]]);
    }
    int surf_numverts = (int)vrts.size();
    static bool render_surf = true;
    for (uint32_t i = 0; i < model->NumMeshes(); i++) {
        auto& mesh = model->GetMesh(i);
        if (!(mesh.flags & s2::MeshFlags::NoHit)) {
            bounds.push_back(mesh.bounds.min);
            bounds.push_back(mesh.bounds.max);
        }
        for (uint32_t j = 0; j < mesh.NumFaces(); j++) {
            vrts.push_back(mesh.vertices[mesh.faces[3 * j + 0]]);
            vrts.push_back(mesh.vertices[mesh.faces[3 * j + 1]]);
            vrts.push_back(mesh.vertices[mesh.faces[3 * j + 2]]);
        }
    }
    auto start_time = std::chrono::high_resolution_clock::now();
    vector3f center = (surf.bounds.max - surf.bounds.min) * 0.5f + surf.bounds.min;
    wnd.show();
    do {
        static auto prev_time = start_time;
        auto time = std::chrono::high_resolution_clock::now();
        auto frametime = std::chrono::duration_cast<std::chrono::milliseconds>(time - prev_time).count() * 0.001f;
        prev_time = time;
        auto mselapsed = std::chrono::duration_cast<std::chrono::milliseconds>(time - start_time).count();
        float felapsed = mselapsed * 0.001f;
        float ang = felapsed;
        if (GetAsyncKeyState('X') & 1)
            render_surf = !render_surf;
        if (GetAsyncKeyState('Q') & 0x8000)
            campos.z += 200.f * frametime;
        else if (GetAsyncKeyState('Z') & 0x8000)
            campos.z -= 200.f * frametime;
        if (GetAsyncKeyState(VK_UP) & 0x8000)
            campos -= campos.unit() * 200.f * frametime;
        else if (GetAsyncKeyState(VK_DOWN) & 0x8000)
            campos += campos.unit() * 200.f * frametime;
        gfx.clear(Color::White);
        gfx.perspective();
        gfx.lookat(center + vector3f(campos.x * cosf(ang), campos.y * sinf(ang), campos.z), center, vector3f(0.f, 0.f, 1.f));

        gfx.triangles(Color::Blue, vrts.data() + surf_numverts, int(vrts.size() - surf_numverts) / 3);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        gfx.triangles(Color::Forestgreen, vrts.data(), surf_numverts / 3);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        //if(render_surf)
        //  gfx.triangles(Color::Forestgreen, vrts.data(), surf_numverts / 3);
        //else
        //  gfx.triangles(Color::Blue, vrts.data() + surf_numverts, (vrts.size() - surf_numverts) / 3);

        /*for (uint32_t i = 0; i < bounds.size(); i += 2) {
            auto& min = bounds[i];
            auto& max = bounds[i + 1];
            vector3f pts[] = {
                vector3f(min.x, min.y, min.z),
                vector3f(max.x, min.y, min.z),
                vector3f(max.x, max.y, min.z),
                vector3f(min.x, max.y, min.z),

                vector3f(min.x, min.y, max.z),
                vector3f(max.x, min.y, max.z),
                vector3f(max.x, max.y, max.z),
                vector3f(min.x, max.y, max.z),

            };
            vector3f lines[] = {
                pts[0], pts[1],
                pts[1], pts[2],
                pts[2], pts[3],
                pts[3], pts[0],

                pts[4], pts[5],
                pts[5], pts[6],
                pts[6], pts[7],
                pts[7], pts[4],

                pts[0], pts[4],
                pts[1], pts[5],
                pts[2], pts[6],
                pts[3], pts[7]
            };
            auto clr = Color::Red;
            if (!surf.triangles.empty() && i == 0)
                clr = Color::Violet;
            for (int i = 0; i < (sizeof(lines) / sizeof(lines[0])); i += 2)
                gfx.line(clr, lines[i], lines[i + 1]);
        }*/
        gfx.line(Color::Magenta, vector3f(0, 0, 0), vector3f(0.f, 0.f, 10.f));
        gfx.line(Color::Yellow, vector3f(0, 0, 0), vector3f(0.f, 10.f, 0.f));
        gfx.line(Color::Cyan, vector3f(0, 0, 0), vector3f(10.f, 0.f, 0.f));

        gfx.flush();
        wnd.pump();
        Sleep(5);
    } while (0 == (GetAsyncKeyState(VK_RETURN) & 1));
}

int main(int argc, char** argv) {
    core::info("Hello :o\n");
    network::init();
    auto masterserver = s2::masterserver();

    // geometry tests
    if (false) {
        vector3f LA(3.f, 3.f, -5.f);
        vector3f LB(3.f, 3.f, 5.f);
        vector3f PlaneNormal(0.f, 0.f, 1.f);
        vector3f PlanePt(0.8f, 0.5f, 0.5f);
        float tintersect;
        bool bIntersects = math::LineIntersectsPlane(LA, LB, PlaneNormal, PlanePt, &tintersect);
        
        if (bIntersects) {
            core::print("Line %s->%s intersects plane @ [%.2f]%s\n", LA.str(), LB.str(), tintersect, (LA + tintersect*(LB-LA)).str());
        }
        else
            core::print("Line did not intersect plane.\n");

        vector3f tri[3] = {
            vector3f(1.f, 0.f, 0.f),
            vector3f(0.f, 1.f, 1.f),
            vector3f(10.f, 10.f, 0.f)
        };
        bIntersects = math::LineIntersectsTriangle(LA, LB, tri[0], tri[1], tri[2], &tintersect);
        if (bIntersects) {
            core::print("Line %s->%s intersects triangle @ [%.2f]%s\n", LA.str(), LB.str(), tintersect, (LA + tintersect * (LB - LA)).str());
        }
        else
            core::print("Line did not intersect triangle.\n");

        getc(stdin);
    }

    // quadtree tests
    if(false) {
        struct QTest {
            vector3f pos;
            int lol;
        };
        vector<QTest> qtests;
        const int numNodes = 500;
        auto qt = core::quadtree<QTest>(640.f, 480.f, 1.f);
        for (int i = 0; i < numNodes; i++) {
            QTest x;
            x.pos.x = (float)core::random::int32(0,640);
            x.pos.y = (float)core::random::int32(480);
            x.pos.z = 0.f;
            x.lol = core::random::uint32();
            qtests.push_back(x);
        }
        for (int i = 0; i < numNodes; i++) {
            auto x = &qtests[i];
            auto bnds = core::rect(x->pos.x - 1.f, x->pos.y - 1.f, 2.f, 2.f);
            qt.insert(bnds, x);
        }


        core::window wnd(640, 480);
        if (!wnd.create())
            core::error(">:(");

        wnd.show();

        core::glrenderer gfx(wnd);
        gfx.clear(Color::White);
        gfx.ortho();

        auto start_time = std::chrono::high_resolution_clock::now();
        wnd.show();
        while (!(GetAsyncKeyState(VK_ESCAPE) & 1)) {
            auto time = std::chrono::high_resolution_clock::now();
            auto mselapsed = std::chrono::duration_cast<std::chrono::milliseconds>(time - start_time).count();
            float felapsed = mselapsed * 0.001f;
            gfx.clear(Color::White);
            qt.clear();
            for (int i = 0; i < numNodes; i++) {
                QTest& x = qtests[i];
                x.pos.x += 2.f*sinf(felapsed * 2.f * (i / (float)numNodes));//(float)core::random::int32(320 + (int)(320.f*sinf(felapsed)), 640);
                x.pos.y += 2.f*cosf(felapsed * 2.f * (i / (float)numNodes));//(float)core::random::int32(240 + (int)(240.f*cosf(felapsed)));
                x.pos.z = 0.f;
                x.lol = core::random::uint32();
            }
            for (int i = 0; i < numNodes; i++) {
                auto x = &qtests[i];
                auto bnds = core::rect(x->pos.x - 1.f, x->pos.y - 1.f, 2.f, 2.f);
                qt.insert(bnds, x);
            }
            for (int i = 0; i < numNodes; i++) {
                auto& x = qtests[i];
                gfx.rect(Color::Violet, x.pos.x - 1.f, x.pos.y - 1.f, x.pos.x + 1.f, x.pos.y + 1.f);
            }

            auto x = qt.root();
            vector<decltype(&x)> nodes;
            nodes.push_back(&x);
            while (!nodes.empty()) {
                auto current = nodes.back();
                nodes.pop_back();
                if (current->leaf()) {
                    auto bnds = current->bounds;
                    gfx.line(Color::Violet, vector3f(bnds.x, bnds.y, 0.f), vector3f(bnds.x + bnds.w, bnds.y, 0.f));
                    gfx.line(Color::Violet, vector3f(bnds.x + bnds.w, bnds.y, 0.f), vector3f(bnds.x + bnds.w, bnds.y + bnds.h, 0.f));
                    gfx.line(Color::Violet, vector3f(bnds.x + bnds.w, bnds.y + bnds.h, 0.f), vector3f(bnds.x, bnds.y + bnds.h, 0.f));
                    gfx.line(Color::Violet, vector3f(bnds.x, bnds.y + bnds.h, 0.f), vector3f(bnds.x, bnds.y, 0.f));
                }
                else {
                    nodes.push_back(current->nw);
                    nodes.push_back(current->ne);
                    nodes.push_back(current->se);
                    nodes.push_back(current->sw);
                }
            }
            gfx.rect(Color::Gold, 300.f, 220.f, 300.f+40.f, 220.f+40.f);
            auto results = qt.query(core::rect(300.f, 220.f, 40.f, 40.f));
            for(auto e : results) {
                gfx.rect(Color::Cyan, e->pos.x-5.f, e->pos.y-5.f, e->pos.x+5.f, e->pos.y+5.f); 
            }

            gfx.flush();
            wnd.pump();
            Sleep(5);
        }

        qt.clear();
        core::info("lol");
        getc(stdin);
    }

    auto resources = core::zipfile::Load("D:\\vmware\\Win10 20H2\\Shared\\resources0.s2z");
    if (!resources)
        core::error("Couldn't find resources0.s2z");
    s2::gResourceManager->LoadResources(*resources);
    if (false && resources) {
        core::window wnd(640, 480);
        if (!wnd.create())
            core::error(">:(");

        wnd.show();

        core::glrenderer gfx(wnd);
        gfx.clear(Color::White);
        gfx.perspective();
        gfx.lookat(vector3f(200.f, 5.f, 200.f), vector3f(0.f,0.f,0.f), vector3f(0.f, 0.f, 1.f));

        //auto mdldata = resources->file("world/props/village/stone_tower.model");
        auto mdldata = resources->file("world/props/tools/blocker_med.model");
        if (mdldata) {
            core::info("Got model stream.\n");

            auto model = s2::model::Load(*mdldata);
            DisplayModel(gfx, wnd, model);
        }

        //gfx.rect(Color::Red, -10.f, -10.f, 10.f, 10.f);
        gfx.flush();
        wnd.pump();
        Sleep(50);

        auto numfiles = resources->numfiles();
        for (unsigned int i = 0; i < numfiles; i++) {
            auto name = resources->namebyidx(i);
            //core::info("Loading %s\n", name);
            if (name.ends_with(".mdf")) {

                tinyxml2::XMLDocument doc;
                auto xmldata = resources->file(name);
                auto err = doc.Parse((const char*)xmldata->data(), xmldata->length());

                if (err != tinyxml2::XML_SUCCESS) {
                    core::warning("Failed to parse %s error %d\n", name, err);
                }
                else {
                    auto emodel = doc.FirstChildElement("model");
                    const char* filename = nullptr;
                    emodel->QueryStringAttribute("file", &filename);
                    string mdlfilename;
                    if (filename[0] == '/')
                        mdlfilename = &filename[1];
                    else
                        mdlfilename = name.substr(0, name.find_last_of('/') + 1) + filename;
                    if (!resources->file(mdlfilename))
                        core::warning("Model file doesn't exist %s -> %s\n", name, mdlfilename);
                    else {
                        core::info("%s -> %s\n", name, mdlfilename);
                    }
                }
                xmldata = nullptr;
            //  auto mdldata = resources->file(name);
            //  if (mdldata) {
            //      core::info("Got model stream.\n");

            //      auto model = s2::model::Load(*mdldata);
            //      core::info("Loaded %s\n", name);
            //      if (true) {//name.find("unit") != string::npos) {
            //          DisplayModel(gfx, wnd, model);

            //      }
            //      model = nullptr;
            //      mdldata = nullptr;
            //  }
            }
        }
    }

    resources = nullptr;
    core::info("Finished loading resources.\n");
    //getc(stdin);

    //auto Id = core::matrix::identity(4);
    //auto test = core::matrix({
    //  { 2.f, 0.f, 0.f, 0.f }, 
    //  { 0.f, 2.f, 0.f, 0.f },
    //  { 0.f, 0.f, 2.f, 0.f },
    //  { 0.f, 0.f, 0.f, 2.f }
    //  });
    //auto meme = core::matrix{ {1.f}, {2.f}, {5.f}, {10.f} };
    //auto result = test * meme;//Id * core::matrix{ {1.f, 0.f, 0.f, 0.f}, { 0.f,1.f,0.f,0.f }, { 0.f,0.f,1.f,0.f }, { 10.f, 10.f, 50.f, 1.f } };
    //core::info("%.2f, %.2f, %.2f, %.2f\n", result.get(0, 0), result.get(0, 1), result.get(0, 2), result.get(0, 3));
    ////DebugBreak();

    //nn::neuralnetwork<2, 1> netlol;
    //auto optimizer = nn::optimizers::RMSprop(0.0005f);
    //netlol.layer<nn::curves::relu>(2);
    //netlol.layer<nn::curves::relu>(10);
    //netlol.layer<nn::curves::relu>(10);
    //netlol.layer<nn::curves::relu>(10);
    //netlol.layer<nn::curves::relu>(10);
    //netlol.layer<nn::curves::relu>(10);
    //netlol.layer<nn::curves::relu>(10);
    //netlol.layer<nn::curves::relu>(10);
    //netlol.layer<nn::curves::relu>(10);
    //netlol.layer<nn::curves::relu>(10);
    //netlol.layer<nn::curves::relu>(10);
    //netlol.layer<nn::curves::relu>(10);
    //netlol.layer<nn::curves::relu>(10);
    //netlol.layer<nn::curves::relu>(10);
    //netlol.layer<nn::curves::relu>(10);
    //netlol.layer<nn::curves::relu>(10);
    //netlol.layer<nn::curves::relu>(10);
    //netlol.layer<nn::curves::relu>(1);
    //auto intest = core::matrix{ { 1.f }, { 2.f } };
    //auto target_function = [](float x, float y) -> float {
    //  return x * y;
    //};
    //auto target = core::matrix{ { target_function(1.f, 2.f) } };
    //auto out = netlol.forward(intest);
    ////for (int i = 0; i < 50; i++) {
    ////    intest.set(0, 0, float(core::random::uint32(50)));
    ////    intest.set(1, 0, float(core::random::uint32(50)));
    ////    float a = intest.get(0, 0), b = intest.get(1, 0);
    ////    target.set(0, 0, a + b);
    ////    for (int j = 0; j < 3; j++) {
    ////        out = netlol.forward(intest);
    ////        netlol.backpropagate(intest, target);
    ////        netlol.update(&optimizer);
    ////    }
    ////}
    //const int batchSize = 128;
    //intest.resize(2, batchSize);
    //target.resize(1, batchSize);
    //int goodcount = 0;
    //int bestcnt = 0;
    //for (int i = 0; goodcount < 100; i++) {
    //  auto getv = []() -> float { return (core::random::uint32(100)) / 100.f; };
    //  for (int b = 0; b < batchSize; b++) {
    //      intest.set(b, 0, getv());
    //      intest.set(b, 1, getv());
    //      target.set(b, 0, target_function(intest.get(b, 0), intest.get(b, 1)));
    //  }
    //  float target0 = target.get(0, 0);
    //  //core::info("input:\n%starget:\n%s\n", intest.str(), target.str());
    //  out = netlol.forward(intest);
    //  //core::info("%f+%f=%f\n", intest.get(0, 0), intest.get(0, 1), out[0]);
    //  float a = intest.get(0, 0), b = intest.get(0, 1);
    //  if ((i % 10000) == 0) {
    //      core::info("[Iter#%d:GC%d%%] %.2f*%.2f->%.2f \t [target:%.2f; acc %.2f%%]\n", i, goodcount, a, b, out[0], target0, 100.f - 100.f * abs((out[0] - target0) / max(out[0], target0)));
    //  }
    //  netlol.backpropagate(intest, target);
    //  netlol.update(&optimizer);
    //  target.set(0, 0, a + b);
    //  if ((fabs(target0-out[0])) <= 0.1f*target0) {
    //      goodcount++;
    //  }
    //  else {
    //      goodcount = 0;
    //      //core::print<core::CON_RED>("%.2f+%.2f->%.2f \t [target:%.2f; acc %.2f%%]\n", a, b, out[0], a + b, 100.f - 100.f * abs(out[0] - (a + b)) / (a + b));
    //  }
    //  if (goodcount > bestcnt && (goodcount % 10) == 0) {
    //      core::print<core::CON_GRN>("[%d%%] %.2f+%.2f->%.2f \t [target:%.2f; acc %.2f%%]\n", goodcount, a, b, out[0], target0, 100.f - 100.f * abs((out[0] - target0) / max(out[0], target0)));
    //      bestcnt = goodcount;
    //  }
    //}
    //DebugBreak();

    //target.resize(1, 1); intest.resize(1, 1);
    //for (int i = 0; i < 10; i++) {
    //  float target0 = target.get(0, 0);
    //  intest.set(0, 0, float(core::random::uint32(50)));
    //  intest.set(1, 0, float(core::random::uint32(50)));
    //  float a = intest.get(0, 0), b = intest.get(1, 0);
    //  target.set(0, 0, target_function(a, b));
    //  out = netlol.forward(intest);
    //  netlol.backpropagate(intest, target);
    //  netlol.update(&optimizer);
    //  core::info("in(%.2f,%.2f) -> out=%.2f [target:%.2f]\n", a, b, out[0], target0);
    //}
    //DebugBreak();

    //float f = GetTickCount() * 0.1f;
    //core::vectorN<float, 3> a{ 1.f,1.f,1.f };
    //core::vectorN<float, 3> b{ 1.f,f,0.f };
    //auto z = a + b;
    //z = z * 5.f;

    //core::info("%.2f, %.2f, %.2f\n", z.v[0], z.v[1], z.v[2]);

    //core::info("%.2f ;;;; %.2f\n", expf(0.f), PS_expf(0.f));
    //core::info("%.2f ;;;; %.2f\n", expf(1.f), PS_expf(1.f));
    //core::info("%.2f ;;;; %.2f\n", expf(0.5f), PS_expf(0.5f));
    //core::info("%.2f ;;;; %.2f\n", expf(-0.5f), PS_expf(-0.5f));
    //core::info("%.2f ;;;; %.2f\n", expf(-1.f), PS_expf(-1.f));
    //core::info("%.2f ;;;; %.2f\n", expf(0.1f), PS_expf(0.1f));
    //DebugBreak();

    string username = "walkingmachine";
    string password = "password123";
    core::info("argc %d\n", argc);
    if (argc == 2) {
        string replayfile = argv[1];
        core::info("Loading replay file \"%s\"\n", replayfile);
        if (s2::replay::LoadFromFile(replayfile)) {
            core::info("Loaded replay successfully.\n");
        }
        else
            core::error("Failed to load replay file.\n");
        return 0;
    }
    else if (argc > 2) {
        username = argv[1];
        password = argv[2];
    }
    core::info("Logging into account %s:%s\n", username, password);
    auto login = masterserver.login(username, password);

    if (!login.success) {
        core::error("Login failed.");
        return -1;
    }

    core::info("Logged in successfully as \"%s\" AccountId=%d; Cookie=%s\n", login.nickname, login.accountid, login.cookie);
    s2::userclient client(login.accountid);
    client.cvar("net_name", login.nickname);
    client.cvar("net_cookie", login.cookie);

    s2::ms_server_info selected;
    auto servers = masterserver.getserverlist();
    for (size_t i = 0; i < servers.size(); i++) {
        auto& server = servers[i];
        core::print("[#%d] %s:%d   \t(%d/%d) ", i, server.ip, server.port, server.numplayers, server.maxplayers);
        core::print<core::CON_GRN>("\"%s\"\n", server.name);
    }
    do {
        core::print(">> ");
        auto input = core::inputline();
        if (input.length() > 0 && input[input.length() - 1] == '\n')
            input = input.substr(0, input.length() - 1);
        try {
            if (input._Equal("localhost")) {
                selected.ip = input;
                selected.port = 11235;
            }
            else if (input.find_first_of(':') != string::npos) {
                selected.ip = input.substr(0, input.find_first_of(':'));
                selected.port = std::stoi(input.substr(input.find_first_of(':') + 1));
            }
            else {
                size_t idx = std::stoi(input);
                core::info("You selected #%d\n", idx);
                if (idx < servers.size())
                    selected = servers[idx];
                else
                    core::warning("Invalid server number.\n");
            }
        }
        catch (const std::exception& exc) {
            core::warning("%s\n", exc.what());
        }
        if (!selected.ip.empty()) {
            core::info("Connecting to %s:%d ...\n", selected.ip, selected.port);
            if (client.connect(selected.ip, selected.port)) {
                core::info("Connected successfully...\n");
                break;
            }
            else {
                core::warning("Failed to connect.\n");
                selected.ip.clear();
            }
        }
    } while (selected.ip.empty() && !client.connected());

    CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)UIThread, (LPVOID)&client, 0, NULL);
    while ((!core::input::iskeydown(VK_SHIFT) || !core::input::iskeydown(VK_ESCAPE)) && client.connected()) {
        client.update();
        //Sleep(1);
    }
    if (client.connected())
        client.disconnect("bye");

    network::destroy();
    core::info("Shutting down...\n");
    getc(stdin); 
    return 0;
}
