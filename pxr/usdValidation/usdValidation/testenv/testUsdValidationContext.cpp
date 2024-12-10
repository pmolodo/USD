//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/arch/systemInfo.h"
#include "pxr/base/plug/registry.h"
#include "pxr/usdValidation/usdValidation/context.h"
#include "pxr/usdValidation/usdValidation/error.h"
#include "pxr/usdValidation/usdValidation/registry.h"
#include "pxr/usdValidation/usdValidation/validator.h"

PXR_NAMESPACE_USING_DIRECTIVE

static std::string
_LayerContents()
{
    static const std::string layerContents =
        R"usda(#usda 1.0
        def "World"
        {
            def BaseTypeTest "baseType"
            {
            }
            def DerivedTypeTest "derivedType"
            {
            }
            def NestedDerivedTypeTest "nestedDerivedType"
            {
            }
            def "somePrim" (
                prepend apiSchemas = ["APISchemaTestAPI"]
            )
            {
            }
        }
    )usda";
    return layerContents;
}

static SdfLayerRefPtr
_CreateTestLayer()
{
    SdfLayerRefPtr testLayer = SdfLayer::CreateAnonymous(".usda");
    testLayer->ImportFromString(_LayerContents());
    return testLayer;
}

static void
_TestError1(const UsdValidationError &error)
{
    TF_AXIOM(error.GetValidator()->GetMetadata().name
             == TfToken("testUsdValidationContextValidatorsPlugin:Test1"));
    TF_AXIOM(error.GetSites().size() == 1);
    TF_AXIOM(error.GetSites()[0].IsPrim());
    TF_AXIOM(error.GetSites()[0].GetPrim().GetPath()
             == SdfPath::AbsoluteRootPath());
}

static void
_TestError2(const UsdValidationError &error)
{
    TF_AXIOM(error.GetValidator()->GetMetadata().name
             == TfToken("testUsdValidationContextValidatorsPlugin:Test2"));
    TF_AXIOM(error.GetSites().size() == 1);
    TF_AXIOM(error.GetSites()[0].IsValidSpecInLayer());
}

static void
_TestError3(const UsdValidationError &error)
{
    TF_AXIOM(error.GetValidator()->GetMetadata().name
             == TfToken("testUsdValidationContextValidatorsPlugin:Test3"));
    const std::set<SdfPath> expectedPrimPaths
        = { SdfPath("/World"), SdfPath("/World/baseType"),
            SdfPath("/World/derivedType"), SdfPath("/World/nestedDerivedType"),
            SdfPath("/World/somePrim") };
    TF_AXIOM(expectedPrimPaths.find(error.GetSites()[0].GetPrim().GetPath())
             != expectedPrimPaths.end());
}

static void
_TestError4(const UsdValidationError &error)
{
    TF_AXIOM(error.GetValidator()->GetMetadata().name
             == TfToken("testUsdValidationContextValidatorsPlugin:Test4"));
    const std::set<SdfPath> expectedPrimPaths
        = { SdfPath("/World/baseType"), SdfPath("/World/derivedType"),
            SdfPath("/World/nestedDerivedType") };
    TF_AXIOM(expectedPrimPaths.find(error.GetSites()[0].GetPrim().GetPath())
             != expectedPrimPaths.end());
}

static void
_TestError5(const UsdValidationError &error)
{
    TF_AXIOM(error.GetValidator()->GetMetadata().name
             == TfToken("testUsdValidationContextValidatorsPlugin:Test5"));
    const std::set<SdfPath> expectedPrimPaths
        = { SdfPath("/World/derivedType"),
            SdfPath("/World/nestedDerivedType") };
    TF_AXIOM(expectedPrimPaths.find(error.GetSites()[0].GetPrim().GetPath())
             != expectedPrimPaths.end());
}

static void
_TestError6(const UsdValidationError &error)
{
    TF_AXIOM(error.GetValidator()->GetMetadata().name
             == TfToken("testUsdValidationContextValidatorsPlugin:Test6"));
    TF_AXIOM(error.GetSites().size() == 1);
    TF_AXIOM(error.GetSites()[0].IsPrim());
    TF_AXIOM(error.GetSites()[0].GetPrim().GetName()
             == TfToken("nestedDerivedType"));
}

static void
_TestError7(const UsdValidationError &error)
{
    TF_AXIOM(error.GetName() == TfToken("Test7Error"));
    TF_AXIOM(error.GetValidator()->GetMetadata().name
             == TfToken("testUsdValidationContextValidatorsPlugin:Test7"));
    TF_AXIOM(error.GetSites().size() == 1);
    TF_AXIOM(error.GetSites()[0].IsPrim());
    TF_AXIOM(error.GetSites()[0].GetPrim().GetName() == TfToken("somePrim"));
}

static void
_TestNonPluginError(const UsdValidationError &error)
{
    TF_AXIOM(error.GetValidator()->GetMetadata().name
             == TfToken("nonPluginValidator"));

    const SdfPath expectedPrimPaths = SdfPath("/");
    TF_AXIOM(error.GetSites()[0].GetPrim().GetPath() == expectedPrimPaths);
}

static void
_TestUsdValidationContext()
{
    // Test the UsdValidationContext here.
    {
        // Create a ValidationContext with a suite
        const UsdValidationValidatorSuite *suite
            = UsdValidationRegistry::GetInstance()
                  .GetOrLoadValidatorSuiteByName(TfToken(
                      "testUsdValidationContextValidatorsPlugin:TestSuite"));
        UsdValidationContext context({ suite });
        SdfLayerRefPtr testLayer = _CreateTestLayer();
        // Run Validate(layer)
        UsdValidationErrorVector errors = context.Validate(testLayer);
        // 1 error for Test2 validator - root layer of the stage
        TF_AXIOM(errors.size() == 1);
        _TestError2(errors[0]);

        // Run Validate(stage)
        errors.clear();
        UsdStageRefPtr stage = UsdStage::Open(testLayer);
        errors = context.Validate(stage);
        // 1 error for Test1 validator (stage)
        // 2 error for Test2 validator - root layer and session layer
        // 5 errors for Test3 generic prim validator which runs on all 5 prims
        TF_AXIOM(errors.size() == 8);
        for (const auto &error : errors) {
            if (error.GetName() == TfToken("Test1Error")) {
                _TestError1(error);
            } else if (error.GetName() == TfToken("Test2Error")) {
                _TestError2(error);
            } else if (error.GetName() == TfToken("Test3Error")) {
                _TestError3(error);
            } else {
                TF_AXIOM(false);
            }
        }
    }
    {
        // Create a ValidationContext with explicit schemaTypes
        UsdValidationContext context({ TfType::FindByName("testBaseType") });
        SdfLayerRefPtr testLayer = _CreateTestLayer();
        // Run Validate(layer)
        UsdValidationErrorVector errors = context.Validate(testLayer);
        // 0 errors as we do not have any layer validators selected in this
        // context.
        TF_AXIOM(errors.empty());

        // Run Validate(stage)
        UsdStageRefPtr stage = UsdStage::Open(testLayer);
        errors = context.Validate(stage);
        // 3 errors for Test4 testBaseType prim type validator which runs on
        // the baseType, derivedType and nestedDerivedType prims
        TF_AXIOM(errors.size() == 3);

        for (const auto &error : errors) {
            _TestError4(error);
        }
    }
    {
        // Create a ValidationContext with explicit schemaType - apiSchema
        UsdValidationContext context(
            { TfType::FindByName("testAPISchemaAPI") });
        SdfLayerRefPtr testLayer = _CreateTestLayer();
        // Run Validate(layer)
        UsdValidationErrorVector errors = context.Validate(testLayer);
        // 0 errors as we do not have any layer validators selected in this
        // context.
        TF_AXIOM(errors.empty());

        // Run Validate(stage)
        UsdStageRefPtr stage = UsdStage::Open(testLayer);
        errors = context.Validate(stage);
        // 1 error for Test7 testAPISchema prim type validator which runs on
        // the somePrim prim
        TF_AXIOM(errors.size() == 1);
        _TestError7(errors[0]);
    }
    {
        // Create a ValidationContext with keywords API and have
        // includeAllAncestors set to default (true)
        UsdValidationContext context({ TfToken("Keyword1") });
        SdfLayerRefPtr testLayer = _CreateTestLayer();
        // Run Validate(layer)
        UsdValidationErrorVector errors = context.Validate(testLayer);
        // 0 errors as we do not have any layer validators selected in this
        // context.
        TF_AXIOM(errors.empty());

        // Run Validate(stage)
        UsdStageRefPtr stage = UsdStage::Open(testLayer);
        errors = context.Validate(stage);
        // 1 error for Test1 validator
        // 5 errors for Test3 generic prim validator which runs on all 5 prims
        // 2 errors for Test5 testDerivedType prim type validator which runs on
        //   the derivedType and nestedDerivedType prims
        // 3 errors for Test4 testBaseType prim type validator which runs on
        // the baseType, derivedType and nestedDerivedType prims (This gets
        // includes as an ancestor type of derivedType)
        // 1 error for Test7 testAPISchema prim type validator which runs on
        //  the somePrim prim
        TF_AXIOM(errors.size() == 12);

        for (const auto &error : errors) {
            if (error.GetName() == TfToken("Test1Error")) {
                _TestError1(error);
            } else if (error.GetName() == TfToken("Test3Error")) {
                _TestError3(error);
            } else if (error.GetName() == TfToken("Test4Error")) {
                _TestError4(error);
            } else if (error.GetName() == TfToken("Test5Error")) {
                _TestError5(error);
            } else if (error.GetName() == TfToken("Test7Error")) {
                _TestError7(error);
            } else {
                TF_AXIOM(false);
            }
        }
    }
    {
        // Create a ValidationContext with keywords API and have
        // includeAllAncestors set to false.
        UsdValidationContext context({ TfToken("Keyword2") }, false);
        SdfLayerRefPtr testLayer = _CreateTestLayer();
        // Run Validate(layer)
        UsdValidationErrorVector errors = context.Validate(testLayer);
        // 1 error for Test2 validator - root layer of the stage
        TF_AXIOM(errors.size() == 1);
        _TestError2(errors[0]);

        errors.clear();
        // Run Validate(prims)
        UsdStageRefPtr stage = UsdStage::Open(testLayer);
        errors = context.Validate(stage->Traverse());
        // 3 errors for Test4 testBaseType prim type validator which runs on
        //  the baseType, derivedType and nestedDerivedType prims
        // 1 error for Test6 testNestedDerivedType prim type validator which
        //  runs on the nestedDerivedType prim
        // 5 errors for testNonPluginValidator which runs on all prims
        // Because of TestSuite:
        // 5 errors for Test3 generic prim validator which runs on all 5 prims
        TF_AXIOM(errors.size() == 14);

        for (const auto &error : errors) {
            if (error.GetName() == TfToken("Test3Error")) {
                _TestError3(error);
            } else if (error.GetName() == TfToken("Test4Error")) {
                _TestError4(error);
            } else if (error.GetName() == TfToken("Test6Error")) {
                _TestError6(error);
            } else if (error.GetName() == TfToken("nonPluginError")) {
                _TestNonPluginError(error);
            } else {
                TF_AXIOM(false);
            }
        }

        errors.clear();
        // Run Validate(stage)
        errors = context.Validate(stage);
        // 2 error for Test2 validator - root layer and session layer
        // 3 errors for Test4 testBaseType prim type validator which runs on
        //  the baseType, derivedType and nestedDerivedType prims
        // 1 error for Test6 testNestedDerivedType prim type validator which
        //  runs on the nestedDerivedType prim
        // 5 errors for testNonPluginValidator which runs on all prims
        // Because of TestSuite:
        // 1 error for Test1 validator
        // 5 errors for Test3 generic prim validator which runs on all 5 prims
        TF_AXIOM(errors.size() == 17);

        for (const auto &error : errors) {
            if (error.GetName() == TfToken("Test1Error")) {
                _TestError1(error);
            } else if (error.GetName() == TfToken("Test2Error")) {
                _TestError2(error);
            } else if (error.GetName() == TfToken("Test3Error")) {
                _TestError3(error);
            } else if (error.GetName() == TfToken("Test4Error")) {
                _TestError4(error);
            } else if (error.GetName() == TfToken("Test6Error")) {
                _TestError6(error);
            } else if (error.GetName() == TfToken("nonPluginError")) {
                _TestNonPluginError(error);
            } else {
                TF_AXIOM(false);
            }
        }
    }
    {
        // Create a ValidationContext with plugins
        UsdValidationContext context(
            { PlugRegistry::GetInstance().GetPluginWithName(
                "testUsdValidationContextValidatorsPlugin") });
        SdfLayerRefPtr testLayer = _CreateTestLayer();
        UsdStageRefPtr stage = UsdStage::Open(testLayer);
        UsdValidationErrorVector errors = context.Validate(stage);
        // 1 error for Test1 validator
        // 2 error for Test2 validator - root layer and session layer
        // 5 errors for Test3 generic prim validator which runs on all 5 prims
        // 3 errors for Test4 testBaseType prim type validator which runs on
        //  the baseType, derivedType and nestedDerivedType prims
        // 2 error for Test5 testDerivedType prim type validator which runs on
        //   the derivedType and nestedDerivedType prims
        // 1 error for Test6 testNestedDerivedType prim type validator which
        //  runs on the nestedDerivedType prim
        // 1 error for Test7 testAPISchema prim type validator which runs on
        //  the somePrim prim
        TF_AXIOM(errors.size() == 15);

        for (const auto &error : errors) {
            if (error.GetName() == TfToken("Test1Error")) {
                _TestError1(error);
            } else if (error.GetName() == TfToken("Test2Error")) {
                _TestError2(error);
            } else if (error.GetName() == TfToken("Test3Error")) {
                _TestError3(error);
            } else if (error.GetName() == TfToken("Test4Error")) {
                _TestError4(error);
            } else if (error.GetName() == TfToken("Test5Error")) {
                _TestError5(error);
            } else if (error.GetName() == TfToken("Test6Error")) {
                _TestError6(error);
            } else if (error.GetName() == TfToken("Test7Error")) {
                _TestError7(error);
            } else {
                TF_AXIOM(false);
            }
        }
    }
}

int
main()
{
    // Register the test plugins
    // Plugin which provides test usd schema types
    const std::string testTypePluginPath = ArchGetCwd() + "/resources";
    TF_AXIOM(!PlugRegistry::GetInstance()
                  .RegisterPlugins(testTypePluginPath)
                  .empty());
    // Plugin which provides test validators
    const std::string testValidatorPluginPath
        = TfStringCatPaths(
              TfGetPathName(ArchGetExecutablePath()),
              "UsdValidationPlugins/lib/TestUsdValidationContextValidators*/"
              "Resources/")
        + "/";
    TF_AXIOM(!PlugRegistry::GetInstance()
                  .RegisterPlugins(testValidatorPluginPath)
                  .empty());

    // Add a non-plugin based validator here.
    {
        UsdValidationValidatorMetadata metadata;
        metadata.name = TfToken("nonPluginValidator");
        metadata.keywords = { TfToken("Keyword2") };
        metadata.pluginPtr = nullptr;
        metadata.doc = "This is a non-plugin based validator.";
        metadata.isSuite = false;

        const UsdValidatePrimTaskFn primTaskFn = [](const UsdPrim &prim) {
            const TfToken errorId("nonPluginError");
            return UsdValidationErrorVector { UsdValidationError(
                errorId, UsdValidationErrorType::Error,
                { UsdValidationErrorSite(prim.GetStage(),
                                         SdfPath::AbsoluteRootPath()) },
                "A non-plugin based validator error") };
        };

        // Register the validator
        TfErrorMark m;
        UsdValidationRegistry::GetInstance().RegisterValidator(metadata,
                                                               primTaskFn);
        TF_AXIOM(m.IsClean());
    }

    _TestUsdValidationContext();
}
