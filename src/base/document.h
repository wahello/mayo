/****************************************************************************
** Copyright (c) 2021, Fougue Ltd. <http://www.fougue.pro>
** All rights reserved.
** See license at https://github.com/fougue/mayo/blob/master/LICENSE.txt
****************************************************************************/

#pragma once

#include "document_ptr.h"
#include "document_tree_node.h"
#include "filepath.h"
#include "libtree.h"
#include "xcaf.h"

#include <QtCore/QObject>
#include <string>
#include <string_view>

namespace Mayo {

class Application;
class DocumentTreeNode;

class Document : public QObject, public TDocStd_Document {
    Q_OBJECT
    Q_PROPERTY(int identifier READ identifier)
    Q_PROPERTY(std::string name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(FilePath filePath READ filePath WRITE setFilePath)
    Q_PROPERTY(bool isXCafDocument READ isXCafDocument)
public:
    using Identifier = int;
    enum class Format { Binary, Xml };

    Identifier identifier() const { return m_identifier; }

    const std::string& name() const;
    void setName(std::string_view name);

    const FilePath& filePath() const;
    void setFilePath(const FilePath& fp);

    static const char NameFormatBinary[];
    static const char NameFormatXml[];
    static const char* toNameFormat(Format format);

    static const char TypeName[];
    virtual const char* dynTypeName() const { return Document::TypeName; }

    bool isXCafDocument() const;
    XCaf& xcaf() { return m_xcaf; }
    const XCaf& xcaf() const { return m_xcaf; }

    TDF_Label rootLabel() const;
    bool isEntity(TreeNodeId nodeId);
    int entityCount() const;
    TDF_Label entityLabel(int index) const;
    TreeNodeId entityTreeNodeId(int index) const;
    DocumentTreeNode entityTreeNode(int index) const;

    const Tree<TDF_Label>& modelTree() const { return m_modelTree; }
    void rebuildModelTree();

    static DocumentPtr findFrom(const TDF_Label& label);

    TDF_Label newEntityLabel();
    void addEntityTreeNode(const TDF_Label& label);
    void destroyEntity(TreeNodeId entityTreeNodeId);

signals:
    void nameChanged(const std::string& name);
    void entityAdded(Mayo::TreeNodeId entityTreeNodeId);
    void entityAboutToBeDestroyed(Mayo::TreeNodeId entityTreeNodeId);
    //void itemPropertyChanged(DocumentItem* docItem, Property* prop);

public: // -- from TDocStd_Document
    void BeforeClose() override;
    void ChangeStorageFormat(const TCollection_ExtendedString& newStorageFormat) override;

    DEFINE_STANDARD_RTTI_INLINE(Document, TDocStd_Document)

private:
    friend class Application;
    class FormatBinaryRetrievalDriver;
    class FormatXmlRetrievalDriver;

    friend class XCafScopeImport;
    friend class SingleScopeImport;

    Document();
    void initXCaf();
    void setIdentifier(Identifier ident) { m_identifier = ident; }

    Identifier m_identifier = -1;
    std::string m_name;
    FilePath m_filePath;
    XCaf m_xcaf;
    Tree<TDF_Label> m_modelTree;
};

} // namespace Mayo
