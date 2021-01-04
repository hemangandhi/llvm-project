// I'm going to start writing "notes" here and then divide them into the necessary files.
// This is since my mind is a disoganized mess between "I need AST nodes" and "how to do unicode".
// "//" are comments for my sake, "///" are about logic I intend to tidy and make "mainstream"

namespace apl {

  namespace ast {

    class AplAstNode {};

    class AplProgram : public AplAstNode {
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
    private:
      std::unique_ptr<OperatorNode> operator;
      std::unique_ptr<AxisIndicatorNode> axis;
    };

    class MonadicFunctionInvocation : public OperatorNode {
    private:
      std::unique_ptr<AplAstNode> omega;
    };

    class DyadicFunctionInvocation : public OperatorNode {
    private:
      std::unique_ptr<AplAstNode> omega;
      std::unique_ptr<AplAstNode> alpha;
    };

    // TODO: I think this is wrong or at least has different jargon.
    // I want to convey something like ∘.+ vs. ∘.x as the same operator (∘.)
    // with differing 
    class OperatorFunctionInvocation : public OperatorNode {
    private:
      std::unique_ptr<AplAstNode> omega;
      std::unique_ptr<AplAstNode> alpha;
      std::unique_ptr<AplAstNode> alpha_alpha;
    };

    /// This is any APL function.
    ///
    /// Variables are considered niladic functions, hence "←" is an operator node
    /// with the operator_id on the left and definition on the right.
    /// There are builtin operator nodes that can nullptr definitions for the builtins.
    class OperatorNode : public AplAstNode {
      // This is a virtual optional to manage the fact that operators may or may not be invocable in various contexts.
      /// This is niladic invocation -- abstractly, constants and variables that aren't functions are niladically invoked.
      virtual std::optional<InvocationNode> Invoke(AplAstNode* omega) = 0; 
      virtual std::optional<MonadicFunctionInvocation> Invoke(AplAstNode* omega) = 0; 
      virtual std::optional<DyadicFunctionInvocation> Invoke(AplAstNode* alpha, AplAstNode* omega) = 0;
      virtual std::optional<OperatorFunctionInvocation> Invoke(AplAstNode* alpha, AplAstNode* omega) = 0;
      virtual ~OperatorNode();
    private:
      // This would be the unicode APL operator or the bound var
      std::string operator_id;
      int precedence;
      std::unique_ptr<AplAstNode> definition;
    };

    /// A scalar or vector in APL. The shape is included to allow for constant folding in
    /// reshapes (if possible/sensible). If the shape vector is {1}, it is a scalar.
    class AplConstantNode : public AplAstNode {
    private:
      std::vector<double> constant;
      std::vector<double> shape;
    };

  } // namespace ast

  namespace tokens {
    class TokenProvider {
      virtual int getNextToken() = 0;
      virtual bool hasNextToken() = 0;
      virtual ~TokenProvider();
    };

    std::unique_ptr<AplProgram> readTokens(TokenProvider* tokens) {
      std::unique_ptr<AplProgram> parsed;
    }
  } // namspace tokens
} // namespace apl
