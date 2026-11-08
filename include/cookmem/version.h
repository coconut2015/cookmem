/*
 * Copyright (c) 2026 Heng Yuan
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/**
 * @file
 * The version of CookMem.
 *
 * This is the only place where the version number is defined.  CMakeLists.txt
 * reads COOKMEM_VERSION_MAJOR, COOKMEM_VERSION_MINOR and COOKMEM_VERSION_PATCH
 * from this file, so keep each of them on its own line in the form
 * "#define COOKMEM_VERSION_XXX number".
 */
#ifndef COOK_MEM_VERSION_H
#define COOK_MEM_VERSION_H

/** The major version number. */
#define COOKMEM_VERSION_MAJOR 0
/** The minor version number.  Must be less than 100. */
#define COOKMEM_VERSION_MINOR 1
/** The patch version number.  Must be less than 100. */
#define COOKMEM_VERSION_PATCH 0

/**
 * The version as a single number, MAJOR * 10000 + MINOR * 100 + PATCH, so
 * that it can be compared in preprocessor conditions.  For example, version
 * 1.2.3 is 10203.
 *
 * @code
 * #if COOKMEM_VERSION >= 10000
 * // code that requires CookMem 1.0 or later
 * #endif
 * @endcode
 */
#define COOKMEM_VERSION \
    (COOKMEM_VERSION_MAJOR * 10000 + COOKMEM_VERSION_MINOR * 100 + COOKMEM_VERSION_PATCH)

/** @cond INTERNAL */
#define COOKMEM_VERSION_STRINGIZE_(x) #x
#define COOKMEM_VERSION_STRINGIZE(x) COOKMEM_VERSION_STRINGIZE_(x)
/** @endcond */

/**
 * The version as a string in the form "MAJOR.MINOR.PATCH", such as "1.2.3".
 */
#define COOKMEM_VERSION_STRING \
    COOKMEM_VERSION_STRINGIZE(COOKMEM_VERSION_MAJOR) "." \
    COOKMEM_VERSION_STRINGIZE(COOKMEM_VERSION_MINOR) "." \
    COOKMEM_VERSION_STRINGIZE(COOKMEM_VERSION_PATCH)

#endif  // COOK_MEM_VERSION_H
