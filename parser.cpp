#include "base.hpp"
#include "compiler.hpp"

using Parser_Result = Result<Node_ID, Syntax_Error>;

static inline
constexpr Option<int> prefix_binding_power(Token_Kind k){
	switch(k){
	case Tk_Minus:
    case Tk_Tilde:
    case Tk_Not:
    case Tk_Plus:
    	return option_some(100);

    default:
    	return option_none();
    }
}

static inline
constexpr Option<Pair<int>> infix_binding_power(Token_Kind k){
	[[maybe_unused]]
	constexpr auto left_assoc = [](int prec) {
		return option_some(Pair<int>{prec, prec + 1});
	};

	[[maybe_unused]]
	constexpr auto right_assoc = [](int prec) {
		return option_some(Pair<int>{prec + 1, prec});
	};

	switch(k){
	case Tk_Star:
	case Tk_Slash:
	case Tk_Mod:
	case Tk_ModFloor:
	case Tk_BitAnd:
	case Tk_ShiftLeft:
	case Tk_ShiftRight:
		return left_assoc(70);

	case Tk_Plus:
	case Tk_Minus:
	case Tk_BitOr:
	case Tk_Tilde:
		return left_assoc(60);

	case Tk_Equal:
	case Tk_NotEqual:
	case Tk_Less:
	case Tk_Greater:
	case Tk_LessEqual:
	case Tk_GreaterEqual:
		return left_assoc(50);

	case Tk_And:
		return left_assoc(40);

	case Tk_Or:
		return left_assoc(30);

	default:
		return option_none();
    }
}

struct Parser {
	Lexer lexer;
	AST* ast;

	bool expect_token(Token_Kind k){
		Token tk = {};
		if(!lexer.peek().unwrap_into(&tk).has_value()){
			return false;
		}
		(void)lexer.next();

		return true;
	}

	Parser_Result parse_expr_rec(i32 min_bp){
		Node_ID lhs = {0};
		Token first = {};

		err_return(lexer.next().unwrap_into(&first));


		if(first.type == Tk_ParenOpen){
			err_return(parse_expr_rec(0).unwrap_into(&lhs));
			lexer.peek().unwrap_or(lexer.eof());
		}
		else {
			i32 rhs_bp = prefix_binding_power(first.type).unwrap_or(0);
			if(rhs_bp){
				auto operand = parse_expr_rec(rhs_bp);

			}
			else {
				panic("not a prefix operator");
			}
		}

		return {};
	}

	Parser_Result parse_file();
};
