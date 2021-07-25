/****************************************************************************
** Copyright (c) 2021, Fougue Ltd. <http://www.fougue.pro>
** All rights reserved.
** See license at https://github.com/fougue/mayo/blob/master/LICENSE.txt
****************************************************************************/

#pragma once

#include "dxf.h"

#include "../base/io_reader.h"
#include <gp_Pnt.hxx>
#include <TopoDS_Shape.hxx>
#include <map>
#include <string>
#include <vector>

namespace Mayo {
namespace IO {

// Reader for DXF file format based on FreeCad's CDxfRead
class DxfReader : public Reader, private CDxfRead {
public:
    bool readFile(const FilePath& filepath, TaskProgress* progress) override;
    TDF_LabelSequence transfer(DocumentPtr doc, TaskProgress* progress) override;

    struct Parameters {
        double scaling = 1.;
        bool importAnnotations = true;
        bool groupLayers = false;
    };

private:
    // CDxfRead's virtual functions
    void OnReadLine(const double* s, const double* e, bool hidden) override;
    void OnReadPoint(const double* s) override;
    void OnReadText(const double* point, const double height, const char* text) override;
    void OnReadArc(const double* s, const double* e, const double* c, bool dir, bool hidden) override;
    void OnReadCircle(const double* s, const double* c, bool dir, bool hidden) override;
    void OnReadEllipse(const double* c, double major_radius, double minor_radius, double rotation, double start_angle, double end_angle, bool dir) override;
    void OnReadSpline(struct SplineData& sd) override;
    void OnReadInsert(const double* point, const double* scale, const char* name, double rotation) override;
    void OnReadDimension(const double* s, const double* e, const double* point, double rotation) override;
    void AddGraphics() const override;

    gp_Pnt toPnt(const double* coords) const;
    void addShape(const TopoDS_Shape& shape);

    TopoDS_Shape m_shape;
    FilePath m_baseFilename;
    std::map<std::string, std::vector<TopoDS_Shape>> m_layers;
    Parameters m_params;
};

} // namespace IO
} // namespace Mayo
