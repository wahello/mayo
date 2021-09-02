/****************************************************************************
** Copyright (c) 2021, Fougue Ltd. <http://www.fougue.pro>
** All rights reserved.
** See license at https://github.com/fougue/mayo/blob/master/LICENSE.txt
****************************************************************************/

#pragma once

#include "../base/document_ptr.h"
#include "../base/io_reader.h"

#include <assimp/Importer.hpp>
#include <TopoDS_Shape.hxx>
#include <unordered_map>

struct aiMesh;
struct aiNode;

namespace Mayo {
namespace IO {

class AssimpReader : public Reader {
public:
    bool readFile(const FilePath& filepath, TaskProgress* progress) override;
    TDF_LabelSequence transfer(DocumentPtr doc, TaskProgress* progress) override;

    static std::unique_ptr<PropertyGroup> createProperties(PropertyGroup* parentGroup);
    void applyProperties(const PropertyGroup* params) override;

private:
    void transferSceneNode(const aiNode* node, DocumentPtr doc, TDF_LabelSequence& seqLabel);

    class Properties;
    Assimp::Importer m_importer;
    const aiScene* m_scene = nullptr;
    std::unordered_map<const aiMesh*, TopoDS_Shape> m_mapMeshShape;
};

} // namespace IO
} // namespace Mayo
