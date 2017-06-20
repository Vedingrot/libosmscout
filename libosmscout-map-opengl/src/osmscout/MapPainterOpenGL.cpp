/*
  This source is part of the libosmscout-map library
  Copyright (C) 2013  Tim Teulings
  Copyright (C) 2017  Fanny Monori

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
*/

#include <utility>
#include <GL/glew.h>
#include <osmscout/MapPainterOpenGL.h>
#include <osmscout/Triangulate.h>
#include <iostream>

namespace osmscout {

  osmscout::MapPainterOpenGL::MapPainterOpenGL() : minLat(0), minLon(0), maxLat(0), maxLon(0), lookX(0.0), lookY(0.0) {
    glewExperimental = GL_TRUE;
    glewInit();

    AreaRenderer.LoadVertexShader("AreaVertexShader.vert");
    AreaRenderer.LoadFragmentShader("AreaFragmentShader.frag");
    bool success = AreaRenderer.InitContext();
    if (!success) {
      std::cerr << "Could not initialize context for area rendering!" << std::endl;
      return;
    }

    zoomLevel = 45.0f;
  }

  osmscout::MapPainterOpenGL::MapPainterOpenGL(int width, int height) : width(width), height(height), minLat(0),
                                                                        minLon(0), maxLat(0), maxLon(0), lookX(0.0),
                                                                        lookY(0.0) {
    glewExperimental = GL_TRUE;
    glewInit();

    AreaRenderer.LoadVertexShader("AreaVertexShader.vert");
    AreaRenderer.LoadFragmentShader("AreaFragmentShader.frag");
    bool success = AreaRenderer.InitContext();
    if (!success) {
      std::cerr << "Could not initialize context for area rendering!" << std::endl;
      return;
    }

    zoomLevel = 45.0f;
  }

  osmscout::MapPainterOpenGL::~MapPainterOpenGL() {

  }

  void osmscout::MapPainterOpenGL::loadData(const osmscout::MapData &data, const osmscout::MapParameter &parameter,
                                            const osmscout::Projection &projection,
                                            const osmscout::StyleConfigRef &styleConfig,
                                            const osmscout::GeoBox &BoundingBox) {
    styleConfig.get()->GetLandFillStyle(projection, landFill);
    styleConfig.get()->GetSeaFillStyle(projection, seaFill);
    if (minLat == 0)
      minLat = BoundingBox.GetMinLat();
    if (minLon == 0)
      minLon = BoundingBox.GetMinLon();
    if (maxLat == 0)
      maxLat = BoundingBox.GetMaxLat();
    if (maxLon == 0)
      maxLon = BoundingBox.GetMaxLon();

    AreaRenderer.clearData();
    AreaRenderer.SetVerticesSize(5);

    ProcessGroundData(data, parameter, projection, styleConfig, BoundingBox);
    //ProcessAreaData(data, parameter, projection, styleConfig, BoundingBox);
    FinishProcess();
  }

  void
  osmscout::MapPainterOpenGL::ProcessAreaData(const osmscout::MapData &data, const osmscout::MapParameter &parameter,
                                              const osmscout::Projection &projection,
                                              const osmscout::StyleConfigRef &styleConfig,
                                              const osmscout::GeoBox &BoundingBox) {

    for (const auto &area : data.areas) {
      size_t ringId = Area::outerRingId;
      bool foundRing = true;

      while (foundRing) {
        foundRing = false;

        for (size_t i = 0; i < area->rings.size(); i++) {
          const Area::Ring &ring = area->rings[i];

          if (ring.IsMasterRing()) {
            continue;
          }

          if (ring.GetRing() != ringId) {
            continue;
          }

          if (!ring.IsOuterRing() &&
              ring.GetType()->GetIgnore()) {
            continue;
          }

          if (!ring.IsOuterRing() && ring.GetType()->GetIgnore())
            continue;

          TypeInfoRef type;
          FillStyleRef fillStyle;
          std::vector<BorderStyleRef> borderStyles;
          BorderStyleRef borderStyle;

          if (ring.IsOuterRing()) {
            type = area->GetType();
          } else {
            type = ring.GetType();
          }

          styleConfig->GetAreaFillStyle(type,
                                        ring.GetFeatureValueBuffer(),
                                        projection,
                                        fillStyle);

          styleConfig->GetAreaBorderStyles(type,
                                           ring.GetFeatureValueBuffer(),
                                           projection,
                                           borderStyles);

          if (!fillStyle && borderStyles.empty()) {
            continue;
          }

          foundRing = true;

          std::vector<Point> p = area->rings[i].nodes;
          std::vector<osmscout::Area::Ring> r;

          size_t j = i + 1;
          int hasClippings = 0;
          while (j < area->rings.size() &&
                 area->rings[j].GetRing() == ringId + 1 &&
                 area->rings[j].GetType()->GetIgnore()) {
            r.push_back(area->rings[j]);
            j++;
            hasClippings = 1;
          }

          //border TODO

          Color c = fillStyle->GetFillColor();

          std::vector<GLfloat> points;
          if (hasClippings == 1) {
            std::vector<std::vector<osmscout::Point>> polygons;
            polygons.push_back(p);
            for (const auto &ring: r) {
              polygons.push_back(ring.nodes);
            }
            points = osmscout::Triangulate::TriangulateWithHoles(polygons);
          } else {
            points = osmscout::Triangulate::TriangulatePolygon(p);
          }

          for (int t = 0; t < points.size(); t++) {
            if (t % 2 == 0) {
              AreaRenderer.AddNewVertex(points[t]);
            } else {
              AreaRenderer.AddNewVertex(points[t]);
              AreaRenderer.AddNewVertex(c.GetR());
              AreaRenderer.AddNewVertex(c.GetG());
              AreaRenderer.AddNewVertex(c.GetB());

              if (AreaRenderer.GetNumOfVertices() <= 5) {
                AreaRenderer.AddNewElement(0);
              } else {
                AreaRenderer.AddNewElement(AreaRenderer.GetVerticesNumber() - 1);
              }

            }
          }

        }
        ringId++;
      }
    }
  }

  void
  osmscout::MapPainterOpenGL::ProcessGroundData(const osmscout::MapData &data, const osmscout::MapParameter &parameter,
                                                const osmscout::Projection &projection,
                                                const osmscout::StyleConfigRef &styleConfig,
                                                const osmscout::GeoBox &BoundingBox) {

    FillStyleRef landFill;

    styleConfig->GetLandFillStyle(projection,
                                  landFill);

    if (!landFill) {
      landFill = this->landFill;
    }

    FillStyleRef seaFill;
    FillStyleRef coastFill;
    FillStyleRef unknownFill;
    std::vector<Point> points;

    styleConfig->GetSeaFillStyle(projection,
                                 seaFill);
    styleConfig->GetCoastFillStyle(projection,
                                   coastFill);
    styleConfig->GetUnknownFillStyle(projection,
                                     unknownFill);

    if (!seaFill) {
      seaFill = this->seaFill;
    }

    for (const auto &tile : data.groundTiles) {
      if (tile.type == GroundTile::unknown &&
          !parameter.GetRenderUnknowns()) {
        continue;
      }

      FillStyleRef fill;

      switch (tile.type) {
        case GroundTile::land:
          fill = landFill;
          break;
        case GroundTile::water:
          fill = seaFill;
          break;
        case GroundTile::coast:
          fill = seaFill;
          break;
        case GroundTile::unknown:
          fill = unknownFill;
          break;
      }

      GeoCoord minCoord(tile.yAbs * tile.cellHeight - 90.0,
                        tile.xAbs * tile.cellWidth - 180.0);
      GeoCoord maxCoord(minCoord.GetLat() + tile.cellHeight,
                        minCoord.GetLon() + tile.cellWidth);

      if (tile.coords.empty()) {
        AreaRenderer.AddNewVertex(minCoord.GetLon());
        AreaRenderer.AddNewVertex(minCoord.GetLat());
        AreaRenderer.AddNewVertex(fill->GetFillColor().GetR());
        AreaRenderer.AddNewVertex(fill->GetFillColor().GetG());
        AreaRenderer.AddNewVertex(fill->GetFillColor().GetB());
        AreaRenderer.AddNewVertex(maxCoord.GetLon());
        AreaRenderer.AddNewVertex(minCoord.GetLat());
        AreaRenderer.AddNewVertex(fill->GetFillColor().GetR());
        AreaRenderer.AddNewVertex(fill->GetFillColor().GetG());
        AreaRenderer.AddNewVertex(fill->GetFillColor().GetB());
        AreaRenderer.AddNewVertex(maxCoord.GetLon());
        AreaRenderer.AddNewVertex(maxCoord.GetLat());
        AreaRenderer.AddNewVertex(fill->GetFillColor().GetR());
        AreaRenderer.AddNewVertex(fill->GetFillColor().GetG());
        AreaRenderer.AddNewVertex(fill->GetFillColor().GetB());

        AreaRenderer.AddNewVertex(minCoord.GetLon());
        AreaRenderer.AddNewVertex(minCoord.GetLat());
        AreaRenderer.AddNewVertex(fill->GetFillColor().GetR());
        AreaRenderer.AddNewVertex(fill->GetFillColor().GetG());
        AreaRenderer.AddNewVertex(fill->GetFillColor().GetB());
        AreaRenderer.AddNewVertex(minCoord.GetLon());
        AreaRenderer.AddNewVertex(maxCoord.GetLat());
        AreaRenderer.AddNewVertex(fill->GetFillColor().GetR());
        AreaRenderer.AddNewVertex(fill->GetFillColor().GetG());
        AreaRenderer.AddNewVertex(fill->GetFillColor().GetB());
        AreaRenderer.AddNewVertex(maxCoord.GetLon());
        AreaRenderer.AddNewVertex(maxCoord.GetLat());
        AreaRenderer.AddNewVertex(fill->GetFillColor().GetR());
        AreaRenderer.AddNewVertex(fill->GetFillColor().GetG());
        AreaRenderer.AddNewVertex(fill->GetFillColor().GetB());

        int num;
        if (AreaRenderer.GetVerticesNumber() <= 6)
          num = 0;
        else
          num = AreaRenderer.GetVerticesNumber();
        for (int i = 0; i < 6; i++)
          AreaRenderer.AddNewElement(num + i);

      } else {
        std::vector<osmscout::Point> p;
        for (int i = 0; i < tile.coords.size(); i++) {
          double lat;
          double lon;
          lat = minCoord.GetLat() + tile.coords[i].y * tile.cellHeight / GroundTile::Coord::CELL_MAX;
          lon = minCoord.GetLon() + tile.coords[i].x * tile.cellWidth / GroundTile::Coord::CELL_MAX;

          int dupl = 0;
          for (int j = 0; j < p.size(); j++) {
            if (fabs(p[j].GetLat() - lat) < 0.00000001 && fabs(p[j].GetLon() - lon) < 0.000000001) {
              dupl = 1;
            }
          }

          if (dupl == 1)
            continue;

          osmscout::GeoCoord g = osmscout::GeoCoord(lat, lon);
          osmscout::Point pt;
          pt.SetCoord(g);
          p.push_back(pt);

        }

        std::vector<GLfloat> points;
        points = osmscout::Triangulate::TriangulatePolygon(p);

        for (int t = 0; t < points.size(); t++) {
          if (t % 2 == 0) {
            AreaRenderer.AddNewVertex(points[t]);
          } else {
            AreaRenderer.AddNewVertex(points[t]);
            AreaRenderer.AddNewVertex(fill->GetFillColor().GetR());
            AreaRenderer.AddNewVertex(fill->GetFillColor().GetG());
            AreaRenderer.AddNewVertex(fill->GetFillColor().GetB());

            if (AreaRenderer.GetNumOfVertices() <= 5) {
              AreaRenderer.AddNewElement(0);
            } else {
              AreaRenderer.AddNewElement(AreaRenderer.GetVerticesNumber() - 1);
            }

          }
        }

      }

    }

  }

  void osmscout::MapPainterOpenGL::FinishProcess() {
    AreaRenderer.LoadVertices();
    AreaRenderer.LoadProgram();
    AreaRenderer.SetProjection(width, height);
    AreaRenderer.SetModel();
    AreaRenderer.SetView(lookX, lookY);
    AreaRenderer.AddAttrib("position", 2, GL_FLOAT, 0);
    AreaRenderer.AddAttrib("color", 3, GL_FLOAT, 2 * sizeof(GLfloat));

    AreaRenderer.AddUniform("minLon", minLon);
    AreaRenderer.AddUniform("minLat", minLat);
    AreaRenderer.AddUniform("maxLon", maxLon);
    AreaRenderer.AddUniform("maxLat", maxLat);
  }

  void osmscout::MapPainterOpenGL::onZoom(float zoom) {
    minLon += (zoom / ((height / (float) width) * 100));
    minLat += (zoom / ((width / (float) height) * 100));
    maxLon -= (zoom / ((height / (float) width) * 100));
    maxLat -= (zoom / ((width / (float) height) * 100));

    AreaRenderer.AddUniform("minLon", minLon);
    AreaRenderer.AddUniform("minLat", minLat);
    AreaRenderer.AddUniform("maxLon", maxLon);
    AreaRenderer.AddUniform("maxLat", maxLat);
  }

  void osmscout::MapPainterOpenGL::onTranslation(int startPointX, int startPointY, int endPointX, int endPointY) {
    float offsetX = startPointX - endPointX;
    float offsetY = endPointY - startPointY;

    lookX += offsetX / 1000;
    lookY += offsetY / 1000;

    AreaRenderer.SetView(lookX, lookY);
  }

  void osmscout::MapPainterOpenGL::DrawMap() {
    glClearColor(this->landFill.get()->GetFillColor().GetR(),
                 this->landFill.get()->GetFillColor().GetB(),
                 this->landFill.get()->GetFillColor().GetG(),
                 this->landFill.get()->GetFillColor().GetA());
    glClear(GL_COLOR_BUFFER_BIT);
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    AreaRenderer.Draw();
  }


}
