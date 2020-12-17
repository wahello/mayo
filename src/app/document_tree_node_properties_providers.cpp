/****************************************************************************
** Copyright (c) 2020, Fougue Ltd. <http://www.fougue.pro>
** All rights reserved.
** See license at https://github.com/fougue/mayo/blob/master/LICENSE.txt
****************************************************************************/

#include "document_tree_node_properties_providers.h"

#include "../base/caf_utils.h"
#include "../base/document.h"
#include "../base/document_tree_node.h"
#include "../base/meta_enum.h"
#include "../base/string_utils.h"
#include "../base/xcaf.h"

#include <TDataXtd_Triangulation.hxx>

namespace Mayo {

class XCaf_DocumentTreeNodePropertiesProvider::Properties : public PropertyGroupSignals {
    MAYO_DECLARE_TEXT_ID_FUNCTIONS(Mayo::XCaf_DocumentTreeNodeProperties)
public:
    Properties(const DocumentTreeNode& treeNode)
        : m_docTreeNode(treeNode),
          m_propertyName(this, textId("Name")),
          m_propertyShapeType(this, textId("Shape")),
          m_propertyXdeShapeKind(this, textId("XdeShape")),
          m_propertyColor(this, textId("Color")),
          m_propertyReferenceLocation(this, textId("Location")),
          m_propertyValidationCentroid(this, textId("Centroid")),
          m_propertyValidationArea(this, textId("Area")),
          m_propertyValidationVolume(this, textId("Volume")),
          m_propertyProductName(this, textId("ProductName")),
          m_propertyProductColor(this, textId("ProductColor")),
          m_propertyProductValidationCentroid(this, textId("ProductCentroid")),
          m_propertyProductValidationArea(this, textId("ProductArea")),
          m_propertyProductValidationVolume(this, textId("ProductVolume")),
          m_label(treeNode.label())
    {
        Mayo_PropertyChangedBlocker(this);

        const TDF_Label& label = m_label;
        const XCaf& xcaf = treeNode.document()->xcaf();

        // Name
        m_propertyName.setValue(CafUtils::labelAttrStdName(label));

        // Shape type
        const TopAbs_ShapeEnum shapeType = XCAFDoc_ShapeTool::GetShape(label).ShapeType();
        m_propertyShapeType.setValue(MetaEnum::nameWithoutPrefix(shapeType, "TopAbs_").data());

        // XDE shape kind
        QStringList listXdeShapeKind;
        if (XCaf::isShapeAssembly(label))
            listXdeShapeKind.push_back(textId("Assembly").tr());

        if (XCaf::isShapeReference(label))
            listXdeShapeKind.push_back(textId("Reference").tr());

        if (XCaf::isShapeComponent(label))
            listXdeShapeKind.push_back(textId("Component").tr());

        if (XCaf::isShapeCompound(label))
            listXdeShapeKind.push_back(textId("Compound").tr());

        if (XCaf::isShapeSimple(label))
            listXdeShapeKind.push_back(textId("Simple").tr());

        if (XCaf::isShapeSub(label))
            listXdeShapeKind.push_back(textId("Sub").tr());

        m_propertyXdeShapeKind.setValue(listXdeShapeKind.join('+'));

        // Reference location
        if (XCaf::isShapeReference(label)) {
            const TopLoc_Location loc = XCaf::shapeReferenceLocation(label);
            m_propertyReferenceLocation.setValue(loc.Transformation());
        }
        else {
            this->removeProperty(&m_propertyReferenceLocation);
        }

        // Color
        if (xcaf.hasShapeColor(label))
            m_propertyColor.setValue(xcaf.shapeColor(label));
        else
            this->removeProperty(&m_propertyColor);

        // Validation properties
        {
            auto validProps = XCaf::validationProperties(label);
            m_propertyValidationCentroid.setValue(validProps.centroid);
            if (!validProps.hasCentroid)
                this->removeProperty(&m_propertyValidationCentroid);

            m_propertyValidationArea.setQuantity(validProps.area);
            if (!validProps.hasArea)
                this->removeProperty(&m_propertyValidationArea);

            m_propertyValidationVolume.setQuantity(validProps.volume);
            if (!validProps.hasVolume)
                this->removeProperty(&m_propertyValidationVolume);
        }

        // Product entity's properties
        if (XCaf::isShapeReference(label)) {
            m_labelProduct = XCaf::shapeReferred(label);
            m_propertyProductName.setValue(CafUtils::labelAttrStdName(m_labelProduct));
            auto validProps = XCaf::validationProperties(m_labelProduct);
            m_propertyProductValidationCentroid.setValue(validProps.centroid);
            if (!validProps.hasCentroid)
                this->removeProperty(&m_propertyProductValidationCentroid);

            m_propertyProductValidationArea.setQuantity(validProps.area);
            if (!validProps.hasArea)
                this->removeProperty(&m_propertyProductValidationArea);

            m_propertyProductValidationVolume.setQuantity(validProps.volume);
            if (!validProps.hasVolume)
                this->removeProperty(&m_propertyProductValidationVolume);

            if (xcaf.hasShapeColor(m_labelProduct))
                m_propertyProductColor.setValue(xcaf.shapeColor(m_labelProduct));
            else
                this->removeProperty(&m_propertyProductColor);
        }
        else {
            this->removeProperty(&m_propertyProductName);
            this->removeProperty(&m_propertyProductValidationCentroid);
            this->removeProperty(&m_propertyProductValidationArea);
            this->removeProperty(&m_propertyProductValidationVolume);
            this->removeProperty(&m_propertyProductColor);
        }

        for (Property* prop : this->properties())
            prop->setUserReadOnly(true);

        m_propertyName.setUserReadOnly(false);
        m_propertyProductName.setUserReadOnly(false);
        m_propertyProductColor.setUserReadOnly(false);
    }

    void onPropertyChanged(Property* prop) override
    {
        if (prop == &m_propertyName)
            CafUtils::setLabelAttrStdName(m_label, m_propertyName.value());
        else if (prop == &m_propertyProductName)
            CafUtils::setLabelAttrStdName(m_labelProduct, m_propertyProductName.value());

        if (prop == &m_propertyColor) {
            // Not supported for now, shape graphics need to be improved(don't rely on XCAFPrs_AISObject)
        }
        else if (prop == &m_propertyProductColor) {
            const DocumentPtr& doc = m_docTreeNode.document();
            const Tree<TDF_Label>& modelTree = doc->modelTree();
            TreeNodeId treeNodeId = m_docTreeNode.id();
            // In case current node is an "instance" then apply the color change on the "pointed"
            // product which is the single child of the current node
            if (doc->xcaf().isShapeReference(m_docTreeNode.label())
                    && modelTree.nodeChildFirst(treeNodeId) == modelTree.nodeChildLast(treeNodeId))
            {
                treeNodeId = modelTree.nodeChildFirst(treeNodeId);
            }

            m_docTreeNode.document()->changeColor(treeNodeId, m_propertyProductColor.value());
        }

        PropertyGroupSignals::onPropertyChanged(prop);
    }

    DocumentTreeNode m_docTreeNode;

    PropertyQString m_propertyName;
    PropertyQString m_propertyShapeType;
    PropertyQString m_propertyXdeShapeKind;
    PropertyOccColor m_propertyColor;
    PropertyOccTrsf m_propertyReferenceLocation;
    PropertyOccPnt m_propertyValidationCentroid;
    PropertyArea m_propertyValidationArea;
    PropertyVolume m_propertyValidationVolume;

    PropertyQString m_propertyProductName;
    PropertyOccColor m_propertyProductColor;
    PropertyOccPnt m_propertyProductValidationCentroid;
    PropertyArea m_propertyProductValidationArea;
    PropertyVolume m_propertyProductValidationVolume;

    TDF_Label m_label;
    TDF_Label m_labelProduct;
};

bool XCaf_DocumentTreeNodePropertiesProvider::supports(const DocumentTreeNode& treeNode) const
{
    return XCaf::isShape(treeNode.label());
}

std::unique_ptr<PropertyGroupSignals>
XCaf_DocumentTreeNodePropertiesProvider::properties(const DocumentTreeNode& treeNode) const
{
    if (!treeNode.isValid())
        return {};

    return std::make_unique<Properties>(treeNode);
}

class Mesh_DocumentTreeNodePropertiesProvider::Properties : public PropertyGroupSignals {
    MAYO_DECLARE_TEXT_ID_FUNCTIONS(Mayo::Mesh_DocumentTreeNodeProperties)
public:
        Properties(const DocumentTreeNode& treeNode)
      : m_propertyNodeCount(this, textId("NodeCount")),
        m_propertyTriangleCount(this, textId("TriangleCount"))
    {
        auto attrTriangulation = CafUtils::findAttribute<TDataXtd_Triangulation>(treeNode.label());
        Handle_Poly_Triangulation polyTri;
        if (!attrTriangulation.IsNull())
            polyTri = attrTriangulation->Get();

        m_propertyNodeCount.setValue(!polyTri.IsNull() ? polyTri->NbNodes() : 0);
        m_propertyTriangleCount.setValue(!polyTri.IsNull() ? polyTri->NbTriangles() : 0);
        m_propertyNodeCount.setUserReadOnly(true);
        m_propertyTriangleCount.setUserReadOnly(true);
    }

    PropertyInt m_propertyNodeCount; // Read-only
    PropertyInt m_propertyTriangleCount; // Read-only
};

bool Mesh_DocumentTreeNodePropertiesProvider::supports(const DocumentTreeNode& treeNode) const
{
    return CafUtils::hasAttribute<TDataXtd_Triangulation>(treeNode.label());
}

std::unique_ptr<PropertyGroupSignals>
Mesh_DocumentTreeNodePropertiesProvider::properties(const DocumentTreeNode& treeNode) const
{
    if (!treeNode.isValid())
        return {};

    return std::make_unique<Properties>(treeNode);
}

} // namespace Mayo
