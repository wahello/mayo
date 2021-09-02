/****************************************************************************
** Copyright (c) 2021, Fougue Ltd. <http://www.fougue.pro>
** All rights reserved.
** See license at https://github.com/fougue/mayo/blob/master/LICENSE.txt
****************************************************************************/

#include "io_assimp_reader.h"
#include "../base/caf_utils.h"
#include "../base/document.h"
#include "../base/messenger.h"
#include "../base/property.h"
#include "../base/string_conv.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <gp_Trsf.hxx>
#include <BRep_Builder.hxx>
#include <Poly_Polygon3D.hxx>
#include <Poly_Triangulation.hxx>
#include <TDataStd_Name.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>

namespace Mayo {
namespace IO {

namespace {

gp_Trsf toOccTrsf(const aiMatrix4x4& matrix)
{
    const ai_real scalingX = aiVector3D(matrix.a1, matrix.a2, matrix.a3).Length();
    const ai_real scalingY = aiVector3D(matrix.b1, matrix.b2, matrix.b3).Length();
    const ai_real scalingZ = aiVector3D(matrix.c1, matrix.c2, matrix.c3).Length();

    gp_Trsf trsf;
    // TODO Check scalingXYZ != 0
    trsf.SetValues(matrix.a1 / scalingX, matrix.a2 / scalingX, matrix.a3 / scalingX, matrix.a4,
                   matrix.b1 / scalingY, matrix.b2 / scalingY, matrix.b3 / scalingY, matrix.b4,
                   matrix.c1 / scalingZ, matrix.c2 / scalingZ, matrix.c3 / scalingZ, matrix.c4);
    return trsf;
}

} // namespace

class AssimpReader::Properties : public PropertyGroup {
    //MAYO_DECLARE_TEXT_ID_FUNCTIONS(Mayo::IO::AssimpReader::Properties)
public:
    Properties(PropertyGroup* parentGroup)
        : PropertyGroup(parentGroup)
    {
    }
};

bool AssimpReader::readFile(const FilePath& filepath, TaskProgress* progress)
{
    // TODO Handle progress
    // Assimp::Importer::SetProgressHandler()
    const unsigned flags =
            aiProcess_Triangulate
            | aiProcess_JoinIdenticalVertices
            | aiProcess_SortByPType
            | aiProcess_FlipUVs
            | aiProcess_FindInstances
            //| aiProcess_FixInfacingNormals
            //| aiProcess_GlobalScale -> AI_CONFIG_GLOBAL_SCALE_FACTOR_KEY
            //| aiProcess_GenNormals
            //| aiProcess_GenSmoothNormals
            //| aiProcess_CalcTangentSpace
            //| aiProcess_ValidateDataStructure
            ;
    m_scene = m_importer.ReadFile(filepath.u8string().c_str(), flags);
    m_mapMeshShape.clear();
    if (!m_scene || (m_scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) || !m_scene->mRootNode) {
        this->messenger()->emitError(m_importer.GetErrorString());
        return false;
    }

    for (unsigned imesh = 0; imesh < m_scene->mNumMeshes; ++imesh) {
        const aiMesh* mesh = m_scene->mMeshes[imesh];
        if (mesh->mPrimitiveTypes & aiPrimitiveType_TRIANGLE) {
            Handle_Poly_Triangulation triangulation = new Poly_Triangulation(mesh->mNumVertices, mesh->mNumFaces, false/*UVnodes*/);
            for (unsigned ivertex = 0; ivertex < mesh->mNumVertices; ++ivertex) {
                const aiVector3D& vertex = mesh->mVertices[ivertex];
                triangulation->ChangeNode(ivertex + 1).SetXYZ({ vertex.x, vertex.y, vertex.z });
            }

            for (unsigned iface = 0; iface < mesh->mNumFaces; ++iface) {
                const auto indices = mesh->mFaces[iface].mIndices;
                triangulation->ChangeTriangle(iface + 1).Set(indices[0] + 1, indices[1] + 1, indices[2] + 1);
            }

            // TODO Handle materials
            // TODO Handle normals

            TopoDS_Face face;
            BRep_Builder builder;
            builder.MakeFace(face, triangulation);
            m_mapMeshShape.insert({ mesh, face });
        }
        else if (mesh->mPrimitiveTypes & aiPrimitiveType_LINE) {
            // TODO Create and add a Poly_Polygon3D object
        } // endif
    }

    return true;
}

TDF_LabelSequence AssimpReader::transfer(DocumentPtr doc, TaskProgress* progress)
{
    if (!m_scene)
        return {};

    // TODO Handle progress
    TDF_LabelSequence seqLabel;
    this->transferSceneNode(m_scene->mRootNode, doc, seqLabel);
    return seqLabel;
}

std::unique_ptr<PropertyGroup> AssimpReader::createProperties(PropertyGroup* parentGroup)
{
    return {};
}

void AssimpReader::applyProperties(const PropertyGroup* group)
{
    auto ptr = dynamic_cast<const Properties*>(group);
    if (ptr) {
    }
}

void AssimpReader::transferSceneNode(const aiNode* node, DocumentPtr doc, TDF_LabelSequence& seqLabel)
{
    // Compute node absolute name and transformation
    std::string nodeName;
    gp_Trsf nodeTrsf;
    {
        const aiNode* itNode = node;
        while (itNode) {
            nodeName.insert(0, node->mName.C_Str());
            nodeTrsf = toOccTrsf(node->mTransformation) * nodeTrsf;
            itNode = itNode->mParent;
            if (itNode)
                nodeName.insert(0, " / ");
        }
    }

    // Helper function to find the pseudo shape mapped to a mesh
    auto fnFindShape = [=](const aiMesh* mesh) {
        auto itFound = m_mapMeshShape.find(mesh);
        return itFound != m_mapMeshShape.cend() ? itFound->second : TopoDS_Shape();
    };

    // Produce shape corresponding to the node
    TopoDS_Shape nodeShape;
    if (node->mNumMeshes == 1) {
        const aiMesh* mesh = m_scene->mMeshes[node->mMeshes[0]];
        nodeShape = fnFindShape(mesh);
    }
    else if (node->mNumMeshes > 1) {
        TopoDS_Compound cmpd;
        BRep_Builder builder;
        builder.MakeCompound(cmpd);
        for (unsigned imesh = 0; imesh < node->mNumMeshes; ++imesh) {
            const aiMesh* mesh = m_scene->mMeshes[node->mMeshes[imesh]];
            builder.Add(cmpd, fnFindShape(mesh));
        }

        nodeShape = cmpd;
    }

    // Add shape into document
    if (!nodeShape.IsNull()) {
        const TDF_Label nodeLabel = doc->xcaf().shapeTool()->NewShape();
        doc->xcaf().shapeTool()->SetShape(nodeLabel, nodeShape);
        TDataStd_Name::Set(nodeLabel, to_OccExtString(nodeName));
        seqLabel.Append(nodeLabel);
    }

    // Process child nodes
    for (unsigned ichild = 0; ichild < node->mNumChildren; ++ichild)
        this->transferSceneNode(node->mChildren[ichild], doc, seqLabel);
}

} // namespace IO
} // namespace Mayo

