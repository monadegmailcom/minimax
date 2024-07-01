#include "tree.h"

using namespace std;

void print_preamble( ostream& stream, TreeType type )
{
    stream << "graph T {\n"
           << "node [fontname=\"Courier New\"]\n";
    if (type == Circular)
        stream << "graph [layout=circo]\n";
}

void print_closing( ostream& stream )
{
    stream << "}\n\n" << flush;
}
