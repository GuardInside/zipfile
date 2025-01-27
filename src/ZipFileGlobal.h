#pragma once

#include <QtGlobal>

#if defined(ZIP_FILE_LIBRARY)
#define ZIP_FILE_EXPORT Q_DECL_EXPORT
#else
#define ZIP_FILE_EXPORT Q_DECL_IMPORT
#endif
