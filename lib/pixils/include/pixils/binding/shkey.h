
#ifndef PIXILS__BINDING__SHKEY_H
#define PIXILS__BINDING__SHKEY_H

#include <lisple/runtime/value.h>

/**
 * Declare a shared keyword constant defined in a corresponding .cpp.
 */
#define DECL_SHKEY(NAME) extern const Lisple::sptr_val NAME;

/**
 * Define a shared keyword constant with external linkage.
 * The extern keyword gives the const variable external linkage, making it
 * visible to translation units that see the DECL_SHKEY declaration.
 */
#define SHKEY(NAME, STR) extern const Lisple::sptr_val NAME = Lisple::keyword(STR);

#endif /* PIXILS__BINDING__SHKEY_H */
