/****************************************************************************
** Copyright (c) 2021, Fougue Ltd. <http://www.fougue.pro>
** All rights reserved.
** See license at https://github.com/fougue/mayo/blob/master/LICENSE.txt
****************************************************************************/

#include "io_format.h"
#include "meta_enum.h"

#include <algorithm>

namespace Mayo {
namespace IO {

std::string_view formatIdentifier(Format format)
{
    switch (format) {
    case Format_Unknown: return "";
    case Format_STEP: return "STEP";
    case Format_IGES: return "IGES";
    case Format_OCCBREP: return "OCCBREP";
    case Format_STL:  return "STL";
    case Format_OBJ:  return "OBJ";
    case Format_GLTF: return "GLTF";
    case Format_VRML: return "VRML";
    case Format_AMF:  return "AMF";
    case Format_DXF:  return "DXF";
    }

    return MetaEnum::nameWithoutPrefix(format, "Format_");
}

std::string_view formatName(Format format)
{
    switch (format) {
    case Format_Unknown: return "Format_Unknown";
    case Format_AMF:  return "Additive manufacturing file format(ISO/ASTM 52915:2016)";
    case Format_GLTF: return "glTF(GL Transmission Format)";
    case Format_IGES: return "IGES(ASME Y14.26M)";
    case Format_OBJ:  return "Wavefront OBJ";
    case Format_OCCBREP: return "OpenCascade BREP";
    case Format_STEP: return "STEP(ISO 10303)";
    case Format_STL:  return "STL(STereo-Lithography)";
    case Format_VRML: return "VRML(ISO/CEI 14772-2)";
    case Format_DXF:  return "Drawing Exchange Format";
        //
    case Format_3DS: return "3DS Max File";
    case Format_3MF: return "3D Manufacturing Format";
    case Format_COLLADA: return "COLLAborative Design Activity(ISO/PAS 17506)";
    case Format_FBX: return "Filmbox";
    case Format_IFC: return "Industry Foundation Classes(ISO 16739)";
    case Format_OFF: return "Object File Format";
    case Format_PLY: return "Polygon File Format";
    case Format_X3D: return "Extensible 3D Graphics(ISO/IEC 19775/19776/19777)";
    }

    return "";
}

Span<std::string_view> formatFileSuffixes(Format format)
{
    static std::string_view step_suffix[] = { "step", "stp" };
    static std::string_view iges_suffix[] = { "iges", "igs" };
    static std::string_view occ_suffix[] =  { "brep", "rle", "occ" };
    static std::string_view stl_suffix[] =  { "stl" };
    static std::string_view obj_suffix[] =  { "obj" };
    static std::string_view gltf_suffix[] = { "gltf", "glb" };
    static std::string_view vrml_suffix[] = { "wrl", "wrz", "vrml" };
    static std::string_view amf_suffix[] =  { "amf" };
    static std::string_view dxf_suffix[] =  { "dxf" };
    //
    static std::string_view suffix_3ds[] =  { "3ds" };
    static std::string_view suffix_3mf[] =  { "3mf" };
    static std::string_view suffix_collada[] =  { "dae" };
    static std::string_view suffix_fbx[] =  { "fbx" };
    static std::string_view suffix_ifc[] =  { "ifc", "ifcXML", "ifczip" };
    static std::string_view suffix_off[] =  { "off" };
    static std::string_view suffix_ply[] =  { "ply" };
    static std::string_view suffix_x3d[] =  { "x3d", "x3dv", "x3db", "x3dz", "x3dbz", "x3dvz" };

    switch (format) {
    case Format_Unknown: return {};
    case Format_STEP: return step_suffix;
    case Format_IGES: return iges_suffix;
    case Format_OCCBREP: return occ_suffix;
    case Format_STL:  return stl_suffix;
    case Format_OBJ:  return obj_suffix;
    case Format_GLTF: return gltf_suffix;
    case Format_VRML: return vrml_suffix;
    case Format_AMF:  return amf_suffix;
    case Format_DXF:  return dxf_suffix;
        //
    case Format_3DS: return suffix_3ds;
    case Format_3MF: return suffix_3mf;
    case Format_COLLADA: return suffix_collada;
    case Format_FBX: return suffix_fbx;
    case Format_IFC: return suffix_ifc;
    case Format_OFF: return suffix_off;
    case Format_PLY: return suffix_ply;
    case Format_X3D: return suffix_x3d;
    }

    return {};
}

bool formatProvidesBRep(Format format)
{
    static const Format brepFormats[] = { Format_STEP, Format_IGES, Format_OCCBREP, Format_DXF };
    return std::any_of(
                std::cbegin(brepFormats),
                std::cend(brepFormats),
                [=](Format candidate) { return candidate == format; });
}

bool formatProvidesMesh(Format format)
{
    return !formatProvidesBRep(format) && format != Format_Unknown;
}

} // namespace IO
} // namespace Mayo
