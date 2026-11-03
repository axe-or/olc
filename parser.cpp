#include "compiler.hpp"

using Parser_Result = Result<Node_ID, Syntax_Error>;

struct Parser {
	Lexer lexer;
	AST* ast;

	Parser_Result parse_file();

};
