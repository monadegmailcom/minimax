#include "minimax.h"

#include <vector>
#include <iostream>
#include <sstream>

void print_preamble( std::ostream& );
void print_closing( std::ostream& );

template< typename MoveT >
struct PrintTree
{
    PrintTree( std::ostream& stream, std::vector< Vertex< MoveT > > const& vertices,
               GenericRule< MoveT >& rule, Player player ) :
        stream( stream ), vertices( vertices ), rule( rule ),
        out_stream( OutStream { stream, "<B>", "</B>", "<BR/>", "&nbsp;" } ),
        vertex_idx( 0 )
    {
        assert (!vertices.empty());
        print_preamble( stream );
        rec( "v", true, player );
        print_closing( stream );
    }

    void rec( std::string const& name, bool is_best_thread, Player player )
    {
        // print vertex
        stream << "\"" << name << "\" [label=<";
        Vertex< MoveT > const& vertex = vertices[vertex_idx];
        rule.print_board( out_stream, vertex.move );
        ++vertex_idx;

        stream << ">";

        if (is_best_thread)
            stream << " color=\"red\"";

        if (player == player1)
            stream << " shape=box";
        stream << "]\n";

        for (size_t idx = vertex.child_count; idx; --idx)
        {
            Vertex< MoveT > const& child = vertices[vertex_idx];

            std::ostringstream s;
            rule.print_move( s, *child.move );
            const std::string new_name = name + " " + s.str();

            // plot edge to child
            stream << "\"" << name << "\"" << " -- \"" << new_name << "\""
                   << " [label=\"" << child.value << "\"";
            if (is_best_thread && child.move == vertex.best_move)
                stream << " color=\"red\"";
            stream << "]\n";

            // apply move
            rule.apply_move( *child.move, player );

            rec(
                new_name, is_best_thread && child.move == vertex.best_move,
                Player( -player ));

            // undo move
            rule.undo_move( *child.move, player );
        }
    }

    std::ostream& stream;
    std::vector< Vertex< MoveT > > const& vertices;
    GenericRule< MoveT >& rule;
    OutStream out_stream;
    size_t vertex_idx;
};
