//because windows sucks...

#ifndef FILEHASHQTSO_CROSSPLATFORMMACROS_HPP
#define FILEHASHQTSO_CROSSPLATFORMMACROS_HPP

#include <QtCore/QtGlobal>

//remember to define this variable in the .pro file
#if defined(FILEHASHQTSO_LIBRARY)
#  define EXPIMP_FILEHASHQTSO Q_DECL_EXPORT
#else
#  define EXPIMP_FILEHASHQTSO Q_DECL_IMPORT
#endif

#endif // FILEHASHQTSO_CROSSPLATFORMMACROS_HPP
