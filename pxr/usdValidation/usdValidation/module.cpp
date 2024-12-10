//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/base/tf/pyModule.h"

PXR_NAMESPACE_USING_DIRECTIVE

TF_WRAP_MODULE
{
    TF_WRAP(UsdValidationContext);
    TF_WRAP(UsdValidationError);
    TF_WRAP(UsdValidationRegistry);
    TF_WRAP(UsdValidationValidator);
}
