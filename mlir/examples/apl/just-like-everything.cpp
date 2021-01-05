// I'm going to start writing "notes" here and then divide them into the necessary files.
// This is since my mind is a disoganized mess between "I need AST nodes" and "how to do unicode".
// "//" are comments for my sake, "///" are about logic I intend to tidy and make "mainstream"
#include "llvm/Support/ErrorOr.h"

namespace apl {

  namespace ast {

    class AplAstNode {};

    class AplProgram : public AplAstNode {
      // Default constructors plz notice me

      void setFirstExpr(std::unique_ptr<AplAstNode> new_fst) {
	first_expr = std::move(new_fst);
      }
      void setRestExprs(std::unique_ptr<AplProgram> new_rst) {
	rest_exprs = std::move(new_rst);
      }

      std::unique_ptr<AplProgram> getRestExprs() {
	return rest_exprs;
      }

      AplProgram& getOrAddNextToProgram() {
	if (!rest_exprs) rest_exprs = std::make_unique<AplProgram>();
	return *rest_exprs;
      }
    private:
      std::unique_ptr<AplAstNode> first_expr;
      std::unique_ptr<AplProgram> rest_exprs;
    };

    class AxisIndicatorNode : public AplAstNode {
    private:
      double axis;
    }

    class OperatorNode;

    class InvocationNode : public AplAstNode {
      void setOperator(std::unique_ptr<OperatorNode> op) {
	operator = std::move(op);
      }
    private:
      std::unique_ptr<OperatorNode> operator;
      std::unique_ptr<AxisIndicatorNode> axis;
    };

    class MonadicFunctionInvocation : public OperatorNode {
      MonadicFunctionInvocation(std::unique_ptr<AplAstNode> omega_arg) : OperatorNode(), omega(std::move(omega_arg)) {}
    private:
      std::unique_ptr<AplAstNode> omega;
    };

    class DyadicFunctionInvocation : public OperatorNode {
      MonadicFunctionInvocation(std::unique_ptr<AplAstNode> omega_arg, std::unique_ptr<AplAstNode> alpha_arg) : OperatorNode(), omega(std::move(omega_arg)), alpha(std::move(alpha_arg)) {}
    private:
      std::unique_ptr<AplAstNode> omega;
      std::unique_ptr<AplAstNode> alpha;
    };

    /// This is any APL function.
    ///
    /// Variables are considered niladic functions, hence "←" is an operator node
    /// with the operator_id on the left and definition on the right.
    /// There are builtin operator nodes that can nullptr definitions for the builtins.
    // TODO: s/virtual//?
    class OperatorNode : public AplAstNode {
      enum OperatorMode {
			 NILADIC = 1,
			 MONADIC = 2,
			 DYADIC = 4,
      };

      OperatorNode(std::string name, int apl_code, int modes) : operator_name(name), operator_unicode(apl_code), mode(modes), definition(nullptr) {}
      OperatorNode(std::string name, int fmode, std::unique_ptr<AplProgram> body) operator_name(name), operator_unicode(0), mode(fmode), definition(std::move(body)) {}
      
      // This is a virtual optional to manage the fact that operators may or may not be invocable in various contexts.
      /// This is niladic invocation -- abstractly, constants and variables that aren't functions are niladically invoked.
      virtual std::optional<InvocationNode> Invoke() {
	if (!(mode & OperatorNode::NILADIC)) return std::nullopt;
	return InvocationNode();
      }
      virtual std::optional<MonadicFunctionInvocation> Invoke(std::unique_ptr<AplAstNode> omega) {
	if (!(mode & OperatorMode::MONADIC)) return std::nullopt;
	return MonadicFunctionInvocation(std::move(omega));
      }
      virtual std::optional<DyadicFunctionInvocation> Invoke(std::unique_ptr<AplAstNode> alpha, std::unique_ptr<AplAstNode> omega) {
	if (!(mode & OperatorMode::DYADIC)) return std::nullopt;
	return DyadicFunctionInvocation(std::move(alpha), std::move(omega));
      }
      virtual ~OperatorNode();

      /// A small helper to write the proper "Invoke" above that also consumes the this pointer.
      template<typename T>
      static std::optional<std::unique_ptr<T>> handleInvoke(std::unique_ptr<OperatorNode> op, std::unique_ptr<AplAstNode>... args) {
	auto invoked = op->Invoke(std::move(args)...);
	if (invoked) invoked->setOperator(std::move(op));
	return std::make_unique<T>(*invoked);
      }
    private:
      // This would be the unicode APL operator or the bound var
      std::string operator_name;
      // Thanks C++.
      /// The unicode codepoint for the APL operator.
      int operator_unicode;
      int mode;
      std::unique_ptr<AplProgram> definition;
    };

    /// A scalar or vector in APL. The shape is included to allow for constant folding in
    /// reshapes (if possible/sensible). If the shape vector is {}, it is a scalar.
    /// Called an operator to manage niladic invocation, particularly if named.
    /// Can also be a string literal.
    class AplConstantNode : public OperatorNode {
      AplConstantNode(std::string name, std::unique_ptr body) : OperatorNode(name, OperatorNode::NILADIC, body) {}
    private:
      std::vector<double> constant;
      std::vector<double> shape;
      std::string value;
    };

    static const std::vector<OperatorNode>& getBuiltInOperators() {
      static std::vector<OperatorNode>* builtins = new std::vector<OperatorNode> {
	      OperatorNode("Plus", '+', OperatorNode::MONADIC | OperatorNode::DYADIC),
	      OperatorNode("Minus", '-', OperatorNode::MONADIC | OperatorNode::DYADIC),
	      OperatorNode("Times", 0x00d7 /*×*/, OperatorNode::MONADIC | OperatorNode::DYADIC),
	      OperatorNode("Divide", 0x00f7 /*÷*/, OperatorNode::MONADIC | OperatorNode::DYADIC),
	      OperatorNode("Pow", 0x22c6 /*⋆*/, OperatorNode::MONADIC | OperatorNode::DYADIC),
	      OperatorNode("Circle", 0x25cb /*○*/, OperatorNode::MONADIC | OperatorNode::DYADIC),
	      OperatorNode("Roll", 0x003f /*?*/, OperatorNode::MONADIC | OperatorNode::DYADIC),
	      OperatorNode("Element", 0x2208 /*∈*/, OperatorNode::DYADIC),
	      OperatorNode("Ceiling", 0x2308 /*⌈*/, OperatorNode::MONADIC | OperatorNode::DYADIC),
	      OperatorNode("Floor", 0x230a /*⌊*/, OperatorNode::MONADIC | OperatorNode::DYADIC),
	      OperatorNode("Shape", 0x2374 /*⍴*/, OperatorNode::MONADIC | OperatorNode::DYADIC),
	      OperatorNode("Take", 0x2191 /*↑*/, OperatorNode::DYADIC),
	      OperatorNode("Drop", 0x2193 /*↓*/, OperatorNode::DYADIC),
	      OperatorNode("Encode", 0x22a5 /*⊥*/, OperatorNode::DYADIC),
	      OperatorNode("Decode", 0x22a4 /*⊤*/, OperatorNode::DYADIC),
	      OperatorNode("Mod", 0x2223 /*∣*/, OperatorNode::MONADIC | OperatorNode::DYADIC),
	      OperatorNode("Catenation", 0x2c /*,*/, OperatorNode::MONADIC | OperatorNode::DYADIC),
      };
      return *builtins;
    }
  } // namespace ast

  namespace tokens {
    class TokenProvider {
      virtual std::optional<int> getNextToken() = 0;
      virtual bool hasNext() = 0;
      /// The line and column the token is at.
      // TODO: understand how unicode works here.
      virtual std::pair<int, int> getLocation() = 0;
      virtual ~TokenProvider();
    };

    // Private implementation of https://dfns.dyalog.com/n_parse.htm
    namespace {
      using llvm::ErrorOr;

      ErrorOr<std::unqiue_ptr<AplAstNode>> handleAplExpr(TokenProvider* tokens) {
	std::optional<int> next_tok = tokens->getNextToken();
	if (!next_tok) return nullptr; // empty program is ok
	int next_token = *next_tok;
      }
    } // namespace

    // TODO: how to have custom errors? It looks like I'm stuck to a std::error_code
    // May be just have a custom std::variant?
    ErrorOr<std::unique_ptr<AplProgram>> readTokens(TokenProvider* tokens) {
      AplProgram parsed;
      AplProgram& tail = parsed;
      while(tokens->hasNext()) {
	tail = tail.getOrAddNextToProgram();
	auto first_expr = handleAplExpr(tokens);
	if(!first_expr) return first_expr;
	tail.setFirstExpr(std::move(*first_expr));
      }
      return parsed.getRestExprs();
    }
  } // namspace tokens
} // namespace apl
