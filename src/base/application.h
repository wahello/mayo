/****************************************************************************
** Copyright (c) 2021, Fougue Ltd. <http://www.fougue.pro>
** All rights reserved.
** See license at https://github.com/fougue/mayo/blob/master/LICENSE.txt
****************************************************************************/

#pragma once

#include "application_ptr.h"
#include "document.h"
#include <CDF_DirectoryIterator.hxx>

namespace Mayo {

class Settings;
class DocumentTreeNodePropertiesProviderTable;

namespace IO { class System; }

class Application : public QObject, public TDocStd_Application {
    Q_OBJECT
public:
    ~Application();

    static const ApplicationPtr& instance();

    struct DocumentIterator : private CDF_DirectoryIterator {
        DocumentIterator(const ApplicationPtr& app);
        DocumentIterator(const Application* app);
        bool hasNext() const;
        void next();
        DocumentPtr current() const;
        int currentIndex() const { return m_currentIndex; }
    private:
        int m_currentIndex = 0;
    };

    int documentCount() const;
    DocumentPtr newDocument(Document::Format docFormat = Document::Format::Binary);
    DocumentPtr openDocument(const FilePath& filepath, PCDM_ReaderStatus* ptrReadStatus = nullptr);
    DocumentPtr findDocumentByIndex(int docIndex) const;
    DocumentPtr findDocumentByIdentifier(Document::Identifier docIdent) const;
    DocumentPtr findDocumentByLocation(const FilePath& location) const;
    int findIndexOfDocument(const DocumentPtr& doc) const;

    void closeDocument(const DocumentPtr& doc);

    Settings* settings() const;
    IO::System* ioSystem() const;
    DocumentTreeNodePropertiesProviderTable* documentTreeNodePropertiesProviderTable() const;

    static void setOpenCascadeEnvironment(const FilePath& settingsFilepath);

public: //  from TDocStd_Application
    void NewDocument(
            const TCollection_ExtendedString& format,
            opencascade::handle<TDocStd_Document>& outDoc) override;
    void InitDocument(const opencascade::handle<TDocStd_Document>& doc) const override;

// TODO: Redefine TDocStd_Document::BeforeClose() to emit signal documentClosed
// class Document : public TDocStd_Document { ... };
// using DocumentPtr = Handle_Document
// -> Can't do because PCDM_RetrievalDriver subclasses create explicitly "new TDocStd_Document(...)"
//    This would break TDocStd_Application::Open(...)

    DEFINE_STANDARD_RTTI_INLINE(Application, TDocStd_Application)

signals:
    void documentAdded(const Mayo::DocumentPtr& doc);
    void documentAboutToClose(const Mayo::DocumentPtr& doc);
    void documentNameChanged(const Mayo::DocumentPtr& doc, const std::string& name);
    void documentEntityAdded(const Mayo::DocumentPtr& doc, Mayo::TreeNodeId entityId);
    void documentEntityAboutToBeDestroyed(const Mayo::DocumentPtr& doc, Mayo::TreeNodeId entityId);

private: // Implementation
    friend class Document;

    Application();
    void notifyDocumentAboutToClose(Document::Identifier docIdent);
    void addDocument(const DocumentPtr& doc);

    struct Private;
    Private* const d;
};

} // namespace Mayo
