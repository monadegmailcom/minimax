#include "tree.h"

using namespace std;

void print_preamble( ostream& stream )
{
    stream << "graph T {\n"
           << "node [fontname=\"Courier New\"]\n";
}

void print_closing( ostream& stream )
{
    stream << "}\n\n";
}
