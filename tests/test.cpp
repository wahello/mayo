/****************************************************************************
** Copyright (c) 2021, Fougue Ltd. <http://www.fougue.pro>
** All rights reserved.
** See license at https://github.com/fougue/mayo/blob/master/LICENSE.txt
****************************************************************************/

// Need to include this first because of MSVC conflicts with M_E, M_LOG2, ...
#include <BRepPrimAPI_MakeBox.hxx>

#include "test.h"
#include "../src/base/application.h"
#include "../src/base/brep_utils.h"
#include "../src/base/caf_utils.h"
#include "../src/base/filepath.h"
#include "../src/base/geom_utils.h"
#include "../src/base/io_system.h"
#include "../src/base/occ_static_variables_rollback.h"
#include "../src/base/libtree.h"
#include "../src/base/mesh_utils.h"
#include "../src/base/meta_enum.h"
#include "../src/base/property_builtins.h"
#include "../src/base/property_enumeration.h"
#include "../src/base/property_value_conversion.h"
#include "../src/base/string_conv.h"
#include "../src/base/task_manager.h"
#include "../src/base/tkernel_utils.h"
#include "../src/base/unit.h"
#include "../src/base/unit_system.h"
#include "../src/app/qstring_utils.h"
#include "../src/io_occ/io_occ.h"
#include "../src/gui/qtgui_utils.h"

#include <BRep_Tool.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <GCPnts_TangentialDeflection.hxx>
#include <Interface_ParamType.hxx>
#include <Interface_Static.hxx>
#include <TopAbs_ShapeEnum.hxx>
#include <QtCore/QtDebug>
#include <QtCore/QFile>
#include <QtCore/QVariant>
#include <QtTest/QSignalSpy>
#include <gsl/util>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <iostream>
#include <memory>
#include <sstream>
#include <utility>
#include <vector>

Q_DECLARE_METATYPE(Mayo::UnitSystem::TranslateResult)
// For Application_test()
Q_DECLARE_METATYPE(Mayo::IO::Format)
// For MeshUtils_orientation_test()
Q_DECLARE_METATYPE(std::vector<gp_Pnt2d>)
Q_DECLARE_METATYPE(Mayo::MeshUtils::Orientation)

namespace Mayo {

// For the sake of QCOMPARE()
static bool operator==(
        const UnitSystem::TranslateResult& lhs,
        const UnitSystem::TranslateResult& rhs)
{
    return std::abs(lhs.value - rhs.value) < 1e-6
            && std::strcmp(lhs.strUnit, rhs.strUnit) == 0
            && std::abs(lhs.factor - rhs.factor) < 1e-6;
}

void Test::Application_test()
{
    auto app = Application::instance();
    auto ioSystem = Application::instance()->ioSystem();
    auto fnImportInDocument = [=](const DocumentPtr& doc, const FilePath& fp) {
        return ioSystem->importInDocument()
                .targetDocument(doc)
                .withFilepath(fp)
                .execute();
    };
    QCOMPARE(app->documentCount(), 0);

    {   // Add & remove a document
        QSignalSpy sigSpy_documentAdded(app.get(), &Application::documentAdded);
        DocumentPtr doc = app->newDocument();
        QVERIFY(!doc.IsNull());
        QCOMPARE(sigSpy_documentAdded.count(), 1);
        QCOMPARE(app->documentCount(), 1);
        QCOMPARE(app->findIndexOfDocument(doc), 0);
        QCOMPARE(app->findDocumentByIndex(0).get(), doc.get());
        QCOMPARE(app->findDocumentByIdentifier(doc->identifier()).get(), doc.get());

        QSignalSpy sigSpy_documentAboutToClose(app.get(), &Application::documentAboutToClose);
        app->closeDocument(doc);
        QCOMPARE(sigSpy_documentAboutToClose.count(), 1);
        QCOMPARE(app->documentCount(), 0);
    }

    {   // Add & remove an entity
        DocumentPtr doc = app->newDocument();
        auto _ = gsl::finally([=]{ app->closeDocument(doc); });
        QCOMPARE(doc->entityCount(), 0);
        QSignalSpy sigSpy_docEntityAdded(doc.get(), &Document::entityAdded);
        const bool okImport = fnImportInDocument(doc, "inputs/cube.step");
        QVERIFY(okImport);
        QCOMPARE(sigSpy_docEntityAdded.count(), 1);
        QCOMPARE(doc->entityCount(), 1);
        QVERIFY(XCaf::isShape(doc->entityLabel(0)));
        QCOMPARE(CafUtils::labelAttrStdName(doc->entityLabel(0)), to_OccExtString("Cube"));

        QSignalSpy sigSpy_docEntityAboutToBeDestroyed(doc.get(), &Document::entityAboutToBeDestroyed);
        doc->destroyEntity(doc->entityTreeNodeId(0));
        QCOMPARE(sigSpy_docEntityAboutToBeDestroyed.count(), 1);
        QCOMPARE(doc->entityCount(), 0);
    }

    {   // Add mesh entity
        // Add XCAF entity
        // Try to remove mesh and XCAF entities
        // Note: order of entities matters
        QCOMPARE(app->documentCount(), 0);
        DocumentPtr doc = app->newDocument();
        auto _ = gsl::finally([=]{ app->closeDocument(doc); });
        bool okImport = true;
        okImport = fnImportInDocument(doc, "inputs/cube.stlb");
        QVERIFY(okImport);
        QCOMPARE(doc->entityCount(), 1);

        okImport = fnImportInDocument(doc, "inputs/cube.step");
        QVERIFY(okImport);
        QCOMPARE(doc->entityCount(), 2);

        doc->destroyEntity(doc->entityTreeNodeId(0));
        QCOMPARE(doc->entityCount(), 1);
        doc->destroyEntity(doc->entityTreeNodeId(0));
        QCOMPARE(doc->entityCount(), 0);
    }

    QCOMPARE(app->documentCount(), 0);
}

void Test::TextId_test()
{
    struct TextIdContext {
        MAYO_DECLARE_TEXT_ID_FUNCTIONS(Test::TextIdContext)
    };

    QVERIFY(TextIdContext::textIdContext() == "Test::TextIdContext");
    QVERIFY(TextIdContext::textId("foof").trContext == "Test::TextIdContext");
    QVERIFY(TextIdContext::textId("bark").key == "bark");
    QVERIFY(TextIdContext::textIdTr("shktu") == "shktu");
}

void Test::FilePath_test()
{
    const char strTestPath[] = "../as1-oc-214 - 測試文件.stp";

    {
        const QByteArray testPathQt = strTestPath;
        const FilePath testPath = filepathFrom(testPathQt);
        QCOMPARE(filepathTo<QByteArray>(testPath), testPathQt);
    }

    {
        const QString testPathQt = strTestPath;
        const FilePath testPath = filepathFrom(testPathQt);
        QCOMPARE(filepathTo<QString>(testPath), testPathQt);
    }

    {
        const TCollection_ExtendedString extStrTestPath(strTestPath, true/*multi-byte*/);
        const FilePath testPath = std::filesystem::u8path(strTestPath);
        QCOMPARE(filepathTo<TCollection_ExtendedString>(testPath), extStrTestPath);
    }
}

void Test::PropertyValueConversion_test()
{
    QFETCH(QString, strPropertyName);
    QFETCH(QVariant, variantValue);

    std::unique_ptr<Property> prop;
    if (strPropertyName == PropertyBool::TypeName) {
        prop.reset(new PropertyBool(nullptr, {}));
    }
    else if (strPropertyName == PropertyInt::TypeName) {
        prop.reset(new PropertyInt(nullptr, {}));
    }
    else if (strPropertyName == PropertyDouble::TypeName) {
        prop.reset(new PropertyDouble(nullptr, {}));
    }
    else if (strPropertyName == PropertyString::TypeName) {
        prop.reset(new PropertyString(nullptr, {}));
    }
    else if (strPropertyName == PropertyOccColor::TypeName) {
        prop.reset(new PropertyOccColor(nullptr, {}));
    }
    else if (strPropertyName == PropertyEnumeration::TypeName) {
        enum class MayoTest_Color { Bleu, Blanc, Rouge };
        prop.reset(new PropertyEnum<MayoTest_Color>(nullptr, {}));
    }

    QVERIFY(prop);

    PropertyValueConversion conv;
    QVERIFY(conv.fromVariant(prop.get(), variantValue));
    QCOMPARE(conv.toVariant(*prop.get()), variantValue);
}

void Test::PropertyValueConversion_test_data()
{
    QTest::addColumn<QString>("strPropertyName");
    QTest::addColumn<QVariant>("variantValue");
    QTest::newRow("bool(false)") << PropertyBool::TypeName << QVariant(false);
    QTest::newRow("bool(true)") << PropertyBool::TypeName << QVariant(true);
    QTest::newRow("int(-50)") << PropertyInt::TypeName << QVariant(-50);
    QTest::newRow("int(1979)") << PropertyInt::TypeName << QVariant(1979);
    QTest::newRow("double(-1e6)") << PropertyDouble::TypeName << QVariant(-1e6);
    QTest::newRow("double(3.1415926535)") << PropertyDouble::TypeName << QVariant(3.1415926535);
    QTest::newRow("QString(\"test\")") << PropertyString::TypeName << QVariant("test");
    QTest::newRow("QString(\"1558\")") << PropertyString::TypeName << QVariant(1558);
    QTest::newRow("OccColor(#0000AA)") << PropertyOccColor::TypeName << QVariant("#0000AA");
    QTest::newRow("OccColor(#FFFFFF)") << PropertyOccColor::TypeName << QVariant("#FFFFFF");
    QTest::newRow("OccColor(#BB0000)") << PropertyOccColor::TypeName << QVariant("#BB0000");
    QTest::newRow("Enumeration(Color)") << PropertyEnumeration::TypeName << QVariant("Blanc");
}

void Test::PropertyQuantityValueConversion_test()
{
    QFETCH(QString, strPropertyName);
    QFETCH(QVariant, variantFrom);
    QFETCH(QVariant, variantTo);

    std::unique_ptr<Property> prop;
    if (strPropertyName == "PropertyLength") {
        prop.reset(new PropertyLength(nullptr, {}));
    }
    else if (strPropertyName == "PropertyAngle") {
        prop.reset(new PropertyAngle(nullptr, {}));
    }

    QVERIFY(prop);

    PropertyValueConversion conv;
    conv.setDoubleToStringPrecision(7);
    QVERIFY(conv.fromVariant(prop.get(), variantFrom));
    QCOMPARE(conv.toVariant(*prop.get()), variantTo);
}

void Test::PropertyQuantityValueConversion_test_data()
{
    QTest::addColumn<QString>("strPropertyName");
    QTest::addColumn<QVariant>("variantFrom");
    QTest::addColumn<QVariant>("variantTo");
    QTest::newRow("Length(25mm)") << "PropertyLength" << QVariant("25mm") << QVariant("25mm");
    QTest::newRow("Length(2m)") << "PropertyLength" << QVariant("2m") << QVariant("2000mm");
    QTest::newRow("Length(1.57079rad)") << "PropertyAngle" << QVariant("1.57079rad") << QVariant("1.57079rad");
    QTest::newRow("Length(90°)") << "PropertyAngle" << QVariant("90°") << QVariant("1.570796rad");
}

void Test::IO_test()
{
    QFETCH(QString, strFilePath);
    QFETCH(IO::Format, expectedPartFormat);

    auto ioSystem = Application::instance()->ioSystem();
    QCOMPARE(ioSystem->probeFormat(filepathFrom(strFilePath)), expectedPartFormat);
}

void Test::IO_test_data()
{
    QTest::addColumn<QString>("strFilePath");
    QTest::addColumn<IO::Format>("expectedPartFormat");

    QTest::newRow("cube.step") << "inputs/cube.step" << IO::Format_STEP;
    QTest::newRow("cube.iges") << "inputs/cube.iges" << IO::Format_IGES;
    QTest::newRow("cube.brep") << "inputs/cube.brep" << IO::Format_OCCBREP;
    QTest::newRow("bezier_curve.brep") << "inputs/mayo_bezier_curve.brep" << IO::Format_OCCBREP;
    QTest::newRow("cube.stla") << "inputs/cube.stla" << IO::Format_STL;
    QTest::newRow("cube.stlb") << "inputs/cube.stlb" << IO::Format_STL;
    QTest::newRow("cube.obj") << "inputs/cube.obj" << IO::Format_OBJ;
}

void Test::IO_OccStaticVariablesRollback_test()
{
    QFETCH(QString, varName);
    QFETCH(QVariant, varInitValue);
    QFETCH(QVariant, varChangeValue);

    auto fnStaticVariableType = [](QVariant value) {
        switch (value.type()) {
        case QVariant::Int: return Interface_ParamInteger;
        case QVariant::Double: return Interface_ParamReal;
        case QVariant::String: return Interface_ParamText;
        default: return Interface_ParamMisc;
        }
    };
    auto fnStaticVariableValue = [](const char* varName, QVariant::Type valueType) -> QVariant {
        switch (valueType) {
        case QVariant::Int: return Interface_Static::IVal(varName);
        case QVariant::Double: return Interface_Static::RVal(varName);
        case QVariant::String: return Interface_Static::CVal(varName);
        default: return {};
        }
    };

    QCOMPARE(varInitValue.type(), varChangeValue.type());
    const QByteArray bytesVarName = varName.toLatin1();
    const char* cVarName = bytesVarName.constData();
    const QByteArray bytesVarInitValue = varInitValue.toString().toLatin1();
    Interface_Static::Init("MAYO", cVarName, fnStaticVariableType(varInitValue), bytesVarInitValue.constData());
    QVERIFY(Interface_Static::IsPresent(cVarName));
    QCOMPARE(fnStaticVariableValue(cVarName, varInitValue.type()), varInitValue);

    {
        IO::OccStaticVariablesRollback rollback;
        if (varChangeValue.type() == QVariant::Int)
            rollback.change(cVarName, varChangeValue.toInt());
        else if (varChangeValue.type() == QVariant::Double)
            rollback.change(cVarName, varChangeValue.toDouble());
        else if (varChangeValue.type() == QVariant::String)
            rollback.change(cVarName, varChangeValue.toString().toStdString());

        QCOMPARE(fnStaticVariableValue(cVarName, varChangeValue.type()), varChangeValue);
    }

    QCOMPARE(fnStaticVariableValue(cVarName, varInitValue.type()), varInitValue);
}

void Test::IO_OccStaticVariablesRollback_test_data()
{
    QTest::addColumn<QString>("varName");
    QTest::addColumn<QVariant>("varInitValue");
    QTest::addColumn<QVariant>("varChangeValue");

    QTest::newRow("var_int1") << "mayo.test.variable_int1" << QVariant(25) << QVariant(40);
    QTest::newRow("var_int2") << "mayo.test.variable_int2" << QVariant(0) << QVariant(5);
    QTest::newRow("var_double1") << "mayo.test.variable_double1" << QVariant(1.5) << QVariant(4.5);
    QTest::newRow("var_double2") << "mayo.test.variable_double2" << QVariant(50.7) << QVariant(25.8);
    QTest::newRow("var_str1") << "mayo.test.variable_str1" << QVariant("") << QVariant("value");
    QTest::newRow("var_str2") << "mayo.test.variable_str2" << QVariant("foo") << QVariant("blah");
}

void Test::BRepUtils_test()
{
    QVERIFY(BRepUtils::moreComplex(TopAbs_COMPOUND, TopAbs_SOLID));
    QVERIFY(BRepUtils::moreComplex(TopAbs_SOLID, TopAbs_SHELL));
    QVERIFY(BRepUtils::moreComplex(TopAbs_SHELL, TopAbs_FACE));
    QVERIFY(BRepUtils::moreComplex(TopAbs_FACE, TopAbs_EDGE));
    QVERIFY(BRepUtils::moreComplex(TopAbs_EDGE, TopAbs_VERTEX));

    {
        const TopoDS_Shape shapeNull;
        const TopoDS_Shape shapeBase = BRepPrimAPI_MakeBox(25, 25, 25);
        const TopoDS_Shape shapeCopy = shapeBase;
        QCOMPARE(BRepUtils::hashCode(shapeNull), -1);
        QVERIFY(BRepUtils::hashCode(shapeBase) >= 0);
        QCOMPARE(BRepUtils::hashCode(shapeBase), BRepUtils::hashCode(shapeCopy));
    }
}

void Test::CafUtils_test()
{
    // TODO Add CafUtils::labelTag() test for multi-threaded safety
}

void Test::MeshUtils_orientation_test()
{
    struct BasicPolyline2d : public Mayo::MeshUtils::AdaptorPolyline2d {
        gp_Pnt2d pointAt(int index) const override { return this->vecPoint.at(index); }
        int pointCount() const override { return int(this->vecPoint.size()); }
        std::vector<gp_Pnt2d> vecPoint;
    };

    QFETCH(std::vector<gp_Pnt2d>, vecPoint);
    QFETCH(Mayo::MeshUtils::Orientation, orientation);
    BasicPolyline2d polyline2d;
    polyline2d.vecPoint = std::move(vecPoint);
    QCOMPARE(Mayo::MeshUtils::orientation(polyline2d), orientation);
}

void Test::MeshUtils_orientation_test_data()
{
    QTest::addColumn<std::vector<gp_Pnt2d>>("vecPoint");
    QTest::addColumn<Mayo::MeshUtils::Orientation>("orientation");

    std::vector<gp_Pnt2d> vecPoint;
    vecPoint.push_back(gp_Pnt2d(0, 0));
    vecPoint.push_back(gp_Pnt2d(0, 10));
    vecPoint.push_back(gp_Pnt2d(10, 10));
    vecPoint.push_back(gp_Pnt2d(10, 0));
    vecPoint.push_back(gp_Pnt2d(0, 0)); // Closed polyline
    QTest::newRow("case1") << vecPoint << Mayo::MeshUtils::Orientation::Clockwise;

    vecPoint.erase(std::prev(vecPoint.end())); // Open polyline
    QTest::newRow("case2") << vecPoint << Mayo::MeshUtils::Orientation::Clockwise;

    std::reverse(vecPoint.begin(), vecPoint.end());
    QTest::newRow("case3") << vecPoint << Mayo::MeshUtils::Orientation::CounterClockwise;

    vecPoint.clear();
    vecPoint.push_back(gp_Pnt2d(0, 0));
    vecPoint.push_back(gp_Pnt2d(0, 10));
    vecPoint.push_back(gp_Pnt2d(10, 10));
    QTest::newRow("case4") << vecPoint << Mayo::MeshUtils::Orientation::Clockwise;

    vecPoint.clear();
    vecPoint.push_back(gp_Pnt2d(0, 0));
    vecPoint.push_back(gp_Pnt2d(0, 10));
    vecPoint.push_back(gp_Pnt2d(-10, 10));
    vecPoint.push_back(gp_Pnt2d(-10, 0));
    QTest::newRow("case5") << vecPoint << Mayo::MeshUtils::Orientation::CounterClockwise;

    std::reverse(vecPoint.begin(), vecPoint.end());
    QTest::newRow("case6") << vecPoint << Mayo::MeshUtils::Orientation::Clockwise;

    {
        QFile file("inputs/mayo_bezier_curve.brep");
        QVERIFY(file.open(QIODevice::ReadOnly));

        const TopoDS_Shape shape = BRepUtils::shapeFromString(file.readAll().toStdString());
        QVERIFY(!shape.IsNull());

        TopoDS_Edge edge;
        BRepUtils::forEachSubShape(shape, TopAbs_EDGE, [&](const TopoDS_Shape& subShape) {
            edge = TopoDS::Edge(subShape);
        });
        QVERIFY(!edge.IsNull());

        const GCPnts_TangentialDeflection discr(BRepAdaptor_Curve(edge), 0.1, 0.1);
        vecPoint.clear();
        for (int i = 1; i <= discr.NbPoints(); ++i) {
            const gp_Pnt pnt = discr.Value(i);
            vecPoint.push_back(gp_Pnt2d(pnt.X(), pnt.Y()));
        }

        QTest::newRow("case7") << vecPoint << Mayo::MeshUtils::Orientation::CounterClockwise;

        std::reverse(vecPoint.begin(), vecPoint.end());
        QTest::newRow("case8") << vecPoint << Mayo::MeshUtils::Orientation::Clockwise;
    }
}

void Test::MetaEnum_test()
{
    QCOMPARE(MetaEnum::name(TopAbs_VERTEX), "TopAbs_VERTEX");
    QCOMPARE(MetaEnum::name(TopAbs_EDGE), "TopAbs_EDGE");
    QCOMPARE(MetaEnum::name(TopAbs_WIRE), "TopAbs_WIRE");
    QCOMPARE(MetaEnum::name(TopAbs_FACE), "TopAbs_FACE");
    QCOMPARE(MetaEnum::name(TopAbs_SHELL), "TopAbs_SHELL");
    QCOMPARE(MetaEnum::nameWithoutPrefix(TopAbs_SOLID, "TopAbs_"), "SOLID");
    QCOMPARE(MetaEnum::nameWithoutPrefix(TopAbs_COMPOUND, "TopAbs_"), "COMPOUND");

    QCOMPARE(MetaEnum::nameWithoutPrefix(TopAbs_VERTEX, "Abs"), "TopAbs_VERTEX");
    QCOMPARE(MetaEnum::nameWithoutPrefix(TopAbs_VERTEX, ""), "TopAbs_VERTEX");
}

void Test::MeshUtils_test()
{
    // Create box
    QFETCH(double, boxDx);
    QFETCH(double, boxDy);
    QFETCH(double, boxDz);
    const TopoDS_Shape shapeBox = BRepPrimAPI_MakeBox(boxDx, boxDy, boxDz);

    // Mesh box
    {
        BRepMesh_IncrementalMesh mesher(shapeBox, 0.1);
        mesher.Perform();
        QVERIFY(mesher.IsDone());
    }

    // Count nodes and triangles
    int countNode = 0;
    int countTriangle = 0;
    BRepUtils::forEachSubFace(shapeBox, [&](const TopoDS_Face& face) {
        TopLoc_Location loc;
        const Handle_Poly_Triangulation& polyTri = BRep_Tool::Triangulation(face, loc);
        if (!polyTri.IsNull()) {
            countNode += polyTri->NbNodes();
            countTriangle += polyTri->NbTriangles();
        }
    });

    // Merge all face triangulations into one
    Handle_Poly_Triangulation polyTriBox = new Poly_Triangulation(countNode, countTriangle, false);
    {
        int idNodeOffset = 0;
        int idTriangleOffset = 0;
        BRepUtils::forEachSubFace(shapeBox, [&](const TopoDS_Face& face) {
            TopLoc_Location loc;
            const Handle_Poly_Triangulation& polyTri = BRep_Tool::Triangulation(face, loc);
            if (!polyTri.IsNull()) {
                for (int i = 1; i <= polyTri->NbNodes(); ++i)
                    polyTriBox->ChangeNode(idNodeOffset + i) = polyTri->Node(i);

                for (int i = 1; i <= polyTri->NbTriangles(); ++i) {
                    int n1, n2, n3;
                    polyTri->Triangle(i).Get(n1, n2, n3);
                    polyTriBox->ChangeTriangle(idTriangleOffset + i).Set(
                                idNodeOffset + n1, idNodeOffset + n2, idNodeOffset + n3);
                }

                idNodeOffset += polyTri->NbNodes();
                idTriangleOffset += polyTri->NbTriangles();
            }
        });
    }

    // Checks
    QCOMPARE(MeshUtils::triangulationVolume(polyTriBox),
             double(boxDx * boxDy * boxDz));
    QCOMPARE(MeshUtils::triangulationArea(polyTriBox),
             double(2 * boxDx * boxDy + 2 * boxDy * boxDz + 2 * boxDx * boxDz));
}

void Test::MeshUtils_test_data()
{
    QTest::addColumn<double>("boxDx");
    QTest::addColumn<double>("boxDy");
    QTest::addColumn<double>("boxDz");

    QTest::newRow("case1") << 10. << 15. << 20.;
    QTest::newRow("case2") << 0.1 << 0.25 << 0.044;
    QTest::newRow("case3") << 1e5 << 1e6 << 1e7;
    QTest::newRow("case4") << 40. << 50. << 70.;
}

void Test::Quantity_test()
{
    const QuantityArea area = (10 * Quantity_Millimeter) * (5 * Quantity_Centimeter);
    QCOMPARE(area.value(), 500.);
    QCOMPARE((Quantity_Millimeter / 5.).value(), 1/5.);
}

void Test::QStringUtils_append_test()
{
    QFETCH(QString, strExpected);
    QFETCH(QString, str1);
    QFETCH(QString, str2);
    QFETCH(QLocale, locale);

    QString strActual = str1;
    QStringUtils::append(&strActual, str2, locale);
    QCOMPARE(strActual, strExpected);
}

void Test::QStringUtils_append_test_data()
{
    QTest::addColumn<QString>("strExpected");
    QTest::addColumn<QString>("str1");
    QTest::addColumn<QString>("str2");
    QTest::addColumn<QLocale>("locale");

    QTest::newRow("locale_c")
            << QString("1234")
            << QStringLiteral("12")
            << QStringLiteral("34")
            << QLocale::c();
    QTest::newRow("locale_fr")
            << QString("1234")
            << QStringLiteral("12")
            << QStringLiteral("34")
            << QLocale(QLocale::French);
    QTest::newRow("locale_arabic")
            << QString("3412")
            << QStringLiteral("12")
            << QStringLiteral("34")
            << QLocale(QLocale::Arabic);
    QTest::newRow("locale_syriac")
            << QString("3412")
            << QStringLiteral("12")
            << QStringLiteral("34")
            << QLocale(QLocale::Syriac);
}

void Test::QStringUtils_text_test()
{
    QFETCH(QString, strActual);
    QFETCH(QString, strExpected);
    QCOMPARE(strActual, strExpected);
}

void Test::QStringUtils_text_test_data()
{
    QTest::addColumn<QString>("strActual");
    QTest::addColumn<QString>("strExpected");

    const QStringUtils::TextOptions opts_c_si_2 = { QLocale::c(), UnitSystem::SI, 2 };
    const QStringUtils::TextOptions opts_fr_si_2 = {
        QLocale(QLocale::French, QLocale::France), UnitSystem::SI, 2 };
    QTest::newRow("c_0.1")
            << QStringUtils::text(0.1, opts_c_si_2)
            << QStringLiteral("0.1");
    QTest::newRow("c_0.155")
            << QStringUtils::text(0.155, opts_c_si_2)
            << QStringLiteral("0.15");
    QTest::newRow("c_0.159")
            << QStringUtils::text(0.159, opts_c_si_2)
            << QStringLiteral("0.16");
    QTest::newRow("fr_1.4995")
            << QStringUtils::text(1.4995, opts_fr_si_2)
            << QStringLiteral("1,5");
    QTest::newRow("c_pnt0.55,4.8977,15.1445")
            << QStringUtils::text(gp_Pnt(0.55, 4.8977, 15.1445), opts_c_si_2)
            << QStringLiteral("(0.55mm 4.9mm 15.14mm)");
}

void Test::StringConv_test()
{
    const QString text = "test_éç²µ§_测试_Тест";
    QCOMPARE(to_QString(string_conv<TCollection_AsciiString>(text)), text);
    QCOMPARE(to_QString(string_conv<Handle(TCollection_HAsciiString)>(text)), text);
    QCOMPARE(to_QString(to_stdString(text)), text);
    QCOMPARE(to_QString(to_OccExtString(text)), text);
}

void Test::TKernelUtils_colorToHex_test()
{
    QFETCH(int, red);
    QFETCH(int, green);
    QFETCH(int, blue);
    QFETCH(QString, strHexColor);

    const Quantity_Color color(red / 255., green / 255., blue / 255., Quantity_TOC_RGB);
    const std::string strHexColorActual = TKernelUtils::colorToHex(color);
    QCOMPARE(QString::fromStdString(strHexColorActual), strHexColor);
}

void Test::TKernelUtils_colorToHex_test_data()
{
    QTest::addColumn<int>("red");
    QTest::addColumn<int>("green");
    QTest::addColumn<int>("blue");
    QTest::addColumn<QString>("strHexColor");

    QTest::newRow("RGB(  0,  0,  0)") << 0 << 0 << 0 << "#000000";
    QTest::newRow("RGB(255,255,255)") << 255 << 255 << 255 << "#FFFFFF";
    QTest::newRow("RGB(  5,  5,  5)") << 5 << 5 << 5 << "#050505";
    QTest::newRow("RGB(155,208, 67)") << 155 << 208 << 67 << "#9BD043";
    QTest::newRow("RGB(100,150,200)") << 100 << 150 << 200 << "#6496C8";
}

void Test::TKernelUtils_colorFromHex_test()
{
    QFETCH(int, red);
    QFETCH(int, green);
    QFETCH(int, blue);
    QFETCH(QString, strHexColor);
    const Quantity_Color expectedColor(red / 255., green / 255., blue / 255., Quantity_TOC_RGB);

    Quantity_Color actualColor;
    QVERIFY(TKernelUtils::colorFromHex(strHexColor.toStdString(), &actualColor));
    QCOMPARE(actualColor, expectedColor);
}

void Test::TKernelUtils_colorFromHex_test_data()
{
    QTest::addColumn<int>("red");
    QTest::addColumn<int>("green");
    QTest::addColumn<int>("blue");
    QTest::addColumn<QString>("strHexColor");

    QTest::newRow("RGB(  0,  0,  0)") << 0 << 0 << 0 << "#000000";
    QTest::newRow("RGB(255,255,255)") << 255 << 255 << 255 << "#FFFFFF";
    QTest::newRow("RGB(  5,  5,  5)") << 5 << 5 << 5 << "#050505";
    QTest::newRow("RGB(155,208, 67)") << 155 << 208 << 67 << "#9BD043";
    QTest::newRow("RGB(100,150,200)") << 100 << 150 << 200 << "#6496C8";
}

void Test::UnitSystem_test()
{
    QFETCH(UnitSystem::TranslateResult, trResultActual);
    QFETCH(UnitSystem::TranslateResult, trResultExpected);
    QCOMPARE(trResultActual, trResultExpected);
}

void Test::UnitSystem_test_data()
{
    QTest::addColumn<UnitSystem::TranslateResult>("trResultActual");
    QTest::addColumn<UnitSystem::TranslateResult>("trResultExpected");

    const UnitSystem::Schema schemaSI = UnitSystem::SI;
    QTest::newRow("80mm")
            << UnitSystem::translate(schemaSI, 80 * Quantity_Millimeter)
            << UnitSystem::TranslateResult{ 80., "mm", 1. };
    QTest::newRow("8cm")
            << UnitSystem::translate(schemaSI, 8 * Quantity_Centimeter)
            << UnitSystem::TranslateResult{ 80., "mm", 1. };
    QTest::newRow("8m")
            << UnitSystem::translate(schemaSI, 8 * Quantity_Meter)
            << UnitSystem::TranslateResult{ 8000., "mm", 1. };
    QTest::newRow("50mm²")
            << UnitSystem::translate(schemaSI, 0.5 * Quantity_SquaredCentimer)
            << UnitSystem::TranslateResult{ 50., "mm²", 1. };
    constexpr double radDeg = Quantity_Degree.value();
    QTest::newRow("degrees(PIrad)")
            << UnitSystem::degrees(3.14159265358979323846 * Quantity_Radian)
            << UnitSystem::TranslateResult{ 180., "°", radDeg };
}

void Test::LibTask_test()
{
    struct ProgressRecord {
        TaskId taskId;
        int value;
    };

    TaskManager taskMgr;
    const TaskId taskId = taskMgr.newTask([=](TaskProgress* progress) {
        {
            TaskProgress subProgress(progress, 40);
            for (int i = 0; i <= 100; ++i)
                subProgress.setValue(i);
        }

        {
            TaskProgress subProgress(progress, 60);
            for (int i = 0; i <= 100; ++i)
                subProgress.setValue(i);
        }
    });
    std::vector<ProgressRecord> vecProgressRec;
    QObject::connect(
                &taskMgr, &TaskManager::progressChanged,
                [&](TaskId taskId, int pct) { vecProgressRec.push_back({ taskId, pct }); });

    QSignalSpy sigSpy_started(&taskMgr, &TaskManager::started);
    QSignalSpy sigSpy_ended(&taskMgr, &TaskManager::ended);
    taskMgr.run(taskId);
    taskMgr.waitForDone(taskId);

    QCOMPARE(sigSpy_started.count(), 1);
    QCOMPARE(sigSpy_ended.count(), 1);
    QCOMPARE(qvariant_cast<TaskId>(sigSpy_started.front().at(0)), taskId);
    QCOMPARE(qvariant_cast<TaskId>(sigSpy_ended.front().at(0)), taskId);
    QVERIFY(!vecProgressRec.empty());
    int prevPct = 0;
    for (const ProgressRecord& rec : vecProgressRec) {
        QCOMPARE(rec.taskId, taskId);
        QVERIFY(prevPct <= rec.value);
        prevPct = rec.value;
    }

    QCOMPARE(vecProgressRec.front().value, 0);
    QCOMPARE(vecProgressRec.back().value, 100);
}

void Test::LibTree_test()
{
    const TreeNodeId nullptrId = 0;
    Tree<std::string> tree;
    std::vector<TreeNodeId> vecTreeNodeId;
    auto fnAppendChild = [&](TreeNodeId parentId, const char* str) {
        const TreeNodeId id = tree.appendChild(parentId, str);
        vecTreeNodeId.push_back(id);
        return id;
    };
    const TreeNodeId n0 = fnAppendChild(nullptrId, "0");
    const TreeNodeId n0_1 = fnAppendChild(n0, "0-1");
    const TreeNodeId n0_2 = fnAppendChild(n0, "0-2");
    const TreeNodeId n0_1_1 = fnAppendChild(n0_1, "0-1-1");
    const TreeNodeId n0_1_2 = fnAppendChild(n0_1, "0-1-2");
    std::sort(vecTreeNodeId.begin(), vecTreeNodeId.end());

    QCOMPARE(tree.nodeParent(n0_1), n0);
    QCOMPARE(tree.nodeParent(n0_2), n0);
    QCOMPARE(tree.nodeParent(n0_1_1), n0_1);
    QCOMPARE(tree.nodeParent(n0_1_2), n0_1);
    QCOMPARE(tree.nodeChildFirst(n0_1), n0_1_1);
    QCOMPARE(tree.nodeChildLast(n0_1), n0_1_2);
    QCOMPARE(tree.nodeSiblingNext(n0_1_1), n0_1_2);
    QCOMPARE(tree.nodeSiblingPrevious(n0_1_2), n0_1_1);
    QCOMPARE(tree.nodeSiblingNext(n0_1_2), nullptrId);

    {
        std::string strPreOrder;
        traverseTree_preOrder(tree, [&](TreeNodeId id) {
            strPreOrder += " " + tree.nodeData(id);
        });
        QCOMPARE(strPreOrder, " 0 0-1 0-1-1 0-1-2 0-2");
    }

    {
        std::string strPostOrder;
        traverseTree_postOrder(tree, [&](TreeNodeId id) {
            strPostOrder += " " + tree.nodeData(id);
        });
        QCOMPARE(strPostOrder, " 0-1-1 0-1-2 0-1 0-2 0");
    }

    {
        std::vector<TreeNodeId> vecTreeNodeIdVisited;
        traverseTree_unorder(tree, [&](TreeNodeId id) {
            vecTreeNodeIdVisited.push_back(id);
        });
        std::sort(vecTreeNodeIdVisited.begin(), vecTreeNodeIdVisited.end());
        QCOMPARE(vecTreeNodeIdVisited, vecTreeNodeId);
    }
}

void Test::QtGuiUtils_test()
{
    const QColor qtColor(51, 75, 128);
    const QColor qtColorA(51, 75, 128, 87);
    auto occColor = QtGuiUtils::toColor<Quantity_Color>(qtColor);
    auto occColorA = QtGuiUtils::toColor<Quantity_ColorRGBA>(qtColorA);
    QCOMPARE(QtGuiUtils::toQColor(occColor), qtColor);
    QCOMPARE(QtGuiUtils::toQColor(occColorA), qtColorA);
}

void Test::initTestCase()
{
    IO::System* ioSystem = Application::instance()->ioSystem();
    ioSystem->addFactoryReader(std::make_unique<IO::OccFactoryReader>());
    ioSystem->addFactoryWriter(std::make_unique<IO::OccFactoryWriter>());
    IO::addPredefinedFormatProbes(ioSystem);
}

} // namespace Mayo

